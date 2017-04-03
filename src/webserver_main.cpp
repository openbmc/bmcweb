#include <webassets.hpp>
#include <web_kvm.hpp>
#include "ssl_key_handler.hpp"

#include <crow/bmc_app_type.hpp>

#include "crow/app.h"
#include "crow/ci_map.h"
#include "crow/common.h"
#include "crow/dumb_timer_queue.h"
#include "crow/http_connection.h"
#include "crow/http_parser_merged.h"
#include "crow/http_request.h"
#include "crow/http_response.h"
#include "crow/http_server.h"
#include "crow/json.h"
#include "crow/logging.h"
#include "crow/middleware.h"
#include "crow/middleware_context.h"
#include "crow/mustache.h"
#include "crow/parser.h"
#include "crow/query_string.h"
#include "crow/routing.h"
#include "crow/settings.h"
#include "crow/socket_adaptors.h"
#include "crow/utility.h"
#include "crow/websocket.h"


#include "color_cout_g3_sink.hpp"
#include "webassets.hpp"


#include <boost/asio.hpp>
#include <boost/endian/arithmetic.hpp>


#include <iostream>
#include <memory>
#include <string>
#include <unordered_set>

int main(int argc, char** argv) {
  auto worker(g3::LogWorker::createLogWorker());
  std::string logger_name("bmcweb");
  std::string folder("/tmp/");
  auto handle = worker->addDefaultLogger(logger_name, folder);
  g3::initializeLogging(worker.get());
  auto sink_handle = worker->addSink(std::make_unique<crow::ColorCoutSink>(),
                                     &crow::ColorCoutSink::ReceiveLogMessage);

  std::string ssl_pem_file("server.pem");
  ensuressl::ensure_openssl_key_present_and_valid(ssl_pem_file);

  BmcAppType app;

  crow::webassets::request_routes(app);
  crow::kvm::request_routes(app);

  crow::logger::setLogLevel(crow::LogLevel::INFO);

  CROW_ROUTE(app, "/systeminfo")
  ([]() {

    crow::json::wvalue j;
    j["device_id"] = 0x7B;
    j["device_provides_sdrs"] = true;
    j["device_revision"] = true;
    j["device_available"] = true;
    j["firmware_revision"] = "0.68";

    j["ipmi_revision"] = "2.0";
    j["supports_chassis_device"] = true;
    j["supports_bridge"] = true;
    j["supports_ipmb_event_generator"] = true;
    j["supports_ipmb_event_receiver"] = true;
    j["supports_fru_inventory_device"] = true;
    j["supports_sel_device"] = true;
    j["supports_sdr_repository_device"] = true;
    j["supports_sensor_device"] = true;

    j["firmware_aux_revision"] = "0.60.foobar";

    return j;
  });

  CROW_ROUTE(app, "/ipmiws")
      .websocket()
      .onopen([&](crow::websocket::connection& conn) {

      })
      .onclose(
          [&](crow::websocket::connection& conn, const std::string& reason) {

          })
      .onmessage([&](crow::websocket::connection& conn, const std::string& data,
                     bool is_binary) {
        boost::asio::io_service io_service;
        using boost::asio::ip::udp;
        udp::resolver resolver(io_service);
        udp::resolver::query query(udp::v4(), "10.243.48.31", "623");
        udp::endpoint receiver_endpoint = *resolver.resolve(query);

        udp::socket socket(io_service);
        socket.open(udp::v4());

        socket.send_to(boost::asio::buffer(data), receiver_endpoint);

        std::array<char, 255> recv_buf;

        udp::endpoint sender_endpoint;
        size_t len =
            socket.receive_from(boost::asio::buffer(recv_buf), sender_endpoint);
        // TODO(ed) THis is ugly.  Find a way to not make a copy (ie, use
        // std::string::data() to 
        std::string str(std::begin(recv_buf), std::end(recv_buf));
        LOG(DEBUG) << "Got " << str << "back \n";
        conn.send_binary(str);

      });

  app.port(18080)
      //.ssl(std::move(ensuressl::get_ssl_context(ssl_pem_file)))
      .run();
}
