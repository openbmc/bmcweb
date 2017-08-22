#include <boost/asio.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/container/stable_vector.hpp>

#include "crow/app.h"
#include "crow/ci_map.h"
#include "crow/common.h"
#include "crow/dumb_timer_queue.h"
#include "crow/http_connection.h"
#include "crow/http_parser_merged.h"
#include "crow/http_request.h"
#include "crow/http_response.h"
#include "crow/http_server.h"
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

#include "redfish_v1.hpp"
#include "security_headers_middleware.hpp"
#include "ssl_key_handler.hpp"
#include "token_authorization_middleware.hpp"
#include "web_kvm.hpp"
#include "webassets.hpp"

#include "nlohmann/json.hpp"

#include <dbus/connection.hpp>
#include <dbus/endpoint.hpp>
#include <dbus/filter.hpp>
#include <dbus/match.hpp>
#include <dbus/message.hpp>

#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_set>

static std::shared_ptr<dbus::connection> system_bus;
static std::vector<dbus::match> dbus_matches;
static std::shared_ptr<dbus::filter> sensor_filter;

struct DbusWebsocketSession {
  std::vector<dbus::match> matches;
  std::vector<dbus::filter> filters;
};

static boost::container::flat_map<crow::websocket::connection*,
                                  DbusWebsocketSession>
    sessions;

void on_property_update(dbus::filter& filter, boost::system::error_code ec,
                        dbus::message s) {
  std::string object_name;
  std::vector<std::pair<std::string, dbus::dbus_variant>> values;
  s.unpack(object_name).unpack(values);
  nlohmann::json j;
  for (auto& value : values) {
    boost::apply_visitor([&](auto val) { j[s.get_path()] = val; },
                         value.second);
  }
  auto data_to_send = j.dump();

  for (auto& session : sessions) {
    session.first->send_text(data_to_send);
  }
  filter.async_dispatch([&](boost::system::error_code ec, dbus::message s) {
    on_property_update(filter, ec, s);;
  });
};

int main(int argc, char** argv) {
  // Build an io_service (there should only be 1)
  auto io = std::make_shared<boost::asio::io_service>();

  bool enable_ssl = true;
  std::string ssl_pem_file("server.pem");

  if (enable_ssl) {
    ensuressl::ensure_openssl_key_present_and_valid(ssl_pem_file);
  }

  crow::App<
      crow::TokenAuthorizationMiddleware, crow::SecurityHeadersMiddleware>
      app(io);

  crow::webassets::request_routes(app);
  crow::kvm::request_routes(app);
  crow::redfish::request_routes(app);

  crow::logger::setLogLevel(crow::LogLevel::INFO);

  CROW_ROUTE(app, "/dbus_monitor")
      .websocket()
      .onopen([&](crow::websocket::connection& conn) {
        sessions[&conn] = DbusWebsocketSession();

        sessions[&conn].matches.emplace_back(
            system_bus,
            "type='signal',path_namespace='/xyz/openbmc_project/sensors'");

        sessions[&conn].filters.emplace_back(system_bus, [](dbus::message m) {
          auto member = m.get_member();
          return member == "PropertiesChanged";
        });
        auto& this_filter = sessions[&conn].filters.back();
        this_filter.async_dispatch(
            [&](boost::system::error_code ec, dbus::message s) {
              on_property_update(this_filter, ec, s);;
            });

      })
      .onclose(
          [&](crow::websocket::connection& conn, const std::string& reason) {
            sessions.erase(&conn);
          })
      .onmessage([&](crow::websocket::connection& conn, const std::string& data,
                     bool is_binary) {
        CROW_LOG_ERROR << "Got unexpected message from client on sensorws";
      });

  CROW_ROUTE(app, "/intel/firmwareupload")
      .methods("POST"_method)([](const crow::request& req) {
        auto filepath = "/tmp/fw_update_image";
        std::ofstream out(filepath, std::ofstream::out | std::ofstream::binary |
                                        std::ofstream::trunc);
        out << req.body;
        out.close();

        nlohmann::json j;
        j["status"] = "Upload Successfull";

        dbus::endpoint fw_update_endpoint(
            "xyz.openbmc_project.fwupdate1.server",
            "/xyz/openbmc_project/fwupdate1", "xyz.openbmc_project.fwupdate1");

        auto m = dbus::message::new_call(fw_update_endpoint, "start");

        m.pack(std::string("file://") + filepath);
        system_bus->send(m);

        return j;
      });

  CROW_ROUTE(app, "/intel/system_config").methods("GET"_method)([]() {
    nlohmann::json j;
    std::ifstream file("/var/configuration/system.json");

    if(!file.good()){
      return crow::response(400);
    }
    file >> j;
    file.close();

    auto res = crow::response(200);
    res.json_value = j;
    return res;
  });

  crow::logger::setLogLevel(crow::LogLevel::DEBUG);
  auto test = app.get_routes();
  app.debug_print();
  std::cout << "Building SSL context\n";

  int port = 18080;

  std::cout << "Starting webserver on port " << port << "\n";
  app.port(port);
  if (enable_ssl) {
    std::cout << "SSL Enabled\n";
    auto ssl_context = ensuressl::get_ssl_context(ssl_pem_file);
    app.ssl(std::move(ssl_context));
  }
  // app.concurrency(4);

  // Start dbus connection
  system_bus = std::make_shared<dbus::connection>(*io, dbus::bus::system);

  app.run();
}
