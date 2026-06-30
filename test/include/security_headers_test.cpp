// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "http_response.hpp"
#include "security_headers.hpp"

#include <boost/beast/http/field.hpp>

#include <array>
#include <iterator>
#include <string_view>

#include <gtest/gtest.h>

namespace
{

using bf = boost::beast::http::field;

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

TEST(SecurityHeaders, CommonHeadersAlwaysSet)
{
    crow::Response res;
    addSecurityHeaders(res);

    EXPECT_EQ(std::distance(res.fields().begin(), res.fields().end()), 4);
    EXPECT_EQ(res.getHeaderValue(bf::strict_transport_security),
              "max-age=31536000; includeSubdomains");
    EXPECT_EQ(res.getHeaderValue(bf::pragma), "no-cache");
    EXPECT_EQ(res.getHeaderValue("X-Content-Type-Options"), "nosniff");
}

TEST(SecurityHeaders, CacheControlSetWhenAbsent)
{
    crow::Response res;
    addSecurityHeaders(res);

    EXPECT_EQ(std::distance(res.fields().begin(), res.fields().end()), 4);
    EXPECT_EQ(res.getHeaderValue(bf::cache_control), "no-store, max-age=0");
}

TEST(SecurityHeaders, CacheControlNotOverwrittenWhenPresent)
{
    crow::Response res;
    res.addHeader(bf::cache_control, "public, max-age=3600");
    addSecurityHeaders(res);

    EXPECT_EQ(std::distance(res.fields().begin(), res.fields().end()), 4);
    EXPECT_EQ(res.getHeaderValue(bf::cache_control), "public, max-age=3600");
}

TEST(SecurityHeaders, NoContentTypeNoHtmlHeaders)
{
    crow::Response res;
    addSecurityHeaders(res);

    EXPECT_EQ(std::distance(res.fields().begin(), res.fields().end()), 4);
    EXPECT_EQ(res.getHeaderValue(bf::x_frame_options), "");
    EXPECT_EQ(res.getHeaderValue("Content-Security-Policy"), "");
}

TEST(SecurityHeaders, HtmlHeadersNotSetForNonHtml)
{
    crow::Response res;
    res.addHeader("Content-Type", "application/json");
    addSecurityHeaders(res);

    EXPECT_EQ(std::distance(res.fields().begin(), res.fields().end()), 5);
    EXPECT_EQ(res.getHeaderValue(bf::x_frame_options), "");
    EXPECT_EQ(res.getHeaderValue("Referrer-Policy"), "");
    EXPECT_EQ(res.getHeaderValue("Permissions-Policy"), "");
    EXPECT_EQ(res.getHeaderValue("Content-Security-Policy"), "");
    EXPECT_EQ(res.getHeaderValue("Cross-Origin-Embedder-Policy"), "");
    EXPECT_EQ(res.getHeaderValue("Cross-Origin-Opener-Policy"), "");
    EXPECT_EQ(res.getHeaderValue("Cross-Origin-Resource-Policy"), "");
    EXPECT_EQ(res.getHeaderValue("X-Permitted-Cross-Domain-Policies"), "");
}

TEST(SecurityHeaders, HtmlHeadersSetForHtmlContentTypes)
{
    constexpr std::array<std::string_view, 2> htmlContentTypes = {
        "text/html",
        "text/html; charset=utf-8",
    };

    for (std::string_view contentType : htmlContentTypes)
    {
        SCOPED_TRACE(contentType);

        crow::Response res;
        res.addHeader("Content-Type", contentType);
        addSecurityHeaders(res);

        EXPECT_EQ(std::distance(res.fields().begin(), res.fields().end()), 13);
        EXPECT_EQ(res.getHeaderValue(bf::x_frame_options), "DENY");
        EXPECT_EQ(res.getHeaderValue("Referrer-Policy"), "no-referrer");
        EXPECT_EQ(res.getHeaderValue("Permissions-Policy"),
                  expectedPermissionsPolicy);
        EXPECT_EQ(res.getHeaderValue("X-Permitted-Cross-Domain-Policies"),
                  "none");
        EXPECT_EQ(res.getHeaderValue("Cross-Origin-Embedder-Policy"),
                  "require-corp");
        EXPECT_EQ(res.getHeaderValue("Cross-Origin-Opener-Policy"),
                  "same-origin");
        EXPECT_EQ(res.getHeaderValue("Cross-Origin-Resource-Policy"),
                  "same-origin");
        EXPECT_EQ(res.getHeaderValue("Content-Security-Policy"),
                  expectedContentSecurityPolicy);
    }
}

} // namespace
