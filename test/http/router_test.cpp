#include "async_resp.hpp" // IWYU pragma: keep
#include "http_request.hpp"
#include "routing.hpp"
#include "utility.hpp"

#include <boost/beast/http/message.hpp> // IWYU pragma: keep
#include <boost/beast/http/verb.hpp>

#include <memory>
#include <string>
#include <string_view>
#include <system_error>

#include <gtest/gtest.h> // IWYU pragma: keep

// IWYU pragma: no_include <boost/beast/http/impl/message.hpp>
// IWYU pragma: no_include "gtest/gtest_pred_impl.h"
// IWYU pragma: no_include <boost/intrusive/detail/list_iterator.hpp>
// IWYU pragma: no_include <gtest/gtest-message.h>
// IWYU pragma: no_include <gtest/gtest-test-part.h>
// IWYU pragma: no_forward_declare bmcweb::AsyncResp

namespace crow
{
namespace
{

using ::crow::black_magic::getParameterTag;

TEST(Router, AllowHeader)
{
    // Callback handler that does nothing
    auto nullCallback = [](const Request&,
                           const std::shared_ptr<bmcweb::AsyncResp>&) {};

    Router router;
    std::error_code ec;

    constexpr std::string_view url = "/foo";

    Request req{{boost::beast::http::verb::get, url, 11}, ec};

    // No route should return no methods.
    router.validate();
    EXPECT_EQ(router.findRoute(req).allowHeader, "");
    EXPECT_EQ(router.findRoute(req).route.rule, nullptr);

    router.newRuleTagged<getParameterTag(url)>(std::string(url))
        .methods(boost::beast::http::verb::get)(nullCallback);
    router.validate();
    EXPECT_EQ(router.findRoute(req).allowHeader, "GET");
    EXPECT_NE(router.findRoute(req).route.rule, nullptr);

    Request patchReq{{boost::beast::http::verb::patch, url, 11}, ec};
    EXPECT_EQ(router.findRoute(patchReq).route.rule, nullptr);

    router.newRuleTagged<getParameterTag(url)>(std::string(url))
        .methods(boost::beast::http::verb::patch)(nullCallback);
    router.validate();
    EXPECT_EQ(router.findRoute(req).allowHeader, "GET, PATCH");
    EXPECT_NE(router.findRoute(req).route.rule, nullptr);
    EXPECT_NE(router.findRoute(patchReq).route.rule, nullptr);
}

TEST(Router, 404)
{
    bool notFoundCalled = false;
    // Callback handler that does nothing
    auto nullCallback =
        [&notFoundCalled](const Request&,
                          const std::shared_ptr<bmcweb::AsyncResp>&) {
        notFoundCalled = true;
    };

    Router router;
    std::error_code ec;

    constexpr std::string_view url = "/foo/bar";

    Request req{{boost::beast::http::verb::get, url, 11}, ec};

    router.newRuleTagged<getParameterTag(url)>("/foo/<path>")
        .notFound()(nullCallback);
    router.validate();
    {
        std::shared_ptr<bmcweb::AsyncResp> asyncResp =
            std::make_shared<bmcweb::AsyncResp>();

        router.handle(req, asyncResp);
    }
    EXPECT_TRUE(notFoundCalled);
}

TEST(Router, 405)
{
    // Callback handler that does nothing
    auto nullCallback = [](const Request&,
                           const std::shared_ptr<bmcweb::AsyncResp>&) {};
    bool called = false;
    auto notAllowedCallback =
        [&called](const Request&, const std::shared_ptr<bmcweb::AsyncResp>&) {
        called = true;
    };

    Router router;
    std::error_code ec;

    constexpr std::string_view url = "/foo/bar";

    Request req{{boost::beast::http::verb::patch, url, 11}, ec};

    router.newRuleTagged<getParameterTag(url)>(std::string(url))
        .methods(boost::beast::http::verb::get)(nullCallback);
    router.newRuleTagged<getParameterTag(url)>("/foo/<path>")
        .methodNotAllowed()(notAllowedCallback);
    router.validate();
    {
        std::shared_ptr<bmcweb::AsyncResp> asyncResp =
            std::make_shared<bmcweb::AsyncResp>();

        router.handle(req, asyncResp);
    }
    EXPECT_TRUE(called);
}
} // namespace
} // namespace crow
