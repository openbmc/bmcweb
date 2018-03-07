#pragma once
#include <dbus/filter.hpp>
#include <dbus/match.hpp>
#include <dbus_singleton.hpp>
#include <crow/app.h>
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/algorithm/string.hpp>

#include <chrono>
#include <cmath>
#include <iostream>
#include <string>


namespace crow {
namespace sensor_monitor {

class SensorStore;

struct SensorWebsocketSession {
  std::vector<std::unique_ptr<dbus::match>> matches;
  std::vector<dbus::filter> filters;
  std::shared_ptr<SensorStore> sensors;
};

static boost::container::flat_map<crow::websocket::connection*,
                                  SensorWebsocketSession>
    sessions;

static constexpr bool DEBUG = false;

using DbusSensor = std::pair<dbus::object_path, // sensor name
  std::vector<     // list of interfaces, xyz.openbmc_project.Sensor, etc.
    std::pair<std::string, // interface name
      std::vector<         // interface properties
        std::pair<std::string, dbus::dbus_variant>
      >
    >
  >
>;
using ManagedObjectDbusType = std::vector<DbusSensor>;
using GetSubTreeDbusType = std::vector<
  std::pair<std::string,
    std::vector<
      std::pair<std::string,
        std::vector<std::string>
      >
    >
  >
>;

class SensorStore {
  public:
    explicit SensorStore(crow::websocket::connection* websocket) :
      websocket_(websocket), outstanding_root_requests(0) {}
    SensorStore(SensorStore&& other) = delete;
    SensorStore(SensorStore& other) = delete;

    void PushNewSensors(const ManagedObjectDbusType& sensors) {
      nlohmann::json jsensors;
      for (const auto& sensor : sensors) {
        const auto& sensor_name = sensor.first.value;
        if (SensorNames.find(sensor_name) ==
            SensorNames.end()) {
          SensorNames.insert(sensor_name);
          auto iter = NewSensors.find(sensor_name);
          if (iter != NewSensors.end()) {
            NewSensors.erase(iter);
          }
          CROW_LOG_INFO << __FUNCTION__ << "(" << sensor.first.value << ")";
          // push this sensor to the websocket
          // form json with all parts
          nlohmann::json parts;
          parts.clear();
          for (const auto& iface : sensor.second) {
            // only deal with sensor interfaces (including thresholds)
            if (iface.first.find("xyz.openbmc_project.Sensor") == 0) {
              for (const auto& p : iface.second) {
                boost::apply_visitor([&](auto val) { parts[p.first] = val; },
                               p.second);
              }
            }
          }
          jsensors[sensor.first.value] = nlohmann::json(parts);
        }
      }
      if (jsensors.size() > 0) {
        auto data_to_send = jsensors.dump();
        websocket_->send_text(data_to_send);
      }
    }

    // call with name as "/" for all sensors, or full path for a single new sensor
    void GetSensorList(const std::string& sensorConnection,
        const std::string& name) {
      CROW_LOG_INFO << __FUNCTION__ << "(" << sensorConnection << ", " << name << ")";
      if (name == "/") {
        outstanding_root_requests++;
      }
      else if (NewSensors.find(name) != NewSensors.end()) {
        return;
      }
      // mark this name as in-progress
      NewSensors.insert(name);
      crow::connections::system_bus->async_method_call(
          [=](const boost::system::error_code ec,
            const ManagedObjectDbusType& managedObjects) {
            if (!ec) {
              PushNewSensors(managedObjects);
              if (name == "/") {
                outstanding_root_requests--;
              }
            } else {
              CROW_LOG_ERROR << ec;
            }
          },
          {sensorConnection.c_str(), name.c_str(),
          "org.freedesktop.DBus.ObjectManager", "GetManagedObjects"});
    }

    void GetSensorSubtree() {
      CROW_LOG_INFO << __FUNCTION__;
      constexpr int queryDepth = 2;
      const std::vector<std::string> interfaces = {"xyz.openbmc_project.Sensor"};

      crow::connections::system_bus->async_method_call(
          [&](const boost::system::error_code ec,
            const GetSubTreeDbusType& subtree) {
          if (!ec) {
            for (const auto& branch : subtree) {
              const auto& emitter = branch.second[0].first;
              if (SensorEmitters.find(emitter) ==
                  SensorEmitters.end()) {
                SensorEmitters.insert(emitter);
                GetSensorList(emitter, "/");
              } 
            }
          } else {
            CROW_LOG_ERROR << ec;
          }
        },
        {"xyz.openbmc_project.ObjectMapper", "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree"},
        "/xyz/openbmc_project/Sensors", queryDepth, interfaces);
    }
    bool ready() const {
      return outstanding_root_requests == 0 && SensorNames.size() > 0;
    }
    bool SensorPresent(const std::string& name) const {
      return SensorNames.find(name) != SensorNames.end();
    }

  private:
    // websocket for this sensor store
    crow::websocket::connection* websocket_;
    // mark as ready after enumeration
    int outstanding_root_requests;
    // all the services exporting the xyz.openbmc_project.Sensor interface
    boost::container::flat_set<std::string> SensorEmitters;
    // all the sensors that we have seen and reported to the websocket
    // sensor events from that we have not seen must be GetManagedObjects'd
    boost::container::flat_set<std::string> SensorNames;
    boost::container::flat_set<std::string> NewSensors;
};

void on_property_update(dbus::filter& filter, boost::system::error_code ec,
                        dbus::message s) {
  if (!ec) {
    std::string object_name;
    std::vector<std::pair<std::string, dbus::dbus_variant>> values;
    s.unpack(object_name, values);

    auto name = s.get_path();
    nlohmann::json j;
    for (auto& value : values) {
      boost::apply_visitor(
          [&](const auto& val) {
            j[name][value.first] = val;
          },
          value.second);
    }
    auto data_to_send = j.dump();

    for (auto& session : sessions) {
      if (session.second.sensors->ready()) {
        if (session.second.sensors->SensorPresent(name)) {
          session.first->send_text(data_to_send);
        }
        else {
          session.second.sensors->GetSensorList(s.get_sender(), name);
        }
      }
    }
  } else {
    CROW_LOG_ERROR << ec;
  }
  filter.async_dispatch([&](boost::system::error_code ec, dbus::message s) {
    on_property_update(filter, ec, s);
  });
};

template <typename... Middlewares>
void request_routes(Crow<Middlewares...>& app) {
  CROW_ROUTE(app, "/sensor_monitor")
      .websocket()
      .onopen([&](crow::websocket::connection& conn) {
        sessions[&conn] = SensorWebsocketSession();
        std::string match_string(
            "type='signal',"
            "interface='org.freedesktop.DBus.Properties',"
            "path_namespace='/xyz/openbmc_project/Sensors'");
        sessions[&conn].matches.push_back(std::make_unique<dbus::match>(
            crow::connections::system_bus, std::move(match_string)));

        sessions[&conn].filters.emplace_back(
            crow::connections::system_bus, [](dbus::message m) {
              return m.get_member() == "PropertiesChanged" &&
                     boost::starts_with(m.get_path(),
                         "/xyz/openbmc_project/Sensors/");
            });
        auto& this_filter = sessions[&conn].filters.back();
        this_filter.async_dispatch(
            [&](boost::system::error_code ec, dbus::message s) {
              on_property_update(this_filter, ec, s);
            });
        sessions[&conn].sensors = std::make_shared<SensorStore>(&conn);
        conn.get_io_service().post([&]() {
            sessions[&conn].sensors->GetSensorSubtree();
          });
      })
      .onclose([&](crow::websocket::connection& conn,
                   const std::string& reason) {
          sessions.erase(&conn);
          CROW_LOG_INFO << "websocket closed";
      })
      .onmessage([&](crow::websocket::connection& conn, const std::string& data,
                     bool is_binary) {
        CROW_LOG_ERROR << "Got unexpected message from client on sensorws";
      });
}
}  // namespace sensor_monitor
}  // namespace crow

/*

How this works:
1) at route registration:
  a) Add a filter to watch for sensor value changes (async events)
  b) post async enumeration of sensor providers (with async reply)
2) Async events later:
  a) enumerate:
      i) ask mapper for all providers that export the xyz.openbmc_project.Sensor
        interface
     ii) on async reply from mapper, for each provider, call GetManagedObjects
    iii) on async reply from providers, for each managed object not already in
         the cache, add a sensor to the cache
     iv) send all new sensors to websocket connection
      v) mark enumeration complete
  b) sensor value change events:
      i) if enumeration is not complete, ignore the update
     ii) if sensor exists in cache, update it, send updates to open websockets
    iii) if sensor doesn't exist in cache, re-enumerate provider for this sensor


JSON output looks like this:
{
  "/sensor/path/name": {
    "CriticalLow": V,
    "CriticalHigh": V,
    "WarningLow": V,
    "WarningHigh": V,
    "Scale": 1, (?)
    "Value": V,
    "Unit": U,
  },
  "/a/b/c/d": {
    other sensor
  },...
}


all items are optional except the value. No point in reporting without a value
if the sensor gets reported without the other stuff, it may get dropped if the
client-side javascript cache doesn't already know the rest of the unchanged
properties.

Sensor update JSON looks like this:
{
  "/sensor/path/name": {
    "Value": V
  },
  "/other/sensor/name": {
    "Value": V
  }
}
*/
