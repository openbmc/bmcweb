#include "http_utility.hpp"

#include <gtest/gtest.h>

namespace http_helpers
{
namespace
{


TEST(HttpUtility, requestPrefersHtml)
{
    EXPECT_FALSE(requestPrefersHtml("*/*, application/octet-stream"));
    EXPECT_TRUE(isOctetAccepted("*/*, application/octet-stream"));

    EXPECT_TRUE(requestPrefersHtml("text/html, application/json"));
    EXPECT_FALSE(isOctetAccepted("text/html, application/json"));

    EXPECT_FALSE(requestPrefersHtml("application/json"));
    EXPECT_FALSE(isOctetAccepted("application/json"));
}

} // namespace
} // namespace http_helpers