#include "http_utility.hpp"

#include <gtest/gtest.h> // IWYU pragma: keep

// IWYU pragma: no_include <gtest/gtest-message.h>
// IWYU pragma: no_include <gtest/gtest-test-part.h>
// IWYU pragma: no_include "gtest/gtest_pred_impl.h"

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

TEST(IsOctetAccepted, ContainsAnyMimeTypeReturnsTrue)
{
    EXPECT_TRUE(http_helpers::isOctetAccepted("text/html, */*"));
}

TEST(IsOctetAccepted, ContainsQFactorWeightingReturnsTrue)
{
    EXPECT_TRUE(http_helpers::isOctetAccepted("text/html, */*;q=0.8"));
}

TEST(IsOctetAccepted, NoOctetReturnsFalse)
{
    EXPECT_FALSE(isOctetAccepted("text/html, application/json"));
    EXPECT_FALSE(isOctetAccepted("application/json"));
}

} // namespace
} // namespace http_helpers
