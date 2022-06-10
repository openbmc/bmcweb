#include "app.hpp"
#include "kvm_websocket.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/streambuf.hpp>

#include <future>
#include <iostream>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace crow::obmc_kvm
{
namespace
{

using ::testing::Contains;
using ::testing::Eq;
using ::testing::Pointee;
using ::testing::UnorderedElementsAre;

constexpr int port = 45451;

// Tests static files are loaded correctly
TEST(Kvm, BasicRfb)
{
    auto appIOContext = std::make_shared<boost::asio::io_context>();
    App app(appIOContext);

    requestRoutes(app);
    app.bindaddr("127.0.0.1").port(port);
    app.validate();
    auto _ = async(std::launch::async, [&app, appIOContext] {
        app.run();
        appIOContext->run();
    });
    auto routes = app.getRoutes();
    EXPECT_THAT(routes, UnorderedElementsAre(Pointee(Eq("/kvm/0"))));
    boost::asio::io_context ioContext;

    {
        // Retry a couple of times waiting for the server to come up
        // TODO(ed)  This is really unfortunate, and should use some form of
        // mock
        for (int i = 0; i < 200; i++)
        {
            try
            {
                boost::asio::ip::tcp::socket s(ioContext);
                s.connect(boost::asio::ip::tcp::endpoint(
                    boost::asio::ip::make_address("127.0.0.1"), port));
                s.close();
                BMCWEB_LOG_DEBUG << "Server is up; iterations=" << i;
                break;
            }
            catch (const std::exception& e)
            {
                // do nothing.  We expect this to fail while the server is
                // starting up
                BMCWEB_LOG_DEBUG << "Failed to connect; iterations=" << i;
                sleep(1);
            }
        }
    }

    // Get the websocket
    std::string sendMsg = ("GET /kvm/0 HTTP/1.1\r\n"
                           "Host: localhost:45451\r\n"
                           "Connection: Upgrade\r\n"
                           "Upgrade: websocket\r\n"
                           "Sec-WebSocket-Version: 13\r\n"
                           "Sec-WebSocket-Key: aLeGkmLPZmdv5tTyEpJ3jQ==\r\n"
                           "Sec-WebSocket-Extensions: permessage-deflate; "
                           "client_max_window_bits\r\n"
                           "Sec-WebSocket-Protocol: binary\r\n"
                           "\r\n");

    boost::asio::ip::tcp::socket socket(ioContext);
    socket.connect(boost::asio::ip::tcp::endpoint(
        boost::asio::ip::make_address("127.0.0.1"), port));
    socket.send(boost::asio::buffer(sendMsg));

    // Read the Response status line. The Response streambuf will automatically
    // grow to accommodate the entire line. The growth may be limited by passing
    // a maximum size to the streambuf constructor.
    boost::asio::streambuf response;
    boost::asio::read_until(socket, response, "\r\n");
    BMCWEB_LOG_ERROR << "First message got";
    // Check that Response is OK.
    std::istream response_stream(&response);
    std::string http_response;
    std::getline(response_stream, http_response);

    EXPECT_EQ(http_response, "HTTP/1.1 101 Switching Protocols\r");

    // Read the Response headers, which are terminated by a blank line.
    boost::asio::read_until(socket, response, "\r\n\r\n");

    // Process the Response headers.
    std::string header;
    std::vector<std::string> headers;
    while (std::getline(response_stream, header) && header != "\r")
    {
        headers.push_back(header);
    }

    EXPECT_THAT(headers, Contains("Upgrade: websocket\r"));
    EXPECT_THAT(headers, Contains("Connection: upgrade\r"));
    EXPECT_THAT(headers, Contains("Sec-WebSocket-Protocol: binary\r"));
    // TODO(ed) This is the result that it gives today.  Need to check websocket
    // docs and make sure that this calculation is actually being done to spec
    EXPECT_THAT(
        headers,
        Contains("Sec-WebSocket-Accept: /CnDM3l79rIxniLNyxMryXbtLEU=\r"));
    std::array<char, 13> rfb_open_string{};

    boost::asio::read(socket, boost::asio::buffer(rfb_open_string));
    auto open_string =
        std::string(std::begin(rfb_open_string), std::end(rfb_open_string));
    // Todo(ed) find out what the two characters at the end of the websocket
    // stream are
    open_string = open_string.substr(2, 2);
    std::string expected;
    expected += static_cast<char>(0x3);
    expected += static_cast<char>(0xE8);
    EXPECT_EQ(open_string, expected);

    app.stop();
}
} // namespace
} // namespace crow::obmc_kvm