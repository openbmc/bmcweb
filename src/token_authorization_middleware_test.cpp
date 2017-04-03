#include "token_authorization_middleware.hpp"
#include <crow/app.h>
#include "gtest/gtest.h"

using namespace crow;
using namespace std;

// Tests that static urls are correctly passed
TEST(TokenAuthentication, TestBasicReject) {
  App<crow::TokenAuthorizationMiddleware> app;
  decltype(app)::server_t server(&app, "127.0.0.1", 45451);
  CROW_ROUTE(app, "/")([]() { return 200; });
  auto _ = async(launch::async, [&] { server.run(); });
  asio::io_service is;
  std::string sendmsg;

  static char buf[2048];

  // Homepage should be passed with no credentials
  sendmsg = "GET /\r\n\r\n";
  {
    asio::ip::tcp::socket c(is);
    c.connect(asio::ip::tcp::endpoint(
        asio::ip::address::from_string("127.0.0.1"), 45451));
    c.send(asio::buffer(sendmsg));
    auto received_count = c.receive(asio::buffer(buf, 2048));
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

  server.stop();
}

// Tests that Base64 basic strings work
TEST(TokenAuthentication, TestRejectedResource) {
  App<crow::TokenAuthorizationMiddleware> app;
  app.bindaddr("127.0.0.1").port(45451);
  CROW_ROUTE(app, "/")([]() { return 200; });
  auto _ = async(launch::async, [&] { app.run(); });

  asio::io_service is;
  static char buf[2048];

  // Other resources should not be passed
  std::string sendmsg = "GET /foo\r\n\r\n";
  asio::ip::tcp::socket c(is);
  for (int i = 0; i < 200; i++) {
    try {
      c.connect(asio::ip::tcp::endpoint(
          asio::ip::address::from_string("127.0.0.1"), 45451));
    } catch (std::exception e) {
      // do nothing
    }
  }
  c.send(asio::buffer(sendmsg));
  auto received_count = c.receive(asio::buffer(buf, 2048));
  c.close();
  EXPECT_EQ("401", std::string(buf + 9, buf + 12));

  app.stop();
}

// Tests that Base64 basic strings work
TEST(TokenAuthentication, TestGetLoginUrl) {
  App<crow::TokenAuthorizationMiddleware> app;
  app.bindaddr("127.0.0.1").port(45451);
  CROW_ROUTE(app, "/")([]() { return 200; });
  auto _ = async(launch::async, [&] { app.run(); });

  asio::io_service is;
  static char buf[2048];

  // Other resources should not be passed
  std::string sendmsg = "GET /login\r\n\r\n";
  asio::ip::tcp::socket c(is);
  for (int i = 0; i < 200; i++) {
    try {
      c.connect(asio::ip::tcp::endpoint(
          asio::ip::address::from_string("127.0.0.1"), 45451));
    } catch (std::exception e) {
      // do nothing
    }
  }
  c.send(asio::buffer(sendmsg));
  auto received_count = c.receive(asio::buffer(buf, 2048));
  c.close();
  EXPECT_EQ("401", std::string(buf + 9, buf + 12));

  app.stop();
}

// Tests boundary conditions on login
TEST(TokenAuthentication, TestPostBadLoginUrl) {
  App<crow::TokenAuthorizationMiddleware> app;
  app.bindaddr("127.0.0.1").port(45451);
  CROW_ROUTE(app, "/")([]() { return 200; });
  auto _ = async(launch::async, [&] { app.run(); });

  asio::io_service is;
  std::array<char, 2048> buf;
  std::string sendmsg;

  auto send_to_localhost = [&is, &buf](std::string sendmsg) {
    asio::ip::tcp::socket c(is);
    c.connect(asio::ip::tcp::endpoint(
        asio::ip::address::from_string("127.0.0.1"), 45451));
    c.send(asio::buffer(sendmsg));
    auto received_count = c.receive(asio::buffer(buf));
    c.close();
  };

  {
    // Retry a couple of times waiting for the server to come up
    asio::ip::tcp::socket c(is);
    for (int i = 0; i < 200; i++) {
      try {
        c.connect(asio::ip::tcp::endpoint(
            asio::ip::address::from_string("127.0.0.1"), 45451));
        c.close();
        break;
      } catch (std::exception e) {
        // do nothing.  We expect this to fail while the server is starting up
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
  sendmsg =
      "POST /login\r\nContent-Length:38\r\n\r\n{\"username\": \"foo\", "
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

// Tests boundary conditions on login
TEST(TokenAuthentication, TestSuccessfulLogin) {
  App<crow::TokenAuthorizationMiddleware> app;
  app.bindaddr("127.0.0.1").port(45451);
  CROW_ROUTE(app, "/")([]() { return 200; });
  auto _ = async(launch::async, [&] { app.run(); });

  asio::io_service is;
  std::array<char, 2048> buf;
  std::string sendmsg;

  auto send_to_localhost = [&is, &buf](std::string sendmsg) {
    asio::ip::tcp::socket c(is);
    c.connect(asio::ip::tcp::endpoint(
        asio::ip::address::from_string("127.0.0.1"), 45451));
    c.send(asio::buffer(sendmsg));
    auto received_count = c.receive(asio::buffer(buf));
    c.close();
  };

  {
    // Retry a couple of times waiting for the server to come up
    asio::ip::tcp::socket c(is);
    for (int i = 0; i < 200; i++) {
      try {
        c.connect(asio::ip::tcp::endpoint(
            asio::ip::address::from_string("127.0.0.1"), 45451));
        c.close();
        break;
      } catch (std::exception e) {
        // do nothing.  We expect this to fail while the server is starting up
      }
    }
  }

  // Test correct login credentials
  sendmsg =
      "POST /login\r\nContent-Length:40\r\n\r\n{\"username\": \"dude\", "
      "\"password\": \"dude\"}\r\n";
  {
    send_to_localhost(sendmsg);
    auto return_code = std::string(&buf[9], &buf[12]);
    EXPECT_EQ("200", return_code);
  }


  // Try to use those login credentials to access a resource
  sendmsg =
      "GET /\r\nAuthorization: token\r\n\r\n{\"username\": \"dude\", "
      "\"password\": \"dude\"}\r\n";
  {
    send_to_localhost(sendmsg);
    auto return_code = std::string(&buf[9], &buf[12]);
    EXPECT_EQ("200", return_code);
  }

  app.stop();
}