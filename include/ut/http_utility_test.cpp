#include "http_utility.hpp"

#include <gtest/gtest.h>

namespace http_helpers
{
namespace
{

TEST(RequestPrefersHtml, ContainsHtmlReturnsTrue)
{
    EXPECT_TRUE(requestPrefersHtml("text/html, application/json"));
}

TEST(RequestPrefersHtml, NoHtmlReturnsFalse)
{
    EXPECT_FALSE(requestPrefersHtml("*/*, application/octet-stream"));
    EXPECT_FALSE(requestPrefersHtml("application/json"));
}

TEST(IsOctetAccepted, ContainsOctetReturnsTrue)
{
    EXPECT_TRUE(isOctetAccepted("*/*, application/octet-stream"));
}

TEST(IsOctetAccepted, NoOctetReturnsFalse)
{
    EXPECT_FALSE(isOctetAccepted("text/html, application/json"));
    EXPECT_FALSE(isOctetAccepted("application/json"));
}

} // namespace
} // namespace http_helpers