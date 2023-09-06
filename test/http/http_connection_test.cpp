#include "http/http_connection.hpp"
#include "http/http_request.hpp"
#include "http/http_response.hpp"

#include <boost/asio/steady_timer.hpp>
#include <boost/beast/_experimental/test/stream.hpp>

#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <string>

#include "gtest/gtest.h"
namespace crow
{

struct FakeHandler
{
    explicit FakeHandler(boost::asio::io_context& ioIn) : io(ioIn) {}

    static void
        handleUpgrade(Request& /*req*/,
                      const std::shared_ptr<bmcweb::AsyncResp>& /*asyncResp*/,
                      boost::beast::test::stream&& /*adaptor*/)
    {
        // Handle Upgrade should never be called
        EXPECT_FALSE(true);
    }

    void handle(Request& req,
                const std::shared_ptr<bmcweb::AsyncResp>& /*asyncResp*/)
    {
        EXPECT_EQ(req.method(), boost::beast::http::verb::get);
        EXPECT_EQ(req.target(), "/");
        EXPECT_EQ(req.getHeaderValue(boost::beast::http::field::host),
                  "openbmc_project.xyz");
        EXPECT_FALSE(req.keepAlive());
        EXPECT_EQ(req.version(), 11);
        EXPECT_EQ(req.body(), "");

        called = true;
        io.stop();
    }
    boost::asio::io_context& io;
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
    boost::beast::test::stream stream(io);
    boost::beast::test::stream out(io);
    stream.connect(out);
    out.write_some(boost::asio::buffer(
        "GET / HTTP/1.1\r\nHost: openbmc_project.xyz\r\nConnection: close\r\n\r\n"));
    FakeHandler handler(io);
    boost::asio::steady_timer timer(io);
    std::function<std::string()> date(
        std::bind_front(&ClockFake::getDateStr, &clock));
    std::shared_ptr<crow::Connection<boost::beast::test::stream, FakeHandler>>
        conn = std::make_shared<
            crow::Connection<boost::beast::test::stream, FakeHandler>>(
            &handler, std::move(timer), date, std::move(stream));
    conn->start();
    io.run();
    EXPECT_TRUE(handler.called);

    EXPECT_EQ(
        out.str(),
        "HTTP/1.1 200 OK\r\n"
        "Connection: close\r\n"
        "Strict-Transport-Security: max-age=31536000; includeSubdomains\r\n"
        "X-Frame-Options: DENY\r\n"
        "Pragma: no-cache\r\n"
        "Cache-Control: no-store, max-age=0\r\n"
        "X-Content-Type-Options: nosniff\r\n"
        "Referrer-Policy: no-referrer\r\n"
        "Permissions-Policy: accelerometer=(),ambient-light-sensor=(),autoplay=(),battery=(),camera=(),display-capture=(),document-domain=(),encrypted-media=(),fullscreen=(),gamepad=(),geolocation=(),gyroscope=(),layout-animations=(self),legacy-image-formats=(self),magnetometer=(),microphone=(),midi=(),oversized-images=(self),payment=(),picture-in-picture=(),publickey-credentials-get=(),speaker-selection=(),sync-xhr=(self),unoptimized-images=(self),unsized-media=(self),usb=(),screen-wak-lock=(),web-share=(),xr-spatial-tracking=()\r\n"
        "X-Permitted-Cross-Domain-Policies: none\r\n"
        "Cross-Origin-Embedder-Policy: require-corp\r\n"
        "Cross-Origin-Opener-Policy: same-origin\r\n"
        "Cross-Origin-Resource-Policy: same-origin\r\n"
        "Content-Security-Policy: default-src 'none'; img-src 'self' data:; font-src 'self'; style-src 'self'; script-src 'self'; connect-src 'self' wss:; form-action 'none'; frame-ancestors 'none'; object-src 'none'; base-uri 'none'\r\n"
        "Date: TestTime\r\n"
        "Content-Length: 0\r\n\r\n");

    EXPECT_TRUE(clock.wascalled);
}

} // namespace crow
