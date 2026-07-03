// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "cookies.hpp"
#include "http/http_response.hpp"
#include "sessions.hpp"

#include <boost/beast/http/field.hpp>

#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace
{

using ::testing::UnorderedElementsAre;

TEST(Cookies, SetSessionCookiesAddsSessionAndCsrfCookies)
{
    crow::Response res;
    persistent_data::UserSession session;
    session.csrfToken = "testCsrf";
    session.sessionToken = "testSession";

    bmcweb::setSessionCookies(res, session);

    ASSERT_EQ(res.fields().count(boost::beast::http::field::set_cookie), 2);

    std::vector<std::string> cookies;
    const auto [first, last] =
        res.fields().equal_range(boost::beast::http::field::set_cookie);
    for (auto cookie = first; cookie != last; ++cookie)
    {
        cookies.emplace_back(cookie->value());
    }

    EXPECT_THAT(
        cookies,
        UnorderedElementsAre(
            "XSRF-TOKEN=testCsrf; Path=/; SameSite=Strict; Secure",
            "BMCWEB-SESSION=testSession; Path=/; SameSite=Strict; Secure; "
            "HttpOnly"));
}

TEST(Cookies, ClearSessionCookiesExpiresSessionCookie)
{
    crow::Response res;

    bmcweb::clearSessionCookies(res);

    ASSERT_EQ(res.fields().count(boost::beast::http::field::set_cookie), 1);
    EXPECT_EQ(res.getHeaderValue(boost::beast::http::field::set_cookie),
              "BMCWEB-SESSION=; Path=/; SameSite=Strict; Secure; HttpOnly; "
              "expires=Thu, 01 Jan 1970 00:00:00 GMT");
}

} // namespace
