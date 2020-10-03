#include <app.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>
#include <gzip_helper.hpp>
#include <webassets.hpp>

#include <sstream>

#include "gtest/gtest.h"
#include <gmock/gmock.h>

using namespace crow;
using namespace std;
using namespace testing;

// Tests static files are loaded correctly
TEST(Webassets, StaticFilesFixedRoutes)
{
    std::array<char, 2048> buf;
    SimpleApp app;
    webassets::requestRoutes(app);
    Server<SimpleApp> server(&app, "127.0.0.1", 45451);
    auto _ = async(launch::async, [&] { server.run(); });

    // get the homepage
    std::string sendmsg = "GET /\r\n\r\n";

    asio::io_context is;

    asio::ip::tcp::socket c(is);
    c.connect(asio::ip::tcp::endpoint(
        asio::ip::address::from_string("127.0.0.1"), 45451));

    c.send(asio::buffer(sendmsg));

    c.receive(asio::buffer(buf, 2048));
    c.close();

    std::string response(std::begin(buf), std::end(buf));
    // This is a routine to split strings until a newline is hit
    // TODO(ed) this should really use the HTTP parser
    std::vector<std::string> headers;
    std::string::size_type pos = 0;
    std::string::size_type prev = 0;
    int content_length = 0;
    std::string content_encoding("");
    while ((pos = response.find("\r\n", prev)) != std::string::npos)
    {
        auto this_string = response.substr(prev, pos - prev);
        if (this_string == "")
        {
            prev = pos + 2;
            break;
        }

        if (boost::starts_with(this_string, "Content-Length: "))
        {
            content_length = boost::lexical_cast<int>(this_string.substr(16));
            // TODO(ed) This is an unfortunate test, but it's all we have at
            // this point Realistically, the index.html will be more than 500
            // bytes.  This test will need to be improved at some point
            EXPECT_GT(content_length, 500);
        }
        if (boost::starts_with(this_string, "Content-Encoding: "))
        {
            content_encoding = this_string.substr(18);
        }

        headers.push_back(this_string);
        prev = pos + 2;
    }

    auto http_content = response.substr(prev);
    // TODO(ed) ideally the server should support non-compressed gzip assets.
    // Once this occurs, this line will be obsolete
    std::string ungziped_content = http_content;
    if (content_encoding == "gzip")
    {
        EXPECT_TRUE(gzipInflate(http_content, ungziped_content));
    }

    EXPECT_EQ(headers[0], "HTTP/1.1 200 OK");
    EXPECT_THAT(headers,
                ::testing::Contains("Content-Type: text/html;charset=UTF-8"));

    EXPECT_EQ(ungziped_content.substr(0, 21), "<!DOCTYPE html>\n<html");

    server.stop();
}

// Tests static files are loaded correctly
TEST(Webassets, EtagIsSane)
{
    std::array<char, 2048> buf;
    SimpleApp app;
    webassets::requestRoutes(app);
    Server<SimpleApp> server(&app, "127.0.0.1", 45451);
    auto _ = async(launch::async, [&] { server.run(); });

    // get the homepage
    std::string sendmsg = "GET /\r\n\r\n";

    asio::io_context is;

    asio::ip::tcp::socket c(is);
    c.connect(asio::ip::tcp::endpoint(
        asio::ip::address::from_string("127.0.0.1"), 45451));

    c.send(asio::buffer(sendmsg));

    c.receive(asio::buffer(buf, 2048));
    c.close();

    std::string response(std::begin(buf), std::end(buf));
    // This is a routine to split strings until a newline is hit
    // TODO(ed) this should really use the HTTP parser
    std::vector<std::string> headers;
    std::string::size_type pos = 0;
    std::string::size_type prev = 0;
    int content_length = 0;
    std::string content_encoding("");
    while ((pos = response.find("\r\n", prev)) != std::string::npos)
    {
        auto this_string = response.substr(prev, pos - prev);
        if (this_string == "")
        {
            break;
        }

        if (boost::starts_with(this_string, "ETag: "))
        {
            auto etag = this_string.substr(6);
            // ETAG should not be blank
            EXPECT_NE(etag, "");
            // SHa1 is 20 characters long
            EXPECT_EQ(etag.size(), 40);
            EXPECT_THAT(etag, MatchesRegex("^[a-f0-9]+$"));
        }

        headers.push_back(this_string);
        prev = pos + 2;
    }

    server.stop();
}
