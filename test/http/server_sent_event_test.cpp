#include "boost_formatters.hpp"
#include "http/server_sent_event.hpp"

#include <boost/asio/buffer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/read.hpp>
#include <boost/beast/_experimental/test/stream.hpp>

#include <chrono>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "gtest/gtest.h"
namespace crow
{
namespace sse_socket
{

namespace
{

TEST(ServerSentEvent, SseWorks)
{
    boost::asio::io_context io;
    boost::beast::test::stream stream(io);
    boost::beast::test::stream out(io);
    stream.connect(out);

    Request req;

    bool openCalled = false;
    auto openHandler = [&openCalled](Connection&,
                                     const Request& /*handedReq*/) {
        openCalled = true;
    };
    bool closeCalled = false;
    auto closeHandler = [&closeCalled](Connection&) { closeCalled = true; };

    std::shared_ptr<ConnectionImpl<boost::beast::test::stream>> conn =
        std::make_shared<ConnectionImpl<boost::beast::test::stream>>(
            std::move(stream), openHandler, closeHandler);
    conn->start(req);
    // Connect
    {
        constexpr std::string_view expected =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/event-stream\r\n"
            "\r\n";

        while (out.str().size() != expected.size())
        {
            io.run_for(std::chrono::milliseconds(1));
        }

        std::string eventContent;
        eventContent.resize(expected.size());
        boost::asio::read(out, boost::asio::buffer(eventContent));

        EXPECT_EQ(eventContent, expected);
        EXPECT_TRUE(openCalled);
        EXPECT_FALSE(closeCalled);
        EXPECT_TRUE(out.str().empty());
    }
    // Send one event
    {
        conn->sendEvent("TestEventId", "TestEventContent");
        std::string_view expected = "id: TestEventId\n"
                                    "data: TestEventContent\n"
                                    "\n";

        while (out.str().size() < expected.size())
        {
            io.run_for(std::chrono::milliseconds(1));
        }
        EXPECT_EQ(out.str(), expected);

        std::string eventContent;
        eventContent.resize(expected.size());
        boost::asio::read(out, boost::asio::buffer(eventContent));

        EXPECT_EQ(eventContent, expected);
        EXPECT_TRUE(out.str().empty());
    }
    // Send second event
    {
        conn->sendEvent("TestEventId2", "TestEvent\nContent2");
        constexpr std::string_view expected = "id: TestEventId2\n"
                                              "data: TestEvent\n"
                                              "data: Content2\n"
                                              "\n";

        while (out.str().size() < expected.size())
        {
            io.run_for(std::chrono::milliseconds(1));
        }

        std::string eventContent;
        eventContent.resize(expected.size());
        boost::asio::read(out, boost::asio::buffer(eventContent));
        EXPECT_EQ(eventContent, expected);
        EXPECT_TRUE(out.str().empty());
    }
    // close the remote
    {
        out.close();
        while (!closeCalled)
        {
            io.run_for(std::chrono::milliseconds(1));
        }
    }
}
} // namespace

} // namespace sse_socket
} // namespace crow
