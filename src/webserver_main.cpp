#include "crow/ci_map.h"
#include "crow/http_parser_merged.h"
#include "crow/query_string.h"
#include "crow/app.h"
#include "crow/common.h"
#include "crow/dumb_timer_queue.h"
#include "crow/http_connection.h"
#include "crow/http_request.h"
#include "crow/http_response.h"
#include "crow/http_server.h"
#include "crow/json.h"
#include "crow/logging.h"
#include "crow/middleware.h"
#include "crow/middleware_context.h"
#include "crow/mustache.h"
#include "crow/parser.h"
#include "crow/routing.h"
#include "crow/settings.h"
#include "crow/socket_adaptors.h"
#include "crow/utility.h"
#include "crow/websocket.h"

#include "color_cout_g3_sink.hpp"
#include "token_authorization_middleware.hpp"
#include "webassets.hpp"

#include <iostream>
#include <string>
#include "ssl_key_handler.hpp"

#include <webassets.hpp>

crow::ssl_context_t get_ssl_context(std::string ssl_pem_file){
  crow::ssl_context_t m_ssl_context{boost::asio::ssl::context::sslv23};
  m_ssl_context.set_options(boost::asio::ssl::context::default_workarounds | boost::asio::ssl::context::no_sslv2 | boost::asio::ssl::context::no_sslv3 |
                            boost::asio::ssl::context::single_dh_use | boost::asio::ssl::context::no_tlsv1 | boost::asio::ssl::context::no_tlsv1_1);

  // m_ssl_context.set_verify_mode(boost::asio::ssl::verify_peer);
  m_ssl_context.use_certificate_file(ssl_pem_file, boost::asio::ssl::context::pem);
  m_ssl_context.use_private_key_file(ssl_pem_file, boost::asio::ssl::context::pem);

  // Set up EC curves to auto (boost asio doesn't have a method for this)
  // There is a pull request to add this.  Once this is included in an asio drop, use the right way
  // http://stackoverflow.com/questions/18929049/boost-asio-with-ecdsa-certificate-issue
  if (SSL_CTX_set_ecdh_auto(m_ssl_context.native_handle(), 1) != 1) {
    CROW_LOG_ERROR << "Error setting tmp ecdh list\n";
  }

  // From mozilla "compatibility"
  std::string ciphers =
      //"ECDHE-ECDSA-CHACHA20-POLY1305:"
      //"ECDHE-RSA-CHACHA20-POLY1305:"
      //"ECDHE-ECDSA-AES128-GCM-SHA256:"
      //"ECDHE-RSA-AES128-GCM-SHA256:"
      //"ECDHE-ECDSA-AES256-GCM-SHA384:"
      //"ECDHE-RSA-AES256-GCM-SHA384:"
      //"DHE-RSA-AES128-GCM-SHA256:"
      //"DHE-RSA-AES256-GCM-SHA384:"
      //"ECDHE-ECDSA-AES128-SHA256:"
      //"ECDHE-RSA-AES128-SHA256:"
      //"ECDHE-ECDSA-AES128-SHA:"
      //"ECDHE-RSA-AES256-SHA384:"
      //"ECDHE-RSA-AES128-SHA:"
      //"ECDHE-ECDSA-AES256-SHA384:"
      //"ECDHE-ECDSA-AES256-SHA:"
      //"ECDHE-RSA-AES256-SHA:"
      //"DHE-RSA-AES128-SHA256:"
      //"DHE-RSA-AES128-SHA:"
      //"DHE-RSA-AES256-SHA256:"
      //"DHE-RSA-AES256-SHA:"
      //"ECDHE-ECDSA-DES-CBC3-SHA:"
      //"ECDHE-RSA-DES-CBC3-SHA:"
      //"EDH-RSA-DES-CBC3-SHA:"
      "AES128-GCM-SHA256:"
      "AES256-GCM-SHA384:"
      "AES128-SHA256:"
      "AES256-SHA256:"
      "AES128-SHA:"
      "AES256-SHA:"
      "DES-CBC3-SHA:"
      "!DSS";

  // From mozilla "modern"
  std::string modern_ciphers =
      "ECDHE-ECDSA-AES256-GCM-SHA384:"
      "ECDHE-RSA-AES256-GCM-SHA384:"
      "ECDHE-ECDSA-CHACHA20-POLY1305:"
      "ECDHE-RSA-CHACHA20-POLY1305:"
      "ECDHE-ECDSA-AES128-GCM-SHA256:"
      "ECDHE-RSA-AES128-GCM-SHA256:"
      "ECDHE-ECDSA-AES256-SHA384:"
      "ECDHE-RSA-AES256-SHA384:"
      "ECDHE-ECDSA-AES128-SHA256:"
      "ECDHE-RSA-AES128-SHA256";

  if (SSL_CTX_set_cipher_list(m_ssl_context.native_handle(), ciphers.c_str()) != 1) {
    CROW_LOG_ERROR << "Error setting cipher list\n";
  }
  return m_ssl_context;
}


int main(int argc, char** argv) {
  auto worker(g3::LogWorker::createLogWorker());

  //TODO rotating logger isn't working super well
  //auto logger = worker->addSink(std::make_unique<LogRotate>("webserverlog", "/tmp/"),
  //                              &LogRotate::save);

  auto handle = worker->addDefaultLogger(argv[0], "/tmp/");
  g3::initializeLogging(worker.get());
  auto sink_handle = worker->addSink(std::make_unique<crow::ColorCoutSink>(), &crow::ColorCoutSink::ReceiveLogMessage);

  std::string ssl_pem_file("server.pem");
  ensuressl::ensure_openssl_key_present_and_valid(ssl_pem_file);

  //crow::App<crow::TokenAuthorizationMiddleware> app;
  crow::App<crow::TokenAuthorizationMiddleware> app;
  crow::webassets::request_routes(app);

  crow::logger::setLogLevel(crow::LogLevel::INFO);

  auto rules = app.get_rules();
  for (auto& rule : rules) {
    LOG(DEBUG) << "Static route: " << rule;
  }

  CROW_ROUTE(app, "/routes")
  ([&app]() {
    crow::json::wvalue routes;

    routes["routes"] = app.get_rules();
    return routes;
  });

  app.port(18080).ssl(std::move(get_ssl_context(ssl_pem_file))).run();
}
