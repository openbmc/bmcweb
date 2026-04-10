// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "mutual_tls.hpp"

#include "mutual_tls_private.hpp"
#include "ossl_test_memory.hpp"
#include "ossl_wrappers.hpp"
#include "sessions.hpp"

#include <openssl/x509.h>
#include <openssl/x509_vfy.h>

#include <algorithm>
#include <array>
#include <initializer_list>
#include <string>
#include <string_view>

extern "C"
{
#include <openssl/asn1.h>
#include <openssl/bio.h>
#include <openssl/ec.h>
#include <openssl/evp.h>
#include <openssl/obj_mac.h>
#include <openssl/ssl.h>
#include <openssl/types.h>
}

#include <boost/asio/ip/address.hpp>

#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::IsNull;
using ::testing::NotNull;

static const OpenSSLTestMemory osslInit;

namespace
{
// Helper that performs a TLS handshake over memory BIOs so the server-side
// SSL object sees the client certificate as its peer certificate.
class MtlsHandshake
{
    SSL_CTX* serverCtx = nullptr;
    SSL_CTX* clientCtx = nullptr;
    SSL* serverSsl = nullptr;
    SSL* clientSsl = nullptr;

    // Pump data between the two SSL objects via their memory BIOs
    // until both sides report completion (or error).
    static void doHandshake(SSL* client, SSL* server)
    {
        for (int i = 0; i < 100; ++i)
        {
            int clientRet = SSL_do_handshake(client);
            transferBioData(client, server);

            int serverRet = SSL_do_handshake(server);
            transferBioData(server, client);

            if (clientRet == 1 && serverRet == 1)
            {
                return;
            }
        }
    }

    static void transferBioData(SSL* from, SSL* to)
    {
        BIO* fromWbio = SSL_get_wbio(from);
        BIO* toRbio = SSL_get_rbio(to);
        std::array<char, 4096> buf{};
        int pending = static_cast<int>(BIO_ctrl_pending(fromWbio));
        while (pending > 0)
        {
            int n = BIO_read(fromWbio, buf.data(),
                             std::min(pending, static_cast<int>(buf.size())));
            if (n > 0)
            {
                BIO_write(toRbio, buf.data(), n);
            }
            pending = static_cast<int>(BIO_ctrl_pending(fromWbio));
        }
    }

    static EVP_PKEY* generateEcKey()
    {
        EVP_PKEY_CTX* pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, nullptr);
        if (pctx == nullptr)
        {
            return nullptr;
        }
        if (EVP_PKEY_keygen_init(pctx) != 1)
        {
            EVP_PKEY_CTX_free(pctx);
            return nullptr;
        }
        if (EVP_PKEY_CTX_set_ec_paramgen_curve_nid(pctx,
                                                   NID_X9_62_prime256v1) != 1)
        {
            EVP_PKEY_CTX_free(pctx);
            return nullptr;
        }
        EVP_PKEY* pkey = nullptr;
        EVP_PKEY_keygen(pctx, &pkey);
        EVP_PKEY_CTX_free(pctx);
        return pkey;
    }

  public:
    MtlsHandshake& operator=(const MtlsHandshake&) = delete;
    MtlsHandshake& operator=(MtlsHandshake&&) = delete;

    MtlsHandshake(const MtlsHandshake&) = delete;
    MtlsHandshake(MtlsHandshake&&) = delete;

    MtlsHandshake() = default;

    // clientCert: the X509 certificate the client presents to the server.
    //             If nullptr, the client will not present a certificate.
    // Must be called from a TEST body so ASSERT_* can abort the test.
    void init(X509* clientCert)
    {
        serverCtx = SSL_CTX_new(TLS_server_method());
        ASSERT_THAT(serverCtx, NotNull());
        clientCtx = SSL_CTX_new(TLS_client_method());
        ASSERT_THAT(clientCtx, NotNull());

        // ---- Generate a self-signed server certificate + key ----
        EVP_PKEY* serverKey = generateEcKey();
        ASSERT_THAT(serverKey, NotNull());

        X509* serverCert = X509_new();
        ASSERT_THAT(serverCert, NotNull());
        ASN1_INTEGER_set(X509_get_serialNumber(serverCert), 1);
        X509_gmtime_adj(X509_getm_notBefore(serverCert), 0);
        X509_gmtime_adj(X509_getm_notAfter(serverCert),
                        static_cast<long>(60 * 60));
        X509_set_pubkey(serverCert, serverKey);
        X509_NAME* sName = X509_get_subject_name(serverCert);
        std::array<unsigned char, 7> srvCN{'s', 'e', 'r', 'v', 'e', 'r', '\0'};
        X509_NAME_add_entry_by_txt(sName, "CN", MBSTRING_ASC, srvCN.data(),
                                   static_cast<int>(srvCN.size()), -1, 0);
        X509_set_issuer_name(serverCert, sName);
        X509_sign(serverCert, serverKey, EVP_sha256());

        ASSERT_EQ(SSL_CTX_use_certificate(serverCtx, serverCert), 1);
        ASSERT_EQ(SSL_CTX_use_PrivateKey(serverCtx, serverKey), 1);
        // Ask the client for a certificate but don't verify it ourselves
        // (we let verifyMtlsUser handle the verification logic)
        SSL_CTX_set_verify(serverCtx, SSL_VERIFY_PEER, nullptr);

        // Trust the server certificate
        X509_STORE* clientStore = SSL_CTX_get_cert_store(clientCtx);
        ASSERT_THAT(clientStore, NotNull());
        ASSERT_EQ(X509_STORE_add_cert(clientStore, serverCert), 1);
        SSL_CTX_set_verify(clientCtx, SSL_VERIFY_PEER, nullptr);
        X509_free(serverCert);

        // ---- Prepare client certificate with a matching private key ----
        EVP_PKEY* clientKey = nullptr;
        if (clientCert != nullptr)
        {
            // Set required fields for TLS verification to succeed
            X509_NAME* cn = X509_get_subject_name(clientCert);
            ASSERT_THAT(cn, NotNull());
            ASSERT_EQ(X509_set_issuer_name(clientCert, cn), 1);
            ASSERT_EQ(ASN1_INTEGER_set(X509_get_serialNumber(clientCert), 2),
                      1);
            ASSERT_THAT(X509_gmtime_adj(X509_getm_notBefore(clientCert), 0),
                        NotNull());
            ASSERT_THAT(X509_gmtime_adj(X509_getm_notAfter(clientCert),
                                        static_cast<long>(60 * 60)),
                        NotNull());

            // Generate a new key and re-sign the cert so we have both
            // the certificate and its private key for the handshake.
            clientKey = generateEcKey();
            ASSERT_THAT(clientKey, NotNull());
            ASSERT_EQ(X509_set_pubkey(clientCert, clientKey), 1);
            ASSERT_GT(X509_sign(clientCert, clientKey, EVP_sha256()), 0);

            // Trust the (re-signed) client cert on the server side
            X509_STORE* store = SSL_CTX_get_cert_store(serverCtx);
            ASSERT_THAT(store, NotNull());
            ASSERT_EQ(X509_STORE_add_cert(store, clientCert), 1);
        }

        // ---- Create SSL objects with memory BIOs ----
        serverSsl = SSL_new(serverCtx);
        ASSERT_THAT(serverSsl, NotNull());
        BIO* serverRbio = BIO_new(BIO_s_mem());
        ASSERT_THAT(serverRbio, NotNull());
        BIO* serverWbio = BIO_new(BIO_s_mem());
        ASSERT_THAT(serverWbio, NotNull());
        SSL_set_bio(serverSsl, serverRbio, serverWbio);
        SSL_set_accept_state(serverSsl);

        clientSsl = SSL_new(clientCtx);
        ASSERT_THAT(clientSsl, NotNull());
        BIO* clientRbio = BIO_new(BIO_s_mem());
        ASSERT_THAT(clientRbio, NotNull());
        BIO* clientWbio = BIO_new(BIO_s_mem());
        ASSERT_THAT(clientWbio, NotNull());
        SSL_set_bio(clientSsl, clientRbio, clientWbio);
        SSL_set_connect_state(clientSsl);

        if (clientCert != nullptr)
        {
            ASSERT_EQ(SSL_use_certificate(clientSsl, clientCert), 1);
            ASSERT_EQ(SSL_use_PrivateKey(clientSsl, clientKey), 1);
            EVP_PKEY_free(clientKey);
        }

        EVP_PKEY_free(serverKey);

        // ---- Perform the handshake ----
        doHandshake(clientSsl, serverSsl);
    }

    SSL* serverHandle()
    {
        return serverSsl;
    }

    ~MtlsHandshake()
    {
        SSL_free(serverSsl);
        SSL_free(clientSsl);
        SSL_CTX_free(serverCtx);
        SSL_CTX_free(clientCtx);
    }
};

void verifyMtlsCert(std::string_view keyUsage)
{
    OpenSSLX509 x509;
    x509.setSubjectName();

    x509.addExt(NID_key_usage, keyUsage);
    x509.addExt(NID_ext_key_usage, "clientAuth");

    MtlsHandshake handshake;
    handshake.init(x509.get());

    boost::asio::ip::address ip;
    std::shared_ptr<persistent_data::UserSession> session =
        verifyMtlsUser(ip, handshake.serverHandle());
    ASSERT_THAT(session, NotNull());
    EXPECT_THAT(session->username, "user");
}

TEST(MutualTLS, GoodCert)
{
    verifyMtlsCert("digitalSignature, keyAgreement");
}

TEST(MutualTLS, MissingKeyUsage)
{
    for (const char* usageString :
         {"digitalSignature", "keyAgreement", "digitalSignature, keyAgreement"})
    {
        verifyMtlsCert(usageString);
    }
}

TEST(MutualTLS, MissingCert)
{
    MtlsHandshake handshake;
    handshake.init(nullptr);

    boost::asio::ip::address ip;
    std::shared_ptr<persistent_data::UserSession> session =
        verifyMtlsUser(ip, handshake.serverHandle());
    ASSERT_THAT(session, IsNull());
}

TEST(MutualTLS, NullSSL)
{
    boost::asio::ip::address ip;
    std::shared_ptr<persistent_data::UserSession> session =
        verifyMtlsUser(ip, nullptr);
    ASSERT_THAT(session, IsNull());
}

TEST(GetCommonNameFromCert, EmptyCommonName)
{
    OpenSSLX509 x509;
    std::string commonName = getCommonNameFromCert(x509.get());
    EXPECT_THAT(commonName, "");
}

TEST(GetCommonNameFromCert, ValidCommonName)
{
    OpenSSLX509 x509;
    x509.setSubjectName();
    std::string commonName = getCommonNameFromCert(x509.get());
    EXPECT_THAT(commonName, "user");
}

TEST(GetUPNFromCert, EmptySubjectAlternativeName)
{
    OpenSSLX509 x509;
    std::string upn = getUPNFromCert(x509.get(), "");
    EXPECT_THAT(upn, "");
}

TEST(GetUPNFromCert, NonOthernameSubjectAlternativeName)
{
    OpenSSLX509 x509;
    ASSERT_TRUE(x509.addAltNameEmails({"user@domain.com"}));
    std::string upn = getUPNFromCert(x509.get(), "hostname.domain.com");
    EXPECT_THAT(upn, "");
}

TEST(GetUPNFromCert, NonUPNSubjectAlternativeName)
{
    OpenSSLX509 x509;
    ASSERT_TRUE(x509.addAltNames(NID_SRVName, {"user@domain.com"}));
    std::string upn = getUPNFromCert(x509.get(), "hostname.domain.com");
    EXPECT_THAT(upn, "");
}

TEST(GetUPNFromCert, NonUTF8UPNSubjectAlternativeName)
{
    OpenSSLX509 x509;
    ASSERT_TRUE(x509.addAltNameUpns({"0123456789"}));

    std::string upn = getUPNFromCert(x509.get(), "hostname.domain.com");
    EXPECT_THAT(upn, "");
}

TEST(GetUPNFromCert, ValidUPN)
{
    OpenSSLX509 x509;
    ASSERT_TRUE(x509.addAltNameUpns({"user@domain.com"}));

    std::string upn = getUPNFromCert(x509.get(), "hostname.domain.com");
    EXPECT_THAT(upn, "user");
}

TEST(IsUPNMatch, MultipleCases)
{
    EXPECT_FALSE(isUPNMatch("user", "hostname.domain.com"));
    EXPECT_TRUE(isUPNMatch("user@domain.com", "hostname.domain.com"));
    EXPECT_FALSE(isUPNMatch("user@domain.com", "hostname.domain.org"));
    EXPECT_FALSE(isUPNMatch("user@region.com", "hostname.domain.com"));
    EXPECT_TRUE(isUPNMatch("user@com", "hostname.region.domain.com"));
}

TEST(IsUPNMatch, CaseSensitivity)
{
    // Domain names are case-insensitive per RFC standards
    EXPECT_TRUE(isUPNMatch("user@DOMAIN.COM", "host.domain.com"));
    EXPECT_TRUE(isUPNMatch("user@Domain.Com", "host.DOMAIN.COM"));
    EXPECT_TRUE(isUPNMatch("user@domain.com", "host.DOMAIN.COM"));
}

TEST(GetUPNFromCert, DomainMismatchRejects)
{
    OpenSSLX509 x509;
    x509.addAltNameUpns({"user@evil.com"});

    std::string upn = getUPNFromCert(x509.get(), "hostname.domain.com");
    EXPECT_THAT(upn, "");
}

TEST(GetUPNFromCert, MultipleUPNEntriesFirstWins)
{
    OpenSSLX509 x509;
    ASSERT_TRUE(x509.addAltNameUpns({"first@domain.com", "second@domain.com"}));

    std::string upn = getUPNFromCert(x509.get(), "hostname.domain.com");
    EXPECT_THAT(upn, "first");
}

TEST(GetUPNFromCert, UPNWithoutAtSymbol)
{
    OpenSSLX509 x509;
    x509.addAltNameUpns({"userwithoutatsymbol"});

    std::string upn = getUPNFromCert(x509.get(), "hostname.domain.com");
    EXPECT_THAT(upn, "");
}

TEST(GetUPNFromCert, UPNWithEmptyUsername)
{
    OpenSSLX509 x509;
    x509.addAltNameUpns({"@domain.com"});

    std::string upn = getUPNFromCert(x509.get(), "hostname.domain.com");
    EXPECT_THAT(upn, "");
}

TEST(MutualTLS, CertificateWithoutClientAuthPurpose)
{
    OpenSSLX509 x509;
    x509.setSubjectName();

    x509.addExt(NID_key_usage, "digitalSignature");
    x509.addExt(NID_ext_key_usage, "serverAuth");

    MtlsHandshake handshake;
    handshake.init(x509.get());

    boost::asio::ip::address ip;
    std::shared_ptr<persistent_data::UserSession> session =
        verifyMtlsUser(ip, handshake.serverHandle());
    ASSERT_THAT(session, IsNull());
}

TEST(MutualTLS, UPNValidCertSuccess)
{
    persistent_data::SessionStore::getInstance().getAuthMethodsConfig().tls =
        true;
    persistent_data::SessionStore::getInstance()
        .getAuthMethodsConfig()
        .mTLSCommonNameParsingMode =
        persistent_data::MTLSCommonNameParseMode::CommonName;

    OpenSSLX509 x509;
    x509.setSubjectName();

    x509.addExt(NID_key_usage, "digitalSignature, keyAgreement");
    x509.addExt(NID_ext_key_usage, "clientAuth");

    MtlsHandshake handshake;
    handshake.init(x509.get());

    boost::asio::ip::address ip;
    std::shared_ptr<persistent_data::UserSession> session =
        verifyMtlsUser(ip, handshake.serverHandle());
    ASSERT_THAT(session, NotNull());
    EXPECT_THAT(session->username, "user");
    EXPECT_THAT(session->sessionType, persistent_data::SessionType::MutualTLS);
}

TEST(MutualTLS, UPNDomainMismatchRejectsAuth)
{
    persistent_data::SessionStore::getInstance().getAuthMethodsConfig().tls =
        true;
    persistent_data::SessionStore::getInstance()
        .getAuthMethodsConfig()
        .mTLSCommonNameParsingMode =
        persistent_data::MTLSCommonNameParseMode::UserPrincipalName;

    OpenSSLX509 x509;
    x509.addAltNameUpns({"attacker@evil.com"});

    x509.addExt(NID_key_usage, "digitalSignature, keyAgreement");
    x509.addExt(NID_ext_key_usage, "clientAuth");

    MtlsHandshake handshake;
    handshake.init(x509.get());

    boost::asio::ip::address ip;
    std::shared_ptr<persistent_data::UserSession> session =
        verifyMtlsUser(ip, handshake.serverHandle());
    ASSERT_THAT(session, IsNull());
}

TEST(MutualTLS, CommonNameModeReturnsCommonName)
{
    persistent_data::SessionStore::getInstance().getAuthMethodsConfig().tls =
        true;
    persistent_data::SessionStore::getInstance()
        .getAuthMethodsConfig()
        .mTLSCommonNameParsingMode =
        persistent_data::MTLSCommonNameParseMode::CommonName;

    OpenSSLX509 x509;
    x509.setSubjectName();
    x509.addAltNameUpns({"upnuser@domain.com"});
    x509.addExt(NID_key_usage, "digitalSignature, keyAgreement");
    x509.addExt(NID_ext_key_usage, "clientAuth");

    MtlsHandshake handshake;
    handshake.init(x509.get());

    boost::asio::ip::address ip;
    std::shared_ptr<persistent_data::UserSession> session =
        verifyMtlsUser(ip, handshake.serverHandle());
    ASSERT_THAT(session, NotNull());
    EXPECT_THAT(session->username, "user");
}

TEST(MutualTLS, InvalidModeRejectsAuth)
{
    persistent_data::SessionStore::getInstance().getAuthMethodsConfig().tls =
        true;
    persistent_data::SessionStore::getInstance()
        .getAuthMethodsConfig()
        .mTLSCommonNameParsingMode =
        persistent_data::MTLSCommonNameParseMode::Invalid;

    OpenSSLX509 x509;
    x509.setSubjectName();
    x509.addExt(NID_key_usage, "digitalSignature, keyAgreement");
    x509.addExt(NID_ext_key_usage, "clientAuth");

    MtlsHandshake handshake;
    handshake.init(x509.get());

    boost::asio::ip::address ip;
    std::shared_ptr<persistent_data::UserSession> session =
        verifyMtlsUser(ip, handshake.serverHandle());
    ASSERT_THAT(session, IsNull());
}

TEST(GetUPNFromCert, UPNWithMultipleAtSymbols)
{
    OpenSSLX509 x509;
    x509.addAltNameUpns({"user@sub@domain.com"});

    std::string upn = getUPNFromCert(x509.get(), "hostname.domain.com");
    EXPECT_THAT(upn, "");
}

TEST(GetUPNFromCert, UPNWithEmptyDomain)
{
    OpenSSLX509 x509;
    x509.addAltNameUpns({"user@"});

    std::string upn = getUPNFromCert(x509.get(), "hostname.domain.com");
    EXPECT_THAT(upn, "");
}

TEST(IsUPNMatch, InternationalizedDomain)
{
    EXPECT_TRUE(isUPNMatch("user@münchen.de", "host.münchen.de"));
    EXPECT_FALSE(isUPNMatch("user@münchen.de", "host.berlin.de"));
}

TEST(GetUPNFromCert, UPNWithSpecialCharacters)
{
    OpenSSLX509 x509;
    x509.addAltNameUpns({"user.name+tag@domain.com"});

    std::string upn = getUPNFromCert(x509.get(), "hostname.domain.com");
    EXPECT_THAT(upn, "user.name+tag");
}

TEST(GetUPNFromCert, FirstUPNInvalidSecondValid)
{
    OpenSSLX509 x509;
    x509.addAltNameUpns({"first@wrong.com", "second@domain.com"});

    std::string upn = getUPNFromCert(x509.get(), "hostname.domain.com");
    EXPECT_THAT(upn, "second");
}
} // namespace
