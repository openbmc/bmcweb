
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
    bmcweb::Response res;
    {
        // No protocol, schema on url
        std::optional<boost::urls::url> ret =
            parseSimpleUpdateUrl("tftp://1.1.1.1/path", std::nullopt, res);
        ASSERT_TRUE(ret);
        if (!ret)
        {
            return;
        }
        EXPECT_EQ(ret->encoded_host_and_port(), "1.1.1.1");
        EXPECT_EQ(ret->encoded_path(), "/path");
        EXPECT_EQ(ret->scheme(), "tftp");
    }
    {
        // Protocol, no schema on url
        std::optional<boost::urls::url> ret =
            parseSimpleUpdateUrl("1.1.1.1/path", "TFTP", res);
        ASSERT_TRUE(ret);
        if (!ret)
        {
            return;
        }
        EXPECT_EQ(ret->encoded_host_and_port(), "1.1.1.1");
        EXPECT_EQ(ret->encoded_path(), "/path");
        EXPECT_EQ(ret->scheme(), "tftp");
    }
    {
        // Both protocol and schema on url
        std::optional<boost::urls::url> ret =
            parseSimpleUpdateUrl("tftp://1.1.1.1/path", "TFTP", res);
        ASSERT_TRUE(ret);
        if (!ret)
        {
            return;
        }
        EXPECT_EQ(ret->encoded_host_and_port(), "1.1.1.1");
        EXPECT_EQ(ret->encoded_path(), "/path");
        EXPECT_EQ(ret->scheme(), "tftp");
    }
}

TEST(UpdateService, ParseHTTPSPostitive)
{
    bmcweb::Response res;
    {
        // No protocol, schema on url
        std::optional<boost::urls::url> ret =
            parseSimpleUpdateUrl("https://1.1.1.1/path", std::nullopt, res);
        ASSERT_TRUE(ret);
        if (!ret)
        {
            return;
        }
        EXPECT_EQ(ret->encoded_host_and_port(), "1.1.1.1");
        EXPECT_EQ(ret->encoded_path(), "/path");
        EXPECT_EQ(ret->scheme(), "https");
    }
    {
        // Protocol, no schema on url
        std::optional<boost::urls::url> ret =
            parseSimpleUpdateUrl("1.1.1.1/path", "HTTPS", res);
        ASSERT_TRUE(ret);
        if (!ret)
        {
            return;
        }
        EXPECT_EQ(ret->encoded_host_and_port(), "1.1.1.1");
        EXPECT_EQ(ret->encoded_path(), "/path");
        EXPECT_EQ(ret->scheme(), "https");
    }
    {
        // Both protocol and schema on url with path
        std::optional<boost::urls::url> ret =
            parseSimpleUpdateUrl("https://1.1.1.1/path", "HTTPS", res);
        ASSERT_TRUE(ret);
        if (!ret)
        {
            return;
        }
        EXPECT_EQ(ret->encoded_host_and_port(), "1.1.1.1");
        EXPECT_EQ(ret->encoded_path(), "/path");
        EXPECT_EQ(ret->scheme(), "https");
    }
    {
        // Both protocol and schema on url without path
        std::optional<boost::urls::url> ret =
            parseSimpleUpdateUrl("https://1.1.1.1", "HTTPS", res);
        ASSERT_TRUE(ret);
        if (!ret)
        {
            return;
        }
        EXPECT_EQ(ret->encoded_host_and_port(), "1.1.1.1");
        EXPECT_EQ(ret->encoded_path(), "/");
        EXPECT_EQ(ret->scheme(), "https");
    }
    {
        // Both protocol and schema on url without path
        std::optional<boost::urls::url> ret =
            parseSimpleUpdateUrl("https://[2001:db8::1]", "HTTPS", res);
        ASSERT_TRUE(ret);
        if (!ret)
        {
            return;
        }
        EXPECT_EQ(ret->encoded_host_and_port(), "[2001:db8::1]");
        EXPECT_EQ(ret->encoded_path(), "/");
        EXPECT_EQ(ret->scheme(), "https");
    }
}

TEST(UpdateService, ParseTFTPNegative)
{
    bmcweb::Response res;
    // No protocol, no schema
    ASSERT_EQ(parseSimpleUpdateUrl("1.1.1.1/path", std::nullopt, res),
              std::nullopt);
    // No host
    ASSERT_EQ(parseSimpleUpdateUrl("/path", "TFTP", res), std::nullopt);

    // No host
    ASSERT_EQ(parseSimpleUpdateUrl("path", "TFTP", res), std::nullopt);

    // No path
    ASSERT_EQ(parseSimpleUpdateUrl("tftp://1.1.1.1", "TFTP", res),
              std::nullopt);
    ASSERT_EQ(parseSimpleUpdateUrl("tftp://1.1.1.1/", "TFTP", res),
              std::nullopt);
}
} // namespace
} // namespace redfish
