#include "http_utility.hpp"

#include <array>

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

TEST(getPreferredContentType, PositiveTest)
{
    std::array<ContentType, 1> contentType{ContentType::HTML};
    EXPECT_EQ(
        getPreferredContentType("text/html, application/json", contentType),
        ContentType::HTML);

    std::array<ContentType, 2> htmlJson{ContentType::HTML, ContentType::JSON};
    EXPECT_EQ(getPreferredContentType("text/html, application/json", htmlJson),
              ContentType::HTML);

    // String the chrome gives
    EXPECT_EQ(getPreferredContentType(
                  "text/html,"
                  "application/xhtml+xml,"
                  "application/xml;q=0.9,"
                  "image/avif,"
                  "image/webp,"
                  "image/apng,*/*;q=0.8,"
                  "application/signed-exchange;v=b3;q=0.7",
                  htmlJson),
              ContentType::HTML);

    std::array<ContentType, 2> jsonHtml{ContentType::JSON, ContentType::HTML};
    EXPECT_EQ(getPreferredContentType("text/html, application/json", jsonHtml),
              ContentType::HTML);

    std::array<ContentType, 2> cborJson{ContentType::CBOR, ContentType::JSON};
    EXPECT_EQ(getPreferredContentType("application/cbor, application::json",
                                      cborJson),
              ContentType::CBOR);

    EXPECT_EQ(getPreferredContentType("application/json", cborJson),
              ContentType::JSON);
    EXPECT_EQ(getPreferredContentType("*/*", cborJson), ContentType::ANY);

    // Application types with odd characters
    EXPECT_EQ(getPreferredContentType(
                  "application/prs.nprend, application/json", cborJson),
              ContentType::JSON);

    EXPECT_EQ(getPreferredContentType("application/rdf+xml, application/json",
                                      cborJson),
              ContentType::JSON);

    // Q values are ignored, but should parse
    EXPECT_EQ(getPreferredContentType(
                  "application/rdf+xml;q=0.9, application/json", cborJson),
              ContentType::JSON);
    EXPECT_EQ(getPreferredContentType(
                  "application/rdf+xml;q=1, application/json", cborJson),
              ContentType::JSON);
    EXPECT_EQ(getPreferredContentType("application/json;q=0.9", cborJson),
              ContentType::JSON);
    EXPECT_EQ(getPreferredContentType("application/json;q=1", cborJson),
              ContentType::JSON);
}

TEST(getPreferredContentType, NegativeTest)
{
    std::array<ContentType, 1> contentType{ContentType::CBOR};
    EXPECT_EQ(
        getPreferredContentType("text/html, application/json", contentType),
        ContentType::NoMatch);
}

TEST(getPreferredEncoding, PositiveTest)
{
    std::array<Encoding, 1> encodingsGzip{Encoding::GZIP};
    EXPECT_EQ(getPreferredEncoding("gzip", encodingsGzip), Encoding::GZIP);

    std::array<Encoding, 2> encodingsGzipZstd{Encoding::GZIP, Encoding::ZSTD};
    EXPECT_EQ(getPreferredEncoding("gzip", encodingsGzipZstd), Encoding::GZIP);
    EXPECT_EQ(getPreferredEncoding("zstd", encodingsGzipZstd), Encoding::ZSTD);

    EXPECT_EQ(getPreferredEncoding("*", encodingsGzipZstd), Encoding::GZIP);

    EXPECT_EQ(getPreferredEncoding("zstd, gzip;q=1.0", encodingsGzipZstd),
              Encoding::ZSTD);
}

TEST(getPreferredEncoding, NegativeTest)
{
    std::array<Encoding, 2> contentType{Encoding::GZIP, Encoding::UnencodedBytes};
    EXPECT_EQ(getPreferredEncoding("noexist", contentType),
              Encoding::UnencodedBytes);
}

} // namespace
} // namespace http_helpers
