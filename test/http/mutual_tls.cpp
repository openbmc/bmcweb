// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "mutual_tls.hpp"

#include "mutual_tls_private.hpp"
#include "sessions.hpp"

#include <cstring>
#include <string>

extern "C"
{
#include <openssl/asn1.h>
#include <openssl/ec.h>
#include <openssl/evp.h>
#include <openssl/obj_mac.h>
#include <openssl/objects.h>
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
