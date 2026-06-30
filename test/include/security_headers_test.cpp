// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "security_headers.hpp"

#include "http_response.hpp"

#include <boost/beast/http/field.hpp>

#include <string_view>

#include <gtest/gtest.h>

namespace
{

using bf = boost::beast::http::field;

TEST(SecurityHeaders, CommonHeadersAlwaysSet)
{
    crow::Response res;
    addSecurityHeaders(res);

    EXPECT_EQ(res.getHeaderValue(bf::strict_transport_security),
              "max-age=31536000; includeSubdomains");
    EXPECT_EQ(res.getHeaderValue(bf::pragma), "no-cache");
    EXPECT_EQ(res.getHeaderValue("X-Content-Type-Options"), "nosniff");
}

TEST(SecurityHeaders, CacheControlSetWhenAbsent)
{
    crow::Response res;
    addSecurityHeaders(res);

    EXPECT_EQ(res.getHeaderValue(bf::cache_control), "no-store, max-age=0");
}

TEST(SecurityHeaders, CacheControlNotOverwrittenWhenPresent)
{
    crow::Response res;
    res.addHeader(bf::cache_control, "public, max-age=3600");
    addSecurityHeaders(res);

    EXPECT_EQ(res.getHeaderValue(bf::cache_control), "public, max-age=3600");
}

TEST(SecurityHeaders, NoContentTypeNoHtmlHeaders)
{
    crow::Response res;
    addSecurityHeaders(res);

    EXPECT_EQ(res.getHeaderValue(bf::x_frame_options), "");
    EXPECT_EQ(res.getHeaderValue("Content-Security-Policy"), "");
}

TEST(SecurityHeaders, HtmlHeadersNotSetForNonHtml)
{
    crow::Response res;
    res.addHeader("Content-Type", "application/json");
    addSecurityHeaders(res);

    EXPECT_EQ(res.getHeaderValue(bf::x_frame_options), "");
    EXPECT_EQ(res.getHeaderValue("Referrer-Policy"), "");
    EXPECT_EQ(res.getHeaderValue("Permissions-Policy"), "");
    EXPECT_EQ(res.getHeaderValue("Content-Security-Policy"), "");
    EXPECT_EQ(res.getHeaderValue("Cross-Origin-Embedder-Policy"), "");
    EXPECT_EQ(res.getHeaderValue("Cross-Origin-Opener-Policy"), "");
    EXPECT_EQ(res.getHeaderValue("Cross-Origin-Resource-Policy"), "");
    EXPECT_EQ(res.getHeaderValue("X-Permitted-Cross-Domain-Policies"), "");
}

TEST(SecurityHeaders, HtmlHeadersSetForTextHtml)
{
    crow::Response res;
    res.addHeader("Content-Type", "text/html");
    addSecurityHeaders(res);

    EXPECT_EQ(res.getHeaderValue(bf::x_frame_options), "DENY");
    EXPECT_EQ(res.getHeaderValue("Referrer-Policy"), "no-referrer");
    EXPECT_EQ(res.getHeaderValue("X-Permitted-Cross-Domain-Policies"), "none");
    EXPECT_EQ(res.getHeaderValue("Cross-Origin-Embedder-Policy"),
              "require-corp");
    EXPECT_EQ(res.getHeaderValue("Cross-Origin-Opener-Policy"), "same-origin");
    EXPECT_EQ(res.getHeaderValue("Cross-Origin-Resource-Policy"),
              "same-origin");
}

TEST(SecurityHeaders, HtmlHeadersSetForTextHtmlWithCharset)
{
    crow::Response res;
    res.addHeader("Content-Type", "text/html; charset=utf-8");
    addSecurityHeaders(res);

    EXPECT_EQ(res.getHeaderValue(bf::x_frame_options), "DENY");
    EXPECT_EQ(res.getHeaderValue("Referrer-Policy"), "no-referrer");
}

TEST(SecurityHeaders, ContentSecurityPolicyValue)
{
    crow::Response res;
    res.addHeader("Content-Type", "text/html");
    addSecurityHeaders(res);

    std::string_view csp = res.getHeaderValue("Content-Security-Policy");
    EXPECT_NE(csp.find("default-src 'none'"), std::string_view::npos);
    EXPECT_NE(csp.find("img-src 'self' data:"), std::string_view::npos);
    EXPECT_NE(csp.find("script-src 'self'"), std::string_view::npos);
    EXPECT_NE(csp.find("frame-ancestors 'none'"), std::string_view::npos);
}

TEST(SecurityHeaders, PermissionsPolicyValue)
{
    crow::Response res;
    res.addHeader("Content-Type", "text/html");
    addSecurityHeaders(res);

    std::string_view policy = res.getHeaderValue("Permissions-Policy");
    EXPECT_NE(policy.find("camera=()"), std::string_view::npos);
    EXPECT_NE(policy.find("microphone=()"), std::string_view::npos);
    EXPECT_NE(policy.find("geolocation=()"), std::string_view::npos);
    EXPECT_NE(policy.find("usb=()"), std::string_view::npos);
}

} // namespace
