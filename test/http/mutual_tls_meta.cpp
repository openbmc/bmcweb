#include "http/mutual_tls_meta.hpp"

#include <optional>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

TEST(MetaParseSslUser, userTest)
{
    std::string sslUser = "user:kawajiri/hostname.facebook.com";
    EXPECT_EQ(mtlsMetaParseSslUser(sslUser), "kawajiri");
}

TEST(MetaParseSslUser, userNohostnameTest)
{
    // hostname is optional
    std::string sslUser = "user:kawajiri";
    EXPECT_EQ(mtlsMetaParseSslUser(sslUser), "kawajiri");
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
        "user: space/hostname.facebook.com",
        "svc:",
        "svc:/",
        "svc:/hostname.facebook.com",
        "host:/",
        "host:unexpected_user/",
    };

    for (const std::string& sslUser : invalidSslUsers)
    {
        EXPECT_EQ(mtlsMetaParseSslUser(sslUser), std::nullopt);
    }
}

} // namespace
} // namespace redfish
