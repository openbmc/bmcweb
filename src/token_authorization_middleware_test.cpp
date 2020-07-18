#include "token_authorization_middleware.hpp"

#include <condition_variable>
#include <future>
#include <mutex>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using namespace crow;

class TokenAuth : public ::testing::Test
{
  public:
    TokenAuth() :
        lk(std::unique_lock<std::mutex>(m)),
        io(std::make_shared<boost::asio::io_context>())
    {}

    std::mutex m;
    std::condition_variable cv;
    std::unique_lock<std::mutex> lk;
    std::shared_ptr<boost::asio::io_context> io;
    int testPort = 45451;
};

TEST_F(TokenAuth, SpecialResourcesAreAcceptedWithoutAuth)
{
    App app(io);
    crow::token_authorization::requestRoutes(app);
    BMCWEB_ROUTE(app, "/redfish/v1")
    ([]() { return boost::beast::http::status::ok; });
    auto _ = std::async(std::launch::async, [&] {
        app.port(testPort).run();
        cv.notify_one();
        io->run();
    });

    asio::io_context is;
    std::string sendmsg;

    static char buf[2048];

    // Homepage should be passed with no credentials
    sendmsg = "GET /\r\n\r\n";
    {
        asio::ip::tcp::socket c(is);
        c.connect(asio::ip::tcp::endpoint(
            asio::ip::address::from_string("127.0.0.1"), 45451));
        c.send(asio::buffer(sendmsg));
        c.receive(asio::buffer(buf, 2048));
        c.close();
        EXPECT_EQ("200", std::string(buf + 9, buf + 12));
    }

    // static should be passed with no credentials
    sendmsg = "GET /static/index.html\r\n\r\n";
    {
        asio::ip::tcp::socket c(is);
        c.connect(asio::ip::tcp::endpoint(
            asio::ip::address::from_string("127.0.0.1"), 45451));
        c.send(asio::buffer(sendmsg));
        c.receive(asio::buffer(buf, 2048));
        c.close();
        EXPECT_EQ("404", std::string(buf + 9, buf + 12));
    }

    app.stop();
}

// Tests that Base64 basic strings work
TEST(TokenAuthentication, TestRejectedResource)
{
    App app;
    app.bindaddr("127.0.0.1").port(45451);
    BMCWEB_ROUTE(app, "/")([]() { return boost::beast::http::status::ok; });
    auto _ = async(std::launch::async, [&] { app.run(); });

    asio::io_context is;
    static char buf[2048];

    // Other resources should not be passed
    std::string sendmsg = "GET /foo\r\n\r\n";
    asio::ip::tcp::socket c(is);
    for (int i = 0; i < 200; i++)
    {
        try
        {
            c.connect(asio::ip::tcp::endpoint(
                asio::ip::address::from_string("127.0.0.1"), 45451));
        }
        catch (std::exception e)
        {
            // do nothing
        }
    }
    c.send(asio::buffer(sendmsg));
    c.receive(asio::buffer(buf, 2048));
    c.close();
    EXPECT_EQ("401", std::string(buf + 9, buf + 12));

    app.stop();
}

// Tests that Base64 basic strings work
TEST(TokenAuthentication, TestGetLoginUrl)
{
    App app;
    app.bindaddr("127.0.0.1").port(45451);
    BMCWEB_ROUTE(app, "/")([]() { return boost::beast::http::status::ok; });
    auto _ = async(std::launch::async, [&] { app.run(); });

    asio::io_context is;
    static char buf[2048];

    // Other resources should not be passed
    std::string sendmsg = "GET /login\r\n\r\n";
    asio::ip::tcp::socket c(is);
    for (int i = 0; i < 200; i++)
    {
        try
        {
            c.connect(asio::ip::tcp::endpoint(
                asio::ip::address::from_string("127.0.0.1"), 45451));
        }
        catch (std::exception e)
        {
            // do nothing
        }
    }
    c.send(asio::buffer(sendmsg));
    c.receive(asio::buffer(buf, 2048));
    c.close();
    EXPECT_EQ("401", std::string(buf + 9, buf + 12));

    app.stop();
}

// Tests boundary conditions on login
TEST(TokenAuthentication, TestPostBadLoginUrl)
{
    App app;
    app.bindaddr("127.0.0.1").port(45451);
    BMCWEB_ROUTE(app, "/")([]() { return boost::beast::http::status::ok; });
    auto _ = async(std::launch::async, [&] { app.run(); });

    asio::io_context is;
    std::array<char, 2048> buf;
    std::string sendmsg;

    auto send_to_localhost = [&is, &buf](std::string sendmsg) {
        asio::ip::tcp::socket c(is);
        c.connect(asio::ip::tcp::endpoint(
            asio::ip::address::from_string("127.0.0.1"), 45451));
        c.send(asio::buffer(sendmsg));
        c.receive(asio::buffer(buf));
        c.close();
    };

    {
        // Retry a couple of times waiting for the server to come up
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

    // Test blank login credentials
    sendmsg = "POST /login\r\nContent-Length:0\r\n\r\n\r\n";
    {
        send_to_localhost(sendmsg);
        auto return_code = std::string(&buf[9], &buf[12]);
        EXPECT_EQ("400", return_code);
    }

    // Test wrong login credentials
    sendmsg = "POST /login\r\nContent-Length:38\r\n\r\n{\"username\": \"foo\", "
              "\"password\": \"bar\"}\r\n";
    {
        send_to_localhost(sendmsg);
        auto return_code = std::string(&buf[9], &buf[12]);
        EXPECT_EQ("401", return_code);
        // TODO(ed) need to test more here.  Response string?
    }

    // Test only sending a username
    sendmsg =
        "POST /login\r\nContent-Length:19\r\n\r\n{\"username\": \"foo\"}\r\n";
    {
        send_to_localhost(sendmsg);
        auto return_code = std::string(&buf[9], &buf[12]);
        EXPECT_EQ("400", return_code);
    }

    // Test only sending a password
    sendmsg =
        "POST /login\r\nContent-Length:19\r\n\r\n{\"password\": \"foo\"}\r\n";
    {
        send_to_localhost(sendmsg);
        auto return_code = std::string(&buf[9], &buf[12]);
        EXPECT_EQ("400", return_code);
    }

    app.stop();
}

// Test class that allows login for a fixed password.
class KnownLoginAuthenticator
{
  public:
    inline bool authenticate(const std::string& username,
                             const std::string& password)
    {
        return (username == "dude") && (password == "foo");
    }
};

TEST(TokenAuthentication, TestSuccessfulLogin)
{
    App app;
    app.bindaddr("127.0.0.1").port(45451);
    BMCWEB_ROUTE(app, "/")([]() { return boost::beast::http::status::ok; });
    auto _ = async(std::launch::async, [&] { app.run(); });

    asio::io_context is;
    std::array<char, 2048> buf;
    std::string sendmsg;

    auto send_to_localhost = [&is, &buf](std::string sendmsg) {
        asio::ip::tcp::socket c(is);
        c.connect(asio::ip::tcp::endpoint(
            asio::ip::address::from_string("127.0.0.1"), 45451));
        c.send(asio::buffer(sendmsg));
        c.receive(asio::buffer(buf));
        c.close();
    };

    {
        // Retry a couple of times waiting for the server to come up
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
    sendmsg =
        "POST /login\r\nContent-Length:40\r\n\r\n{\"username\": \"dude\", "
        "\"password\": \"foo\"}\r\n";
    {
        send_to_localhost(sendmsg);
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

            headers.push_back(this_string);
            prev = pos + 2;
        }
        EXPECT_EQ(headers[0], "HTTP/1.1 200 OK");
        EXPECT_THAT(headers,
                    testing::Contains("Content-Type: application/json"));
        auto http_content = response.substr(prev);
    }

    // Try to use those login credentials to access a resource
    sendmsg = "GET /\r\nAuthorization: token\r\n\r\n{\"username\": \"dude\", "
              "\"password\": \"dude\"}\r\n";
    {
        send_to_localhost(sendmsg);
        auto return_code = std::string(&buf[9], &buf[12]);
        EXPECT_EQ("200", return_code);
    }

    app.stop();
}
