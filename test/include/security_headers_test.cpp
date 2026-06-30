// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "http_response.hpp"
#include "security_headers.hpp"

#include <boost/beast/http/field.hpp>

#include <array>
#include <cstddef>
#include <iterator>
#include <string_view>

#include <gtest/gtest.h>

namespace
{

using bf = boost::beast::http::field;

std::size_t headerCount(const crow::Response& res)
{
    return static_cast<std::size_t>(
        std::distance(res.fields().begin(), res.fields().end()));
}

constexpr std::string_view expectedContentSecurityPolicy =
    "default-src 'none'; "
    "img-src 'self' data:; "
    "font-src 'self'; "
    "style-src 'self'; "
    "script-src 'self'; "
    "connect-src 'self' wss:; "
    "form-action 'none'; "
    "frame-ancestors 'none'; "
    "object-src 'none'; "
    "base-uri 'none'";

constexpr std::string_view expectedPermissionsPolicy =
    "accelerometer=(),"
    "ambient-light-sensor=(),"
    "autoplay=(),"
    "battery=(),"
    "camera=(),"
    "display-capture=(),"
    "document-domain=(),"
    "encrypted-media=(),"
    "fullscreen=(),"
    "gamepad=(),"
    "geolocation=(),"
    "gyroscope=(),"
    "layout-animations=(self),"
    "legacy-image-formats=(self),"
    "magnetometer=(),"
    "microphone=(),"
    "midi=(),"
    "oversized-images=(self),"
    "payment=(),"
    "picture-in-picture=(),"
    "publickey-credentials-get=(),"
    "speaker-selection=(),"
    "sync-xhr=(self),"
    "unoptimized-images=(self),"
    "unsized-media=(self),"
    "usb=(),"
    "screen-wak-lock=(),"
    "web-share=(),"
    "xr-spatial-tracking=()";

void expectCommonHeaders(const crow::Response& res,
                         std::string_view expectedCacheControl)
{
    EXPECT_EQ(res.getHeaderValue(bf::strict_transport_security),
              "max-age=31536000; includeSubdomains");
    EXPECT_EQ(res.getHeaderValue(bf::pragma), "no-cache");
    EXPECT_EQ(res.getHeaderValue(bf::cache_control), expectedCacheControl);
    EXPECT_EQ(res.getHeaderValue("X-Content-Type-Options"), "nosniff");
}

void expectHtmlHeadersSet(const crow::Response& res)
{
    EXPECT_EQ(res.getHeaderValue(bf::x_frame_options), "DENY");
    EXPECT_EQ(res.getHeaderValue("Referrer-Policy"), "no-referrer");
    EXPECT_EQ(res.getHeaderValue("Permissions-Policy"),
              expectedPermissionsPolicy);
    EXPECT_EQ(res.getHeaderValue("X-Permitted-Cross-Domain-Policies"), "none");
    EXPECT_EQ(res.getHeaderValue("Cross-Origin-Embedder-Policy"),
              "require-corp");
    EXPECT_EQ(res.getHeaderValue("Cross-Origin-Opener-Policy"), "same-origin");
    EXPECT_EQ(res.getHeaderValue("Cross-Origin-Resource-Policy"),
              "same-origin");
    EXPECT_EQ(res.getHeaderValue("Content-Security-Policy"),
              expectedContentSecurityPolicy);
}

void expectHtmlHeadersNotSet(const crow::Response& res)
{
    EXPECT_EQ(res.getHeaderValue(bf::x_frame_options), "");
    EXPECT_EQ(res.getHeaderValue("Referrer-Policy"), "");
    EXPECT_EQ(res.getHeaderValue("Permissions-Policy"), "");
    EXPECT_EQ(res.getHeaderValue("Content-Security-Policy"), "");
    EXPECT_EQ(res.getHeaderValue("Cross-Origin-Embedder-Policy"), "");
    EXPECT_EQ(res.getHeaderValue("Cross-Origin-Opener-Policy"), "");
    EXPECT_EQ(res.getHeaderValue("Cross-Origin-Resource-Policy"), "");
    EXPECT_EQ(res.getHeaderValue("X-Permitted-Cross-Domain-Policies"), "");
}

TEST(SecurityHeaders, DefaultResponseNoContentType)
{
    crow::Response res;
    addSecurityHeaders(res);

    EXPECT_EQ(headerCount(res), 4);
    expectCommonHeaders(res, "no-store, max-age=0");
    expectHtmlHeadersNotSet(res);
}

TEST(SecurityHeaders, CacheControlNotOverwrittenWhenPresent)
{
    crow::Response res;
    res.addHeader(bf::cache_control, "public, max-age=3600");
    addSecurityHeaders(res);

    EXPECT_EQ(headerCount(res), 4);
    expectCommonHeaders(res, "public, max-age=3600");
}

TEST(SecurityHeaders, HtmlHeadersNotSetForJsonContentType)
{
    crow::Response res;
    res.addHeader("Content-Type", "application/json");
    addSecurityHeaders(res);

    EXPECT_EQ(headerCount(res), 5);
    expectCommonHeaders(res, "no-store, max-age=0");
    EXPECT_EQ(res.getHeaderValue("Content-Type"), "application/json");
    expectHtmlHeadersNotSet(res);
}

TEST(SecurityHeaders, HtmlHeadersSetForMatchingContentTypes)
{
    for (std::string_view contentType : std::to_array<std::string_view>(
             {"text/html", "text/html; charset=utf-8", "text/htmlx"}))
    {
        crow::Response res;
        res.addHeader("Content-Type", contentType);
        addSecurityHeaders(res);

        EXPECT_EQ(headerCount(res), 13) << "Content-Type: " << contentType;
        expectCommonHeaders(res, "no-store, max-age=0");
        EXPECT_EQ(res.getHeaderValue("Content-Type"), contentType)
            << "Content-Type: " << contentType;
        expectHtmlHeadersSet(res);
    }
}

} // namespace
