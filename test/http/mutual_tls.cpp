#include "mutual_tls.hpp"

#include "sessions.hpp"

extern "C"
{
#include <openssl/asn1.h>
#include <openssl/ec.h>
#include <openssl/evp.h>
#include <openssl/obj_mac.h>
#include <openssl/types.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
#include <openssl/x509v3.h>
}

#include <boost/asio/ip/address.hpp>
#include <boost/asio/ssl/verify_context.hpp>

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
        ASSERT_EQ(X509_set_pubkey(ptr, pkey), 1);
        ASSERT_GT(X509_sign(ptr, pkey, EVP_sha256()), 0);
        EVP_PKEY_free(pkey);
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

class OSSLX509StoreCTX
{
    X509_STORE_CTX* ptr = X509_STORE_CTX_new();

  public:
    OSSLX509StoreCTX& operator=(const OSSLX509StoreCTX&) = delete;
    OSSLX509StoreCTX& operator=(OSSLX509StoreCTX&&) = delete;

    OSSLX509StoreCTX(const OSSLX509StoreCTX&) = delete;
    OSSLX509StoreCTX(OSSLX509StoreCTX&&) = delete;

    OSSLX509StoreCTX() = default;
    X509_STORE_CTX* get()
    {
        return ptr;
    }
    ~OSSLX509StoreCTX()
    {
        X509_STORE_CTX_free(ptr);
    }
};

TEST(MutualTLS, GoodCert)
{
    OSSLX509 x509;

    x509.setSubjectName();
    X509_EXTENSION* ex = X509V3_EXT_conf_nid(nullptr, nullptr, NID_key_usage,
                                             "digitalSignature, keyAgreement");
    ASSERT_THAT(ex, NotNull());
    ASSERT_EQ(X509_add_ext(x509.get(), ex, -1), 1);
    X509_EXTENSION_free(ex);
    ex = X509V3_EXT_conf_nid(nullptr, nullptr, NID_ext_key_usage, "clientAuth");
    ASSERT_THAT(ex, NotNull());
    ASSERT_EQ(X509_add_ext(x509.get(), ex, -1), 1);
    X509_EXTENSION_free(ex);

    x509.sign();

    OSSLX509StoreCTX x509Store;
    X509_STORE_CTX_set_current_cert(x509Store.get(), x509.get());

    boost::asio::ip::address ip;
    boost::asio::ssl::verify_context ctx(x509Store.get());
    std::shared_ptr<persistent_data::UserSession> session =
        verifyMtlsUser(ip, ctx);
    ASSERT_THAT(session, NotNull());
    EXPECT_THAT(session->username, "user");
}

TEST(MutualTLS, MissingKeyUsage)
{
    for (const char* usageString :
         {"digitalSignature", "keyAgreement", "digitalSignature, keyAgreement"})
    {
        OSSLX509 x509;
        x509.setSubjectName();

        X509_EXTENSION* ex =
            X509V3_EXT_conf_nid(nullptr, nullptr, NID_key_usage, usageString);

        ASSERT_THAT(ex, NotNull());
        ASSERT_EQ(X509_add_ext(x509.get(), ex, -1), 1);
        X509_EXTENSION_free(ex);
        ex = X509V3_EXT_conf_nid(nullptr, nullptr, NID_ext_key_usage,
                                 "clientAuth");
        ASSERT_THAT(ex, NotNull());
        ASSERT_EQ(X509_add_ext(x509.get(), ex, -1), 1);
        X509_EXTENSION_free(ex);
        x509.sign();

        OSSLX509StoreCTX x509Store;
        X509_STORE_CTX_set_current_cert(x509Store.get(), x509.get());

        boost::asio::ip::address ip;
        boost::asio::ssl::verify_context ctx(x509Store.get());
        std::shared_ptr<persistent_data::UserSession> session =
            verifyMtlsUser(ip, ctx);
        ASSERT_THAT(session, NotNull());
    }
}

TEST(MutualTLS, MissingCert)
{
    OSSLX509StoreCTX x509Store;

    boost::asio::ip::address ip;
    boost::asio::ssl::verify_context ctx(x509Store.get());
    std::shared_ptr<persistent_data::UserSession> session =
        verifyMtlsUser(ip, ctx);
    ASSERT_THAT(session, IsNull());
}
} // namespace
