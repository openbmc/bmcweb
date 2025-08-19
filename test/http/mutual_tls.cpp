// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "mutual_tls.hpp"

#include "mutual_tls_private.hpp"
#include "ossl_wrappers.hpp"
#include "sessions.hpp"

#include <cstring>
#include <string>

extern "C"
{
#include <openssl/asn1.h>
#include <openssl/obj_mac.h>
#include <openssl/objects.h>
#include <openssl/types.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
}

#include <boost/asio/ip/address.hpp>
#include <boost/asio/ssl/verify_context.hpp>

#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::IsNull;
using ::testing::NotNull;

namespace
{

TEST(MutualTLS, GoodCert)
{
    OpenSSLX509 x509;

    x509.setSubjectName();

    OpenSSLX509Extension ex(nullptr, NID_key_usage,
                                             "digitalSignature, keyAgreement");
    ASSERT_THAT(ex.get(), NotNull());
    ASSERT_EQ(X509_add_ext(x509.get(), ex.get(), -1), 1);
    OpenSSLX509Extension ex2( nullptr, NID_ext_key_usage, "clientAuth");
    ASSERT_THAT(ex2.get(), NotNull());
    ASSERT_EQ(X509_add_ext(x509.get(), ex2.get(), -1), 1);

    ASSERT_TRUE(x509.sign());

    OpenSSLX509StoreCTX x509Store;
    x509Store.setCurrentCert(x509);

    boost::asio::ip::address ip;
    boost::asio::ssl::verify_context ctx = x509Store.releaseToVerifyContext();
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
        OpenSSLX509 x509;
        x509.setSubjectName();

        OpenSSLX509Extension ex(nullptr, NID_key_usage, usageString);
        ASSERT_THAT(ex.get(), NotNull());
        ASSERT_EQ(X509_add_ext(x509.get(), ex.get(), -1), 1);
        OpenSSLX509Extension ex2(nullptr, NID_ext_key_usage, "clientAuth");
        ASSERT_THAT(ex2.get(), NotNull());
        ASSERT_EQ(X509_add_ext(x509.get(), ex2.get(), -1), 1);
        ASSERT_TRUE(x509.sign());

        OpenSSLX509StoreCTX x509Store;
        x509Store.setCurrentCert(x509);

        boost::asio::ip::address ip;
        boost::asio::ssl::verify_context ctx =
            x509Store.releaseToVerifyContext();
        std::shared_ptr<persistent_data::UserSession> session =
            verifyMtlsUser(ip, ctx);
        ASSERT_THAT(session, NotNull());
    }
}

TEST(MutualTLS, MissingCert)
{
    OpenSSLX509StoreCTX x509Store;

    boost::asio::ip::address ip;
    boost::asio::ssl::verify_context ctx = x509Store.releaseToVerifyContext();
    std::shared_ptr<persistent_data::UserSession> session =
        verifyMtlsUser(ip, ctx);
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

    constexpr std::string_view user = "user@domain.com";
    OpenSSLASN1String ia5(user);

    OpenSSLGeneralNames gens;
    OpenSSLGeneralName gen;
    gen.set0Value(GEN_EMAIL, ia5);
    ASSERT_EQ(sk_GENERAL_NAME_push(gens.get(), gen.release()), 1);

    ASSERT_EQ(X509_add1_ext_i2d(x509.get(), NID_subject_alt_name, gens.get(), 0, 0),
              1);

    std::string upn = getUPNFromCert(x509.get(), "hostname.domain.com");
    EXPECT_THAT(upn, "");
}

TEST(GetUPNFromCert, NonUPNSubjectAlternativeName)
{
    OpenSSLX509 x509;

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
    OpenSSLX509 x509;

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
    OpenSSLX509 x509;

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
