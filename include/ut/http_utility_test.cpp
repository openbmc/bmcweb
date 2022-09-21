#include "http_utility.hpp"

#include <gtest/gtest.h> // IWYU pragma: keep

// IWYU pragma: no_include <gtest/gtest-message.h>
// IWYU pragma: no_include <gtest/gtest-test-part.h>
// IWYU pragma: no_include "gtest/gtest_pred_impl.h"

namespace http_helpers
{
namespace
{

TEST(isContentTypeAllowed, PositiveTest)
{
    EXPECT_TRUE(isContentTypeAllowed("*/*", ContentType::HTML, true));
    EXPECT_TRUE(isContentTypeAllowed("application/octet-stream",
                                     ContentType::OctetStream, false));
    EXPECT_TRUE(isContentTypeAllowed("text/html", ContentType::HTML, false));
    EXPECT_TRUE(
        isContentTypeAllowed("application/json", ContentType::JSON, false));
    EXPECT_TRUE(
        isContentTypeAllowed("application/cbor", ContentType::CBOR, false));
    EXPECT_TRUE(isContentTypeAllowed("application/json, text/html",
                                     ContentType::HTML, false));
}

TEST(isContentTypeAllowed, NegativeTest)
{
    EXPECT_FALSE(isContentTypeAllowed("application/octet-stream",
                                      ContentType::HTML, false));
    EXPECT_FALSE(
        isContentTypeAllowed("application/html", ContentType::JSON, false));
    EXPECT_FALSE(
        isContentTypeAllowed("application/json", ContentType::CBOR, false));
    EXPECT_FALSE(
        isContentTypeAllowed("application/cbor", ContentType::HTML, false));
    EXPECT_FALSE(isContentTypeAllowed("application/json, text/html",
                                      ContentType::OctetStream, false));
}

TEST(isContentTypeAllowed, ContainsAnyMimeTypeReturnsTrue)
{
    EXPECT_TRUE(
        isContentTypeAllowed("text/html, */*", ContentType::OctetStream, true));
}

TEST(isContentTypeAllowed, ContainsQFactorWeightingReturnsTrue)
{
    EXPECT_TRUE(isContentTypeAllowed("text/html, */*;q=0.8",
                                     ContentType::OctetStream, true));
}

TEST(getPreferedContentType, PositiveTest)
{
    std::array<ContentType, 1> contentType{ContentType::HTML};
    EXPECT_EQ(
        getPreferedContentType("text/html, application/json", contentType),
        ContentType::HTML);

    std::array<ContentType, 2> htmlJson{ContentType::HTML, ContentType::JSON};
    EXPECT_EQ(getPreferedContentType("text/html, application/json", htmlJson),
              ContentType::HTML);

    std::array<ContentType, 2> jsonHtml{ContentType::JSON, ContentType::HTML};
    EXPECT_EQ(getPreferedContentType("text/html, application/json", jsonHtml),
              ContentType::HTML);

    std::array<ContentType, 2> cborJson{ContentType::CBOR, ContentType::JSON};
    EXPECT_EQ(
        getPreferedContentType("application/cbor, application::json", cborJson),
        ContentType::CBOR);

    EXPECT_EQ(getPreferedContentType("application/json", cborJson),
              ContentType::JSON);
    EXPECT_EQ(getPreferedContentType("*/*", cborJson), ContentType::ANY);
}

TEST(getPreferedContentType, NegativeTest)
{
    std::array<ContentType, 1> contentType{ContentType::CBOR};
    EXPECT_EQ(
        getPreferedContentType("text/html, application/json", contentType),
        ContentType::NoMatch);
}
} // namespace
} // namespace http_helpers
