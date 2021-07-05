#include <http_utility.hpp>

#include "gmock/gmock.h"

TEST(HttpUtility, requestPrefersHtml)
{
    boost::beast::http::request<boost::beast::http::string_body> req{};

    req.set("Accept", "*/*, application/octet-stream");
    crow::Request req1(req);
    EXPECT_FALSE(
        http_helpers::requestPrefersHtml(req1.getHeaderValue("Accept")));
    EXPECT_TRUE(http_helpers::isOctetAccepted(req1.getHeaderValue("Accept")));

    req.set("Accept", "text/html, application/json");
    crow::Request req2(req);
    EXPECT_TRUE(
        http_helpers::requestPrefersHtml(req2.getHeaderValue("Accept")));
    EXPECT_FALSE(http_helpers::isOctetAccepted(req2.getHeaderValue("Accept")));

    req.set("Accept", "application/json");
    crow::Request req3(req);
    EXPECT_FALSE(
        http_helpers::requestPrefersHtml(req3.getHeaderValue("Accept")));
    EXPECT_FALSE(http_helpers::isOctetAccepted(req3.getHeaderValue("Accept")));
}
