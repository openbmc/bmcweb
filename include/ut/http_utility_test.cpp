#include <http_utility.hpp>

#include "gmock/gmock.h"

TEST(HttpUtility, requestPrefersHtml)
{
    EXPECT_FALSE(
        http_helpers::requestPrefersHtml("*/*, application/octet-stream"));
    EXPECT_TRUE(http_helpers::isOctetAccepted("*/*, application/octet-stream"));

    EXPECT_TRUE(http_helpers::isOctetAccepted("text/html, */*;q=0.8"));
    EXPECT_TRUE(http_helpers::isOctetAccepted("application/xhtml+xml,application/octet-stream,text/html"));

    EXPECT_TRUE(
        http_helpers::requestPrefersHtml("text/html, application/json"));
    EXPECT_FALSE(http_helpers::isOctetAccepted("text/html, application/json"));

    EXPECT_FALSE(http_helpers::requestPrefersHtml("application/json"));
    EXPECT_FALSE(http_helpers::isOctetAccepted("application/json"));
}
