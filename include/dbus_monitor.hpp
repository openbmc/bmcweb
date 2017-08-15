#pragma once
#include <dbus/filter.hpp>
#include <dbus/match.hpp>
#include <dbus_singleton.hpp>
#include <crow/app.h>
#include <boost/container/flat_map.hpp>

namespace crow {
namespace dbus_monitor {

struct DbusWebsocketSession {
  std::vector<std::unique_ptr<dbus::match>> matches;
  std::vector<dbus::filter> filters;
};

static boost::container::flat_map<crow::websocket::connection*,
                                  DbusWebsocketSession>
    sessions;

void on_property_update(dbus::filter& filter, boost::system::error_code ec,
                        dbus::message s) {
  if (!ec) {
    std::string object_name;
    std::vector<std::pair<std::string, dbus::dbus_variant>> values;
    s.unpack(object_name, values);
    nlohmann::json j;
    for (auto& value : values) {
      boost::apply_visitor([&](auto val) { j[s.get_path()] = val; },
                           value.second);
    }
    auto data_to_send = j.dump();

    for (auto& session : sessions) {
      session.first->send_text(data_to_send);
    }
  }
  filter.async_dispatch([&](boost::system::error_code ec, dbus::message s) {
    on_property_update(filter, ec, s);
  });
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
        sessions[&conn].matches.push_back(std::make_unique<dbus::match>(
            crow::connections::system_bus, std::move(match_string)));

        sessions[&conn].filters.emplace_back(
            crow::connections::system_bus, [path_namespace](dbus::message m) {
              return m.get_member() == "PropertiesChanged" &&
                     boost::starts_with(m.get_path(), path_namespace);
            });
        auto& this_filter = sessions[&conn].filters.back();
        this_filter.async_dispatch(
            [&](boost::system::error_code ec, dbus::message s) {
              on_property_update(this_filter, ec, s);
            });

      })
      .onclose([&](crow::websocket::connection& conn,
                   const std::string& reason) { sessions.erase(&conn); })
      .onmessage([&](crow::websocket::connection& conn, const std::string& data,
                     bool is_binary) {
        CROW_LOG_ERROR << "Got unexpected message from client on sensorws";
      });
}
}  // namespace redfish
}  // namespace crow
