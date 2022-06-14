#include <app.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/beast/http/status.hpp>
#include <logging.hpp>

#include <future>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace crow;
using namespace std;

// Tests that the security headers are added correctly
TEST(SecurityHeaders, TestHeadersExist)
{
    auto appIOContext = std::make_shared<boost::asio::io_context>();
    App app(appIOContext);
    app.bindaddr("127.0.0.1").port(45451);
    BMCWEB_ROUTE(app, "/")([]() { return boost::beast::http::status::ok; });
    auto _ = async(std::launch::async, [&app, appIOContext] {
        app.run();
        appIOContext->run();
    });

    boost::asio::io_context is;
    std::array<char, 2048> buf;
    std::string sendMsg;

    {
        // Retry a couple of times waiting for the server to come up
        // TODO(ed)  This is really unfortunate, and should use some form of
        // mock
        boost::asio::ip::tcp::socket c(is);
        for (int i = 0; i < 200; i++)
        {
            try
            {
                c.connect(boost::asio::ip::tcp::endpoint(
                    boost::asio::ip::make_address("127.0.0.1"), 45451));
                c.close();
                BMCWEB_LOG_DEBUG << "Server is up; iterations=" << i;
                break;
            }
            catch (const std::exception& e)
            {
                // do nothing.  We expect this to fail while the server is
                // starting up
                sleep(1);
            }
        }
    }

    // Test correct login credentials
    sendMsg = ("GET / HTTP/1.1\r\n"
               "Host: localhost:45451\r\n"
               "\r\n");

    boost::asio::ip::tcp::socket c(is);
    c.connect(boost::asio::ip::tcp::endpoint(
        boost::asio::ip::make_address("127.0.0.1"), 45451));
    c.send(boost::asio::buffer(sendMsg));
    c.receive(boost::asio::buffer(buf));
    c.close();
    auto return_code = std::string(&buf[9], &buf[12]);
    EXPECT_EQ("200", return_code);
    std::string response(std::begin(buf), std::end(buf));

    // This is a routine to split strings until a blank is hit
    // TODO(ed) this should really use the HTTP parser
    std::vector<std::string> headers;
    std::string::size_type pos = 0;
    std::string::size_type prev = 0;
    while ((pos = response.find("\r\n", prev)) != std::string::npos)
    {
        auto this_string = response.substr(prev, pos - prev);
        if (this_string == "")
        {
            break;
        }
        headers.push_back(this_string);
        prev = pos + 2;
    }
    headers.push_back(response.substr(prev));

    EXPECT_EQ(headers[0], "HTTP/1.1 200 OK");
    EXPECT_THAT(headers, testing::Contains("Strict-Transport-Security: "
                                           "max-age=31536000; "
                                           "includeSubdomains; preload"));
    EXPECT_THAT(headers, testing::Contains("X-Frame-Options: DENY"));
    EXPECT_THAT(headers, testing::Contains("Pragma: no-cache"));
    EXPECT_THAT(headers, testing::Contains("Cache-Control: no-Store,no-Cache"));
    EXPECT_THAT(headers, testing::Contains("X-XSS-Protection: 1; mode=block"));
    EXPECT_THAT(headers, testing::Contains("X-Content-Type-Options: nosniff"));
    EXPECT_THAT(headers, testing::Contains(
                             "Content-Security-Policy: default-src 'none'; "
                             "img-src 'self' data:; "
                             "font-src 'self'; "
                             "style-src 'self'; "
                             "script-src 'self'; "
                             "connect-src 'self' wss:; "
                             "form-action 'none'; "
                             "frame-ancestors 'none'; "
                             "object-src 'none'; "
                             "base-uri 'none'"));
    app.stop();
}
