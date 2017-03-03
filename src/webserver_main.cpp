#include "crow/ci_map.h"
#include "crow/http_parser_merged.h"
#include "crow/query_string.h"
//#include "crow/TinySHA1.hpp"
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

int main(int argc, char** argv) {
  auto worker = g3::LogWorker::createLogWorker();
  auto handle = worker->addDefaultLogger(argv[0], "/tmp/");
  g3::initializeLogging(worker.get());
  auto log_file_name = handle->call(&g3::FileSink::fileName);
  auto sink_handle = worker->addSink(std::make_unique<crow::ColorCoutSink>(), &crow::ColorCoutSink::ReceiveLogMessage);

  LOG(DEBUG) << "Logging to " << log_file_name.get() << "\n";

  std::string ssl_pem_file("server.pem");
  ensuressl::ensure_openssl_key_present_and_valid(ssl_pem_file);

  crow::App<crow::TokenAuthorizationMiddleware> app;

  crow::webassets::request_routes(app);

  crow::logger::setLogLevel(crow::LogLevel::DEBUG);

  app.port(18080).run();
}
