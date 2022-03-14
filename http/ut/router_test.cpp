#include "http_request.hpp"
#include "routing.hpp"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

TEST(Router, AllowHeader)
{
    // Callback handler that does nothing
    auto nullCallback = [](const crow::Request&,
                           const std::shared_ptr<bmcweb::AsyncResp>&) {};

    crow::Router router;
    std::error_code ec;

    constexpr const std::string_view url = "/foo";

    crow::Request req{{boost::beast::http::verb::get, url, 11}, ec};

    // No route should return no methods.
    router.validate();
    EXPECT_EQ(router.buildAllowHeader(req), "");

    router
        .newRuleTagged<crow::black_magic::getParameterTag(url)>(
            std::string(url))
        .methods(boost::beast::http::verb::get)(nullCallback);
    router.validate();
    EXPECT_EQ(router.buildAllowHeader(req), "GET");

    router
        .newRuleTagged<crow::black_magic::getParameterTag(url)>(
            std::string(url))
        .methods(boost::beast::http::verb::patch)(nullCallback);
    router.validate();
    EXPECT_EQ(router.buildAllowHeader(req), "GET, PATCH");
}