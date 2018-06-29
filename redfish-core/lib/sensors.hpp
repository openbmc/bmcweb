/*
// Copyright (c) 2018 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
#pragma once

#include <math.h>
#include <dbus_singleton.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/range/algorithm/replace_copy_if.hpp>

namespace redfish {

constexpr const char* DBUS_SENSOR_PREFIX = "/xyz/openbmc_project/Sensors/";

using GetSubTreeType = std::vector<
    std::pair<std::string,
              std::vector<std::pair<std::string, std::vector<std::string>>>>>;

using SensorVariant = sdbusplus::message::variant<int64_t, double>;

using ManagedObjectsVectorType = std::vector<std::pair<
    sdbusplus::message::object_path,
    boost::container::flat_map<
        std::string, boost::container::flat_map<std::string, SensorVariant>>>>;

/**
 * SensorsAsyncResp
 * Gathers data needed for response processing after async calls are done
 */
class SensorsAsyncResp {
 public:
  SensorsAsyncResp(crow::response& response, const std::string& chassisId,
                   const std::initializer_list<const char*> types)
      : res(response), chassisId(chassisId), types(types) {
    res.json_value["@odata.id"] =
        "/redfish/v1/Chassis/" + chassisId + "/Thermal";
  }

  ~SensorsAsyncResp() {
    if (res.result() == boost::beast::http::status::internal_server_error) {
      // Reset the json object to clear out any data that made it in before the
      // error happened
      // todo(ed) handle error condition with proper code
      res.json_value = nlohmann::json::object();
    }
    res.end();
  }

  void setErrorStatus() {
    res.result(boost::beast::http::status::internal_server_error);
  }

  crow::response& res;
  std::string chassisId{};
  const std::vector<const char*> types;
};

/**
 * @brief Creates connections necessary for chassis sensors
 * @param SensorsAsyncResp Pointer to object holding response data
 * @param sensorNames Sensors retrieved from chassis
 * @param callback Callback for processing gathered connections
 */
template <typename Callback>
void getConnections(std::shared_ptr<SensorsAsyncResp> SensorsAsyncResp,
                    const boost::container::flat_set<std::string>& sensorNames,
                    Callback&& callback) {
  CROW_LOG_DEBUG << "getConnections enter";
  const std::string path = "/xyz/openbmc_project/Sensors";
  const std::array<std::string, 1> interfaces = {
      "xyz.openbmc_project.Sensor.Value"};

  // Response handler for parsing objects subtree
  auto resp_handler =
      [ callback{std::move(callback)}, SensorsAsyncResp, sensorNames ](
          const boost::system::error_code ec, const GetSubTreeType& subtree) {
    CROW_LOG_DEBUG << "getConnections resp_handler enter";
    if (ec) {
      SensorsAsyncResp->setErrorStatus();
      CROW_LOG_ERROR << "getConnections resp_handler: Dbus error " << ec;
      return;
    }

    CROW_LOG_DEBUG << "Found " << subtree.size() << " subtrees";

    // Make unique list of connections only for requested sensor types and
    // found in the chassis
    boost::container::flat_set<std::string> connections;
    // Intrinsic to avoid malloc.  Most systems will have < 8 sensor producers
    connections.reserve(8);

    CROW_LOG_DEBUG << "sensorNames list count: " << sensorNames.size();
    for (const std::string& tsensor : sensorNames) {
      CROW_LOG_DEBUG << "Sensor to find: " << tsensor;
    }

    for (const std::pair<
             std::string,
             std::vector<std::pair<std::string, std::vector<std::string>>>>&
             object : subtree) {
      for (const char* type : SensorsAsyncResp->types) {
        if (boost::starts_with(object.first, type)) {
          auto lastPos = object.first.rfind('/');
          if (lastPos != std::string::npos) {
            std::string sensorName = object.first.substr(lastPos + 1);

            if (sensorNames.find(sensorName) != sensorNames.end()) {
              // For each connection name
              for (const std::pair<std::string, std::vector<std::string>>&
                       objData : object.second) {
                CROW_LOG_DEBUG << "Adding connection: " << objData.first;
                connections.insert(objData.first);
              }
            }
          }
          break;
        }
      }
    }
    CROW_LOG_DEBUG << "Found " << connections.size() << " connections";
    callback(std::move(connections));
    CROW_LOG_DEBUG << "getConnections resp_handler exit";
  };

  // Make call to ObjectMapper to find all sensors objects
  crow::connections::system_bus->async_method_call(
      std::move(resp_handler), "xyz.openbmc_project.ObjectMapper",
      "/xyz/openbmc_project/object_mapper", "xyz.openbmc_project.ObjectMapper",
      "GetSubTree", path, 2, interfaces);
  CROW_LOG_DEBUG << "getConnections exit";
}

/**
 * @brief Retrieves requested chassis sensors and redundancy data from DBus .
 * @param SensorsAsyncResp   Pointer to object holding response data
 * @param callback  Callback for next step in gathered sensor processing
 */
template <typename Callback>
void getChassis(std::shared_ptr<SensorsAsyncResp> SensorsAsyncResp,
                Callback&& callback) {
  CROW_LOG_DEBUG << "getChassis enter";
  // Process response from EntityManager and extract chassis data
  auto resp_handler = [ callback{std::move(callback)}, SensorsAsyncResp ](
      const boost::system::error_code ec, ManagedObjectsVectorType& resp) {
    CROW_LOG_DEBUG << "getChassis resp_handler enter";
    if (ec) {
      CROW_LOG_ERROR << "getChassis resp_handler DBUS error: " << ec;
      SensorsAsyncResp->setErrorStatus();
      return;
    }
    boost::container::flat_set<std::string> sensorNames;

    //   SensorsAsyncResp->chassisId
    bool foundChassis = false;
    std::vector<std::string> split;
    // Reserve space for
    // /xyz/openbmc_project/inventory/<name>/<subname> + 3 subnames
    split.reserve(8);

    for (const auto& objDictEntry : resp) {
      const std::string& objectPath =
          static_cast<const std::string&>(objDictEntry.first);
      boost::algorithm::split(split, objectPath, boost::is_any_of("/"));
      if (split.size() < 2) {
        CROW_LOG_ERROR << "Got path that isn't long enough " << objectPath;
        split.clear();
        continue;
      }
      const std::string& sensorName = split.end()[-1];
      const std::string& chassisName = split.end()[-2];

      if (chassisName != SensorsAsyncResp->chassisId) {
        split.clear();
        continue;
      }
      CROW_LOG_DEBUG << "New sensor: " << sensorName;
      foundChassis = true;
      sensorNames.emplace(sensorName);
      split.clear();
    };
    CROW_LOG_DEBUG << "Found " << sensorNames.size() << " Sensor names";

    if (!foundChassis) {
      CROW_LOG_INFO << "Unable to find chassis named "
                    << SensorsAsyncResp->chassisId;
      SensorsAsyncResp->res.result(boost::beast::http::status::not_found);
    } else {
      callback(sensorNames);
    }
    CROW_LOG_DEBUG << "getChassis resp_handler exit";
  };

  // Make call to EntityManager to find all chassis objects
  crow::connections::system_bus->async_method_call(
      resp_handler, "xyz.openbmc_project.EntityManager", "/",
      "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
  CROW_LOG_DEBUG << "getChassis exit";
}

/**
 * @brief Builds a json sensor representation of a sensor.
 * @param sensorName  The name of the sensor to be built
 * @param sensorType  The type (temperature, fan_tach, etc) of the sensor to
 * build
 * @param interfacesDict  A dictionary of the interfaces and properties of said
 * interfaces to be built from
 * @param sensor_json  The json object to fill
 */
void objectInterfacesToJson(
    const std::string& sensorName, const std::string& sensorType,
    const boost::container::flat_map<
        std::string, boost::container::flat_map<std::string, SensorVariant>>&
        interfacesDict,
    nlohmann::json& sensor_json) {
  // We need a value interface before we can do anything with it
  auto value_it = interfacesDict.find("xyz.openbmc_project.Sensor.Value");
  if (value_it == interfacesDict.end()) {
    CROW_LOG_ERROR << "Sensor doesn't have a value interface";
    return;
  }

  // Assume values exist as is (10^0 == 1) if no scale exists
  int64_t scaleMultiplier = 0;

  auto scale_it = value_it->second.find("Scale");
  // If a scale exists, pull value as int64, and use the scaling.
  if (scale_it != value_it->second.end()) {
    const int64_t* int64Value =
        mapbox::get_ptr<const int64_t>(scale_it->second);
    if (int64Value != nullptr) {
      scaleMultiplier = *int64Value;
    }
  }

  sensor_json["MemberId"] = sensorName;
  sensor_json["Name"] = sensorName;
  sensor_json["Status"]["State"] = "Enabled";
  sensor_json["Status"]["Health"] = "OK";

  // Parameter to set to override the type we get from dbus, and force it to
  // int, regardless of what is available.  This is used for schemas like fan,
  // that require integers, not floats.
  bool forceToInt = false;

  const char* unit = "Reading";
  if (sensorType == "temperature") {
    unit = "ReadingCelsius";
    sensor_json["@odata.type"] = "#Thermal.v1_3_0.Temperature";
    // TODO(ed) Documentation says that path should be type fan_tach,
    // implementation seems to implement fan
  } else if (sensorType == "fan" || sensorType == "fan_type") {
    unit = "Reading";
    sensor_json["ReadingUnits"] = "RPM";
    sensor_json["@odata.type"] = "#Thermal.v1_3_0.Fan";
    forceToInt = true;
  } else if (sensorType == "voltage") {
    unit = "ReadingVolts";
    sensor_json["@odata.type"] = "#Power.v1_0_0.Voltage";
  } else {
    CROW_LOG_ERROR << "Redfish cannot map object type for " << sensorName;
    return;
  }
  // Map of dbus interface name, dbus property name and redfish property_name
  std::vector<std::tuple<const char*, const char*, const char*>> properties;
  properties.reserve(7);

  properties.emplace_back("xyz.openbmc_project.Sensor.Value", "Value", unit);
  properties.emplace_back("xyz.openbmc_project.Sensor.Threshold.Warning",
                          "WarningHigh", "UpperThresholdNonCritical");
  properties.emplace_back("xyz.openbmc_project.Sensor.Threshold.Warning",
                          "WarningLow", "LowerThresholdNonCritical");
  properties.emplace_back("xyz.openbmc_project.Sensor.Threshold.Critical",
                          "CriticalHigh", "UpperThresholdCritical");
  properties.emplace_back("xyz.openbmc_project.Sensor.Threshold.Critical",
                          "CriticalLow", "LowerThresholdCritical");

  if (sensorType == "temperature") {
    properties.emplace_back("xyz.openbmc_project.Sensor.Value", "MinValue",
                            "MinReadingRangeTemp");
    properties.emplace_back("xyz.openbmc_project.Sensor.Value", "MaxValue",
                            "MaxReadingRangeTemp");
  } else {
    properties.emplace_back("xyz.openbmc_project.Sensor.Value", "MinValue",
                            "MinReadingRange");
    properties.emplace_back("xyz.openbmc_project.Sensor.Value", "MaxValue",
                            "MaxReadingRange");
  }

  for (const std::tuple<const char*, const char*, const char*>& p :
       properties) {
    auto interfaceProperties = interfacesDict.find(std::get<0>(p));
    if (interfaceProperties != interfacesDict.end()) {
      auto value_it = interfaceProperties->second.find(std::get<1>(p));
      if (value_it != interfaceProperties->second.end()) {
        const SensorVariant& valueVariant = value_it->second;
        nlohmann::json& value_it = sensor_json[std::get<2>(p)];

        // Attempt to pull the int64 directly
        const int64_t* int64Value =
            mapbox::get_ptr<const int64_t>(valueVariant);

        if (int64Value != nullptr) {
          if (forceToInt || scaleMultiplier >= 0) {
            value_it = *int64Value * std::pow(10, scaleMultiplier);
          } else {
            value_it = *int64Value *
                       std::pow(10, static_cast<double>(scaleMultiplier));
          }
        }
        // Attempt to pull the float directly
        const double* doubleValue = mapbox::get_ptr<const double>(valueVariant);

        if (doubleValue != nullptr) {
          if (!forceToInt) {
            value_it = *doubleValue *
                       std::pow(10, static_cast<double>(scaleMultiplier));
          } else {
            value_it = static_cast<int64_t>(*doubleValue *
                                            std::pow(10, scaleMultiplier));
          }
        }
      }
    }
  }
  CROW_LOG_DEBUG << "Added sensor " << sensorName;
}

/**
 * @brief Entry point for retrieving sensors data related to requested
 *        chassis.
 * @param SensorsAsyncResp   Pointer to object holding response data
 */
void getChassisData(std::shared_ptr<SensorsAsyncResp> SensorsAsyncResp) {
  CROW_LOG_DEBUG << "getChassisData enter";
  auto getChassisCb = [&, SensorsAsyncResp](
                          boost::container::flat_set<std::string>&
                              sensorNames) {
    CROW_LOG_DEBUG << "getChassisCb enter";
    auto getConnectionCb =
        [&, SensorsAsyncResp, sensorNames](
            const boost::container::flat_set<std::string>& connections) {
          CROW_LOG_DEBUG << "getConnectionCb enter";
          // Get managed objects from all services exposing sensors
          for (const std::string& connection : connections) {
            // Response handler to process managed objects
            auto getManagedObjectsCb = [&, SensorsAsyncResp, sensorNames](
                                           const boost::system::error_code ec,
                                           ManagedObjectsVectorType& resp) {
              CROW_LOG_DEBUG << "getManagedObjectsCb enter";
              if (ec) {
                CROW_LOG_ERROR << "getManagedObjectsCb DBUS error: " << ec;
                SensorsAsyncResp->setErrorStatus();
                return;
              }
              // Go through all objects and update response with
              // sensor data
              for (const auto& objDictEntry : resp) {
                const std::string& objPath =
                    static_cast<const std::string&>(objDictEntry.first);
                CROW_LOG_DEBUG << "getManagedObjectsCb parsing object "
                               << objPath;

                std::vector<std::string> split;
                // Reserve space for
                // /xyz/openbmc_project/Sensors/<name>/<subname>
                split.reserve(6);
                boost::algorithm::split(split, objPath, boost::is_any_of("/"));
                if (split.size() < 6) {
                  CROW_LOG_ERROR << "Got path that isn't long enough "
                                 << objPath;
                  continue;
                }
                // These indexes aren't intuitive, as boost::split puts an empty
                // string at the beggining
                const std::string& sensorType = split[4];
                const std::string& sensorName = split[5];
                CROW_LOG_DEBUG << "sensorName " << sensorName << " sensorType "
                               << sensorType;
                if (sensorNames.find(sensorName) == sensorNames.end()) {
                  CROW_LOG_ERROR << sensorName << " not in sensor list ";
                  continue;
                }

                const char* fieldName = nullptr;
                if (sensorType == "temperature") {
                  fieldName = "Temperatures";
                } else if (sensorType == "fan" || sensorType == "fan_tach") {
                  fieldName = "Fans";
                } else if (sensorType == "voltage") {
                  fieldName = "Voltages";
                } else if (sensorType == "current") {
                  fieldName = "PowerSupply";
                } else if (sensorType == "power") {
                  fieldName = "PowerSupply";
                } else {
                  CROW_LOG_ERROR << "Unsure how to handle sensorType "
                                 << sensorType;
                  continue;
                }

                nlohmann::json& temp_array =
                    SensorsAsyncResp->res.json_value[fieldName];

                // Create the array if it doesn't yet exist
                if (temp_array.is_array() == false) {
                  temp_array = nlohmann::json::array();
                }

                temp_array.push_back(
                    {{"@odata.id", "/redfish/v1/Chassis/" +
                                       SensorsAsyncResp->chassisId +
                                       "/Thermal#/" + sensorName}});
                nlohmann::json& sensor_json = temp_array.back();
                objectInterfacesToJson(sensorName, sensorType,
                                       objDictEntry.second, sensor_json);
              }
              CROW_LOG_DEBUG << "getManagedObjectsCb exit";
            };
            crow::connections::system_bus->async_method_call(
                getManagedObjectsCb, connection, "/",
                "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
          };
          CROW_LOG_DEBUG << "getConnectionCb exit";
        };
    // Get connections and then pass it to get sensors
    getConnections(SensorsAsyncResp, sensorNames, std::move(getConnectionCb));
    CROW_LOG_DEBUG << "getChassisCb exit";
  };

  // Get chassis information related to sensors
  getChassis(SensorsAsyncResp, std::move(getChassisCb));
  CROW_LOG_DEBUG << "getChassisData exit";
};

}  // namespace redfish
