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
    std::array<ContentType, 1> prefered{ContentType::JSON};
    EXPECT_EQ(getPreferedContentType("text/html, application/json", prefered),
              ContentType::JSON);
}

TEST(IsOctetAccepted, ContainsOctetReturnsTrue)
{
    std::array<ContentType, 1> prefered{ContentType::OctetStream};
    EXPECT_EQ(getPreferedContentType("*/*, application/octet-stream", prefered),
              ContentType::OctetStream);
}

} // namespace
} // namespace http_helpers
