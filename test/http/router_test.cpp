// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "async_resp.hpp"
#include "http_request.hpp"
#include "routing.hpp"
#include "utility.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/http/verb.hpp>

#include <memory>
#include <string>
#include <string_view>
#include <system_error>

#include <gtest/gtest.h>

namespace crow
{
namespace
{

using utility::getParameterTag;

TEST(Router, AllowHeader)
{
    // Callback handler that does nothing
    auto nullCallback =
        [](const Request&, const std::shared_ptr<bmcweb::AsyncResp>&) {};

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

TEST(Router, OverlapingRoutes)
{
    // Callback handler that does nothing
    auto fooCallback =
        [](const Request&, const std::shared_ptr<bmcweb::AsyncResp>&) {
            EXPECT_FALSE(true);
        };
    bool barCalled = false;
    auto foobarCallback =
        [&barCalled](const Request&, const std::shared_ptr<bmcweb::AsyncResp>&,
                     const std::string& bar) {
            barCalled = true;
            EXPECT_EQ(bar, "bar");
        };

    Router router;
    std::error_code ec;

    router.newRuleTagged<getParameterTag("/foo/<str>")>("/foo/<str>")(
        foobarCallback);
    router.newRuleTagged<getParameterTag("/foo")>("/foo")(fooCallback);
    router.validate();
    {
        constexpr std::string_view url = "/foo/bar";

        auto req = std::make_shared<Request>(
            Request::Body{boost::beast::http::verb::get, url, 11}, ec);

        std::shared_ptr<bmcweb::AsyncResp> asyncResp =
            std::make_shared<bmcweb::AsyncResp>();

        router.handle(req, asyncResp);
    }
    EXPECT_TRUE(barCalled);
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

    auto req = std::make_shared<Request>(
        Request::Body{boost::beast::http::verb::get, url, 11}, ec);

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
    auto nullCallback =
        [](const Request&, const std::shared_ptr<bmcweb::AsyncResp>&) {};
    bool called = false;
    auto notAllowedCallback =
        [&called](const Request&, const std::shared_ptr<bmcweb::AsyncResp>&) {
            called = true;
        };

    Router router;
    std::error_code ec;

    constexpr std::string_view url = "/foo/bar";

    auto req = std::make_shared<Request>(
        Request::Body{boost::beast::http::verb::patch, url, 11}, ec);

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

// When validatePrivilege() does not run the upgrade continuation, the stream
// must stay with the connection so an error response can still be written on
// it. A request with no session makes validatePrivilege() return without
// running the continuation; that is the same outcome as an authenticated user
// who lacks the route privilege (a 403 after GetUserInfo), which needs a D-Bus
// backend and is not driven here.
std::shared_ptr<Request> deniedUpgradeRequest(std::string_view url)
{
    std::error_code ec;
    auto req = std::make_shared<Request>(
        Request::Body{boost::beast::http::verb::get, url, 11}, ec);
    EXPECT_FALSE(ec);
    EXPECT_EQ(req->session, nullptr);
    return req;
}

TEST(Router, DeniedUpgradeKeepsPlainSocket)
{
    boost::asio::io_context io;
    Router router;
    router.newRuleTagged<getParameterTag("/ws")>("/ws")
        .privileges({{"ConfigureComponents"}})
        .websocket();
    router.validate();

    boost::asio::ip::tcp::socket socket(io);
    socket.open(boost::asio::ip::tcp::v4());
    ASSERT_TRUE(socket.is_open());

    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    router.handleUpgrade(deniedUpgradeRequest("/ws"), asyncResp, socket);

    EXPECT_TRUE(socket.is_open());
}

TEST(Router, DeniedUpgradeKeepsTlsStream)
{
    boost::asio::io_context io;
    Router router;
    router.newRuleTagged<getParameterTag("/ws")>("/ws")
        .privileges({{"ConfigureComponents"}})
        .websocket();
    router.validate();

    boost::asio::ssl::context ctx(boost::asio::ssl::context::tls_server);
    boost::asio::ssl::stream<boost::asio::ip::tcp::socket> stream(io, ctx);
    ASSERT_NE(stream.native_handle(), nullptr);

    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    router.handleUpgrade(deniedUpgradeRequest("/ws"), asyncResp, stream);

    // Writing the error response calls SSL_write() on this handle.
    EXPECT_NE(stream.native_handle(), nullptr);
}
} // namespace
} // namespace crow
