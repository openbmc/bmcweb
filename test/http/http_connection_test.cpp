// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "bmcweb_config.h"

#include "async_resp.hpp"
#include "http/http_body.hpp"
#include "http/http_connection.hpp"
#include "http/http_request.hpp"
#include "http/http_response.hpp"
#include "http_connect_types.hpp"
#include "test_stream.hpp"

#include <boost/asio/buffer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/_experimental/test/stream.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/verb.hpp>

#include <chrono>
#include <cstdint>
#include <format>
#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "gtest/gtest.h"
namespace crow
{

struct FakeHandler
{
    template <typename Adaptor>
    static void handleUpgrade(
        const std::shared_ptr<Request>& /*req*/,
        const std::shared_ptr<bmcweb::AsyncResp>& /*asyncResp*/,
        Adaptor&& /*adaptor*/)
    {
        // Handle Upgrade should never be called
        EXPECT_FALSE(true);
    }

    void handle(const std::shared_ptr<Request>& req,
                const std::shared_ptr<bmcweb::AsyncResp>& /*asyncResp*/)
    {
        EXPECT_EQ(req->method(), boost::beast::http::verb::get);
        EXPECT_EQ(req->target(), "/");
        EXPECT_EQ(req->getHeaderValue(boost::beast::http::field::host),
                  "openbmc_project.xyz");
        EXPECT_FALSE(req->keepAlive());
        EXPECT_EQ(req->version(), 11);
        EXPECT_EQ(req->body(), "Hello, World!");

        called = true;
    }
    bool called = false;
};

struct ClockFake
{
    bool wascalled = false;
    std::string getDateStr()
    {
        wascalled = true;
        return "TestTime";
    }
};

TEST(http_connection, RequestPropogates)
{
    boost::asio::io_context io;
    ClockFake clock;
    TestStream stream(io);
    TestStream out(io);
    stream.connect(out);

    out.write_some(boost::asio::buffer(
        "GET / HTTP/1.1\r\n"
        "Host: openbmc_project.xyz\r\n"
        "Connection: close\r\n"
        "Content-Length: 13\r\n\r\n"
        "Hello, World!"));
    FakeHandler handler;
    boost::asio::steady_timer timer(io);
    std::function<std::string()> date(
        std::bind_front(&ClockFake::getDateStr, &clock));

    boost::asio::ssl::context context{boost::asio::ssl::context::tls};
    std::shared_ptr<Connection<TestStream, FakeHandler>> conn =
        std::make_shared<Connection<TestStream, FakeHandler>>(
            &handler, HttpType::HTTP, std::move(timer), date,
            boost::asio::ssl::stream<TestStream>(std::move(stream), context));
    conn->disableAuth();
    conn->start();
    io.run_for(std::chrono::seconds(1000));
    EXPECT_TRUE(handler.called);
    std::string outStr = out.str();

    std::string expected =
        "HTTP/1.1 200 OK\r\n"
        "Connection: close\r\n"
        "Strict-Transport-Security: max-age=31536000; includeSubdomains\r\n"
        "Pragma: no-cache\r\n"
        "Cache-Control: no-store, max-age=0\r\n"
        "X-Content-Type-Options: nosniff\r\n"
        "Date: TestTime\r\n"
        "Content-Length: 0\r\n\r\n";
    EXPECT_EQ(outStr, expected);
    EXPECT_TRUE(clock.wascalled);
}

namespace
{

// Status line for a logged-out POST that declares contentLength bytes and
// sends none of them.
std::string loggedOutStatusLine(uint64_t contentLength)
{
    boost::asio::io_context io;
    ClockFake clock;
    TestStream stream(io);
    TestStream out(io);
    stream.connect(out);

    std::string request =
        std::format("POST /redfish/v1/SessionService/Sessions HTTP/1.1\r\n"
                    "Host: openbmc_project.xyz\r\n"
                    "Content-Length: {}\r\n\r\n",
                    contentLength);
    out.write_some(boost::asio::buffer(request));
    FakeHandler handler;
    boost::asio::steady_timer timer(io);
    std::function<std::string()> date(
        std::bind_front(&ClockFake::getDateStr, &clock));

    boost::asio::ssl::context context{boost::asio::ssl::context::tls};
    std::shared_ptr<Connection<TestStream, FakeHandler>> conn =
        std::make_shared<Connection<TestStream, FakeHandler>>(
            &handler, HttpType::HTTP, std::move(timer), date,
            boost::asio::ssl::stream<TestStream>(std::move(stream), context));
    conn->start();
    io.run_for(std::chrono::seconds(1));
    EXPECT_FALSE(handler.called);
    std::string outStr = out.str();
    return outStr.substr(0, outStr.find("\r\n"));
}

} // namespace

// A logged-out body that would fit the logged-in limit gets 401, so the
// client knows to log in; anything larger gets 413.
TEST(http_connection, LoggedOutDeclaredLengthStatus)
{
    if constexpr (BMCWEB_INSECURE_DISABLE_AUTH)
    {
        GTEST_SKIP() << "insecure-disable-auth has no logged-out limit";
    }
    EXPECT_EQ(loggedOutStatusLine(loggedOutPostBodyLimit + 1),
              "HTTP/1.1 401 Unauthorized");
    EXPECT_EQ(loggedOutStatusLine(httpReqBodyLimit),
              "HTTP/1.1 401 Unauthorized");
    EXPECT_EQ(loggedOutStatusLine(httpReqBodyLimit + 1),
              "HTTP/1.1 413 Payload Too Large");
}

} // namespace crow
