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

#include "security_headers_middleware.hpp"
#include "ssl_key_handler.hpp"
#include "token_authorization_middleware.hpp"
#include "web_kvm.hpp"
#include "webassets.hpp"

#include <boost/asio.hpp>
#include <boost/endian/arithmetic.hpp>

#include <dbus/connection.hpp>
#include <dbus/endpoint.hpp>
#include <dbus/filter.hpp>
#include <dbus/match.hpp>
#include <dbus/message.hpp>
#include <dbus/utility.hpp>

#include <iostream>
#include <memory>
#include <string>
#include <unordered_set>

static std::shared_ptr<dbus::connection> system_bus;
static std::shared_ptr<dbus::match> sensor_match;
static std::shared_ptr<dbus::filter> sensor_filter;

std::unordered_set<crow::websocket::connection*> users;

void on_sensor_update(boost::system::error_code ec, dbus::message s) {
  std::string object_name;
  std::vector<std::pair<std::string, dbus::dbus_variant>> values;
  s.unpack(object_name).unpack(values);
  crow::json::wvalue j;
  for (auto& value : values) {
    // std::cout << "Got sensor value for " << s.get_path() << "\n";
    boost::apply_visitor([&](auto val) { j[s.get_path()] = val; },
                         value.second);
  }
  auto data_to_send = crow::json::dump(j);
  for (auto conn : users) {
    conn->send_text(data_to_send);
  }
  sensor_filter->async_dispatch(on_sensor_update);
};

int main(int argc, char** argv) {
  bool enable_ssl = true;
  std::string ssl_pem_file("server.pem");

  if (enable_ssl) {
    ensuressl::ensure_openssl_key_present_and_valid(ssl_pem_file);
  }

  crow::App<crow::TokenAuthorizationMiddleware, crow::SecurityHeadersMiddleware>
      app;

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

  CROW_ROUTE(app, "/sensorws")
      .websocket()
      .onopen([&](crow::websocket::connection& conn) {
        if (!system_bus) {
          system_bus = std::make_shared<dbus::connection>(conn.get_io_service(),
                                                          dbus::bus::system);
        }
        if (!sensor_match) {
          sensor_match = std::make_shared<dbus::match>(
              system_bus,
              "type='signal',path_namespace='/xyz/openbmc_project/sensors'");
        }
        if (!sensor_filter) {
          sensor_filter =
              std::make_shared<dbus::filter>(system_bus, [](dbus::message& m) {
                auto member = m.get_member();
                return member == "PropertiesChanged";
              });
          sensor_filter->async_dispatch(on_sensor_update);
        }

        users.insert(&conn);
      })
      .onclose(
          [&](crow::websocket::connection& conn, const std::string& reason) {
            // TODO(ed) needs lock
            users.erase(&conn);
          })
      .onmessage([&](crow::websocket::connection& conn, const std::string& data,
                     bool is_binary) {
        CROW_LOG_ERROR << "Got unexpected message from client on sensorws";
      });

  CROW_ROUTE(app, "/sensortest")
  ([](const crow::request& req, crow::response& res) {
    crow::json::wvalue j;

    dbus::connection system_bus(*req.io_service, dbus::bus::system);
    dbus::endpoint test_daemon("org.openbmc.Sensors",
                               "/org/openbmc/sensors/tach",
                               "org.freedesktop.DBus.Introspectable");
    dbus::message m = dbus::message::new_call(test_daemon, "Introspect");
    system_bus.async_send(
        m,
        [&j, &system_bus](const boost::system::error_code ec, dbus::message r) {
          if (ec) {
            
          } else {
            std::string xml;
            r.unpack(xml);
            std::vector<std::string> dbus_objects;
            dbus::read_dbus_xml_names(xml, dbus_objects);

            for (auto& object : dbus_objects) {
              dbus::endpoint test_daemon("org.openbmc.Sensors",
                                         "/org/openbmc/sensors/tach/" + object,
                                         "org.openbmc.SensorValue");
              dbus::message m2 =
                  dbus::message::new_call(test_daemon, "getValue");

              system_bus.async_send(
                  m2, [&](const boost::system::error_code ec, dbus::message r) {
                    int32_t value;
                    r.unpack(value);
                    // TODO(ed) if we ever go multithread, j needs a lock
                    j[object] = value;
                  });
            }
          }
        });

  });

  CROW_ROUTE(app, "/intel/firmwareupload")
      .methods("POST"_method)([](const crow::request& req) {
        // TODO(ed) handle errors here (file exists already and is locked, ect)
        std::ofstream out(
            "/tmp/fw_update_image",
            std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
        out << req.body;
        out.close();

        crow::json::wvalue j;
        j["status"] = "Upload Successfull";

        return j;
      });

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
  app.run();
}
