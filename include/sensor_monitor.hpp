#pragma once
#include <dbus_singleton.hpp>
#include <crow/app.h>
#include <boost/algorithm/string.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/unordered_set.hpp>

#include <chrono>
#include <cmath>
#include <iostream>
#include <string>

namespace crow {
namespace sensor_monitor {

class SensorStore;

struct SensorWebsocketSession {
  std::vector<std::unique_ptr<sdbusplus::bus::match::match>> matches;
  std::shared_ptr<SensorStore> sensors;
};

static boost::container::flat_map<crow::websocket::connection*,
                                  SensorWebsocketSession>
    sessions;

static constexpr bool DEBUG = false;

using DbusSensor = std::pair<
    sdbusplus::message::object_path,  // sensor name
    std::vector<  // list of interfaces, xyz.openbmc_project.Sensor, etc.
        std::pair<std::string,  // interface name
                  std::vector<  // interface properties
                      std::pair<std::string, sdbusplus::message::variant<
                                                 double, int64_t>>>>>>;

// to reduce https traffic, set a periodic timer and send updates
// to the client every 0.25 seconds
static const boost::posix_time::milliseconds sensorInterval(250);

class SensorStore {
 public:
  explicit SensorStore(crow::websocket::connection* websocket)
      : websocket_(websocket),
        outstandingRootRequests(0),
        timer_(websocket->get_io_service()),
        updatesBusy_(0) {}
  SensorStore(SensorStore&& other) = delete;
  SensorStore(SensorStore& other) = delete;
  ~SensorStore() { timer_.cancel(); }

  void PushNewSensors(const std::vector<DbusSensor>& sensors) {
    nlohmann::json jsensors;
    for (const auto& sensor : sensors) {
      const std::string& sensor_name = sensor.first;
      auto inserted = SensorNames.insert(sensor_name);
      if (inserted.second) {
        // only report sensors that haven't been seen yet
        NewSensors.erase(sensor_name);
        CROW_LOG_INFO << __FUNCTION__ << "(" << sensor_name << ")";
        // push this sensor to the websocket
        // form json with all parts
        nlohmann::json parts;
        for (const auto& iface : sensor.second) {
          // only deal with sensor interfaces (including thresholds)
          if (iface.first.find("xyz.openbmc_project.Sensor") == 0) {
            for (const auto& p : iface.second) {
              mapbox::util::apply_visitor(
                  [&](auto&& val) { parts[p.first] = val; }, p.second);
            }
          }
        }
        jsensors[sensor_name] = std::move(parts);
      }
    }
    if (jsensors.size() > 0) {
      std::string data_to_send = jsensors.dump(4);
      websocket_->send_text(data_to_send);
    }
  }

  // call with name as "/" for all sensors, or full path for a single new sensor
  void GetSensorList(const std::string& sensorConnection,
                     const std::string& name) {
    CROW_LOG_INFO << __FUNCTION__ << "(" << sensorConnection << ", " << name
                  << ")";
    if (name == "/") {
      outstandingRootRequests++;
    } else if (NewSensors.find(name) != NewSensors.end()) {
      return;
    }
    // mark this name as in-progress
    NewSensors.insert(name);
    crow::connections::system_bus->async_method_call(
        [=](const boost::system::error_code ec,
            const std::vector<DbusSensor>& managedObjects) {
          if (!ec) {
            PushNewSensors(managedObjects);
            if (name == "/") {
              outstandingRootRequests--;
              if (ready()) {
                timer_.expires_from_now(sensorInterval);
                timer_.async_wait([=](const boost::system::error_code& ec) {
                  if (!ec) {
                    SendUpdates();
                  }
                });
              }
            }
          } else {
            CROW_LOG_ERROR << ec;
          }
        },
        sensorConnection.c_str(), name.c_str(),
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
  }

  void SendUpdates() {
    if (++updatesBusy_ == 1) {
      if (updates_.size() > 0) {
        std::string text = updates_.dump(4);
        updates_ = nlohmann::json();
        websocket_->send_text(text);
      }
    }
    updatesBusy_--;
    timer_.expires_from_now(sensorInterval);
    timer_.async_wait([&](const boost::system::error_code& ec) {
      if (!ec) {
        SendUpdates();
      }
    });
  }

  void GetSensorSubtree() {
    CROW_LOG_INFO << __FUNCTION__;
    constexpr int queryDepth = 2;
    const std::vector<std::string> interfaces = {"xyz.openbmc_project.Sensor"};

    crow::connections::system_bus->async_method_call(
        [&](const boost::system::error_code ec,
            const std::vector<std::pair<
                std::string,
                std::vector<std::pair<std::string, std::vector<std::string>>>>>&
                subtree) {
          if (!ec) {
            for (const auto& branch : subtree) {
              const auto& emitter = branch.second[0].first;
              if (SensorEmitters.insert(emitter).second) {
                GetSensorList(emitter, "/");
              }
            }
          } else {
            CROW_LOG_ERROR << ec;
          }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/Sensors", queryDepth, interfaces);
  }
  bool ready() const {
    return outstandingRootRequests == 0 && SensorNames.size() > 0;
  }
  bool SensorPresent(const std::string& name) const {
    return SensorNames.find(name) != SensorNames.end();
  }
  void updateSensor(const std::string& name, const std::string& key,
                    const sdbusplus::message::variant<double, int64_t>& value) {
    if (++updatesBusy_ == 1) {
      mapbox::util::apply_visitor(
          [&](auto&& val) { updates_[name][key] = val; }, value);
    }
    updatesBusy_--;
  }

 private:
  // websocket for this sensor store
  crow::websocket::connection* websocket_;
  // mark as ready after enumeration
  std::atomic<size_t> outstandingRootRequests;
  // all the services exporting the xyz.openbmc_project.Sensor interface
  boost::container::flat_set<std::string> SensorEmitters;
  // all the sensors that we have seen and reported to the websocket
  // sensor events from that we have not seen must be GetManagedObjects'd
  boost::unordered_set<std::string> SensorNames;
  boost::unordered_set<std::string> NewSensors;
  boost::asio::deadline_timer timer_;
  nlohmann::json updates_;
  std::atomic<size_t> updatesBusy_;
};

int on_property_update(sd_bus_message* m, void* userdata,
                       sd_bus_error* ret_error) {
  if (ret_error == nullptr || sd_bus_error_is_set(ret_error)) {
    CROW_LOG_ERROR << "Sdbus error in on_property_update";
    return 0;
  }
  std::string interface_name;
  std::vector<
      std::pair<std::string, sdbusplus::message::variant<double, int64_t>>>
      values;
  sdbusplus::message::message message(m);

  message.read(interface_name, values);

  const std::string& name = message.get_path();

  for (auto& session : sessions) {
    auto& sensors = session.second.sensors;
    if (sensors->ready()) {
      if (sensors->SensorPresent(name)) {
        for (auto& value : values) {
          sensors->updateSensor(name, value.first, value.second);
        }
      } else {
        sensors->GetSensorList(message.get_sender(), name);
      }
    }
  }
};

template <typename... Middlewares>
void request_routes(Crow<Middlewares...>& app) {
  CROW_ROUTE(app, "/sensor_monitor")
      .websocket()
      .onopen([&](crow::websocket::connection& conn) {

        auto& thisSession = sessions[&conn];
        thisSession = SensorWebsocketSession();
        std::string match_string(
            "type='signal',"
            "interface='org.freedesktop.DBus.Properties',"
            "path_namespace='/xyz/openbmc_project/Sensors'");
        thisSession.matches.emplace_back(
            std::make_unique<sdbusplus::bus::match::match>(
                *crow::connections::system_bus, match_string,
                on_property_update));

        thisSession.sensors = std::make_shared<SensorStore>(&conn);
        conn.get_io_service().post(
            [&]() { thisSession.sensors->GetSensorSubtree(); });
      })
      .onclose(
          [&](crow::websocket::connection& conn, const std::string& reason) {
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
