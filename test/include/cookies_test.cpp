// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "cookies.hpp"
#include "http/http_response.hpp"
#include "sessions.hpp"

#include <boost/beast/http/field.hpp>

#include <algorithm>
#include <iterator>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace
{

std::vector<std::string> getSortedSetCookieHeaders(const crow::Response& res)
{
    auto [begin, end] =
        res.fields().equal_range(boost::beast::http::field::set_cookie);

    std::vector<std::string> cookies;
    for (auto it = begin; it != end; ++it)
    {
        cookies.emplace_back(it->value());
    }

    std::sort(cookies.begin(), cookies.end());
    return cookies;
}

constexpr std::string_view expectedSessionCookieHeader =
    "BMCWEB-SESSION=testSession; Path=/; SameSite=Strict; Secure; HttpOnly";
constexpr std::string_view expectedCsrfCookieHeader =
    "XSRF-TOKEN=testCsrf; Path=/; SameSite=Strict; Secure";
constexpr std::string_view expectedExpiredSessionCookieHeader =
    "BMCWEB-SESSION=; Path=/; SameSite=Strict; Secure; HttpOnly; "
    "expires=Thu, 01 Jan 1970 00:00:00 GMT";

TEST(Cookies, SetSessionCookiesAddsSessionAndCsrfCookies)
{
    crow::Response res;
    persistent_data::UserSession session;
    session.csrfToken = "testCsrf";
    session.sessionToken = "testSession";

    bmcweb::setSessionCookies(res, session);

    const std::vector<std::string> cookies = getSortedSetCookieHeaders(res);

    ASSERT_EQ(std::distance(res.fields().begin(), res.fields().end()), 2);
    ASSERT_EQ(cookies.size(), 2U);
    EXPECT_EQ(cookies[0], expectedSessionCookieHeader);
    EXPECT_EQ(cookies[1], expectedCsrfCookieHeader);
}

TEST(Cookies, ClearSessionCookiesExpiresSessionCookie)
{
    crow::Response res;

    bmcweb::clearSessionCookies(res);

    const std::vector<std::string> cookies = getSortedSetCookieHeaders(res);

    ASSERT_EQ(std::distance(res.fields().begin(), res.fields().end()), 1);
    ASSERT_EQ(cookies.size(), 1U);
    EXPECT_EQ(cookies[0], expectedExpiredSessionCookieHeader);
}

} // namespace
