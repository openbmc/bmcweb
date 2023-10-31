#include "http/mutual_tls_meta.hpp"

#include <gtest/gtest.h> // IWYU pragma: keep

namespace redfish
{
namespace
{

TEST(MetaParseSslUser, userTest)
{
    std::string sslUser = "user:kawajiri/hostname.facebook.com";
    EXPECT_EQ(mtlsMetaParseSslUser(sslUser), "user_kawajiri");
}

TEST(MetaParseSslUser, svcTest)
{
    std::string sslUser = "svc:an_internal.service";
    EXPECT_EQ(mtlsMetaParseSslUser(sslUser), "svc_an_internal.service");
}

TEST(MetaParseSslUser, hostTest)
{
    std::string sslUser = "host:/ab12345.cd0.facebook.com";

    EXPECT_EQ(mtlsMetaParseSslUser(sslUser), "host_ab12345.cd0");
}

TEST(MetaParseSslUser, hostTestSuffixes)
{
    std::vector<std::string> sslUsers = {
        "host:/hostname.facebook.com",
        "host:/hostname.tfbnw.net",
        "host:/hostname.thefacebook.com",
    };

    for (const std::string& sslUser : sslUsers)
    {
        // Must strip suffix
        EXPECT_EQ(mtlsMetaParseSslUser(sslUser), "host_hostname");
    }
}

TEST(MetaParseSslUser, invalidUsers)
{
    std::vector<std::string> invalidSslUsers = {
        "",
        ":",
        ":/",
        "ijslakd",
        "user:",
        "user:/",
        "user:/hostname.facebook.com",
        "user:/hostname.facebook.c om",
        "user:a/hostname.facebook.c om",
        "user:a//hostname.facebook.com",
        "user: space/hostname.facebook.com",
        "user:space/hostname.facebook.com ",
        "svc:",
        "svc:/",
        "svc:/hostname.facebook.com",
        "host:/",
        "host:unexpected_user/",
    };

    for (std::string sslUser : invalidSslUsers)
    {
        EXPECT_EQ(mtlsMetaParseSslUser(sslUser), std::nullopt);
    }
}

} // namespace
} // namespace redfish
