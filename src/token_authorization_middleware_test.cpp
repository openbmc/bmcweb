#include "token_authorization_middleware.hpp"
#include <crow/app.h>
#include "gtest/gtest.h"

using namespace crow;
using namespace std;

// Tests that Base64 basic strings work
TEST(TokenAuthentication, TestBasicReject)
{
    App<crow::TokenAuthorizationMiddleware> app;
    decltype(app)::server_t server(&app, "127.0.0.1", 45451);
    CROW_ROUTE(app, "/")([]()
    {
        return 200;
    });
    auto _ = async(launch::async, [&]{server.run();});
    asio::io_service is;
    std::string sendmsg;

    static char buf[2048];

    // Homepage should be passed with no credentials
    sendmsg = "GET /\r\n\r\n";
    {
        asio::ip::tcp::socket c(is);
        c.connect(asio::ip::tcp::endpoint(asio::ip::address::from_string("127.0.0.1"), 45451));
        c.send(asio::buffer(sendmsg));
        auto received_count = c.receive(asio::buffer(buf, 2048));
        c.close();
        ASSERT_EQ("200", std::string(buf + 9, buf + 12));
    }

     // static should be passed with no credentials
    sendmsg = "GET /static/index.html\r\n\r\n";
    {
        asio::ip::tcp::socket c(is);
        c.connect(asio::ip::tcp::endpoint(asio::ip::address::from_string("127.0.0.1"), 45451));
        c.send(asio::buffer(sendmsg));
        c.receive(asio::buffer(buf, 2048));
        c.close();
        ASSERT_EQ("404", std::string(buf + 9, buf + 12));
    }

    
   server.stop();
   
}


// Tests that Base64 basic strings work
TEST(TokenAuthentication, TestSuccessfulLogin)
{
    App<crow::TokenAuthorizationMiddleware> app;
    app.bindaddr("127.0.0.1").port(45451);
    CROW_ROUTE(app, "/")([]()
    {
        return 200;
    });
    auto _ = async(launch::async, [&]{app.run();});

    asio::io_service is;
    static char buf[2048];
    //ASSERT_NO_THROW(
    // Other resources should not be passed
    std::string sendmsg = "GET /foo\r\n\r\n";
    asio::ip::tcp::socket c(is);
    c.connect(asio::ip::tcp::endpoint(asio::ip::address::from_string("127.0.0.1"), 45451));
    c.send(asio::buffer(sendmsg));
    auto received_count = c.receive(asio::buffer(buf, 2048));
    c.close();
    ASSERT_EQ("401", std::string(buf + 9, buf + 12));
    
    app.stop();
    raise;
   
}