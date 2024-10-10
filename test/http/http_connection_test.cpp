#include "async_resp.hpp"
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
    static void
        handleUpgrade(const std::shared_ptr<Request>& /*req*/,
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
        EXPECT_EQ(req->body(), "");

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
        "GET / HTTP/1.1\r\nHost: openbmc_project.xyz\r\nConnection: close\r\n\r\n"));
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

} // namespace crow
