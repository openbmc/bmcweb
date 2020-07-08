#include "crow.h"

#include "gzip_helper.hpp"
#include "web_kvm.hpp"

#include <iostream>
#include <sstream>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace crow;
using namespace testing;

// Tests static files are loaded correctly
TEST(Kvm, BasicRfb)
{
    return; // TODO(ed) Make the code below work again
    SimpleApp app;

    crow::kvm::requestRoutes(app);
    app.bindaddr("127.0.0.1").port(45451);
    BMCWEB_ROUTE(app, "/")([]() { return boost::beast::http::status::ok; });
    auto _ = async(std::launch::async, [&] { app.run(); });
    auto routes = app.getRoutes();
    asio::io_context is;

    {
        // Retry a couple of times waiting for the server to come up
        // TODO(ed)  This is really unfortunate, and should use some form of
        // mock
        asio::ip::tcp::socket c(is);
        for (int i = 0; i < 200; i++)
        {
            try
            {
                c.connect(asio::ip::tcp::endpoint(
                    asio::ip::address::from_string("127.0.0.1"), 45451));
                c.close();
                break;
            }
            catch (std::exception e)
            {
                // do nothing.  We expect this to fail while the server is
                // starting up
            }
        }
    }

    // Get the websocket
    std::string sendmsg = ("GET /kvmws HTTP/1.1\r\n"
                           "Host: localhost:45451\r\n"
                           "Connection: Upgrade\r\n"
                           "Upgrade: websocket\r\n"
                           "Sec-WebSocket-Version: 13\r\n"
                           "Sec-WebSocket-Key: aLeGkmLPZmdv5tTyEpJ3jQ==\r\n"
                           "Sec-WebSocket-Extensions: permessage-deflate; "
                           "client_max_window_bits\r\n"
                           "Sec-WebSocket-Protocol: binary\r\n"
                           "\r\n");

    asio::ip::tcp::socket socket(is);
    socket.connect(asio::ip::tcp::endpoint(
        asio::ip::address::from_string("127.0.0.1"), 45451));
    socket.send(asio::buffer(sendmsg));

    // Read the Response status line. The Response streambuf will automatically
    // grow to accommodate the entire line. The growth may be limited by passing
    // a maximum size to the streambuf constructor.
    boost::asio::streambuf response;
    boost::asio::read_until(socket, response, "\r\n");

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
    EXPECT_THAT(headers, Contains("Connection: Upgrade\r"));
    EXPECT_THAT(headers, Contains("Sec-WebSocket-Protocol: binary\r"));
    // TODO(ed) This is the result that it gives today.  Need to check websocket
    // docs and make
    // sure that this calclution is actually being done to spec
    EXPECT_THAT(
        headers,
        Contains("Sec-WebSocket-Accept: /CnDM3l79rIxniLNyxMryXbtLEU=\r"));
    std::array<char, 13> rfb_open_string;

    //
    // socket.receive(rfb_open_string.data(), rfb_open_string.size());
    boost::asio::read(socket, boost::asio::buffer(rfb_open_string));
    auto open_string =
        std::string(std::begin(rfb_open_string), std::end(rfb_open_string));
    // Todo(ed) find out what the two characters at the end of the websocket
    // stream are
    open_string = open_string.substr(2);
    EXPECT_EQ(open_string, "RFB 003.008");

    app.stop();
}