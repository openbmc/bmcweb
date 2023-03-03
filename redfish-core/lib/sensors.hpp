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

#include "app.hpp"
#include "dbus_singleton.hpp"
#include "dbus_utility.hpp"
#include "generated/enums/sensor.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "str_utility.hpp"
#include "utils/dbus_utils.hpp"
#include "utils/json_utils.hpp"
#include "utils/query_param.hpp"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/find.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/range/algorithm/replace_copy_if.hpp>
#include <boost/system/error_code.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <array>
#include <cmath>
#include <iterator>
#include <map>
#include <set>
#include <string_view>
#include <utility>
#include <variant>

namespace redfish
{

namespace sensors
{
namespace node
{
static constexpr std::string_view power = "Power";
static constexpr std::string_view sensors = "Sensors";
static constexpr std::string_view thermal = "Thermal";
} // namespace node

// clang-format off
namespace dbus
{
constexpr auto powerPaths = std::to_array<std::string_view>({
    "/xyz/openbmc_project/sensors/voltage",
    "/xyz/openbmc_project/sensors/power"
});

constexpr auto sensorPaths = std::to_array<std::string_view>({
    "/xyz/openbmc_project/sensors/power",
    "/xyz/openbmc_project/sensors/current",
    "/xyz/openbmc_project/sensors/airflow",
    "/xyz/openbmc_project/sensors/humidity",
#ifdef BMCWEB_NEW_POWERSUBSYSTEM_THERMALSUBSYSTEM
    "/xyz/openbmc_project/sensors/voltage",
    "/xyz/openbmc_project/sensors/fan_tach",
    "/xyz/openbmc_project/sensors/temperature",
    "/xyz/openbmc_project/sensors/fan_pwm",
    "/xyz/openbmc_project/sensors/altitude",
    "/xyz/openbmc_project/sensors/energy",
#endif
    "/xyz/openbmc_project/sensors/utilization"
});

constexpr auto thermalPaths = std::to_array<std::string_view>({
    "/xyz/openbmc_project/sensors/fan_tach",
    "/xyz/openbmc_project/sensors/temperature",
    "/xyz/openbmc_project/sensors/fan_pwm"
});

} // namespace dbus
// clang-format on

using sensorPair =
    std::pair<std::string_view, std::span<const std::string_view>>;
static constexpr std::array<sensorPair, 3> paths = {
    {{node::power, dbus::powerPaths},
     {node::sensors, dbus::sensorPaths},
     {node::thermal, dbus::thermalPaths}}};

inline sensor::ReadingType toReadingType(std::string_view sensorType)
{
    if (sensorType == "voltage")
    {
        return sensor::ReadingType::Voltage;
    }
    if (sensorType == "power")
    {
        return sensor::ReadingType::Power;
    }
    if (sensorType == "current")
    {
        return sensor::ReadingType::Current;
    }
    if (sensorType == "fan_tach")
    {
        return sensor::ReadingType::Rotational;
    }
    if (sensorType == "temperature")
    {
        return sensor::ReadingType::Temperature;
    }
    if (sensorType == "fan_pwm" || sensorType == "utilization")
    {
        return sensor::ReadingType::Percent;
    }
    if (sensorType == "humidity")
    {
        return sensor::ReadingType::Humidity;
    }
    if (sensorType == "altitude")
    {
        return sensor::ReadingType::Altitude;
    }
    if (sensorType == "airflow")
    {
        return sensor::ReadingType::AirFlow;
    }
    if (sensorType == "energy")
    {
        return sensor::ReadingType::EnergyJoules;
    }
    return sensor::ReadingType::Invalid;
}

inline std::string_view toReadingUnits(std::string_view sensorType)
{
    if (sensorType == "voltage")
    {
        return "V";
    }
    if (sensorType == "power")
    {
        return "W";
    }
    if (sensorType == "current")
    {
        return "A";
    }
    if (sensorType == "fan_tach")
    {
        return "RPM";
    }
    if (sensorType == "temperature")
    {
        return "Cel";
    }
    if (sensorType == "fan_pwm" || sensorType == "utilization" ||
        sensorType == "humidity")
    {
        return "%";
    }
    if (sensorType == "altitude")
    {
        return "m";
    }
    if (sensorType == "airflow")
    {
        return "cft_i/min";
    }
    if (sensorType == "energy")
    {
        return "J";
    }
    return "";
}
} // namespace sensors

/**
 * SensorsAsyncResp
 * Gathers data needed for response processing after async calls are done
 */
class SensorsAsyncResp
{
  public:
    using DataCompleteCb = std::function<void(
        const boost::beast::http::status status,
        const std::map<std::string, std::string>& uriToDbus)>;

    struct SensorData
    {
        const std::string name;
        std::string uri;
        const std::string dbusPath;
    };

    SensorsAsyncResp(const std::shared_ptr<bmcweb::AsyncResp>& asyncRespIn,
                     const std::string& chassisIdIn,
                     std::span<const std::string_view> typesIn,
                     std::string_view subNode) :
        asyncResp(asyncRespIn),
        chassisId(chassisIdIn), types(typesIn), chassisSubNode(subNode),
        efficientExpand(false)
    {}

    // Store extra data about sensor mapping and return it in callback
    SensorsAsyncResp(const std::shared_ptr<bmcweb::AsyncResp>& asyncRespIn,
                     const std::string& chassisIdIn,
                     std::span<const std::string_view> typesIn,
                     std::string_view subNode,
                     DataCompleteCb&& creationComplete) :
        asyncResp(asyncRespIn),
        chassisId(chassisIdIn), types(typesIn), chassisSubNode(subNode),
        efficientExpand(false), metadata{std::vector<SensorData>()},
        dataComplete{std::move(creationComplete)}
    {}

    // sensor collections expand
    SensorsAsyncResp(const std::shared_ptr<bmcweb::AsyncResp>& asyncRespIn,
                     const std::string& chassisIdIn,
                     std::span<const std::string_view> typesIn,
                     const std::string_view& subNode, bool efficientExpandIn) :
        asyncResp(asyncRespIn),
        chassisId(chassisIdIn), types(typesIn), chassisSubNode(subNode),
        efficientExpand(efficientExpandIn)
    {}

    ~SensorsAsyncResp()
    {
        if (asyncResp->res.result() ==
            boost::beast::http::status::internal_server_error)
        {
            // Reset the json object to clear out any data that made it in
            // before the error happened todo(ed) handle error condition with
            // proper code
            asyncResp->res.jsonValue = nlohmann::json::object();
        }

        if (dataComplete && metadata)
        {
            std::map<std::string, std::string> map;
            if (asyncResp->res.result() == boost::beast::http::status::ok)
            {
                for (auto& sensor : *metadata)
                {
                    map.emplace(sensor.uri, sensor.dbusPath);
                }
            }
            dataComplete(asyncResp->res.result(), map);
        }
    }

    SensorsAsyncResp(const SensorsAsyncResp&) = delete;
    SensorsAsyncResp(SensorsAsyncResp&&) = delete;
    SensorsAsyncResp& operator=(const SensorsAsyncResp&) = delete;
    SensorsAsyncResp& operator=(SensorsAsyncResp&&) = delete;

    void addMetadata(const nlohmann::json& sensorObject,
                     const std::string& dbusPath)
    {
        if (metadata)
        {
            metadata->emplace_back(SensorData{
                sensorObject["Name"], sensorObject["@odata.id"], dbusPath});
        }
    }

    void updateUri(const std::string& name, const std::string& uri)
    {
        if (metadata)
        {
            for (auto& sensor : *metadata)
            {
                if (sensor.name == name)
                {
                    sensor.uri = uri;
                }
            }
        }
    }

    const std::shared_ptr<bmcweb::AsyncResp> asyncResp;
    const std::string chassisId;
    const std::span<const std::string_view> types;
    const std::string chassisSubNode;
    const bool efficientExpand;

  private:
    std::optional<std::vector<SensorData>> metadata;
    DataCompleteCb dataComplete;
};

/**
 * Possible states for physical inventory leds
 */
enum class LedState
{
    OFF,
    ON,
    BLINK,
    UNKNOWN
};

/**
 * D-Bus inventory item associated with one or more sensors.
 */
class InventoryItem
{
  public:
    explicit InventoryItem(const std::string& objPath) : objectPath(objPath)
    {
        // Set inventory item name to last node of object path
        sdbusplus::message::object_path path(objectPath);
        name = path.filename();
        if (name.empty())
        {
            BMCWEB_LOG_ERROR << "Failed to find '/' in " << objectPath;
        }
    }

    std::string objectPath;
    std::string name;
    bool isPresent = true;
    bool isFunctional = true;
    bool isPowerSupply = false;
    int powerSupplyEfficiencyPercent = -1;
    std::string manufacturer;
    std::string model;
    std::string partNumber;
    std::string serialNumber;
    std::set<std::string> sensors;
    std::string ledObjectPath;
    LedState ledState = LedState::UNKNOWN;
};

/**
 * @brief Get objects with connection necessary for sensors
 * @param SensorsAsyncResp Pointer to object holding response data
 * @param sensorNames Sensors retrieved from chassis
 * @param callback Callback for processing gathered connections
 */
template <typename Callback>
void getObjectsWithConnection(
    const std::shared_ptr<SensorsAsyncResp>& sensorsAsyncResp,
    const std::shared_ptr<std::set<std::string>>& sensorNames,
    Callback&& callback)
{
    BMCWEB_LOG_DEBUG << "getObjectsWithConnection enter";
    const std::string path = "/xyz/openbmc_project/sensors";
    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Sensor.Value"};

    // Make call to ObjectMapper to find all sensors objects
    dbus::utility::getSubTree(
        path, 2, interfaces,
        [callback{std::forward<Callback>(callback)}, sensorsAsyncResp,
         sensorNames](const boost::system::error_code& ec,
                      const dbus::utility::MapperGetSubTreeResponse& subtree) {
        // Response handler for parsing objects subtree
        BMCWEB_LOG_DEBUG << "getObjectsWithConnection resp_handler enter";
        if (ec)
        {
            messages::internalError(sensorsAsyncResp->asyncResp->res);
            BMCWEB_LOG_ERROR
                << "getObjectsWithConnection resp_handler: Dbus error " << ec;
            return;
        }

        BMCWEB_LOG_DEBUG << "Found " << subtree.size() << " subtrees";

        // Make unique list of connections only for requested sensor types and
        // found in the chassis
        std::set<std::string> connections;
        std::set<std::pair<std::string, std::string>> objectsWithConnection;

        BMCWEB_LOG_DEBUG << "sensorNames list count: " << sensorNames->size();
        for (const std::string& tsensor : *sensorNames)
        {
            BMCWEB_LOG_DEBUG << "Sensor to find: " << tsensor;
        }

        for (const std::pair<
                 std::string,
                 std::vector<std::pair<std::string, std::vector<std::string>>>>&
                 object : subtree)
        {
            if (sensorNames->find(object.first) != sensorNames->end())
            {
                for (const std::pair<std::string, std::vector<std::string>>&
                         objData : object.second)
                {
                    BMCWEB_LOG_DEBUG << "Adding connection: " << objData.first;
                    connections.insert(objData.first);
                    objectsWithConnection.insert(
                        std::make_pair(object.first, objData.first));
                }
            }
        }
        BMCWEB_LOG_DEBUG << "Found " << connections.size() << " connections";
        callback(std::move(connections), std::move(objectsWithConnection));
        BMCWEB_LOG_DEBUG << "getObjectsWithConnection resp_handler exit";
        });
    BMCWEB_LOG_DEBUG << "getObjectsWithConnection exit";
}

/**
 * @brief Create connections necessary for sensors
 * @param SensorsAsyncResp Pointer to object holding response data
 * @param sensorNames Sensors retrieved from chassis
 * @param callback Callback for processing gathered connections
 */
template <typename Callback>
void getConnections(std::shared_ptr<SensorsAsyncResp> sensorsAsyncResp,
                    const std::shared_ptr<std::set<std::string>> sensorNames,
                    Callback&& callback)
{
    auto objectsWithConnectionCb =
        [callback](const std::set<std::string>& connections,
                   const std::set<std::pair<std::string, std::string>>&
                   /*objectsWithConnection*/) { callback(connections); };
    getObjectsWithConnection(sensorsAsyncResp, sensorNames,
                             std::move(objectsWithConnectionCb));
}

/**
 * @brief Shrinks the list of sensors for processing
 * @param SensorsAysncResp  The class holding the Redfish response
 * @param allSensors  A list of all the sensors associated to the
 * chassis element (i.e. baseboard, front panel, etc...)
 * @param activeSensors A list that is a reduction of the incoming
 * allSensors list.  Eliminate Thermal sensors when a Power request is
 * made, and eliminate Power sensors when a Thermal request is made.
 */
inline void reduceSensorList(
    crow::Response& res, std::string_view chassisSubNode,
    std::span<const std::string_view> sensorTypes,
    const std::vector<std::string>* allSensors,
    const std::shared_ptr<std::set<std::string>>& activeSensors)
{
    if ((allSensors == nullptr) || (activeSensors == nullptr))
    {
        messages::resourceNotFound(res, chassisSubNode,
                                   chassisSubNode == sensors::node::thermal
                                       ? "Temperatures"
                                       : "Voltages");

        return;
    }
    if (allSensors->empty())
    {
        // Nothing to do, the activeSensors object is also empty
        return;
    }

    for (std::string_view type : sensorTypes)
    {
        for (const std::string& sensor : *allSensors)
        {
            if (sensor.starts_with(type))
            {
                activeSensors->emplace(sensor);
            }
        }
    }
}

/*
 *Populates the top level collection for a given subnode.  Populates
 *SensorCollection, Power, or Thermal schemas.
 *
 * */
inline void populateChassisNode(nlohmann::json& jsonValue,
                                std::string_view chassisSubNode)
{
    if (chassisSubNode == sensors::node::power)
    {
        jsonValue["@odata.type"] = "#Power.v1_5_2.Power";
    }
    else if (chassisSubNode == sensors::node::thermal)
    {
        jsonValue["@odata.type"] = "#Thermal.v1_4_0.Thermal";
        jsonValue["Fans"] = nlohmann::json::array();
        jsonValue["Temperatures"] = nlohmann::json::array();
    }
    else if (chassisSubNode == sensors::node::sensors)
    {
        jsonValue["@odata.type"] = "#SensorCollection.SensorCollection";
        jsonValue["Description"] = "Collection of Sensors for this Chassis";
        jsonValue["Members"] = nlohmann::json::array();
        jsonValue["Members@odata.count"] = 0;
    }

    if (chassisSubNode != sensors::node::sensors)
    {
        jsonValue["Id"] = chassisSubNode;
    }
    jsonValue["Name"] = chassisSubNode;
}

/**
 * @brief Retrieves requested chassis sensors and redundancy data from DBus .
 * @param SensorsAsyncResp   Pointer to object holding response data
 * @param callback  Callback for next step in gathered sensor processing
 */
template <typename Callback>
void getChassis(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                std::string_view chassisId, std::string_view chassisSubNode,
                std::span<const std::string_view> sensorTypes,
                Callback&& callback)
{
    BMCWEB_LOG_DEBUG << "getChassis enter";
    constexpr std::array<std::string_view, 2> interfaces = {
        "xyz.openbmc_project.Inventory.Item.Board",
        "xyz.openbmc_project.Inventory.Item.Chassis"};

    // Get the Chassis Collection
    dbus::utility::getSubTreePaths(
        "/xyz/openbmc_project/inventory", 0, interfaces,
        [callback{std::forward<Callback>(callback)}, asyncResp,
         chassisIdStr{std::string(chassisId)},
         chassisSubNode{std::string(chassisSubNode)}, sensorTypes](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreePathsResponse& chassisPaths) {
        BMCWEB_LOG_DEBUG << "getChassis respHandler enter";
        if (ec)
        {
            BMCWEB_LOG_ERROR << "getChassis respHandler DBUS error: " << ec;
            messages::internalError(asyncResp->res);
            return;
        }
        const std::string* chassisPath = nullptr;
        for (const std::string& chassis : chassisPaths)
        {
            sdbusplus::message::object_path path(chassis);
            std::string chassisName = path.filename();
            if (chassisName.empty())
            {
                BMCWEB_LOG_ERROR << "Failed to find '/' in " << chassis;
                continue;
            }
            if (chassisName == chassisIdStr)
            {
                chassisPath = &chassis;
                break;
            }
        }
        if (chassisPath == nullptr)
        {
            messages::resourceNotFound(asyncResp->res, "Chassis", chassisIdStr);
            return;
        }
        populateChassisNode(asyncResp->res.jsonValue, chassisSubNode);

        asyncResp->res.jsonValue["@odata.id"] = crow::utility::urlFromPieces(
            "redfish", "v1", "Chassis", chassisIdStr, chassisSubNode);

        // Get the list of all sensors for this Chassis element
        std::string sensorPath = *chassisPath + "/all_sensors";
        dbus::utility::getAssociationEndPoints(
            sensorPath,
            [asyncResp, chassisSubNode, sensorTypes,
             callback{std::forward<const Callback>(callback)}](
                const boost::system::error_code& e,
                const dbus::utility::MapperEndPoints& nodeSensorList) {
            if (e)
            {
                if (e.value() != EBADR)
                {
                    messages::internalError(asyncResp->res);
                    return;
                }
            }
            const std::shared_ptr<std::set<std::string>> culledSensorList =
                std::make_shared<std::set<std::string>>();
            reduceSensorList(asyncResp->res, chassisSubNode, sensorTypes,
                             &nodeSensorList, culledSensorList);
            BMCWEB_LOG_DEBUG << "Finishing with " << culledSensorList->size();
            callback(culledSensorList);
            });
        });
    BMCWEB_LOG_DEBUG << "getChassis exit";
}

/**
 * @brief Returns the Redfish State value for the specified inventory item.
 * @param inventoryItem D-Bus inventory item associated with a sensor.
 * @return State value for inventory item.
 */
inline std::string getState(const InventoryItem* inventoryItem)
{
    if ((inventoryItem != nullptr) && !(inventoryItem->isPresent))
    {
        return "Absent";
    }

    return "Enabled";
}

/**
 * @brief Returns the Redfish Health value for the specified sensor.
 * @param sensorJson Sensor JSON object.
 * @param valuesDict Map of all sensor DBus values.
 * @param inventoryItem D-Bus inventory item associated with the sensor.  Will
 * be nullptr if no associated inventory item was found.
 * @return Health value for sensor.
 */
inline std::string getHealth(nlohmann::json& sensorJson,
                             const dbus::utility::DBusPropertiesMap& valuesDict,
                             const InventoryItem* inventoryItem)
{
    // Get current health value (if any) in the sensor JSON object.  Some JSON
    // objects contain multiple sensors (such as PowerSupplies).  We want to set
    // the overall health to be the most severe of any of the sensors.
    std::string currentHealth;
    auto statusIt = sensorJson.find("Status");
    if (statusIt != sensorJson.end())
    {
        auto healthIt = statusIt->find("Health");
        if (healthIt != statusIt->end())
        {
            std::string* health = healthIt->get_ptr<std::string*>();
            if (health != nullptr)
            {
                currentHealth = *health;
            }
        }
    }

    // If current health in JSON object is already Critical, return that. This
    // should override the sensor health, which might be less severe.
    if (currentHealth == "Critical")
    {
        return "Critical";
    }

    const bool* criticalAlarmHigh = nullptr;
    const bool* criticalAlarmLow = nullptr;
    const bool* warningAlarmHigh = nullptr;
    const bool* warningAlarmLow = nullptr;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), valuesDict, "CriticalAlarmHigh",
        criticalAlarmHigh, "CriticalAlarmLow", criticalAlarmLow,
        "WarningAlarmHigh", warningAlarmHigh, "WarningAlarmLow",
        warningAlarmLow);

    if (success)
    {
        // Check if sensor has critical threshold alarm
        if ((criticalAlarmHigh != nullptr && *criticalAlarmHigh) ||
            (criticalAlarmLow != nullptr && *criticalAlarmLow))
        {
            return "Critical";
        }
    }

    // Check if associated inventory item is not functional
    if ((inventoryItem != nullptr) && !(inventoryItem->isFunctional))
    {
        return "Critical";
    }

    // If current health in JSON object is already Warning, return that. This
    // should override the sensor status, which might be less severe.
    if (currentHealth == "Warning")
    {
        return "Warning";
    }

    if (success)
    {
        // Check if sensor has warning threshold alarm
        if ((warningAlarmHigh != nullptr && *warningAlarmHigh) ||
            (warningAlarmLow != nullptr && *warningAlarmLow))
        {
            return "Warning";
        }
    }

    return "OK";
}

inline void setLedState(nlohmann::json& sensorJson,
                        const InventoryItem* inventoryItem)
{
    if (inventoryItem != nullptr && !inventoryItem->ledObjectPath.empty())
    {
        switch (inventoryItem->ledState)
        {
            case LedState::OFF:
                sensorJson["IndicatorLED"] = "Off";
                break;
            case LedState::ON:
                sensorJson["IndicatorLED"] = "Lit";
                break;
            case LedState::BLINK:
                sensorJson["IndicatorLED"] = "Blinking";
                break;
            case LedState::UNKNOWN:
                break;
        }
    }
}

/**
 * @brief Builds a json sensor representation of a sensor.
 * @param sensorName  The name of the sensor to be built
 * @param sensorType  The type (temperature, fan_tach, etc) of the sensor to
 * build
 * @param chassisSubNode The subnode (thermal, sensor, ect) of the sensor
 * @param propertiesDict A dictionary of the properties to build the sensor
 * from.
 * @param sensorJson  The json object to fill
 * @param inventoryItem D-Bus inventory item associated with the sensor.  Will
 * be nullptr if no associated inventory item was found.
 */
inline void objectPropertiesToJson(
    std::string_view sensorName, std::string_view sensorType,
    std::string_view chassisSubNode,
    const dbus::utility::DBusPropertiesMap& propertiesDict,
    nlohmann::json& sensorJson, InventoryItem* inventoryItem)
{
    if (chassisSubNode == sensors::node::sensors)
    {
        std::string subNodeEscaped(sensorType);
        subNodeEscaped.erase(
            std::remove(subNodeEscaped.begin(), subNodeEscaped.end(), '_'),
            subNodeEscaped.end());

        // For sensors in SensorCollection we set Id instead of MemberId,
        // including power sensors.
        subNodeEscaped += '_';
        subNodeEscaped += sensorName;
        sensorJson["Id"] = std::move(subNodeEscaped);

        std::string sensorNameEs(sensorName);
        std::replace(sensorNameEs.begin(), sensorNameEs.end(), '_', ' ');
        sensorJson["Name"] = std::move(sensorNameEs);
    }
    else if (sensorType != "power")
    {
        // Set MemberId and Name for non-power sensors.  For PowerSupplies and
        // PowerControl, those properties have more general values because
        // multiple sensors can be stored in the same JSON object.
        sensorJson["MemberId"] = sensorName;
        std::string sensorNameEs(sensorName);
        std::replace(sensorNameEs.begin(), sensorNameEs.end(), '_', ' ');
        sensorJson["Name"] = std::move(sensorNameEs);
    }

    sensorJson["Status"]["State"] = getState(inventoryItem);
    sensorJson["Status"]["Health"] =
        getHealth(sensorJson, propertiesDict, inventoryItem);

    // Parameter to set to override the type we get from dbus, and force it to
    // int, regardless of what is available.  This is used for schemas like fan,
    // that require integers, not floats.
    bool forceToInt = false;

    nlohmann::json::json_pointer unit("/Reading");
    if (chassisSubNode == sensors::node::sensors)
    {
        sensorJson["@odata.type"] = "#Sensor.v1_2_0.Sensor";

        sensor::ReadingType readingType = sensors::toReadingType(sensorType);
        if (readingType == sensor::ReadingType::Invalid)
        {
            BMCWEB_LOG_ERROR << "Redfish cannot map reading type for "
                             << sensorType;
        }
        else
        {
            sensorJson["ReadingType"] = readingType;
        }

        std::string_view readingUnits = sensors::toReadingUnits(sensorType);
        if (readingUnits.empty())
        {
            BMCWEB_LOG_ERROR << "Redfish cannot map reading unit for "
                             << sensorType;
        }
        else
        {
            sensorJson["ReadingUnits"] = readingUnits;
        }
    }
    else if (sensorType == "temperature")
    {
        unit = "/ReadingCelsius"_json_pointer;
        sensorJson["@odata.type"] = "#Thermal.v1_3_0.Temperature";
        // TODO(ed) Documentation says that path should be type fan_tach,
        // implementation seems to implement fan
    }
    else if (sensorType == "fan" || sensorType == "fan_tach")
    {
        unit = "/Reading"_json_pointer;
        sensorJson["ReadingUnits"] = "RPM";
        sensorJson["@odata.type"] = "#Thermal.v1_3_0.Fan";
        setLedState(sensorJson, inventoryItem);
        forceToInt = true;
    }
    else if (sensorType == "fan_pwm")
    {
        unit = "/Reading"_json_pointer;
        sensorJson["ReadingUnits"] = "Percent";
        sensorJson["@odata.type"] = "#Thermal.v1_3_0.Fan";
        setLedState(sensorJson, inventoryItem);
        forceToInt = true;
    }
    else if (sensorType == "voltage")
    {
        unit = "/ReadingVolts"_json_pointer;
        sensorJson["@odata.type"] = "#Power.v1_0_0.Voltage";
    }
    else if (sensorType == "power")
    {
        if (boost::iequals(sensorName, "total_power"))
        {
            sensorJson["@odata.type"] = "#Power.v1_0_0.PowerControl";
            // Put multiple "sensors" into a single PowerControl, so have
            // generic names for MemberId and Name. Follows Redfish mockup.
            sensorJson["MemberId"] = "0";
            sensorJson["Name"] = "Chassis Power Control";
            unit = "/PowerConsumedWatts"_json_pointer;
        }
        else if (boost::ifind_first(sensorName, "input").empty())
        {
            unit = "/PowerInputWatts"_json_pointer;
        }
        else
        {
            unit = "/PowerOutputWatts"_json_pointer;
        }
    }
    else
    {
        BMCWEB_LOG_ERROR << "Redfish cannot map object type for " << sensorName;
        return;
    }
    // Map of dbus interface name, dbus property name and redfish property_name
    std::vector<
        std::tuple<const char*, const char*, nlohmann::json::json_pointer>>
        properties;
    properties.reserve(7);

    properties.emplace_back("xyz.openbmc_project.Sensor.Value", "Value", unit);

    if (chassisSubNode == sensors::node::sensors)
    {
        properties.emplace_back(
            "xyz.openbmc_project.Sensor.Threshold.Warning", "WarningHigh",
            "/Thresholds/UpperCaution/Reading"_json_pointer);
        properties.emplace_back(
            "xyz.openbmc_project.Sensor.Threshold.Warning", "WarningLow",
            "/Thresholds/LowerCaution/Reading"_json_pointer);
        properties.emplace_back(
            "xyz.openbmc_project.Sensor.Threshold.Critical", "CriticalHigh",
            "/Thresholds/UpperCritical/Reading"_json_pointer);
        properties.emplace_back(
            "xyz.openbmc_project.Sensor.Threshold.Critical", "CriticalLow",
            "/Thresholds/LowerCritical/Reading"_json_pointer);
    }
    else if (sensorType != "power")
    {
        properties.emplace_back("xyz.openbmc_project.Sensor.Threshold.Warning",
                                "WarningHigh",
                                "/UpperThresholdNonCritical"_json_pointer);
        properties.emplace_back("xyz.openbmc_project.Sensor.Threshold.Warning",
                                "WarningLow",
                                "/LowerThresholdNonCritical"_json_pointer);
        properties.emplace_back("xyz.openbmc_project.Sensor.Threshold.Critical",
                                "CriticalHigh",
                                "/UpperThresholdCritical"_json_pointer);
        properties.emplace_back("xyz.openbmc_project.Sensor.Threshold.Critical",
                                "CriticalLow",
                                "/LowerThresholdCritical"_json_pointer);
    }

    // TODO Need to get UpperThresholdFatal and LowerThresholdFatal

    if (chassisSubNode == sensors::node::sensors)
    {
        properties.emplace_back("xyz.openbmc_project.Sensor.Value", "MinValue",
                                "/ReadingRangeMin"_json_pointer);
        properties.emplace_back("xyz.openbmc_project.Sensor.Value", "MaxValue",
                                "/ReadingRangeMax"_json_pointer);
        properties.emplace_back("xyz.openbmc_project.Sensor.Accuracy",
                                "Accuracy", "/Accuracy"_json_pointer);
    }
    else if (sensorType == "temperature")
    {
        properties.emplace_back("xyz.openbmc_project.Sensor.Value", "MinValue",
                                "/MinReadingRangeTemp"_json_pointer);
        properties.emplace_back("xyz.openbmc_project.Sensor.Value", "MaxValue",
                                "/MaxReadingRangeTemp"_json_pointer);
    }
    else if (sensorType != "power")
    {
        properties.emplace_back("xyz.openbmc_project.Sensor.Value", "MinValue",
                                "/MinReadingRange"_json_pointer);
        properties.emplace_back("xyz.openbmc_project.Sensor.Value", "MaxValue",
                                "/MaxReadingRange"_json_pointer);
    }

    for (const std::tuple<const char*, const char*,
                          nlohmann::json::json_pointer>& p : properties)
    {
        for (const auto& [valueName, valueVariant] : propertiesDict)
        {
            if (valueName != std::get<1>(p))
            {
                continue;
            }

            // The property we want to set may be nested json, so use
            // a json_pointer for easy indexing into the json structure.
            const nlohmann::json::json_pointer& key = std::get<2>(p);

            const double* doubleValue = std::get_if<double>(&valueVariant);
            if (doubleValue == nullptr)
            {
                BMCWEB_LOG_ERROR << "Got value interface that wasn't double";
                continue;
            }
            if (forceToInt)
            {
                sensorJson[key] = static_cast<int64_t>(*doubleValue);
            }
            else
            {
                sensorJson[key] = *doubleValue;
            }
        }
    }
}

/**
 * @brief Builds a json sensor representation of a sensor.
 * @param sensorName  The name of the sensor to be built
 * @param sensorType  The type (temperature, fan_tach, etc) of the sensor to
 * build
 * @param chassisSubNode The subnode (thermal, sensor, ect) of the sensor
 * @param interfacesDict  A dictionary of the interfaces and properties of said
 * interfaces to be built from
 * @param sensorJson  The json object to fill
 * @param inventoryItem D-Bus inventory item associated with the sensor.  Will
 * be nullptr if no associated inventory item was found.
 */
inline void objectInterfacesToJson(
    const std::string& sensorName, const std::string& sensorType,
    const std::string& chassisSubNode,
    const dbus::utility::DBusInteracesMap& interfacesDict,
    nlohmann::json& sensorJson, InventoryItem* inventoryItem)
{

    for (const auto& [interface, valuesDict] : interfacesDict)
    {
        objectPropertiesToJson(sensorName, sensorType, chassisSubNode,
                               valuesDict, sensorJson, inventoryItem);
    }
    BMCWEB_LOG_DEBUG << "Added sensor " << sensorName;
}

inline void populateFanRedundancy(
    const std::shared_ptr<SensorsAsyncResp>& sensorsAsyncResp)
{
    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Control.FanRedundancy"};
    dbus::utility::getSubTree(
        "/xyz/openbmc_project/control", 2, interfaces,
        [sensorsAsyncResp](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreeResponse& resp) {
        if (ec)
        {
            return; // don't have to have this interface
        }
        for (const std::pair<
                 std::string,
                 std::vector<std::pair<std::string, std::vector<std::string>>>>&
                 pathPair : resp)
        {
            const std::string& path = pathPair.first;
            const std::vector<std::pair<std::string, std::vector<std::string>>>&
                objDict = pathPair.second;
            if (objDict.empty())
            {
                continue; // this should be impossible
            }

            const std::string& owner = objDict.begin()->first;
            dbus::utility::getAssociationEndPoints(
                path + "/chassis",
                [path, owner, sensorsAsyncResp](
                    const boost::system::error_code& e,
                    const dbus::utility::MapperEndPoints& endpoints) {
                if (e)
                {
                    return; // if they don't have an association we
                            // can't tell what chassis is
                }
                auto found =
                    std::find_if(endpoints.begin(), endpoints.end(),
                                 [sensorsAsyncResp](const std::string& entry) {
                    return entry.find(sensorsAsyncResp->chassisId) !=
                           std::string::npos;
                    });

                if (found == endpoints.end())
                {
                    return;
                }
                sdbusplus::asio::getAllProperties(
                    *crow::connections::systemBus, owner, path,
                    "xyz.openbmc_project.Control.FanRedundancy",
                    [path, sensorsAsyncResp](
                        const boost::system::error_code& err,
                        const dbus::utility::DBusPropertiesMap& ret) {
                    if (err)
                    {
                        return; // don't have to have this
                                // interface
                    }

                    const uint8_t* allowedFailures = nullptr;
                    const std::vector<std::string>* collection = nullptr;
                    const std::string* status = nullptr;

                    const bool success = sdbusplus::unpackPropertiesNoThrow(
                        dbus_utils::UnpackErrorPrinter(), ret,
                        "AllowedFailures", allowedFailures, "Collection",
                        collection, "Status", status);

                    if (!success)
                    {
                        messages::internalError(
                            sensorsAsyncResp->asyncResp->res);
                        return;
                    }

                    if (allowedFailures == nullptr || collection == nullptr ||
                        status == nullptr)
                    {
                        BMCWEB_LOG_ERROR << "Invalid redundancy interface";
                        messages::internalError(
                            sensorsAsyncResp->asyncResp->res);
                        return;
                    }

                    sdbusplus::message::object_path objectPath(path);
                    std::string name = objectPath.filename();
                    if (name.empty())
                    {
                        // this should be impossible
                        messages::internalError(
                            sensorsAsyncResp->asyncResp->res);
                        return;
                    }
                    std::replace(name.begin(), name.end(), '_', ' ');

                    std::string health;

                    if (status->ends_with("Full"))
                    {
                        health = "OK";
                    }
                    else if (status->ends_with("Degraded"))
                    {
                        health = "Warning";
                    }
                    else
                    {
                        health = "Critical";
                    }
                    nlohmann::json::array_t redfishCollection;
                    const auto& fanRedfish =
                        sensorsAsyncResp->asyncResp->res.jsonValue["Fans"];
                    for (const std::string& item : *collection)
                    {
                        sdbusplus::message::object_path itemPath(item);
                        std::string itemName = itemPath.filename();
                        if (itemName.empty())
                        {
                            continue;
                        }
                        /*
                        todo(ed): merge patch that fixes the names
                        std::replace(itemName.begin(),
                                     itemName.end(), '_', ' ');*/
                        auto schemaItem =
                            std::find_if(fanRedfish.begin(), fanRedfish.end(),
                                         [itemName](const nlohmann::json& fan) {
                            return fan["MemberId"] == itemName;
                            });
                        if (schemaItem != fanRedfish.end())
                        {
                            nlohmann::json::object_t collectionId;
                            collectionId["@odata.id"] =
                                (*schemaItem)["@odata.id"];
                            redfishCollection.emplace_back(
                                std::move(collectionId));
                        }
                        else
                        {
                            BMCWEB_LOG_ERROR << "failed to find fan in schema";
                            messages::internalError(
                                sensorsAsyncResp->asyncResp->res);
                            return;
                        }
                    }

                    size_t minNumNeeded =
                        collection->empty()
                            ? 0
                            : collection->size() - *allowedFailures;
                    nlohmann::json& jResp = sensorsAsyncResp->asyncResp->res
                                                .jsonValue["Redundancy"];

                    nlohmann::json::object_t redundancy;
                    boost::urls::url url = crow::utility::urlFromPieces(
                        "redfish", "v1", "Chassis", sensorsAsyncResp->chassisId,
                        sensorsAsyncResp->chassisSubNode);
                    url.set_fragment(("/Redundancy"_json_pointer / jResp.size())
                                         .to_string());
                    redundancy["@odata.id"] = std::move(url);
                    redundancy["@odata.type"] = "#Redundancy.v1_3_2.Redundancy";
                    redundancy["MinNumNeeded"] = minNumNeeded;
                    redundancy["MemberId"] = name;
                    redundancy["Mode"] = "N+m";
                    redundancy["Name"] = name;
                    redundancy["RedundancySet"] = redfishCollection;
                    redundancy["Status"]["Health"] = health;
                    redundancy["Status"]["State"] = "Enabled";

                    jResp.push_back(std::move(redundancy));
                    });
                });
        }
        });
}

inline void
    sortJSONResponse(const std::shared_ptr<SensorsAsyncResp>& sensorsAsyncResp)
{
    nlohmann::json& response = sensorsAsyncResp->asyncResp->res.jsonValue;
    std::array<std::string, 2> sensorHeaders{"Temperatures", "Fans"};
    if (sensorsAsyncResp->chassisSubNode == sensors::node::power)
    {
        sensorHeaders = {"Voltages", "PowerSupplies"};
    }
    for (const std::string& sensorGroup : sensorHeaders)
    {
        nlohmann::json::iterator entry = response.find(sensorGroup);
        if (entry != response.end())
        {
            std::sort(entry->begin(), entry->end(),
                      [](const nlohmann::json& c1, const nlohmann::json& c2) {
                return c1["Name"] < c2["Name"];
            });

            // add the index counts to the end of each entry
            size_t count = 0;
            for (nlohmann::json& sensorJson : *entry)
            {
                nlohmann::json::iterator odata = sensorJson.find("@odata.id");
                if (odata == sensorJson.end())
                {
                    continue;
                }
                std::string* value = odata->get_ptr<std::string*>();
                if (value != nullptr)
                {
                    *value += "/" + std::to_string(count);
                    count++;
                    sensorsAsyncResp->updateUri(sensorJson["Name"], *value);
                }
            }
        }
    }
}

/**
 * @brief Finds the inventory item with the specified object path.
 * @param inventoryItems D-Bus inventory items associated with sensors.
 * @param invItemObjPath D-Bus object path of inventory item.
 * @return Inventory item within vector, or nullptr if no match found.
 */
inline InventoryItem* findInventoryItem(
    const std::shared_ptr<std::vector<InventoryItem>>& inventoryItems,
    const std::string& invItemObjPath)
{
    for (InventoryItem& inventoryItem : *inventoryItems)
    {
        if (inventoryItem.objectPath == invItemObjPath)
        {
            return &inventoryItem;
        }
    }
    return nullptr;
}

/**
 * @brief Finds the inventory item associated with the specified sensor.
 * @param inventoryItems D-Bus inventory items associated with sensors.
 * @param sensorObjPath D-Bus object path of sensor.
 * @return Inventory item within vector, or nullptr if no match found.
 */
inline InventoryItem* findInventoryItemForSensor(
    const std::shared_ptr<std::vector<InventoryItem>>& inventoryItems,
    const std::string& sensorObjPath)
{
    for (InventoryItem& inventoryItem : *inventoryItems)
    {
        if (inventoryItem.sensors.count(sensorObjPath) > 0)
        {
            return &inventoryItem;
        }
    }
    return nullptr;
}

/**
 * @brief Finds the inventory item associated with the specified led path.
 * @param inventoryItems D-Bus inventory items associated with sensors.
 * @param ledObjPath D-Bus object path of led.
 * @return Inventory item within vector, or nullptr if no match found.
 */
inline InventoryItem*
    findInventoryItemForLed(std::vector<InventoryItem>& inventoryItems,
                            const std::string& ledObjPath)
{
    for (InventoryItem& inventoryItem : inventoryItems)
    {
        if (inventoryItem.ledObjectPath == ledObjPath)
        {
            return &inventoryItem;
        }
    }
    return nullptr;
}

/**
 * @brief Adds inventory item and associated sensor to specified vector.
 *
 * Adds a new InventoryItem to the vector if necessary.  Searches for an
 * existing InventoryItem with the specified object path.  If not found, one is
 * added to the vector.
 *
 * Next, the specified sensor is added to the set of sensors associated with the
 * InventoryItem.
 *
 * @param inventoryItems D-Bus inventory items associated with sensors.
 * @param invItemObjPath D-Bus object path of inventory item.
 * @param sensorObjPath D-Bus object path of sensor
 */
inline void addInventoryItem(
    const std::shared_ptr<std::vector<InventoryItem>>& inventoryItems,
    const std::string& invItemObjPath, const std::string& sensorObjPath)
{
    // Look for inventory item in vector
    InventoryItem* inventoryItem =
        findInventoryItem(inventoryItems, invItemObjPath);

    // If inventory item doesn't exist in vector, add it
    if (inventoryItem == nullptr)
    {
        inventoryItems->emplace_back(invItemObjPath);
        inventoryItem = &(inventoryItems->back());
    }

    // Add sensor to set of sensors associated with inventory item
    inventoryItem->sensors.emplace(sensorObjPath);
}

/**
 * @brief Stores D-Bus data in the specified inventory item.
 *
 * Finds D-Bus data in the specified map of interfaces.  Stores the data in the
 * specified InventoryItem.
 *
 * This data is later used to provide sensor property values in the JSON
 * response.
 *
 * @param inventoryItem Inventory item where data will be stored.
 * @param interfacesDict Map containing D-Bus interfaces and their properties
 * for the specified inventory item.
 */
inline void storeInventoryItemData(
    InventoryItem& inventoryItem,
    const dbus::utility::DBusInteracesMap& interfacesDict)
{
    // Get properties from Inventory.Item interface

    for (const auto& [interface, values] : interfacesDict)
    {
        if (interface == "xyz.openbmc_project.Inventory.Item")
        {
            for (const auto& [name, dbusValue] : values)
            {
                if (name == "Present")
                {
                    const bool* value = std::get_if<bool>(&dbusValue);
                    if (value != nullptr)
                    {
                        inventoryItem.isPresent = *value;
                    }
                }
            }
        }
        // Check if Inventory.Item.PowerSupply interface is present

        if (interface == "xyz.openbmc_project.Inventory.Item.PowerSupply")
        {
            inventoryItem.isPowerSupply = true;
        }

        // Get properties from Inventory.Decorator.Asset interface
        if (interface == "xyz.openbmc_project.Inventory.Decorator.Asset")
        {
            for (const auto& [name, dbusValue] : values)
            {
                if (name == "Manufacturer")
                {
                    const std::string* value =
                        std::get_if<std::string>(&dbusValue);
                    if (value != nullptr)
                    {
                        inventoryItem.manufacturer = *value;
                    }
                }
                if (name == "Model")
                {
                    const std::string* value =
                        std::get_if<std::string>(&dbusValue);
                    if (value != nullptr)
                    {
                        inventoryItem.model = *value;
                    }
                }
                if (name == "SerialNumber")
                {
                    const std::string* value =
                        std::get_if<std::string>(&dbusValue);
                    if (value != nullptr)
                    {
                        inventoryItem.serialNumber = *value;
                    }
                }
                if (name == "PartNumber")
                {
                    const std::string* value =
                        std::get_if<std::string>(&dbusValue);
                    if (value != nullptr)
                    {
                        inventoryItem.partNumber = *value;
                    }
                }
            }
        }

        if (interface ==
            "xyz.openbmc_project.State.Decorator.OperationalStatus")
        {
            for (const auto& [name, dbusValue] : values)
            {
                if (name == "Functional")
                {
                    const bool* value = std::get_if<bool>(&dbusValue);
                    if (value != nullptr)
                    {
                        inventoryItem.isFunctional = *value;
                    }
                }
            }
        }
    }
}

/**
 * @brief Gets D-Bus data for inventory items associated with sensors.
 *
 * Uses the specified connections (services) to obtain D-Bus data for inventory
 * items associated with sensors.  Stores the resulting data in the
 * inventoryItems vector.
 *
 * This data is later used to provide sensor property values in the JSON
 * response.
 *
 * Finds the inventory item data asynchronously.  Invokes callback when data has
 * been obtained.
 *
 * The callback must have the following signature:
 *   @code
 *   callback(void)
 *   @endcode
 *
 * This function is called recursively, obtaining data asynchronously from one
 * connection in each call.  This ensures the callback is not invoked until the
 * last asynchronous function has completed.
 *
 * @param sensorsAsyncResp Pointer to object holding response data.
 * @param inventoryItems D-Bus inventory items associated with sensors.
 * @param invConnections Connections that provide data for the inventory items.
 * implements ObjectManager.
 * @param callback Callback to invoke when inventory data has been obtained.
 * @param invConnectionsIndex Current index in invConnections.  Only specified
 * in recursive calls to this function.
 */
template <typename Callback>
static void getInventoryItemsData(
    std::shared_ptr<SensorsAsyncResp> sensorsAsyncResp,
    std::shared_ptr<std::vector<InventoryItem>> inventoryItems,
    std::shared_ptr<std::set<std::string>> invConnections, Callback&& callback,
    size_t invConnectionsIndex = 0)
{
    BMCWEB_LOG_DEBUG << "getInventoryItemsData enter";

    // If no more connections left, call callback
    if (invConnectionsIndex >= invConnections->size())
    {
        callback();
        BMCWEB_LOG_DEBUG << "getInventoryItemsData exit";
        return;
    }

    // Get inventory item data from current connection
    auto it = invConnections->begin();
    std::advance(it, invConnectionsIndex);
    if (it != invConnections->end())
    {
        const std::string& invConnection = *it;

        // Response handler for GetManagedObjects
        auto respHandler = [sensorsAsyncResp, inventoryItems, invConnections,
                            callback{std::forward<Callback>(callback)},
                            invConnectionsIndex](
                               const boost::system::error_code& ec,
                               const dbus::utility::ManagedObjectType& resp) {
            BMCWEB_LOG_DEBUG << "getInventoryItemsData respHandler enter";
            if (ec)
            {
                BMCWEB_LOG_ERROR
                    << "getInventoryItemsData respHandler DBus error " << ec;
                messages::internalError(sensorsAsyncResp->asyncResp->res);
                return;
            }

            // Loop through returned object paths
            for (const auto& objDictEntry : resp)
            {
                const std::string& objPath =
                    static_cast<const std::string&>(objDictEntry.first);

                // If this object path is one of the specified inventory items
                InventoryItem* inventoryItem =
                    findInventoryItem(inventoryItems, objPath);
                if (inventoryItem != nullptr)
                {
                    // Store inventory data in InventoryItem
                    storeInventoryItemData(*inventoryItem, objDictEntry.second);
                }
            }

            // Recurse to get inventory item data from next connection
            getInventoryItemsData(sensorsAsyncResp, inventoryItems,
                                  invConnections, std::move(callback),
                                  invConnectionsIndex + 1);

            BMCWEB_LOG_DEBUG << "getInventoryItemsData respHandler exit";
        };

        // Get all object paths and their interfaces for current connection
        crow::connections::systemBus->async_method_call(
            std::move(respHandler), invConnection,
            "/xyz/openbmc_project/inventory",
            "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
    }

    BMCWEB_LOG_DEBUG << "getInventoryItemsData exit";
}

/**
 * @brief Gets connections that provide D-Bus data for inventory items.
 *
 * Gets the D-Bus connections (services) that provide data for the inventory
 * items that are associated with sensors.
 *
 * Finds the connections asynchronously.  Invokes callback when information has
 * been obtained.
 *
 * The callback must have the following signature:
 *   @code
 *   callback(std::shared_ptr<std::set<std::string>> invConnections)
 *   @endcode
 *
 * @param sensorsAsyncResp Pointer to object holding response data.
 * @param inventoryItems D-Bus inventory items associated with sensors.
 * @param callback Callback to invoke when connections have been obtained.
 */
template <typename Callback>
static void getInventoryItemsConnections(
    const std::shared_ptr<SensorsAsyncResp>& sensorsAsyncResp,
    const std::shared_ptr<std::vector<InventoryItem>>& inventoryItems,
    Callback&& callback)
{
    BMCWEB_LOG_DEBUG << "getInventoryItemsConnections enter";

    const std::string path = "/xyz/openbmc_project/inventory";
    constexpr std::array<std::string_view, 4> interfaces = {
        "xyz.openbmc_project.Inventory.Item",
        "xyz.openbmc_project.Inventory.Item.PowerSupply",
        "xyz.openbmc_project.Inventory.Decorator.Asset",
        "xyz.openbmc_project.State.Decorator.OperationalStatus"};

    // Make call to ObjectMapper to find all inventory items
    dbus::utility::getSubTree(
        path, 0, interfaces,
        [callback{std::forward<Callback>(callback)}, sensorsAsyncResp,
         inventoryItems](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreeResponse& subtree) {
        // Response handler for parsing output from GetSubTree
        BMCWEB_LOG_DEBUG << "getInventoryItemsConnections respHandler enter";
        if (ec)
        {
            messages::internalError(sensorsAsyncResp->asyncResp->res);
            BMCWEB_LOG_ERROR
                << "getInventoryItemsConnections respHandler DBus error " << ec;
            return;
        }

        // Make unique list of connections for desired inventory items
        std::shared_ptr<std::set<std::string>> invConnections =
            std::make_shared<std::set<std::string>>();

        // Loop through objects from GetSubTree
        for (const std::pair<
                 std::string,
                 std::vector<std::pair<std::string, std::vector<std::string>>>>&
                 object : subtree)
        {
            // Check if object path is one of the specified inventory items
            const std::string& objPath = object.first;
            if (findInventoryItem(inventoryItems, objPath) != nullptr)
            {
                // Store all connections to inventory item
                for (const std::pair<std::string, std::vector<std::string>>&
                         objData : object.second)
                {
                    const std::string& invConnection = objData.first;
                    invConnections->insert(invConnection);
                }
            }
        }

        callback(invConnections);
        BMCWEB_LOG_DEBUG << "getInventoryItemsConnections respHandler exit";
        });
    BMCWEB_LOG_DEBUG << "getInventoryItemsConnections exit";
}

/**
 * @brief Gets associations from sensors to inventory items.
 *
 * Looks for ObjectMapper associations from the specified sensors to related
 * inventory items. Then finds the associations from those inventory items to
 * their LEDs, if any.
 *
 * Finds the inventory items asynchronously.  Invokes callback when information
 * has been obtained.
 *
 * The callback must have the following signature:
 *   @code
 *   callback(std::shared_ptr<std::vector<InventoryItem>> inventoryItems)
 *   @endcode
 *
 * @param sensorsAsyncResp Pointer to object holding response data.
 * @param sensorNames All sensors within the current chassis.
 * implements ObjectManager.
 * @param callback Callback to invoke when inventory items have been obtained.
 */
template <typename Callback>
static void getInventoryItemAssociations(
    const std::shared_ptr<SensorsAsyncResp>& sensorsAsyncResp,
    const std::shared_ptr<std::set<std::string>>& sensorNames,
    Callback&& callback)
{
    BMCWEB_LOG_DEBUG << "getInventoryItemAssociations enter";

    // Response handler for GetManagedObjects
    auto respHandler =
        [callback{std::forward<Callback>(callback)}, sensorsAsyncResp,
         sensorNames](const boost::system::error_code& ec,
                      const dbus::utility::ManagedObjectType& resp) {
        BMCWEB_LOG_DEBUG << "getInventoryItemAssociations respHandler enter";
        if (ec)
        {
            BMCWEB_LOG_ERROR
                << "getInventoryItemAssociations respHandler DBus error " << ec;
            messages::internalError(sensorsAsyncResp->asyncResp->res);
            return;
        }

        // Create vector to hold list of inventory items
        std::shared_ptr<std::vector<InventoryItem>> inventoryItems =
            std::make_shared<std::vector<InventoryItem>>();

        // Loop through returned object paths
        std::string sensorAssocPath;
        sensorAssocPath.reserve(128); // avoid memory allocations
        for (const auto& objDictEntry : resp)
        {
            const std::string& objPath =
                static_cast<const std::string&>(objDictEntry.first);

            // If path is inventory association for one of the specified sensors
            for (const std::string& sensorName : *sensorNames)
            {
                sensorAssocPath = sensorName;
                sensorAssocPath += "/inventory";
                if (objPath == sensorAssocPath)
                {
                    // Get Association interface for object path
                    for (const auto& [interface, values] : objDictEntry.second)
                    {
                        if (interface == "xyz.openbmc_project.Association")
                        {
                            for (const auto& [valueName, value] : values)
                            {
                                if (valueName == "endpoints")
                                {
                                    const std::vector<std::string>* endpoints =
                                        std::get_if<std::vector<std::string>>(
                                            &value);
                                    if ((endpoints != nullptr) &&
                                        !endpoints->empty())
                                    {
                                        // Add inventory item to vector
                                        const std::string& invItemPath =
                                            endpoints->front();
                                        addInventoryItem(inventoryItems,
                                                         invItemPath,
                                                         sensorName);
                                    }
                                }
                            }
                        }
                    }
                    break;
                }
            }
        }

        // Now loop through the returned object paths again, this time to
        // find the leds associated with the inventory items we just found
        std::string inventoryAssocPath;
        inventoryAssocPath.reserve(128); // avoid memory allocations
        for (const auto& objDictEntry : resp)
        {
            const std::string& objPath =
                static_cast<const std::string&>(objDictEntry.first);

            for (InventoryItem& inventoryItem : *inventoryItems)
            {
                inventoryAssocPath = inventoryItem.objectPath;
                inventoryAssocPath += "/leds";
                if (objPath == inventoryAssocPath)
                {
                    for (const auto& [interface, values] : objDictEntry.second)
                    {
                        if (interface == "xyz.openbmc_project.Association")
                        {
                            for (const auto& [valueName, value] : values)
                            {
                                if (valueName == "endpoints")
                                {
                                    const std::vector<std::string>* endpoints =
                                        std::get_if<std::vector<std::string>>(
                                            &value);
                                    if ((endpoints != nullptr) &&
                                        !endpoints->empty())
                                    {
                                        // Add inventory item to vector
                                        // Store LED path in inventory item
                                        const std::string& ledPath =
                                            endpoints->front();
                                        inventoryItem.ledObjectPath = ledPath;
                                    }
                                }
                            }
                        }
                    }

                    break;
                }
            }
        }
        callback(inventoryItems);
        BMCWEB_LOG_DEBUG << "getInventoryItemAssociations respHandler exit";
    };

    // Call GetManagedObjects on the ObjectMapper to get all associations
    crow::connections::systemBus->async_method_call(
        std::move(respHandler), "xyz.openbmc_project.ObjectMapper", "/",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");

    BMCWEB_LOG_DEBUG << "getInventoryItemAssociations exit";
}

/**
 * @brief Gets D-Bus data for inventory item leds associated with sensors.
 *
 * Uses the specified connections (services) to obtain D-Bus data for inventory
 * item leds associated with sensors.  Stores the resulting data in the
 * inventoryItems vector.
 *
 * This data is later used to provide sensor property values in the JSON
 * response.
 *
 * Finds the inventory item led data asynchronously.  Invokes callback when data
 * has been obtained.
 *
 * The callback must have the following signature:
 *   @code
 *   callback()
 *   @endcode
 *
 * This function is called recursively, obtaining data asynchronously from one
 * connection in each call.  This ensures the callback is not invoked until the
 * last asynchronous function has completed.
 *
 * @param sensorsAsyncResp Pointer to object holding response data.
 * @param inventoryItems D-Bus inventory items associated with sensors.
 * @param ledConnections Connections that provide data for the inventory leds.
 * @param callback Callback to invoke when inventory data has been obtained.
 * @param ledConnectionsIndex Current index in ledConnections.  Only specified
 * in recursive calls to this function.
 */
template <typename Callback>
void getInventoryLedData(
    std::shared_ptr<SensorsAsyncResp> sensorsAsyncResp,
    std::shared_ptr<std::vector<InventoryItem>> inventoryItems,
    std::shared_ptr<std::map<std::string, std::string>> ledConnections,
    Callback&& callback, size_t ledConnectionsIndex = 0)
{
    BMCWEB_LOG_DEBUG << "getInventoryLedData enter";

    // If no more connections left, call callback
    if (ledConnectionsIndex >= ledConnections->size())
    {
        callback();
        BMCWEB_LOG_DEBUG << "getInventoryLedData exit";
        return;
    }

    // Get inventory item data from current connection
    auto it = ledConnections->begin();
    std::advance(it, ledConnectionsIndex);
    if (it != ledConnections->end())
    {
        const std::string& ledPath = (*it).first;
        const std::string& ledConnection = (*it).second;
        // Response handler for Get State property
        auto respHandler =
            [sensorsAsyncResp, inventoryItems, ledConnections, ledPath,
             callback{std::forward<Callback>(callback)}, ledConnectionsIndex](
                const boost::system::error_code& ec, const std::string& state) {
            BMCWEB_LOG_DEBUG << "getInventoryLedData respHandler enter";
            if (ec)
            {
                BMCWEB_LOG_ERROR
                    << "getInventoryLedData respHandler DBus error " << ec;
                messages::internalError(sensorsAsyncResp->asyncResp->res);
                return;
            }

            BMCWEB_LOG_DEBUG << "Led state: " << state;
            // Find inventory item with this LED object path
            InventoryItem* inventoryItem =
                findInventoryItemForLed(*inventoryItems, ledPath);
            if (inventoryItem != nullptr)
            {
                // Store LED state in InventoryItem
                if (state.ends_with("On"))
                {
                    inventoryItem->ledState = LedState::ON;
                }
                else if (state.ends_with("Blink"))
                {
                    inventoryItem->ledState = LedState::BLINK;
                }
                else if (state.ends_with("Off"))
                {
                    inventoryItem->ledState = LedState::OFF;
                }
                else
                {
                    inventoryItem->ledState = LedState::UNKNOWN;
                }
            }

            // Recurse to get LED data from next connection
            getInventoryLedData(sensorsAsyncResp, inventoryItems,
                                ledConnections, std::move(callback),
                                ledConnectionsIndex + 1);

            BMCWEB_LOG_DEBUG << "getInventoryLedData respHandler exit";
        };

        // Get the State property for the current LED
        sdbusplus::asio::getProperty<std::string>(
            *crow::connections::systemBus, ledConnection, ledPath,
            "xyz.openbmc_project.Led.Physical", "State",
            std::move(respHandler));
    }

    BMCWEB_LOG_DEBUG << "getInventoryLedData exit";
}

/**
 * @brief Gets LED data for LEDs associated with given inventory items.
 *
 * Gets the D-Bus connections (services) that provide LED data for the LEDs
 * associated with the specified inventory items.  Then gets the LED data from
 * each connection and stores it in the inventory item.
 *
 * This data is later used to provide sensor property values in the JSON
 * response.
 *
 * Finds the LED data asynchronously.  Invokes callback when information has
 * been obtained.
 *
 * The callback must have the following signature:
 *   @code
 *   callback()
 *   @endcode
 *
 * @param sensorsAsyncResp Pointer to object holding response data.
 * @param inventoryItems D-Bus inventory items associated with sensors.
 * @param callback Callback to invoke when inventory items have been obtained.
 */
template <typename Callback>
void getInventoryLeds(
    std::shared_ptr<SensorsAsyncResp> sensorsAsyncResp,
    std::shared_ptr<std::vector<InventoryItem>> inventoryItems,
    Callback&& callback)
{
    BMCWEB_LOG_DEBUG << "getInventoryLeds enter";

    const std::string path = "/xyz/openbmc_project";
    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Led.Physical"};

    // Make call to ObjectMapper to find all inventory items
    dbus::utility::getSubTree(
        path, 0, interfaces,
        [callback{std::forward<Callback>(callback)}, sensorsAsyncResp,
         inventoryItems](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreeResponse& subtree) {
        // Response handler for parsing output from GetSubTree
        BMCWEB_LOG_DEBUG << "getInventoryLeds respHandler enter";
        if (ec)
        {
            messages::internalError(sensorsAsyncResp->asyncResp->res);
            BMCWEB_LOG_ERROR << "getInventoryLeds respHandler DBus error "
                             << ec;
            return;
        }

        // Build map of LED object paths to connections
        std::shared_ptr<std::map<std::string, std::string>> ledConnections =
            std::make_shared<std::map<std::string, std::string>>();

        // Loop through objects from GetSubTree
        for (const std::pair<
                 std::string,
                 std::vector<std::pair<std::string, std::vector<std::string>>>>&
                 object : subtree)
        {
            // Check if object path is LED for one of the specified inventory
            // items
            const std::string& ledPath = object.first;
            if (findInventoryItemForLed(*inventoryItems, ledPath) != nullptr)
            {
                // Add mapping from ledPath to connection
                const std::string& connection = object.second.begin()->first;
                (*ledConnections)[ledPath] = connection;
                BMCWEB_LOG_DEBUG << "Added mapping " << ledPath << " -> "
                                 << connection;
            }
        }

        getInventoryLedData(sensorsAsyncResp, inventoryItems, ledConnections,
                            std::move(callback));
        BMCWEB_LOG_DEBUG << "getInventoryLeds respHandler exit";
        });
    BMCWEB_LOG_DEBUG << "getInventoryLeds exit";
}

/**
 * @brief Gets D-Bus data for Power Supply Attributes such as EfficiencyPercent
 *
 * Uses the specified connections (services) (currently assumes just one) to
 * obtain D-Bus data for Power Supply Attributes. Stores the resulting data in
 * the inventoryItems vector. Only stores data in Power Supply inventoryItems.
 *
 * This data is later used to provide sensor property values in the JSON
 * response.
 *
 * Finds the Power Supply Attributes data asynchronously.  Invokes callback
 * when data has been obtained.
 *
 * The callback must have the following signature:
 *   @code
 *   callback(std::shared_ptr<std::vector<InventoryItem>> inventoryItems)
 *   @endcode
 *
 * @param sensorsAsyncResp Pointer to object holding response data.
 * @param inventoryItems D-Bus inventory items associated with sensors.
 * @param psAttributesConnections Connections that provide data for the Power
 *        Supply Attributes
 * @param callback Callback to invoke when data has been obtained.
 */
template <typename Callback>
void getPowerSupplyAttributesData(
    const std::shared_ptr<SensorsAsyncResp>& sensorsAsyncResp,
    std::shared_ptr<std::vector<InventoryItem>> inventoryItems,
    const std::map<std::string, std::string>& psAttributesConnections,
    Callback&& callback)
{
    BMCWEB_LOG_DEBUG << "getPowerSupplyAttributesData enter";

    if (psAttributesConnections.empty())
    {
        BMCWEB_LOG_DEBUG << "Can't find PowerSupplyAttributes, no connections!";
        callback(inventoryItems);
        return;
    }

    // Assuming just one connection (service) for now
    auto it = psAttributesConnections.begin();

    const std::string& psAttributesPath = (*it).first;
    const std::string& psAttributesConnection = (*it).second;

    // Response handler for Get DeratingFactor property
    auto respHandler =
        [sensorsAsyncResp, inventoryItems,
         callback{std::forward<Callback>(callback)}](
            const boost::system::error_code& ec, const uint32_t value) {
        BMCWEB_LOG_DEBUG << "getPowerSupplyAttributesData respHandler enter";
        if (ec)
        {
            BMCWEB_LOG_ERROR
                << "getPowerSupplyAttributesData respHandler DBus error " << ec;
            messages::internalError(sensorsAsyncResp->asyncResp->res);
            return;
        }

        BMCWEB_LOG_DEBUG << "PS EfficiencyPercent value: " << value;
        // Store value in Power Supply Inventory Items
        for (InventoryItem& inventoryItem : *inventoryItems)
        {
            if (inventoryItem.isPowerSupply)
            {
                inventoryItem.powerSupplyEfficiencyPercent =
                    static_cast<int>(value);
            }
        }

        BMCWEB_LOG_DEBUG << "getPowerSupplyAttributesData respHandler exit";
        callback(inventoryItems);
    };

    // Get the DeratingFactor property for the PowerSupplyAttributes
    // Currently only property on the interface/only one we care about
    sdbusplus::asio::getProperty<uint32_t>(
        *crow::connections::systemBus, psAttributesConnection, psAttributesPath,
        "xyz.openbmc_project.Control.PowerSupplyAttributes", "DeratingFactor",
        std::move(respHandler));

    BMCWEB_LOG_DEBUG << "getPowerSupplyAttributesData exit";
}

/**
 * @brief Gets the Power Supply Attributes such as EfficiencyPercent
 *
 * Gets the D-Bus connection (service) that provides Power Supply Attributes
 * data. Then gets the Power Supply Attributes data from the connection
 * (currently just assumes 1 connection) and stores the data in the inventory
 * item.
 *
 * This data is later used to provide sensor property values in the JSON
 * response. DeratingFactor on D-Bus is mapped to EfficiencyPercent on Redfish.
 *
 * Finds the Power Supply Attributes data asynchronously. Invokes callback
 * when information has been obtained.
 *
 * The callback must have the following signature:
 *   @code
 *   callback(std::shared_ptr<std::vector<InventoryItem>> inventoryItems)
 *   @endcode
 *
 * @param sensorsAsyncResp Pointer to object holding response data.
 * @param inventoryItems D-Bus inventory items associated with sensors.
 * @param callback Callback to invoke when data has been obtained.
 */
template <typename Callback>
void getPowerSupplyAttributes(
    std::shared_ptr<SensorsAsyncResp> sensorsAsyncResp,
    std::shared_ptr<std::vector<InventoryItem>> inventoryItems,
    Callback&& callback)
{
    BMCWEB_LOG_DEBUG << "getPowerSupplyAttributes enter";

    // Only need the power supply attributes when the Power Schema
    if (sensorsAsyncResp->chassisSubNode != sensors::node::power)
    {
        BMCWEB_LOG_DEBUG << "getPowerSupplyAttributes exit since not Power";
        callback(inventoryItems);
        return;
    }

    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Control.PowerSupplyAttributes"};

    // Make call to ObjectMapper to find the PowerSupplyAttributes service
    dbus::utility::getSubTree(
        "/xyz/openbmc_project", 0, interfaces,
        [callback{std::forward<Callback>(callback)}, sensorsAsyncResp,
         inventoryItems](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreeResponse& subtree) {
        // Response handler for parsing output from GetSubTree
        BMCWEB_LOG_DEBUG << "getPowerSupplyAttributes respHandler enter";
        if (ec)
        {
            messages::internalError(sensorsAsyncResp->asyncResp->res);
            BMCWEB_LOG_ERROR
                << "getPowerSupplyAttributes respHandler DBus error " << ec;
            return;
        }
        if (subtree.empty())
        {
            BMCWEB_LOG_DEBUG << "Can't find Power Supply Attributes!";
            callback(inventoryItems);
            return;
        }

        // Currently we only support 1 power supply attribute, use this for
        // all the power supplies. Build map of object path to connection.
        // Assume just 1 connection and 1 path for now.
        std::map<std::string, std::string> psAttributesConnections;

        if (subtree[0].first.empty() || subtree[0].second.empty())
        {
            BMCWEB_LOG_DEBUG << "Power Supply Attributes mapper error!";
            callback(inventoryItems);
            return;
        }

        const std::string& psAttributesPath = subtree[0].first;
        const std::string& connection = subtree[0].second.begin()->first;

        if (connection.empty())
        {
            BMCWEB_LOG_DEBUG << "Power Supply Attributes mapper error!";
            callback(inventoryItems);
            return;
        }

        psAttributesConnections[psAttributesPath] = connection;
        BMCWEB_LOG_DEBUG << "Added mapping " << psAttributesPath << " -> "
                         << connection;

        getPowerSupplyAttributesData(sensorsAsyncResp, inventoryItems,
                                     psAttributesConnections,
                                     std::move(callback));
        BMCWEB_LOG_DEBUG << "getPowerSupplyAttributes respHandler exit";
        });
    BMCWEB_LOG_DEBUG << "getPowerSupplyAttributes exit";
}

/**
 * @brief Gets inventory items associated with sensors.
 *
 * Finds the inventory items that are associated with the specified sensors.
 * Then gets D-Bus data for the inventory items, such as presence and VPD.
 *
 * This data is later used to provide sensor property values in the JSON
 * response.
 *
 * Finds the inventory items asynchronously.  Invokes callback when the
 * inventory items have been obtained.
 *
 * The callback must have the following signature:
 *   @code
 *   callback(std::shared_ptr<std::vector<InventoryItem>> inventoryItems)
 *   @endcode
 *
 * @param sensorsAsyncResp Pointer to object holding response data.
 * @param sensorNames All sensors within the current chassis.
 * implements ObjectManager.
 * @param callback Callback to invoke when inventory items have been obtained.
 */
template <typename Callback>
static void
    getInventoryItems(std::shared_ptr<SensorsAsyncResp> sensorsAsyncResp,
                      const std::shared_ptr<std::set<std::string>> sensorNames,
                      Callback&& callback)
{
    BMCWEB_LOG_DEBUG << "getInventoryItems enter";
    auto getInventoryItemAssociationsCb =
        [sensorsAsyncResp, callback{std::forward<Callback>(callback)}](
            std::shared_ptr<std::vector<InventoryItem>> inventoryItems) {
        BMCWEB_LOG_DEBUG << "getInventoryItemAssociationsCb enter";
        auto getInventoryItemsConnectionsCb =
            [sensorsAsyncResp, inventoryItems,
             callback{std::forward<const Callback>(callback)}](
                std::shared_ptr<std::set<std::string>> invConnections) {
            BMCWEB_LOG_DEBUG << "getInventoryItemsConnectionsCb enter";
            auto getInventoryItemsDataCb = [sensorsAsyncResp, inventoryItems,
                                            callback{std::move(callback)}]() {
                BMCWEB_LOG_DEBUG << "getInventoryItemsDataCb enter";

                auto getInventoryLedsCb = [sensorsAsyncResp, inventoryItems,
                                           callback{std::move(callback)}]() {
                    BMCWEB_LOG_DEBUG << "getInventoryLedsCb enter";
                    // Find Power Supply Attributes and get the data
                    getPowerSupplyAttributes(sensorsAsyncResp, inventoryItems,
                                             std::move(callback));
                    BMCWEB_LOG_DEBUG << "getInventoryLedsCb exit";
                };

                // Find led connections and get the data
                getInventoryLeds(sensorsAsyncResp, inventoryItems,
                                 std::move(getInventoryLedsCb));
                BMCWEB_LOG_DEBUG << "getInventoryItemsDataCb exit";
            };

            // Get inventory item data from connections
            getInventoryItemsData(sensorsAsyncResp, inventoryItems,
                                  invConnections,
                                  std::move(getInventoryItemsDataCb));
            BMCWEB_LOG_DEBUG << "getInventoryItemsConnectionsCb exit";
        };

        // Get connections that provide inventory item data
        getInventoryItemsConnections(sensorsAsyncResp, inventoryItems,
                                     std::move(getInventoryItemsConnectionsCb));
        BMCWEB_LOG_DEBUG << "getInventoryItemAssociationsCb exit";
    };

    // Get associations from sensors to inventory items
    getInventoryItemAssociations(sensorsAsyncResp, sensorNames,
                                 std::move(getInventoryItemAssociationsCb));
    BMCWEB_LOG_DEBUG << "getInventoryItems exit";
}

/**
 * @brief Returns JSON PowerSupply object for the specified inventory item.
 *
 * Searches for a JSON PowerSupply object that matches the specified inventory
 * item.  If one is not found, a new PowerSupply object is added to the JSON
 * array.
 *
 * Multiple sensors are often associated with one power supply inventory item.
 * As a result, multiple sensor values are stored in one JSON PowerSupply
 * object.
 *
 * @param powerSupplyArray JSON array containing Redfish PowerSupply objects.
 * @param inventoryItem Inventory item for the power supply.
 * @param chassisId Chassis that contains the power supply.
 * @return JSON PowerSupply object for the specified inventory item.
 */
inline nlohmann::json& getPowerSupply(nlohmann::json& powerSupplyArray,
                                      const InventoryItem& inventoryItem,
                                      const std::string& chassisId)
{
    // Check if matching PowerSupply object already exists in JSON array
    for (nlohmann::json& powerSupply : powerSupplyArray)
    {
        if (powerSupply["MemberId"] == inventoryItem.name)
        {
            return powerSupply;
        }
    }

    // Add new PowerSupply object to JSON array
    powerSupplyArray.push_back({});
    nlohmann::json& powerSupply = powerSupplyArray.back();
    boost::urls::url url = crow::utility::urlFromPieces(
        "redfish", "v1", "Chassis", chassisId, "Power");
    url.set_fragment(("/PowerSupplies"_json_pointer).to_string());
    powerSupply["@odata.id"] = std::move(url);
    powerSupply["MemberId"] = inventoryItem.name;
    powerSupply["Name"] = boost::replace_all_copy(inventoryItem.name, "_", " ");
    powerSupply["Manufacturer"] = inventoryItem.manufacturer;
    powerSupply["Model"] = inventoryItem.model;
    powerSupply["PartNumber"] = inventoryItem.partNumber;
    powerSupply["SerialNumber"] = inventoryItem.serialNumber;
    setLedState(powerSupply, &inventoryItem);

    if (inventoryItem.powerSupplyEfficiencyPercent >= 0)
    {
        powerSupply["EfficiencyPercent"] =
            inventoryItem.powerSupplyEfficiencyPercent;
    }

    powerSupply["Status"]["State"] = getState(&inventoryItem);
    const char* health = inventoryItem.isFunctional ? "OK" : "Critical";
    powerSupply["Status"]["Health"] = health;

    return powerSupply;
}

/**
 * @brief Gets the values of the specified sensors.
 *
 * Stores the results as JSON in the SensorsAsyncResp.
 *
 * Gets the sensor values asynchronously.  Stores the results later when the
 * information has been obtained.
 *
 * The sensorNames set contains all requested sensors for the current chassis.
 *
 * To minimize the number of DBus calls, the DBus method
 * org.freedesktop.DBus.ObjectManager.GetManagedObjects() is used to get the
 * values of all sensors provided by a connection (service).
 *
 * The connections set contains all the connections that provide sensor values.
 *
 * The InventoryItem vector contains D-Bus inventory items associated with the
 * sensors.  Inventory item data is needed for some Redfish sensor properties.
 *
 * @param SensorsAsyncResp Pointer to object holding response data.
 * @param sensorNames All requested sensors within the current chassis.
 * @param connections Connections that provide sensor values.
 * implements ObjectManager.
 * @param inventoryItems Inventory items associated with the sensors.
 */
inline void getSensorData(
    const std::shared_ptr<SensorsAsyncResp>& sensorsAsyncResp,
    const std::shared_ptr<std::set<std::string>>& sensorNames,
    const std::set<std::string>& connections,
    const std::shared_ptr<std::vector<InventoryItem>>& inventoryItems)
{
    BMCWEB_LOG_DEBUG << "getSensorData enter";
    // Get managed objects from all services exposing sensors
    for (const std::string& connection : connections)
    {
        // Response handler to process managed objects
        auto getManagedObjectsCb =
            [sensorsAsyncResp, sensorNames,
             inventoryItems](const boost::system::error_code& ec,
                             const dbus::utility::ManagedObjectType& resp) {
            BMCWEB_LOG_DEBUG << "getManagedObjectsCb enter";
            if (ec)
            {
                BMCWEB_LOG_ERROR << "getManagedObjectsCb DBUS error: " << ec;
                messages::internalError(sensorsAsyncResp->asyncResp->res);
                return;
            }
            // Go through all objects and update response with sensor data
            for (const auto& objDictEntry : resp)
            {
                const std::string& objPath =
                    static_cast<const std::string&>(objDictEntry.first);
                BMCWEB_LOG_DEBUG << "getManagedObjectsCb parsing object "
                                 << objPath;

                std::vector<std::string> split;
                // Reserve space for
                // /xyz/openbmc_project/sensors/<name>/<subname>
                split.reserve(6);
                // NOLINTNEXTLINE
                bmcweb::split(split, objPath, '/');
                if (split.size() < 6)
                {
                    BMCWEB_LOG_ERROR << "Got path that isn't long enough "
                                     << objPath;
                    continue;
                }
                // These indexes aren't intuitive, as split puts an empty
                // string at the beginning
                const std::string& sensorType = split[4];
                const std::string& sensorName = split[5];
                BMCWEB_LOG_DEBUG << "sensorName " << sensorName
                                 << " sensorType " << sensorType;
                if (sensorNames->find(objPath) == sensorNames->end())
                {
                    BMCWEB_LOG_DEBUG << sensorName << " not in sensor list ";
                    continue;
                }

                // Find inventory item (if any) associated with sensor
                InventoryItem* inventoryItem =
                    findInventoryItemForSensor(inventoryItems, objPath);

                const std::string& sensorSchema =
                    sensorsAsyncResp->chassisSubNode;

                nlohmann::json* sensorJson = nullptr;

                if (sensorSchema == sensors::node::sensors &&
                    !sensorsAsyncResp->efficientExpand)
                {
                    std::string sensorTypeEscaped(sensorType);
                    sensorTypeEscaped.erase(
                        std::remove(sensorTypeEscaped.begin(),
                                    sensorTypeEscaped.end(), '_'),
                        sensorTypeEscaped.end());
                    std::string sensorId(sensorTypeEscaped);
                    sensorId += "_";
                    sensorId += sensorName;

                    sensorsAsyncResp->asyncResp->res.jsonValue["@odata.id"] =
                        crow::utility::urlFromPieces(
                            "redfish", "v1", "Chassis",
                            sensorsAsyncResp->chassisId,
                            sensorsAsyncResp->chassisSubNode, sensorId);
                    sensorJson = &(sensorsAsyncResp->asyncResp->res.jsonValue);
                }
                else
                {
                    std::string fieldName;
                    if (sensorsAsyncResp->efficientExpand)
                    {
                        fieldName = "Members";
                    }
                    else if (sensorType == "temperature")
                    {
                        fieldName = "Temperatures";
                    }
                    else if (sensorType == "fan" || sensorType == "fan_tach" ||
                             sensorType == "fan_pwm")
                    {
                        fieldName = "Fans";
                    }
                    else if (sensorType == "voltage")
                    {
                        fieldName = "Voltages";
                    }
                    else if (sensorType == "power")
                    {
                        if (sensorName == "total_power")
                        {
                            fieldName = "PowerControl";
                        }
                        else if ((inventoryItem != nullptr) &&
                                 (inventoryItem->isPowerSupply))
                        {
                            fieldName = "PowerSupplies";
                        }
                        else
                        {
                            // Other power sensors are in SensorCollection
                            continue;
                        }
                    }
                    else
                    {
                        BMCWEB_LOG_ERROR << "Unsure how to handle sensorType "
                                         << sensorType;
                        continue;
                    }

                    nlohmann::json& tempArray =
                        sensorsAsyncResp->asyncResp->res.jsonValue[fieldName];
                    if (fieldName == "PowerControl")
                    {
                        if (tempArray.empty())
                        {
                            // Put multiple "sensors" into a single
                            // PowerControl. Follows MemberId naming and
                            // naming in power.hpp.
                            nlohmann::json::object_t power;
                            boost::urls::url url = crow::utility::urlFromPieces(
                                "redfish", "v1", "Chassis",
                                sensorsAsyncResp->chassisId,
                                sensorsAsyncResp->chassisSubNode);
                            url.set_fragment((""_json_pointer / fieldName / "0")
                                                 .to_string());
                            power["@odata.id"] = std::move(url);
                            tempArray.push_back(std::move(power));
                        }
                        sensorJson = &(tempArray.back());
                    }
                    else if (fieldName == "PowerSupplies")
                    {
                        if (inventoryItem != nullptr)
                        {
                            sensorJson =
                                &(getPowerSupply(tempArray, *inventoryItem,
                                                 sensorsAsyncResp->chassisId));
                        }
                    }
                    else if (fieldName == "Members")
                    {
                        std::string sensorTypeEscaped(sensorType);
                        sensorTypeEscaped.erase(
                            std::remove(sensorTypeEscaped.begin(),
                                        sensorTypeEscaped.end(), '_'),
                            sensorTypeEscaped.end());
                        std::string sensorId(sensorTypeEscaped);
                        sensorId += "_";
                        sensorId += sensorName;

                        nlohmann::json::object_t member;
                        member["@odata.id"] = crow::utility::urlFromPieces(
                            "redfish", "v1", "Chassis",
                            sensorsAsyncResp->chassisId,
                            sensorsAsyncResp->chassisSubNode, sensorId);
                        tempArray.push_back(std::move(member));
                        sensorJson = &(tempArray.back());
                    }
                    else
                    {
                        nlohmann::json::object_t member;
                        boost::urls::url url = crow::utility::urlFromPieces(
                            "redfish", "v1", "Chassis",
                            sensorsAsyncResp->chassisId,
                            sensorsAsyncResp->chassisSubNode);
                        url.set_fragment(
                            (""_json_pointer / fieldName).to_string());
                        member["@odata.id"] = std::move(url);
                        tempArray.push_back(std::move(member));
                        sensorJson = &(tempArray.back());
                    }
                }

                if (sensorJson != nullptr)
                {
                    objectInterfacesToJson(sensorName, sensorType,
                                           sensorsAsyncResp->chassisSubNode,
                                           objDictEntry.second, *sensorJson,
                                           inventoryItem);

                    std::string path = "/xyz/openbmc_project/sensors/";
                    path += sensorType;
                    path += "/";
                    path += sensorName;
                    sensorsAsyncResp->addMetadata(*sensorJson, path);
                }
            }
            if (sensorsAsyncResp.use_count() == 1)
            {
                sortJSONResponse(sensorsAsyncResp);
                if (sensorsAsyncResp->chassisSubNode ==
                        sensors::node::sensors &&
                    sensorsAsyncResp->efficientExpand)
                {
                    sensorsAsyncResp->asyncResp->res
                        .jsonValue["Members@odata.count"] =
                        sensorsAsyncResp->asyncResp->res.jsonValue["Members"]
                            .size();
                }
                else if (sensorsAsyncResp->chassisSubNode ==
                         sensors::node::thermal)
                {
                    populateFanRedundancy(sensorsAsyncResp);
                }
            }
            BMCWEB_LOG_DEBUG << "getManagedObjectsCb exit";
        };

        crow::connections::systemBus->async_method_call(
            getManagedObjectsCb, connection, "/xyz/openbmc_project/sensors",
            "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
    }
    BMCWEB_LOG_DEBUG << "getSensorData exit";
}

inline void
    processSensorList(const std::shared_ptr<SensorsAsyncResp>& sensorsAsyncResp,
                      const std::shared_ptr<std::set<std::string>>& sensorNames)
{
    auto getConnectionCb = [sensorsAsyncResp, sensorNames](
                               const std::set<std::string>& connections) {
        BMCWEB_LOG_DEBUG << "getConnectionCb enter";
        auto getInventoryItemsCb =
            [sensorsAsyncResp, sensorNames,
             connections](const std::shared_ptr<std::vector<InventoryItem>>&
                              inventoryItems) {
            BMCWEB_LOG_DEBUG << "getInventoryItemsCb enter";
            // Get sensor data and store results in JSON
            getSensorData(sensorsAsyncResp, sensorNames, connections,
                          inventoryItems);
            BMCWEB_LOG_DEBUG << "getInventoryItemsCb exit";
        };

        // Get inventory items associated with sensors
        getInventoryItems(sensorsAsyncResp, sensorNames,
                          std::move(getInventoryItemsCb));

        BMCWEB_LOG_DEBUG << "getConnectionCb exit";
    };

    // Get set of connections that provide sensor values
    getConnections(sensorsAsyncResp, sensorNames, std::move(getConnectionCb));
}

/**
 * @brief Entry point for retrieving sensors data related to requested
 *        chassis.
 * @param SensorsAsyncResp   Pointer to object holding response data
 */
inline void
    getChassisData(const std::shared_ptr<SensorsAsyncResp>& sensorsAsyncResp)
{
    BMCWEB_LOG_DEBUG << "getChassisData enter";
    auto getChassisCb =
        [sensorsAsyncResp](
            const std::shared_ptr<std::set<std::string>>& sensorNames) {
        BMCWEB_LOG_DEBUG << "getChassisCb enter";
        processSensorList(sensorsAsyncResp, sensorNames);
        BMCWEB_LOG_DEBUG << "getChassisCb exit";
    };
    // SensorCollection doesn't contain the Redundancy property
    if (sensorsAsyncResp->chassisSubNode != sensors::node::sensors)
    {
        sensorsAsyncResp->asyncResp->res.jsonValue["Redundancy"] =
            nlohmann::json::array();
    }
    // Get set of sensors in chassis
    getChassis(sensorsAsyncResp->asyncResp, sensorsAsyncResp->chassisId,
               sensorsAsyncResp->chassisSubNode, sensorsAsyncResp->types,
               std::move(getChassisCb));
    BMCWEB_LOG_DEBUG << "getChassisData exit";
}

/**
 * @brief Find the requested sensorName in the list of all sensors supplied by
 * the chassis node
 *
 * @param sensorName   The sensor name supplied in the PATCH request
 * @param sensorsList  The list of sensors managed by the chassis node
 * @param sensorsModified  The list of sensors that were found as a result of
 *                         repeated calls to this function
 */
inline bool
    findSensorNameUsingSensorPath(std::string_view sensorName,
                                  const std::set<std::string>& sensorsList,
                                  std::set<std::string>& sensorsModified)
{
    for (const auto& chassisSensor : sensorsList)
    {
        sdbusplus::message::object_path path(chassisSensor);
        std::string thisSensorName = path.filename();
        if (thisSensorName.empty())
        {
            continue;
        }
        if (thisSensorName == sensorName)
        {
            sensorsModified.emplace(chassisSensor);
            return true;
        }
    }
    return false;
}

inline std::pair<std::string, std::string>
    splitSensorNameAndType(std::string_view sensorId)
{
    size_t index = sensorId.find('_');
    if (index == std::string::npos)
    {
        return std::make_pair<std::string, std::string>("", "");
    }
    std::string sensorType{sensorId.substr(0, index)};
    std::string sensorName{sensorId.substr(index + 1)};
    // fan_pwm and fan_tach need special handling
    if (sensorType == "fantach" || sensorType == "fanpwm")
    {
        sensorType.insert(3, 1, '_');
    }
    return std::make_pair(sensorType, sensorName);
}

/**
 * @brief Entry point for overriding sensor values of given sensor
 *
 * @param sensorAsyncResp   response object
 * @param allCollections   Collections extract from sensors' request patch info
 * @param chassisSubNode   Chassis Node for which the query has to happen
 */
inline void setSensorsOverride(
    const std::shared_ptr<SensorsAsyncResp>& sensorAsyncResp,
    std::unordered_map<std::string, std::vector<nlohmann::json>>&
        allCollections)
{
    BMCWEB_LOG_INFO << "setSensorsOverride for subNode"
                    << sensorAsyncResp->chassisSubNode << "\n";

    const char* propertyValueName = nullptr;
    std::unordered_map<std::string, std::pair<double, std::string>> overrideMap;
    std::string memberId;
    double value = 0.0;
    for (auto& collectionItems : allCollections)
    {
        if (collectionItems.first == "Temperatures")
        {
            propertyValueName = "ReadingCelsius";
        }
        else if (collectionItems.first == "Fans")
        {
            propertyValueName = "Reading";
        }
        else
        {
            propertyValueName = "ReadingVolts";
        }
        for (auto& item : collectionItems.second)
        {
            if (!json_util::readJson(item, sensorAsyncResp->asyncResp->res,
                                     "MemberId", memberId, propertyValueName,
                                     value))
            {
                return;
            }
            overrideMap.emplace(memberId,
                                std::make_pair(value, collectionItems.first));
        }
    }

    auto getChassisSensorListCb =
        [sensorAsyncResp, overrideMap](
            const std::shared_ptr<std::set<std::string>>& sensorsList) {
        // Match sensor names in the PATCH request to those managed by the
        // chassis node
        const std::shared_ptr<std::set<std::string>> sensorNames =
            std::make_shared<std::set<std::string>>();
        for (const auto& item : overrideMap)
        {
            const auto& sensor = item.first;
            std::pair<std::string, std::string> sensorNameType =
                splitSensorNameAndType(sensor);
            if (!findSensorNameUsingSensorPath(sensorNameType.second,
                                               *sensorsList, *sensorNames))
            {
                BMCWEB_LOG_INFO << "Unable to find memberId " << item.first;
                messages::resourceNotFound(sensorAsyncResp->asyncResp->res,
                                           item.second.second, item.first);
                return;
            }
        }
        // Get the connection to which the memberId belongs
        auto getObjectsWithConnectionCb =
            [sensorAsyncResp,
             overrideMap](const std::set<std::string>& /*connections*/,
                          const std::set<std::pair<std::string, std::string>>&
                              objectsWithConnection) {
            if (objectsWithConnection.size() != overrideMap.size())
            {
                BMCWEB_LOG_INFO
                    << "Unable to find all objects with proper connection "
                    << objectsWithConnection.size() << " requested "
                    << overrideMap.size() << "\n";
                messages::resourceNotFound(sensorAsyncResp->asyncResp->res,
                                           sensorAsyncResp->chassisSubNode ==
                                                   sensors::node::thermal
                                               ? "Temperatures"
                                               : "Voltages",
                                           "Count");
                return;
            }
            for (const auto& item : objectsWithConnection)
            {
                sdbusplus::message::object_path path(item.first);
                std::string sensorName = path.filename();
                if (sensorName.empty())
                {
                    messages::internalError(sensorAsyncResp->asyncResp->res);
                    return;
                }

                const auto& iterator = overrideMap.find(sensorName);
                if (iterator == overrideMap.end())
                {
                    BMCWEB_LOG_INFO << "Unable to find sensor object"
                                    << item.first << "\n";
                    messages::internalError(sensorAsyncResp->asyncResp->res);
                    return;
                }
                crow::connections::systemBus->async_method_call(
                    [sensorAsyncResp](const boost::system::error_code& ec) {
                    if (ec)
                    {
                        if (ec.value() ==
                            boost::system::errc::permission_denied)
                        {
                            BMCWEB_LOG_WARNING
                                << "Manufacturing mode is not Enabled...can't "
                                   "Override the sensor value. ";

                            messages::insufficientPrivilege(
                                sensorAsyncResp->asyncResp->res);
                            return;
                        }
                        BMCWEB_LOG_DEBUG
                            << "setOverrideValueStatus DBUS error: " << ec;
                        messages::internalError(
                            sensorAsyncResp->asyncResp->res);
                    }
                    },
                    item.second, item.first, "org.freedesktop.DBus.Properties",
                    "Set", "xyz.openbmc_project.Sensor.Value", "Value",
                    dbus::utility::DbusVariantType(iterator->second.first));
            }
        };
        // Get object with connection for the given sensor name
        getObjectsWithConnection(sensorAsyncResp, sensorNames,
                                 std::move(getObjectsWithConnectionCb));
    };
    // get full sensor list for the given chassisId and cross verify the sensor.
    getChassis(sensorAsyncResp->asyncResp, sensorAsyncResp->chassisId,
               sensorAsyncResp->chassisSubNode, sensorAsyncResp->types,
               std::move(getChassisSensorListCb));
}

/**
 * @brief Retrieves mapping of Redfish URIs to sensor value property to D-Bus
 * path of the sensor.
 *
 * Function builds valid Redfish response for sensor query of given chassis and
 * node. It then builds metadata about Redfish<->D-Bus correlations and provides
 * it to caller in a callback.
 *
 * @param chassis   Chassis for which retrieval should be performed
 * @param node  Node (group) of sensors. See sensors::node for supported values
 * @param mapComplete   Callback to be called with retrieval result
 */
inline void retrieveUriToDbusMap(const std::string& chassis,
                                 const std::string& node,
                                 SensorsAsyncResp::DataCompleteCb&& mapComplete)
{
    decltype(sensors::paths)::const_iterator pathIt =
        std::find_if(sensors::paths.cbegin(), sensors::paths.cend(),
                     [&node](auto&& val) { return val.first == node; });
    if (pathIt == sensors::paths.cend())
    {
        BMCWEB_LOG_ERROR << "Wrong node provided : " << node;
        mapComplete(boost::beast::http::status::bad_request, {});
        return;
    }

    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    auto callback = [asyncResp, mapCompleteCb{std::move(mapComplete)}](
                        const boost::beast::http::status status,
                        const std::map<std::string, std::string>& uriToDbus) {
        mapCompleteCb(status, uriToDbus);
    };

    auto resp = std::make_shared<SensorsAsyncResp>(
        asyncResp, chassis, pathIt->second, node, std::move(callback));
    getChassisData(resp);
}

namespace sensors
{

inline void getChassisCallback(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    std::string_view chassisId, std::string_view chassisSubNode,
    const std::shared_ptr<std::set<std::string>>& sensorNames)
{
    BMCWEB_LOG_DEBUG << "getChassisCallback enter ";

    nlohmann::json& entriesArray = asyncResp->res.jsonValue["Members"];
    for (const std::string& sensor : *sensorNames)
    {
        BMCWEB_LOG_DEBUG << "Adding sensor: " << sensor;

        sdbusplus::message::object_path path(sensor);
        std::string sensorName = path.filename();
        if (sensorName.empty())
        {
            BMCWEB_LOG_ERROR << "Invalid sensor path: " << sensor;
            messages::internalError(asyncResp->res);
            return;
        }
        std::string type = path.parent_path().filename();
        // fan_tach has an underscore in it, so remove it to "normalize" the
        // type in the URI
        type.erase(std::remove(type.begin(), type.end(), '_'), type.end());

        nlohmann::json::object_t member;
        std::string id = type;
        id += "_";
        id += sensorName;
        member["@odata.id"] = crow::utility::urlFromPieces(
            "redfish", "v1", "Chassis", chassisId, chassisSubNode, id);

        entriesArray.push_back(std::move(member));
    }

    asyncResp->res.jsonValue["Members@odata.count"] = entriesArray.size();
    BMCWEB_LOG_DEBUG << "getChassisCallback exit";
}

inline void
    handleSensorCollectionGet(App& app, const crow::Request& req,
                              const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                              const std::string& chassisId)
{
    query_param::QueryCapabilities capabilities = {
        .canDelegateExpandLevel = 1,
    };
    query_param::Query delegatedQuery;
    if (!redfish::setUpRedfishRouteWithDelegation(app, req, aResp,
                                                  delegatedQuery, capabilities))
    {
        return;
    }

    if (delegatedQuery.expandType != query_param::ExpandType::None)
    {
        // we perform efficient expand.
        auto asyncResp = std::make_shared<SensorsAsyncResp>(
            aResp, chassisId, sensors::dbus::sensorPaths,
            sensors::node::sensors,
            /*efficientExpand=*/true);
        getChassisData(asyncResp);

        BMCWEB_LOG_DEBUG
            << "SensorCollection doGet exit via efficient expand handler";
        return;
    }

    // We get all sensors as hyperlinkes in the chassis (this
    // implies we reply on the default query parameters handler)
    getChassis(aResp, chassisId, sensors::node::sensors, dbus::sensorPaths,
               std::bind_front(sensors::getChassisCallback, aResp, chassisId,
                               sensors::node::sensors));
}

inline void
    getSensorFromDbus(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const std::string& sensorPath,
                      const ::dbus::utility::MapperGetObject& mapperResponse)
{
    if (mapperResponse.size() != 1)
    {
        messages::internalError(asyncResp->res);
        return;
    }
    const auto& valueIface = *mapperResponse.begin();
    const std::string& connectionName = valueIface.first;
    BMCWEB_LOG_DEBUG << "Looking up " << connectionName;
    BMCWEB_LOG_DEBUG << "Path " << sensorPath;

    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, connectionName, sensorPath, "",
        [asyncResp,
         sensorPath](const boost::system::error_code& ec,
                     const ::dbus::utility::DBusPropertiesMap& valuesDict) {
        if (ec)
        {
            messages::internalError(asyncResp->res);
            return;
        }
        sdbusplus::message::object_path path(sensorPath);
        std::string name = path.filename();
        path = path.parent_path();
        std::string type = path.filename();
        objectPropertiesToJson(name, type, sensors::node::sensors, valuesDict,
                               asyncResp->res.jsonValue, nullptr);
        });
}

inline void handleSensorGet(App& app, const crow::Request& req,
                            const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& chassisId,
                            const std::string& sensorId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    std::pair<std::string, std::string> nameType =
        splitSensorNameAndType(sensorId);
    if (nameType.first.empty() || nameType.second.empty())
    {
        messages::resourceNotFound(asyncResp->res, sensorId, "Sensor");
        return;
    }

    asyncResp->res.jsonValue["@odata.id"] = crow::utility::urlFromPieces(
        "redfish", "v1", "Chassis", chassisId, "Sensors", sensorId);

    BMCWEB_LOG_DEBUG << "Sensor doGet enter";

    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Sensor.Value"};
    std::string sensorPath = "/xyz/openbmc_project/sensors/" + nameType.first +
                             '/' + nameType.second;
    // Get a list of all of the sensors that implement Sensor.Value
    // and get the path and service name associated with the sensor
    ::dbus::utility::getDbusObject(
        sensorPath, interfaces,
        [asyncResp,
         sensorPath](const boost::system::error_code& ec,
                     const ::dbus::utility::MapperGetObject& subtree) {
        BMCWEB_LOG_DEBUG << "respHandler1 enter";
        if (ec)
        {
            messages::internalError(asyncResp->res);
            BMCWEB_LOG_ERROR << "Sensor getSensorPaths resp_handler: "
                             << "Dbus error " << ec;
            return;
        }
        getSensorFromDbus(asyncResp, sensorPath, subtree);
        BMCWEB_LOG_DEBUG << "respHandler1 exit";
        });
}

} // namespace sensors

inline void requestRoutesSensorCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/Sensors/")
        .privileges(redfish::privileges::getSensorCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(sensors::handleSensorCollectionGet, std::ref(app)));
}

inline void requestRoutesSensor(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/Sensors/<str>/")
        .privileges(redfish::privileges::getSensor)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(sensors::handleSensorGet, std::ref(app)));
}

} // namespace redfish
