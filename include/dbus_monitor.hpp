#pragma once
#include <dbus_singleton.hpp>
#include <sdbusplus/bus/match.hpp>
#include <crow/app.h>
#include <boost/container/flat_map.hpp>

namespace crow {
namespace dbus_monitor {

struct DbusWebsocketSession {
  std::vector<std::unique_ptr<sdbusplus::bus::match::match>> matches;
};

static boost::container::flat_map<crow::websocket::connection*,
                                  DbusWebsocketSession>
    sessions;

int on_property_update(sd_bus_message* m, void* userdata,
                       sd_bus_error* ret_error) {
  if (ret_error == nullptr || sd_bus_error_is_set(ret_error)) {
    CROW_LOG_ERROR << "Sdbus error in on_property_update";
    return 0;
  }
  sdbusplus::message::message message(m);
  std::string object_name;
  std::vector<
      std::pair<std::string, sdbusplus::message::variant<
                                 std::string, bool, int64_t, uint64_t, double>>>
      values;
  message.read(object_name, values);
  nlohmann::json j;
  const std::string& path = message.get_path();
  for (auto& value : values) {
    mapbox::util::apply_visitor([&](auto&& val) { j[path] = val; },
                                value.second);
  }
  std::string data_to_send = j.dump();

  for (const std::pair<crow::websocket::connection*, DbusWebsocketSession>&
           session : sessions) {
    session.first->send_text(data_to_send);
  }
};

template <typename... Middlewares>
void request_routes(Crow<Middlewares...>& app) {
  CROW_ROUTE(app, "/dbus_monitor")
      .websocket()
      .onopen([&](crow::websocket::connection& conn) {
        std::string path_namespace(conn.req.url_params.get("path_namespace"));
        if (path_namespace.empty()) {
          conn.send_text(
              nlohmann::json({"error", "Did not specify path_namespace"}));
          conn.close("error");
        }
        sessions[&conn] = DbusWebsocketSession();
        std::string match_string(
            "type='signal',"
            "interface='org.freedesktop.DBus.Properties',"
            "path_namespace='" +
            path_namespace + "'");
        sessions[&conn].matches.emplace_back(
            std::make_unique<sdbusplus::bus::match::match>(
                *crow::connections::system_bus, match_string,
                on_property_update));

      })
      .onclose([&](crow::websocket::connection& conn,
                   const std::string& reason) { sessions.erase(&conn); })
      .onmessage([&](crow::websocket::connection& conn, const std::string& data,
                     bool is_binary) {
        CROW_LOG_ERROR << "Got unexpected message from client on sensorws";
      });
}
}  // namespace dbus_monitor
}  // namespace crow
