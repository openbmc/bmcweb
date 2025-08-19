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

    ASSERT_EQ(x509.addExt(NID_key_usage, "digitalSignature, keyAgreement"), 0);
    ASSERT_EQ(x509.addExt(NID_ext_key_usage, "clientAuth"), 0);

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

        ASSERT_EQ(x509.addExt(NID_key_usage, usageString), 0);
        ASSERT_EQ(x509.addExt(NID_ext_key_usage, "clientAuth"), 0);
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
    OpenSSLGeneralName gen(GEN_EMAIL, ia5);
    ASSERT_TRUE(gens.push(std::move(gen)));

    ASSERT_EQ(x509.add1ExtI2d(NID_subject_alt_name, gens), 1);
    std::string upn = getUPNFromCert(x509.get(), "hostname.domain.com");
    EXPECT_THAT(upn, "");
}

TEST(GetUPNFromCert, NonUPNSubjectAlternativeName)
{
    OpenSSLX509 x509;
    OpenSSLGeneralNames gens;
    OpenSSLASN1String upnstr("user@domain.com");
    OpenSSLGeneralName gen(NID_SRVName, upnstr);
    // ASSERT_TRUE(gens.push(std::move(gen)));

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

    ASSERT_EQ(GENERAL_NAME_set0_othername(gen.ptr, othType, value), 1);
    ASSERT_TRUE(gens.push(std::move(gen)));

    ASSERT_EQ(x509.add1ExtI2d(NID_subject_alt_name, gens), 1);
    std::string upn = getUPNFromCert(x509.get(), "hostname.domain.com");
    EXPECT_THAT(upn, "");
}

TEST(GetUPNFromCert, NonUTF8UPNSubjectAlternativeName)
{
    OpenSSLX509 x509;
    OpenSSLGeneralNames gens;

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
    OpenSSLGeneralName gen;

    ASSERT_EQ(GENERAL_NAME_set0_othername(gen.ptr, othType, value), 1);
    ASSERT_TRUE(gens.push(std::move(gen)));
    ASSERT_EQ(x509.add1ExtI2d(NID_subject_alt_name, gens), 1);

    std::string upn = getUPNFromCert(x509.get(), "hostname.domain.com");
    EXPECT_THAT(upn, "");
}

TEST(GetUPNFromCert, ValidUPN)
{
    OpenSSLX509 x509;
    OpenSSLGeneralNames gens;

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

    OpenSSLGeneralName gen;
    ASSERT_EQ(GENERAL_NAME_set0_othername(gen.ptr, othType, value), 1);
    ASSERT_TRUE(gens.push(std::move(gen)));
    ASSERT_EQ(x509.add1ExtI2d(NID_subject_alt_name, gens), 1);

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
} // namespace
