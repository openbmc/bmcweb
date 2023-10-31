#include "http/mutual_tls_meta.hpp"

#include <gtest/gtest.h> // IWYU pragma: keep

namespace redfish
{
namespace
{

TEST(MetaParseSslUser, userTest)
{
    std::string sslUserOut;
    std::string sslUser = "user:kawajiri/hostname.facebook.com";

    EXPECT_EQ(mtlsMetaParseSslUser(sslUser, sslUserOut), 0);
    EXPECT_EQ(sslUserOut, "user_kawajiri");
}

TEST(MetaParseSslUser, svcTest)
{
    std::string sslUserOut;
    std::string sslUser = "svc:an_internal.service";

    EXPECT_EQ(mtlsMetaParseSslUser(sslUser, sslUserOut), 0);
    EXPECT_EQ(sslUserOut, "svc_an_internal.service");
}

TEST(MetaParseSslUser, hostTest)
{
    std::string sslUserOut;
    std::string sslUser = "host:/ab12345.cd0.facebook.com";

    EXPECT_EQ(mtlsMetaParseSslUser(sslUser, sslUserOut), 0);
    EXPECT_EQ(sslUserOut, "host_ab12345.cd0");
}

TEST(MetaParseSslUser, hostTestSuffixes)
{
    std::string sslUserOut;
    std::vector<std::string> sslUsers = {
        "host:/hostname.facebook.com",
        "host:/hostname.tfbnw.net",
        "host:/hostname.thefacebook.com",
    };

    for (std::string sslUser : sslUsers)
    {
        EXPECT_EQ(mtlsMetaParseSslUser(sslUser, sslUserOut), 0);
        EXPECT_EQ(sslUserOut, "host_hostname"); // Must strip suffix
    }
}

TEST(MetaParseSslUser, invalidUsers)
{
    int ret = 0;
    std::string sslUserOut;
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
        ret = mtlsMetaParseSslUser(sslUser, sslUserOut);
        if (ret != -1)
        {
            FAIL() << "mtlsMetaParseSslUser() unexpectedly returned " << ret
                   << " when parsing sslUser='" << sslUser << "', sslUserOut='"
                   << sslUserOut << "'";
        }
    }
}

} // namespace
} // namespace redfish
