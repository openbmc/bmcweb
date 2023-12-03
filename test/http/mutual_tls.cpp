#include "mutual_tls.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h> // IWYU pragma: keep

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

    X509_NAME* name = X509_get_subject_name(x509.get());
    std::array<unsigned char, 5> user = {'u', 's', 'e', 'r', '\0'};
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, user.data(), -1, -1,
                               0);

    X509_EXTENSION* ex = X509V3_EXT_conf_nid(nullptr, nullptr, NID_key_usage,
                                             "digitalSignature, keyAgreement");
    ASSERT_THAT(ex, NotNull());
    ASSERT_EQ(X509_add_ext(x509.get(), ex, -1), 1);
    ex = X509V3_EXT_conf_nid(nullptr, nullptr, NID_ext_key_usage, "clientAuth");
    ASSERT_THAT(ex, NotNull());
    ASSERT_EQ(X509_add_ext(x509.get(), ex, -1), 1);

    OSSLX509StoreCTX x509Store;
    X509_STORE_CTX_set_current_cert(x509Store.get(), x509.get());

    boost::asio::ip::address ip;
    boost::asio::ssl::verify_context ctx(x509Store.get());
    std::shared_ptr<persistent_data::UserSession> session = verifyMtlsUser(ip,
                                                                           ctx);
    ASSERT_THAT(session, NotNull());
    EXPECT_THAT(session->username, "user");
}

TEST(MutualTLS, MissingSubject)
{
    OSSLX509 x509;

    X509_EXTENSION* ex = X509V3_EXT_conf_nid(nullptr, nullptr, NID_key_usage,
                                             "digitalSignature, keyAgreement");
    ASSERT_THAT(ex, NotNull());
    ASSERT_EQ(X509_add_ext(x509.get(), ex, -1), 1);
    ex = X509V3_EXT_conf_nid(nullptr, nullptr, NID_ext_key_usage, "clientAuth");
    ASSERT_THAT(ex, NotNull());
    ASSERT_EQ(X509_add_ext(x509.get(), ex, -1), 1);

    OSSLX509StoreCTX x509Store;
    X509_STORE_CTX_set_current_cert(x509Store.get(), x509.get());

    boost::asio::ip::address ip;
    boost::asio::ssl::verify_context ctx(x509Store.get());
    std::shared_ptr<persistent_data::UserSession> session = verifyMtlsUser(ip,
                                                                           ctx);
    ASSERT_THAT(session, IsNull());
}

TEST(MutualTLS, MissingKeyUsage)
{
    for (const char* usageString : {"digitalSignature", "keyAgreement"})
    {
        OSSLX509 x509;

        X509_EXTENSION* ex = X509V3_EXT_conf_nid(nullptr, nullptr,
                                                 NID_key_usage, usageString);

        ASSERT_THAT(ex, NotNull());
        ASSERT_EQ(X509_add_ext(x509.get(), ex, -1), 1);
        ex = X509V3_EXT_conf_nid(nullptr, nullptr, NID_ext_key_usage,
                                 "clientAuth");
        ASSERT_THAT(ex, NotNull());
        ASSERT_EQ(X509_add_ext(x509.get(), ex, -1), 1);

        OSSLX509StoreCTX x509Store;
        X509_STORE_CTX_set_current_cert(x509Store.get(), x509.get());

        boost::asio::ip::address ip;
        boost::asio::ssl::verify_context ctx(x509Store.get());
        std::shared_ptr<persistent_data::UserSession> session =
            verifyMtlsUser(ip, ctx);
        ASSERT_THAT(session, IsNull());
    }
}

TEST(MutualTLS, MissingExtKeyUsage)
{
    OSSLX509 x509;

    X509_EXTENSION* ex = X509V3_EXT_conf_nid(nullptr, nullptr, NID_key_usage,
                                             "digitalSignature, keyAgreement");

    ASSERT_THAT(ex, NotNull());
    ASSERT_EQ(X509_add_ext(x509.get(), ex, -1), 1);

    OSSLX509StoreCTX x509Store;
    X509_STORE_CTX_set_current_cert(x509Store.get(), x509.get());

    boost::asio::ip::address ip;
    boost::asio::ssl::verify_context ctx(x509Store.get());
    std::shared_ptr<persistent_data::UserSession> session = verifyMtlsUser(ip,
                                                                           ctx);
    ASSERT_THAT(session, IsNull());
}

TEST(MutualTLS, MissingCert)
{
    OSSLX509StoreCTX x509Store;

    boost::asio::ip::address ip;
    boost::asio::ssl::verify_context ctx(x509Store.get());
    std::shared_ptr<persistent_data::UserSession> session = verifyMtlsUser(ip,
                                                                           ctx);
    ASSERT_THAT(session, IsNull());
}
} // namespace
