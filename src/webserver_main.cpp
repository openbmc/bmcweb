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

using sensor_values = std::vector<std::pair<std::string, int32_t>>;

sensor_values read_sensor_values() {
  sensor_values values;
  DBusError err;

  int ret;
  bool stat;
  dbus_uint32_t level;

  // initialiset the errors
  dbus_error_init(&err);

  // connect to the system bus and check for errors
  DBusConnection* conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
  if (dbus_error_is_set(&err)) {
    fprintf(stderr, "Connection Error (%s)\n", err.message);
    dbus_error_free(&err);
  }
  if (NULL == conn) {
    exit(1);
  }

  // create a new method call and check for errors
  DBusMessage* msg = dbus_message_new_method_call(
      "org.openbmc.Sensors",                  // target for the method call
      "/org/openbmc/sensors/tach",            // object to call on
      "org.freedesktop.DBus.Introspectable",  // interface to call on
      "Introspect");                          // method name
  if (NULL == msg) {
    fprintf(stderr, "Message Null\n");
    exit(1);
  }

  DBusPendingCall* pending;
  // send message and get a handle for a reply
  if (!dbus_connection_send_with_reply(conn, msg, &pending,
                                       -1)) {  // -1 is default timeout
    fprintf(stderr, "Out Of Memory!\n");
    exit(1);
  }
  if (NULL == pending) {
    fprintf(stderr, "Pending Call Null\n");
    exit(1);
  }
  dbus_connection_flush(conn);

  // free message
  dbus_message_unref(msg);

  // block until we recieve a reply
  dbus_pending_call_block(pending);

  // get the reply message
  msg = dbus_pending_call_steal_reply(pending);
  if (NULL == msg) {
    fprintf(stderr, "Reply Null\n");
    exit(1);
  }
  // free the pending message handle
  dbus_pending_call_unref(pending);

  // read the parameters
  DBusMessageIter args;
  char* xml_struct = NULL;
  if (!dbus_message_iter_init(msg, &args)) {
    fprintf(stderr, "Message has no arguments!\n");
  }

  // read the arguments
  if (!dbus_message_iter_init(msg, &args)) {
    fprintf(stderr, "Message has no arguments!\n");
  } else if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&args)) {
    fprintf(stderr, "Argument is not string!\n");
  } else {
    dbus_message_iter_get_basic(&args, &xml_struct);
  }
  std::vector<std::string> methods;
  if (xml_struct != NULL) {
    std::string xml_data(xml_struct);
    std::vector<std::string> names;
    dbus::read_dbus_xml_names(xml_data, methods);
  }

  fprintf(stdout, "Found %zd sensors \n", methods.size());

  for (auto& method : methods) {
    // TODO(Ed) make sure sensor exposes SensorValue interface
    // create a new method call and check for errors
    DBusMessage* msg = dbus_message_new_method_call(
        "org.openbmc.Sensors",  // target for the method call
        ("/org/openbmc/sensors/tach/" + method).c_str(),  // object to call on
        "org.openbmc.SensorValue",  // interface to call on
        "getValue");                // method name
    if (NULL == msg) {
      fprintf(stderr, "Message Null\n");
      exit(1);
    }

    DBusPendingCall* pending;
    // send message and get a handle for a reply
    if (!dbus_connection_send_with_reply(conn, msg, &pending,
                                         -1)) {  // -1 is default timeout
      fprintf(stderr, "Out Of Memory!\n");
      exit(1);
    }
    if (NULL == pending) {
      fprintf(stderr, "Pending Call Null\n");
      exit(1);
    }
    dbus_connection_flush(conn);

    // free message
    dbus_message_unref(msg);

    // block until we recieve a reply
    dbus_pending_call_block(pending);

    // get the reply message
    msg = dbus_pending_call_steal_reply(pending);
    if (NULL == msg) {
      fprintf(stderr, "Reply Null\n");
      exit(1);
    }
    // free the pending message handle
    dbus_pending_call_unref(pending);

    // read the parameters
    DBusMessageIter args;
    int32_t value;
    if (!dbus_message_iter_init(msg, &args)) {
      fprintf(stderr, "Message has no arguments!\n");
    }

    // read the arguments
    if (!dbus_message_iter_init(msg, &args)) {
      fprintf(stderr, "Message has no arguments!\n");
    } else if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(&args)) {
      fprintf(stderr, "Argument is not string!\n");
    } else {
      DBusMessageIter sub;
      dbus_message_iter_recurse(&args, &sub);
      auto type = dbus_message_iter_get_arg_type(&sub);
      if (DBUS_TYPE_INT32 != type) {
        fprintf(stderr, "Variant subType is not int32 it is %d\n", type);
      } else {
        dbus_message_iter_get_basic(&sub, &value);
        values.emplace_back(method.c_str(), value);
      }
    }
  }

  // free reply and close connection
  dbus_message_unref(msg);
  return values;
}

int main(int argc, char** argv) {
  auto worker(g3::LogWorker::createLogWorker());
  if (false) {
    auto handle = worker->addDefaultLogger("bmcweb", "/tmp/");
  }
  g3::initializeLogging(worker.get());
  auto sink_handle = worker->addSink(std::make_unique<crow::ColorCoutSink>(),
                                     &crow::ColorCoutSink::ReceiveLogMessage);
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
        dbus::connection system_bus(conn.get_io_service(), dbus::bus::system);
        dbus::match ma(system_bus,
                       "type='signal',sender='org.freedesktop.DBus', "
                       "interface='org.freedesktop.DBus.Properties',member="
                       "'PropertiesChanged'");
        dbus::filter f(system_bus, [](dbus::message& m) { return true; });

        f.async_dispatch([&](boost::system::error_code ec, dbus::message s) {
          std::cout << "got event\n";
          //f.async_dispatch(event_handler);
        });

      })
      .onclose(
          [&](crow::websocket::connection& conn, const std::string& reason) {

          })
      .onmessage([&](crow::websocket::connection& conn, const std::string& data,
                     bool is_binary) {

      });

  CROW_ROUTE(app, "/sensortest")
  ([](const crow::request& req, crow::response& res) {
    crow::json::wvalue j;
    auto values = read_sensor_values();

    dbus::connection system_bus(*req.io_service, dbus::bus::system);
    dbus::endpoint test_daemon("org.openbmc.Sensors",
                               "/org/openbmc/sensors/tach",
                               "org.freedesktop.DBus.Introspectable");
    dbus::message m = dbus::message::new_call(test_daemon, "Introspect");
    system_bus.async_send(
        m,
        [&j, &system_bus](const boost::system::error_code ec, dbus::message r) {
          std::string xml;
          r.unpack(xml);
          std::vector<std::string> dbus_objects;
          dbus::read_dbus_xml_names(xml, dbus_objects);

          for (auto& object : dbus_objects) {
            dbus::endpoint test_daemon("org.openbmc.Sensors",
                                       "/org/openbmc/sensors/tach/" + object,
                                       "org.openbmc.SensorValue");
            dbus::message m2 = dbus::message::new_call(test_daemon, "getValue");
            
            system_bus.async_send(
                m2, [&](const boost::system::error_code ec, dbus::message r) {
                  int32_t value;
                  r.unpack(value);
                  // TODO(ed) if we ever go multithread, j needs a lock
                  j[object] = value;
                });
          }

        });

  });

  CROW_ROUTE(app, "/intel/firmwareupload")
      .methods("POST"_method)([](const crow::request& req) {
        // TODO(ed) handle errors here (file exists already and is locked, ect)
        std::ofstream out("/tmp/fw_update_image", std::ofstream::out |
                                                      std::ofstream::binary |
                                                      std::ofstream::trunc);
        out << req.body;
        out.close();

        crow::json::wvalue j;
        j["status"] = "Upload Successfull";

        return j;
      });

  LOG(DEBUG) << "Building SSL context";

  int port = 18080;

  LOG(DEBUG) << "Starting webserver on port " << port;
  app.port(port);
  if (enable_ssl) {
    LOG(DEBUG) << "SSL Enabled";
    auto ssl_context = ensuressl::get_ssl_context(ssl_pem_file);
    app.ssl(std::move(ssl_context));
  }
  //app.concurrency(4);
  app.run();
}
