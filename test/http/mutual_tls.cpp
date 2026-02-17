// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "mutual_tls.hpp"

#include "mutual_tls_private.hpp"
#include "sessions.hpp"
#include "ssl_key_handler.hpp"

#include <algorithm>
#include <cstring>
#include <string>

extern "C"
{
#include <openssl/asn1.h>
#include <openssl/bio.h>
#include <openssl/ec.h>
#include <openssl/evp.h>
#include <openssl/obj_mac.h>
#include <openssl/objects.h>
#include <openssl/ssl.h>
#include <openssl/types.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
#include <openssl/x509v3.h>
}

#include <boost/asio/ip/address.hpp>

#include <array>
#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::IsNull;
using ::testing::NotNull;

namespace
{
class OSSLX509
{
    X509* ptr = X509_new();

  public:
    OSSLX509& operator=(const OSSLX509&) = delete;
    OSSLX509& operator=(OSSLX509&&) = delete;

    OSSLX509(const OSSLX509&) = delete;
    OSSLX509(OSSLX509&&) = delete;

    OSSLX509() = default;

    void setSubjectName()
    {
        X509_NAME* name = X509_get_subject_name(ptr);
        std::array<unsigned char, 5> user = {'u', 's', 'e', 'r', '\0'};
        X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, user.data(), -1,
                                   -1, 0);
    }
    void sign()
    {
        // Generate test key
        EVP_PKEY* pkey = nullptr;
        EVP_PKEY_CTX* pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, nullptr);
        ASSERT_EQ(EVP_PKEY_keygen_init(pctx), 1);
        ASSERT_EQ(
            EVP_PKEY_CTX_set_ec_paramgen_curve_nid(pctx, NID_X9_62_prime256v1),
            1);
        ASSERT_EQ(EVP_PKEY_keygen(pctx, &pkey), 1);
        EVP_PKEY_CTX_free(pctx);

        // Sign cert with key
        signWithKey(pkey);
        EVP_PKEY_free(pkey);
    }

    void signWithKey(EVP_PKEY* pkey)
    {
        ASSERT_EQ(X509_set_pubkey(ptr, pkey), 1);
        ASSERT_GT(X509_sign(ptr, pkey, EVP_sha256()), 0);
    }

    X509* get()
    {
        return ptr;
    }
    ~OSSLX509()
    {
        X509_free(ptr);
    }
};

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
        EVP_PKEY* serverKey = ensuressl::createEcKey();
        ASSERT_THAT(serverKey, NotNull());
        X509* serverCert = ensuressl::constructX509("server", serverKey);
        ASSERT_THAT(serverCert, NotNull());

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
            ASSERT_THAT(X509_gmtime_adj(X509_get_notBefore(clientCert), 0),
                        NotNull());
            ASSERT_THAT(X509_gmtime_adj(X509_get_notAfter(clientCert),
                                        static_cast<long>(60 * 60)),
                        NotNull());

            // Generate a new key and re-sign the cert so we have both
            // the certificate and its private key for the handshake.
            clientKey = ensuressl::createEcKey();
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

void verifyMtlsCert(const char* keyUsage)
{
    OSSLX509 x509;
    x509.setSubjectName();

    X509_EXTENSION* ex =
        X509V3_EXT_conf_nid(nullptr, nullptr, NID_key_usage, keyUsage);
    ASSERT_THAT(ex, NotNull());
    ASSERT_EQ(X509_add_ext(x509.get(), ex, -1), 1);
    X509_EXTENSION_free(ex);
    ex = X509V3_EXT_conf_nid(nullptr, nullptr, NID_ext_key_usage, "clientAuth");
    ASSERT_THAT(ex, NotNull());
    ASSERT_EQ(X509_add_ext(x509.get(), ex, -1), 1);
    X509_EXTENSION_free(ex);
    x509.sign();

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
    OSSLX509 x509;
    std::string commonName = getCommonNameFromCert(x509.get());
    EXPECT_THAT(commonName, "");
}

TEST(GetCommonNameFromCert, ValidCommonName)
{
    OSSLX509 x509;
    x509.setSubjectName();
    std::string commonName = getCommonNameFromCert(x509.get());
    EXPECT_THAT(commonName, "user");
}

TEST(GetUPNFromCert, EmptySubjectAlternativeName)
{
    OSSLX509 x509;
    std::string upn = getUPNFromCert(x509.get(), "");
    EXPECT_THAT(upn, "");
}

TEST(GetUPNFromCert, NonOthernameSubjectAlternativeName)
{
    OSSLX509 x509;

    ASN1_IA5STRING* ia5 = ASN1_IA5STRING_new();
    ASSERT_THAT(ia5, NotNull());

    const char* user = "user@domain.com";
    ASSERT_NE(ASN1_STRING_set(ia5, user, static_cast<int>(strlen(user))), 0);

    GENERAL_NAMES* gens = sk_GENERAL_NAME_new_null();
    ASSERT_THAT(gens, NotNull());

    GENERAL_NAME* gen = GENERAL_NAME_new();
    ASSERT_THAT(gen, NotNull());

    GENERAL_NAME_set0_value(gen, GEN_EMAIL, ia5);
    ASSERT_EQ(sk_GENERAL_NAME_push(gens, gen), 1);

    ASSERT_EQ(X509_add1_ext_i2d(x509.get(), NID_subject_alt_name, gens, 0, 0),
              1);

    std::string upn = getUPNFromCert(x509.get(), "hostname.domain.com");
    EXPECT_THAT(upn, "");

    GENERAL_NAME_free(gen);
    sk_GENERAL_NAME_free(gens);
}

TEST(GetUPNFromCert, NonUPNSubjectAlternativeName)
{
    OSSLX509 x509;

    GENERAL_NAMES* gens = sk_GENERAL_NAME_new_null();
    ASSERT_THAT(gens, NotNull());

    GENERAL_NAME* gen = GENERAL_NAME_new();
    ASSERT_THAT(gen, NotNull());

    ASN1_OBJECT* othType = OBJ_nid2obj(NID_SRVName);

    ASN1_TYPE* value = ASN1_TYPE_new();
    ASSERT_THAT(value, NotNull());
    value->type = V_ASN1_UTF8STRING;

    // NOLINTBEGIN(cppcoreguidelines-pro-type-union-access)
    value->value.utf8string = ASN1_UTF8STRING_new();
    ASSERT_THAT(value->value.utf8string, NotNull());
    const char* user = "user@domain.com";
    ASN1_STRING_set(value->value.utf8string, user,
                    static_cast<int>(strlen(user)));
    // NOLINTEND(cppcoreguidelines-pro-type-union-access)

    ASSERT_EQ(GENERAL_NAME_set0_othername(gen, othType, value), 1);
    ASSERT_EQ(sk_GENERAL_NAME_push(gens, gen), 1);
    ASSERT_EQ(X509_add1_ext_i2d(x509.get(), NID_subject_alt_name, gens, 0, 0),
              1);

    std::string upn = getUPNFromCert(x509.get(), "hostname.domain.com");
    EXPECT_THAT(upn, "");

    sk_GENERAL_NAME_pop_free(gens, GENERAL_NAME_free);
}

TEST(GetUPNFromCert, NonUTF8UPNSubjectAlternativeName)
{
    OSSLX509 x509;

    GENERAL_NAMES* gens = sk_GENERAL_NAME_new_null();
    ASSERT_THAT(gens, NotNull());

    GENERAL_NAME* gen = GENERAL_NAME_new();
    ASSERT_THAT(gen, NotNull());

    ASN1_OBJECT* othType = OBJ_nid2obj(NID_ms_upn);

    ASN1_TYPE* value = ASN1_TYPE_new();
    ASSERT_THAT(value, NotNull());
    value->type = V_ASN1_OCTET_STRING;

    // NOLINTBEGIN(cppcoreguidelines-pro-type-union-access)
    value->value.octet_string = ASN1_OCTET_STRING_new();
    ASSERT_THAT(value->value.octet_string, NotNull());
    const char* user = "0123456789";
    ASN1_STRING_set(value->value.octet_string, user,
                    static_cast<int>(strlen(user)));
    // NOLINTEND(cppcoreguidelines-pro-type-union-access)

    ASSERT_EQ(GENERAL_NAME_set0_othername(gen, othType, value), 1);
    ASSERT_EQ(sk_GENERAL_NAME_push(gens, gen), 1);
    ASSERT_EQ(X509_add1_ext_i2d(x509.get(), NID_subject_alt_name, gens, 0, 0),
              1);

    std::string upn = getUPNFromCert(x509.get(), "hostname.domain.com");
    EXPECT_THAT(upn, "");

    sk_GENERAL_NAME_pop_free(gens, GENERAL_NAME_free);
}

TEST(GetUPNFromCert, ValidUPN)
{
    OSSLX509 x509;

    GENERAL_NAMES* gens = sk_GENERAL_NAME_new_null();
    ASSERT_THAT(gens, NotNull());

    GENERAL_NAME* gen = GENERAL_NAME_new();
    ASSERT_THAT(gen, NotNull());

    ASN1_OBJECT* othType = OBJ_nid2obj(NID_ms_upn);

    ASN1_TYPE* value = ASN1_TYPE_new();
    ASSERT_THAT(value, NotNull());
    value->type = V_ASN1_UTF8STRING;

    // NOLINTBEGIN(cppcoreguidelines-pro-type-union-access)
    value->value.utf8string = ASN1_UTF8STRING_new();
    ASSERT_THAT(value->value.utf8string, NotNull());
    const char* user = "user@domain.com";
    ASN1_STRING_set(value->value.utf8string, user,
                    static_cast<int>(strlen(user)));
    // NOLINTEND(cppcoreguidelines-pro-type-union-access)

    ASSERT_EQ(GENERAL_NAME_set0_othername(gen, othType, value), 1);
    ASSERT_EQ(sk_GENERAL_NAME_push(gens, gen), 1);
    ASSERT_EQ(X509_add1_ext_i2d(x509.get(), NID_subject_alt_name, gens, 0, 0),
              1);

    std::string upn = getUPNFromCert(x509.get(), "hostname.domain.com");
    EXPECT_THAT(upn, "user");

    sk_GENERAL_NAME_pop_free(gens, GENERAL_NAME_free);
}

TEST(IsUPNMatch, MultipleCases)
{
    EXPECT_FALSE(isUPNMatch("user", "hostname.domain.com"));
    EXPECT_TRUE(isUPNMatch("user@domain.com", "hostname.domain.com"));
    EXPECT_FALSE(isUPNMatch("user@domain.com", "hostname.domain.org"));
    EXPECT_FALSE(isUPNMatch("user@region.com", "hostname.domain.com"));
    EXPECT_TRUE(isUPNMatch("user@com", "hostname.region.domain.com"));
}
} // namespace
