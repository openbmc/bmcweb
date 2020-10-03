#include <app.hpp>
#include <security_headers_middleware.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace crow;
using namespace std;

// Tests that the security headers are added correctly
TEST(SecurityHeaders, TestHeadersExist)
{
    App<SecurityHeadersMiddleware> app;
    app.bindaddr("127.0.0.1").port(45451);
    BMCWEB_ROUTE(app, "/")([]() { return boost::beast::http::status::ok; });
    auto _ = async(launch::async, [&] { app.run(); });

    asio::io_context is;
    std::array<char, 2048> buf;
    std::string sendmsg;

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

    // Test correct login credentials
    sendmsg = "GET /\r\n\r\n";

    asio::ip::tcp::socket c(is);
    c.connect(asio::ip::tcp::endpoint(
        asio::ip::address::from_string("127.0.0.1"), 45451));
    c.send(asio::buffer(sendmsg));
    c.receive(asio::buffer(buf));
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
    EXPECT_THAT(headers, ::testing::Contains("Strict-Transport-Security: "
                                             "max-age=31536000; "
                                             "includeSubdomains; preload"));
    EXPECT_THAT(headers, ::testing::Contains("X-UA-Compatible: IE=11"));
    EXPECT_THAT(headers, ::testing::Contains("X-Frame-Options: DENY"));
    EXPECT_THAT(headers,
                ::testing::Contains("X-XSS-Protection: 1; mode=block"));
    EXPECT_THAT(headers, ::testing::Contains(
                             "X-Content-Security-Policy: default-src 'self'"));
    app.stop();
}
