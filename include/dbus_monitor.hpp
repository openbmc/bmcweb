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

static boost::container::flat_map<crow::websocket::Connection*,
                                  DbusWebsocketSession>
    sessions;

int onPropertyUpdate(sd_bus_message* m, void* userdata,
                     sd_bus_error* ret_error) {
  if (ret_error == nullptr || sd_bus_error_is_set(ret_error)) {
    BMCWEB_LOG_ERROR << "Sdbus error in on_property_update";
    return 0;
  }
  sdbusplus::message::message message(m);
  std::string objectName;
  std::vector<
      std::pair<std::string, sdbusplus::message::variant<
                                 std::string, bool, int64_t, uint64_t, double>>>
      values;
  message.read(objectName, values);
  nlohmann::json j;
  const std::string& path = message.get_path();
  for (auto& value : values) {
    mapbox::util::apply_visitor([&](auto&& val) { j[path] = val; },
                                value.second);
  }
  std::string dataToSend = j.dump();

  for (const std::pair<crow::websocket::Connection*, DbusWebsocketSession>&
           session : sessions) {
    session.first->sendText(dataToSend);
  }
};

template <typename... Middlewares>
void requestRoutes(Crow<Middlewares...>& app) {
  BMCWEB_ROUTE(app, "/dbus_monitor")
      .websocket()
      .onopen([&](crow::websocket::Connection& conn) {
        std::string pathNamespace(conn.req.urlParams.get("path_namespace"));
        if (pathNamespace.empty()) {
          conn.sendText(
              nlohmann::json({"error", "Did not specify path_namespace"})
                  .dump());
          conn.close("error");
        }
        sessions[&conn] = DbusWebsocketSession();
        std::string matchString(
            "type='signal',"
            "interface='org.freedesktop.DBus.Properties',"
            "path_namespace='" +
            pathNamespace + "'");
        sessions[&conn].matches.emplace_back(
            std::make_unique<sdbusplus::bus::match::match>(
                *crow::connections::systemBus, matchString, onPropertyUpdate));
      })
      .onclose([&](crow::websocket::Connection& conn,
                   const std::string& reason) { sessions.erase(&conn); })
      .onmessage([&](crow::websocket::Connection& conn, const std::string& data,
                     bool is_binary) {
        BMCWEB_LOG_ERROR << "Got unexpected message from client on sensorws";
      });
}
}  // namespace dbus_monitor
}  // namespace crow
