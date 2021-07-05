// #include <boost/algorithm/string.hpp>
// #include <boost/container/flat_set.hpp>
// #include <dbus_singleton.hpp>
#include <http_utility.hpp>

#include "gmock/gmock.h"

TEST(HttpUtility, requestPrefersHtml)
{
    boost::beast::http::request<boost::beast::http::string_body> req{};

    req.set("Accept", "*/*, application/octet-stream");
    crow::Request req1(req);
    EXPECT_FALSE(http_helpers::requestPrefersHtml(req1));
    EXPECT_TRUE(http_helpers::isOctetAccepted(req1));

    req.set("Accept", "text/html, application/json");
    crow::Request req2(req);
    EXPECT_TRUE(http_helpers::requestPrefersHtml(req2));
    EXPECT_FALSE(http_helpers::isOctetAccepted(req2));

    req.set("Accept", "application/json");
    crow::Request req3(req);
    EXPECT_FALSE(http_helpers::requestPrefersHtml(req3));
    EXPECT_FALSE(http_helpers::isOctetAccepted(req3));
}
