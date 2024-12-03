
#include "http_response.hpp"
#include "update_service.hpp"

#include <boost/url/url.hpp>

#include <optional>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

TEST(UpdateService, ParseTFTPPostitive)
{
    crow::Response res;
    {
        // No protocol, schema on url
        std::optional<TftpUrl> ret =
            parseTftpUrl("tftp://1.1.1.1/path", std::nullopt, res);
        ASSERT_NE(ret, std::nullopt);
        EXPECT_EQ(ret->tftpServer, "1.1.1.1");
        EXPECT_EQ(ret->fwFile, "path");
    }
    {
        // Protocol, no schema on url
        std::optional<TftpUrl> ret = parseTftpUrl("1.1.1.1/path", "TFTP", res);
        ASSERT_NE(ret, std::nullopt);
        EXPECT_EQ(ret->tftpServer, "1.1.1.1");
        EXPECT_EQ(ret->fwFile, "path");
    }
    {
        // Both protocl and schema on url
        std::optional<TftpUrl> ret =
            parseTftpUrl("tftp://1.1.1.1/path", "TFTP", res);
        ASSERT_NE(ret, std::nullopt);
        EXPECT_EQ(ret->tftpServer, "1.1.1.1");
        EXPECT_EQ(ret->fwFile, "path");
    }
}

TEST(UpdateService, ParseTFTPNegative)
{
    crow::Response res;
    // No protocol, no schema
    ASSERT_EQ(parseTftpUrl("1.1.1.1/path", std::nullopt, res), std::nullopt);
    // No host
    ASSERT_EQ(parseTftpUrl("/path", "TFTP", res), std::nullopt);

    // No host
    ASSERT_EQ(parseTftpUrl("path", "TFTP", res), std::nullopt);

    // No path
    ASSERT_EQ(parseTftpUrl("tftp://1.1.1.1", "TFTP", res), std::nullopt);
    ASSERT_EQ(parseTftpUrl("tftp://1.1.1.1/", "TFTP", res), std::nullopt);
}
} // namespace
} // namespace redfish
