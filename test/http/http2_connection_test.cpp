#include "http/http2_connection.hpp"
#include "http/http_request.hpp"
#include "http/http_response.hpp"

#include <boost/asio/steady_timer.hpp>
#include <boost/beast/_experimental/test/stream.hpp>

#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

#include "gtest/gtest.h"
namespace crow
{

struct FakeHandler
{
    static void
        handleUpgrade(Request& /*req*/,
                      const std::shared_ptr<bmcweb::AsyncResp>& /*asyncResp*/,
                      boost::beast::test::stream&& /*adaptor*/)
    {
        // Handle Upgrade should never be called
        EXPECT_FALSE(true);
    }

    void handle(Request& /*req*/,
                const std::shared_ptr<bmcweb::AsyncResp>& /*asyncResp*/)
    {}
};

struct ClockFake
{
    std::string getDateStr()
    {
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
        // Hello
        "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n"
        // Empty SETTINGS frame to stream 0
        "\0\0\0\x4\0\0\0\0\0"
        // ACK the servers SETTINGS frame on stream 0
        "\0\0\0\x4\x1\0\0\0\0"

        // Headers frame on stream 1
        //"\0\0\<len>\x1\0\0\0\0\x1"
        //"\0\0\0\0"
        ));

    FakeHandler handler;
    boost::asio::steady_timer timer(io);
    std::function<std::string()> date(
        std::bind_front(&ClockFake::getDateStr, &clock));
    auto conn = std::make_shared<
        HTTP2Connection<boost::beast::test::stream, FakeHandler>>(
        std::move(stream), &handler, date);
    conn->start();

    // Can't call handler from unit test yet
    // EXPECT_TRUE(handler.called);

    using namespace std::literals;
    std::string_view expected =
        // Settings frame size 15
        "\0\0\f\x4\0\0\0\0\0"
        // 4 max concurrent streams
        "\0\x3\0\0\0\x4"
        // Enable push = false
        "\0\x2\0\0\0\0"
        // Settings ACK from server to client
        "\0\0\0\x4\x1\0\0\0\0"sv;

    std::string outStr;
    while (outStr.size() != expected.size())
    {
        io.run_one();
        outStr = out.str();
    }

    EXPECT_EQ(outStr, expected);
}

} // namespace crow
