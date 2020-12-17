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

#include "node.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/range/algorithm/replace_copy_if.hpp>
#include <dbus_singleton.hpp>
#include <utils/json_utils.hpp>

#include <cmath>
#include <utility>
#include <variant>

namespace redfish
{

using GetSubTreeType = std::vector<
    std::pair<std::string,
              std::vector<std::pair<std::string, std::vector<std::string>>>>>;

using SensorVariant =
    std::variant<int64_t, double, uint32_t, bool, std::string>;

using ManagedObjectsVectorType = std::vector<std::pair<
    sdbusplus::message::object_path,
    boost::container::flat_map<
        std::string, boost::container::flat_map<std::string, SensorVariant>>>>;

namespace sensors
{
namespace node
{
static constexpr std::string_view power = "Power";
static constexpr std::string_view sensors = "Sensors";
static constexpr std::string_view thermal = "Thermal";
} // namespace node

namespace dbus
{
static const boost::container::flat_map<std::string_view,
                                        std::vector<const char*>>
    types = {{node::power,
              {"/xyz/openbmc_project/sensors/voltage",
               "/xyz/openbmc_project/sensors/power"}},
             {node::sensors,
              {"/xyz/openbmc_project/sensors/power",
               "/xyz/openbmc_project/sensors/current",
               "/xyz/openbmc_project/sensors/utilization"}},
             {node::thermal,
              {"/xyz/openbmc_project/sensors/fan_tach",
               "/xyz/openbmc_project/sensors/temperature",
               "/xyz/openbmc_project/sensors/fan_pwm"}}};
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
        const boost::container::flat_map<std::string, std::string>& uriToDbus)>;

    struct SensorData
    {
        const std::string name;
        std::string uri;
        const std::string valueKey;
        const std::string dbusPath;
    };

    SensorsAsyncResp(crow::Response& response, const std::string& chassisIdIn,
                     const std::vector<const char*>& typesIn,
                     const std::string_view& subNode) :
        res(response),
        chassisId(chassisIdIn), types(typesIn), chassisSubNode(subNode)
    {}

    // Store extra data about sensor mapping and return it in callback
    SensorsAsyncResp(crow::Response& response, const std::string& chassisIdIn,
                     const std::vector<const char*>& typesIn,
                     const std::string_view& subNode,
                     DataCompleteCb&& creationComplete) :
        res(response),
        chassisId(chassisIdIn), types(typesIn),
        chassisSubNode(subNode), metadata{std::vector<SensorData>()},
        dataComplete{std::move(creationComplete)}
    {}

    ~SensorsAsyncResp()
    {
        if (res.result() == boost::beast::http::status::internal_server_error)
        {
            // Reset the json object to clear out any data that made it in
            // before the error happened todo(ed) handle error condition with
            // proper code
            res.jsonValue = nlohmann::json::object();
        }

        if (dataComplete && metadata)
        {
            boost::container::flat_map<std::string, std::string> map;
            if (res.result() == boost::beast::http::status::ok)
            {
                for (auto& sensor : *metadata)
                {
                    map.insert(std::make_pair(sensor.uri + sensor.valueKey,
                                              sensor.dbusPath));
                }
            }
            dataComplete(res.result(), map);
        }

        res.end();
    }

    void addMetadata(const nlohmann::json& sensorObject,
                     const std::string& valueKey, const std::string& dbusPath)
    {
        if (metadata)
        {
            metadata->emplace_back(SensorData{sensorObject["Name"],
                                              sensorObject["@odata.id"],
                                              valueKey, dbusPath});
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

    crow::Response& res;
    const std::string chassisId;
    const std::vector<const char*> types;
    const std::string chassisSubNode;

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
    InventoryItem(const std::string& objPath) :
        objectPath(objPath), name(), isPresent(true), isFunctional(true),
        isPowerSupply(false), powerSupplyEfficiencyPercent(-1), manufacturer(),
        model(), partNumber(), serialNumber(), sensors(), ledObjectPath(""),
        ledState(LedState::UNKNOWN)
    {
        // Set inventory item name to last node of object path
        auto pos = objectPath.rfind('/');
        if ((pos != std::string::npos) && ((pos + 1) < objectPath.size()))
        {
            name = objectPath.substr(pos + 1);
        }
    }

    std::string objectPath;
    std::string name;
    bool isPresent;
    bool isFunctional;
    bool isPowerSupply;
    int powerSupplyEfficiencyPercent;
    std::string manufacturer;
    std::string model;
    std::string partNumber;
    std::string serialNumber;
    std::set<std::string> sensors;
    std::string ledObjectPath;
    LedState ledState;
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
    const std::shared_ptr<boost::container::flat_set<std::string>>& sensorNames,
    Callback&& callback)
{
    BMCWEB_LOG_DEBUG << "getObjectsWithConnection enter";
    const std::string path = "/xyz/openbmc_project/sensors";
    const std::array<std::string, 1> interfaces = {
        "xyz.openbmc_project.Sensor.Value"};

    // Response handler for parsing objects subtree
    auto respHandler = [callback{std::move(callback)}, sensorsAsyncResp,
                        sensorNames](const boost::system::error_code ec,
                                     const GetSubTreeType& subtree) {
        BMCWEB_LOG_DEBUG << "getObjectsWithConnection resp_handler enter";
        if (ec)
        {
            messages::internalError(sensorsAsyncResp->res);
            BMCWEB_LOG_ERROR
                << "getObjectsWithConnection resp_handler: Dbus error " << ec;
            return;
        }

        BMCWEB_LOG_DEBUG << "Found " << subtree.size() << " subtrees";

        // Make unique list of connections only for requested sensor types and
        // found in the chassis
        boost::container::flat_set<std::string> connections;
        std::set<std::pair<std::string, std::string>> objectsWithConnection;
        // Intrinsic to avoid malloc.  Most systems will have < 8 sensor
        // producers
        connections.reserve(8);

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
    };
    // Make call to ObjectMapper to find all sensors objects
    crow::connections::systemBus->async_method_call(
        std::move(respHandler), "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree", path, 2, interfaces);
    BMCWEB_LOG_DEBUG << "getObjectsWithConnection exit";
}

/**
 * @brief Create connections necessary for sensors
 * @param SensorsAsyncResp Pointer to object holding response data
 * @param sensorNames Sensors retrieved from chassis
 * @param callback Callback for processing gathered connections
 */
template <typename Callback>
void getConnections(
    std::shared_ptr<SensorsAsyncResp> sensorsAsyncResp,
    const std::shared_ptr<boost::container::flat_set<std::string>> sensorNames,
    Callback&& callback)
{
    auto objectsWithConnectionCb =
        [callback](const boost::container::flat_set<std::string>& connections,
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
    const std::shared_ptr<SensorsAsyncResp>& sensorsAsyncResp,
    const std::vector<std::string>* allSensors,
    const std::shared_ptr<boost::container::flat_set<std::string>>&
        activeSensors)
{
    if (sensorsAsyncResp == nullptr)
    {
        return;
    }
    if ((allSensors == nullptr) || (activeSensors == nullptr))
    {
        messages::resourceNotFound(
            sensorsAsyncResp->res, sensorsAsyncResp->chassisSubNode,
            sensorsAsyncResp->chassisSubNode == sensors::node::thermal
                ? "Temperatures"
                : "Voltages");

        return;
    }
    if (allSensors->empty())
    {
        // Nothing to do, the activeSensors object is also empty
        return;
    }

    for (const char* type : sensorsAsyncResp->types)
    {
        for (const std::string& sensor : *allSensors)
        {
            if (boost::starts_with(sensor, type))
            {
                activeSensors->emplace(sensor);
            }
        }
    }
}

/**
 * @brief Retrieves valid chassis path
 * @param asyncResp   Pointer to object holding response data
 * @param callback  Callback for next step to get valid chassis path
 */
template <typename Callback>
void getValidChassisPath(const std::shared_ptr<SensorsAsyncResp>& asyncResp,
                         Callback&& callback)
{
    BMCWEB_LOG_DEBUG << "checkChassisId enter";
    const std::array<const char*, 2> interfaces = {
        "xyz.openbmc_project.Inventory.Item.Board",
        "xyz.openbmc_project.Inventory.Item.Chassis"};

    auto respHandler =
        [callback{std::move(callback)},
         asyncResp](const boost::system::error_code ec,
                    const std::vector<std::string>& chassisPaths) mutable {
            BMCWEB_LOG_DEBUG << "getValidChassisPath respHandler enter";
            if (ec)
            {
                BMCWEB_LOG_ERROR
                    << "getValidChassisPath respHandler DBUS error: " << ec;
                messages::internalError(asyncResp->res);
                return;
            }

            std::optional<std::string> chassisPath;
            std::string chassisName;
            for (const std::string& chassis : chassisPaths)
            {
                std::size_t lastPos = chassis.rfind('/');
                if (lastPos == std::string::npos)
                {
                    BMCWEB_LOG_ERROR << "Failed to find '/' in " << chassis;
                    continue;
                }
                chassisName = chassis.substr(lastPos + 1);
                if (chassisName == asyncResp->chassisId)
                {
                    chassisPath = chassis;
                    break;
                }
            }
            callback(chassisPath);
        };

    // Get the Chassis Collection
    crow::connections::systemBus->async_method_call(
        respHandler, "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
        "/xyz/openbmc_project/inventory", 0, interfaces);
    BMCWEB_LOG_DEBUG << "checkChassisId exit";
}

/**
 * @brief Retrieves requested chassis sensors and redundancy data from DBus .
 * @param SensorsAsyncResp   Pointer to object holding response data
 * @param callback  Callback for next step in gathered sensor processing
 */
template <typename Callback>
void getChassis(const std::shared_ptr<SensorsAsyncResp>& sensorsAsyncResp,
                Callback&& callback)
{
    BMCWEB_LOG_DEBUG << "getChassis enter";
    const std::array<const char*, 2> interfaces = {
        "xyz.openbmc_project.Inventory.Item.Board",
        "xyz.openbmc_project.Inventory.Item.Chassis"};
    auto respHandler = [callback{std::move(callback)}, sensorsAsyncResp](
                           const boost::system::error_code ec,
                           const std::vector<std::string>& chassisPaths) {
        BMCWEB_LOG_DEBUG << "getChassis respHandler enter";
        if (ec)
        {
            BMCWEB_LOG_ERROR << "getChassis respHandler DBUS error: " << ec;
            messages::internalError(sensorsAsyncResp->res);
            return;
        }

        const std::string* chassisPath = nullptr;
        std::string chassisName;
        for (const std::string& chassis : chassisPaths)
        {
            std::size_t lastPos = chassis.rfind('/');
            if (lastPos == std::string::npos)
            {
                BMCWEB_LOG_ERROR << "Failed to find '/' in " << chassis;
                continue;
            }
            chassisName = chassis.substr(lastPos + 1);
            if (chassisName == sensorsAsyncResp->chassisId)
            {
                chassisPath = &chassis;
                break;
            }
        }
        if (chassisPath == nullptr)
        {
            messages::resourceNotFound(sensorsAsyncResp->res, "Chassis",
                                       sensorsAsyncResp->chassisId);
            return;
        }

        const std::string& chassisSubNode = sensorsAsyncResp->chassisSubNode;
        if (chassisSubNode == sensors::node::power)
        {
            sensorsAsyncResp->res.jsonValue["@odata.type"] =
                "#Power.v1_5_2.Power";
        }
        else if (chassisSubNode == sensors::node::thermal)
        {
            sensorsAsyncResp->res.jsonValue["@odata.type"] =
                "#Thermal.v1_4_0.Thermal";
            sensorsAsyncResp->res.jsonValue["Fans"] = nlohmann::json::array();
            sensorsAsyncResp->res.jsonValue["Temperatures"] =
                nlohmann::json::array();
        }
        else if (chassisSubNode == sensors::node::sensors)
        {
            sensorsAsyncResp->res.jsonValue["@odata.type"] =
                "#SensorCollection.SensorCollection";
            sensorsAsyncResp->res.jsonValue["Description"] =
                "Collection of Sensors for this Chassis";
            sensorsAsyncResp->res.jsonValue["Members"] =
                nlohmann::json::array();
            sensorsAsyncResp->res.jsonValue["Members@odata.count"] = 0;
        }

        if (chassisSubNode != sensors::node::sensors)
        {
            sensorsAsyncResp->res.jsonValue["Id"] = chassisSubNode;
        }

        sensorsAsyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Chassis/" + sensorsAsyncResp->chassisId + "/" +
            chassisSubNode;
        sensorsAsyncResp->res.jsonValue["Name"] = chassisSubNode;

        // Get the list of all sensors for this Chassis element
        std::string sensorPath = *chassisPath + "/all_sensors";
        crow::connections::systemBus->async_method_call(
            [sensorsAsyncResp, callback{std::move(callback)}](
                const boost::system::error_code& e,
                const std::variant<std::vector<std::string>>&
                    variantEndpoints) {
                if (e)
                {
                    if (e.value() != EBADR)
                    {
                        messages::internalError(sensorsAsyncResp->res);
                        return;
                    }
                }
                const std::vector<std::string>* nodeSensorList =
                    std::get_if<std::vector<std::string>>(&(variantEndpoints));
                if (nodeSensorList == nullptr)
                {
                    messages::resourceNotFound(
                        sensorsAsyncResp->res, sensorsAsyncResp->chassisSubNode,
                        sensorsAsyncResp->chassisSubNode ==
                                sensors::node::thermal
                            ? "Temperatures"
                            : sensorsAsyncResp->chassisSubNode ==
                                      sensors::node::power
                                  ? "Voltages"
                                  : "Sensors");
                    return;
                }
                const std::shared_ptr<boost::container::flat_set<std::string>>
                    culledSensorList = std::make_shared<
                        boost::container::flat_set<std::string>>();
                reduceSensorList(sensorsAsyncResp, nodeSensorList,
                                 culledSensorList);
                callback(culledSensorList);
            },
            "xyz.openbmc_project.ObjectMapper", sensorPath,
            "org.freedesktop.DBus.Properties", "Get",
            "xyz.openbmc_project.Association", "endpoints");
    };

    // Get the Chassis Collection
    crow::connections::systemBus->async_method_call(
        respHandler, "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
        "/xyz/openbmc_project/inventory", 0, interfaces);
    BMCWEB_LOG_DEBUG << "getChassis exit";
}

/**
 * @brief Finds all DBus object paths that implement ObjectManager.
 *
 * Creates a mapping from the associated connection name to the object path.
 *
 * Finds the object paths asynchronously.  Invokes callback when information has
 * been obtained.
 *
 * The callback must have the following signature:
 *   @code
 *   callback(std::shared_ptr<boost::container::flat_map<std::string,
 *                std::string>> objectMgrPaths)
 *   @endcode
 *
 * @param sensorsAsyncResp Pointer to object holding response data.
 * @param callback Callback to invoke when object paths obtained.
 */
template <typename Callback>
void getObjectManagerPaths(
    const std::shared_ptr<SensorsAsyncResp>& sensorsAsyncResp,
    Callback&& callback)
{
    BMCWEB_LOG_DEBUG << "getObjectManagerPaths enter";
    const std::array<std::string, 1> interfaces = {
        "org.freedesktop.DBus.ObjectManager"};

    // Response handler for GetSubTree DBus method
    auto respHandler = [callback{std::move(callback)},
                        sensorsAsyncResp](const boost::system::error_code ec,
                                          const GetSubTreeType& subtree) {
        BMCWEB_LOG_DEBUG << "getObjectManagerPaths respHandler enter";
        if (ec)
        {
            messages::internalError(sensorsAsyncResp->res);
            BMCWEB_LOG_ERROR << "getObjectManagerPaths respHandler: DBus error "
                             << ec;
            return;
        }

        // Loop over returned object paths
        std::shared_ptr<boost::container::flat_map<std::string, std::string>>
            objectMgrPaths = std::make_shared<
                boost::container::flat_map<std::string, std::string>>();
        for (const std::pair<
                 std::string,
                 std::vector<std::pair<std::string, std::vector<std::string>>>>&
                 object : subtree)
        {
            // Loop over connections for current object path
            const std::string& objectPath = object.first;
            for (const std::pair<std::string, std::vector<std::string>>&
                     objData : object.second)
            {
                // Add mapping from connection to object path
                const std::string& connection = objData.first;
                (*objectMgrPaths)[connection] = objectPath;
                BMCWEB_LOG_DEBUG << "Added mapping " << connection << " -> "
                                 << objectPath;
            }
        }
        callback(objectMgrPaths);
        BMCWEB_LOG_DEBUG << "getObjectManagerPaths respHandler exit";
    };

    // Query mapper for all DBus object paths that implement ObjectManager
    crow::connections::systemBus->async_method_call(
        std::move(respHandler), "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree", "/", 0, interfaces);
    BMCWEB_LOG_DEBUG << "getObjectManagerPaths exit";
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
 * @param interfacesDict Map of all sensor interfaces.
 * @param inventoryItem D-Bus inventory item associated with the sensor.  Will
 * be nullptr if no associated inventory item was found.
 * @return Health value for sensor.
 */
inline std::string getHealth(
    nlohmann::json& sensorJson,
    const boost::container::flat_map<
        std::string, boost::container::flat_map<std::string, SensorVariant>>&
        interfacesDict,
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

    // If current health in JSON object is already Critical, return that.  This
    // should override the sensor health, which might be less severe.
    if (currentHealth == "Critical")
    {
        return "Critical";
    }

    // Check if sensor has critical threshold alarm
    auto criticalThresholdIt =
        interfacesDict.find("xyz.openbmc_project.Sensor.Threshold.Critical");
    if (criticalThresholdIt != interfacesDict.end())
    {
        auto thresholdHighIt =
            criticalThresholdIt->second.find("CriticalAlarmHigh");
        auto thresholdLowIt =
            criticalThresholdIt->second.find("CriticalAlarmLow");
        if (thresholdHighIt != criticalThresholdIt->second.end())
        {
            const bool* asserted = std::get_if<bool>(&thresholdHighIt->second);
            if (asserted == nullptr)
            {
                BMCWEB_LOG_ERROR << "Illegal sensor threshold";
            }
            else if (*asserted)
            {
                return "Critical";
            }
        }
        if (thresholdLowIt != criticalThresholdIt->second.end())
        {
            const bool* asserted = std::get_if<bool>(&thresholdLowIt->second);
            if (asserted == nullptr)
            {
                BMCWEB_LOG_ERROR << "Illegal sensor threshold";
            }
            else if (*asserted)
            {
                return "Critical";
            }
        }
    }

    // Check if associated inventory item is not functional
    if ((inventoryItem != nullptr) && !(inventoryItem->isFunctional))
    {
        return "Critical";
    }

    // If current health in JSON object is already Warning, return that.  This
    // should override the sensor status, which might be less severe.
    if (currentHealth == "Warning")
    {
        return "Warning";
    }

    // Check if sensor has warning threshold alarm
    auto warningThresholdIt =
        interfacesDict.find("xyz.openbmc_project.Sensor.Threshold.Warning");
    if (warningThresholdIt != interfacesDict.end())
    {
        auto thresholdHighIt =
            warningThresholdIt->second.find("WarningAlarmHigh");
        auto thresholdLowIt =
            warningThresholdIt->second.find("WarningAlarmLow");
        if (thresholdHighIt != warningThresholdIt->second.end())
        {
            const bool* asserted = std::get_if<bool>(&thresholdHighIt->second);
            if (asserted == nullptr)
            {
                BMCWEB_LOG_ERROR << "Illegal sensor threshold";
            }
            else if (*asserted)
            {
                return "Warning";
            }
        }
        if (thresholdLowIt != warningThresholdIt->second.end())
        {
            const bool* asserted = std::get_if<bool>(&thresholdLowIt->second);
            if (asserted == nullptr)
            {
                BMCWEB_LOG_ERROR << "Illegal sensor threshold";
            }
            else if (*asserted)
            {
                return "Warning";
            }
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
 * @param sensorsAsyncResp  Sensor metadata
 * @param interfacesDict  A dictionary of the interfaces and properties of said
 * interfaces to be built from
 * @param sensor_json  The json object to fill
 * @param inventoryItem D-Bus inventory item associated with the sensor.  Will
 * be nullptr if no associated inventory item was found.
 */
inline void objectInterfacesToJson(
    const std::string& sensorName, const std::string& sensorType,
    const std::shared_ptr<SensorsAsyncResp>& sensorsAsyncResp,
    const boost::container::flat_map<
        std::string, boost::container::flat_map<std::string, SensorVariant>>&
        interfacesDict,
    nlohmann::json& sensorJson, InventoryItem* inventoryItem)
{
    // We need a value interface before we can do anything with it
    auto valueIt = interfacesDict.find("xyz.openbmc_project.Sensor.Value");
    if (valueIt == interfacesDict.end())
    {
        BMCWEB_LOG_ERROR << "Sensor doesn't have a value interface";
        return;
    }

    // Assume values exist as is (10^0 == 1) if no scale exists
    int64_t scaleMultiplier = 0;

    auto scaleIt = valueIt->second.find("Scale");
    // If a scale exists, pull value as int64, and use the scaling.
    if (scaleIt != valueIt->second.end())
    {
        const int64_t* int64Value = std::get_if<int64_t>(&scaleIt->second);
        if (int64Value != nullptr)
        {
            scaleMultiplier = *int64Value;
        }
    }

    if (sensorsAsyncResp->chassisSubNode == sensors::node::sensors)
    {
        // For sensors in SensorCollection we set Id instead of MemberId,
        // including power sensors.
        sensorJson["Id"] = sensorName;
        sensorJson["Name"] = boost::replace_all_copy(sensorName, "_", " ");
    }
    else if (sensorType != "power")
    {
        // Set MemberId and Name for non-power sensors.  For PowerSupplies and
        // PowerControl, those properties have more general values because
        // multiple sensors can be stored in the same JSON object.
        sensorJson["MemberId"] = sensorName;
        sensorJson["Name"] = boost::replace_all_copy(sensorName, "_", " ");
    }

    sensorJson["Status"]["State"] = getState(inventoryItem);
    sensorJson["Status"]["Health"] =
        getHealth(sensorJson, interfacesDict, inventoryItem);

    // Parameter to set to override the type we get from dbus, and force it to
    // int, regardless of what is available.  This is used for schemas like fan,
    // that require integers, not floats.
    bool forceToInt = false;

    nlohmann::json::json_pointer unit("/Reading");
    if (sensorsAsyncResp->chassisSubNode == sensors::node::sensors)
    {
        sensorJson["@odata.type"] = "#Sensor.v1_0_0.Sensor";
        if (sensorType == "power")
        {
            sensorJson["ReadingUnits"] = "Watts";
        }
        else if (sensorType == "current")
        {
            sensorJson["ReadingUnits"] = "Amperes";
        }
        else if (sensorType == "utilization")
        {
            sensorJson["ReadingUnits"] = "Percent";
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
        std::string sensorNameLower =
            boost::algorithm::to_lower_copy(sensorName);

        if (!sensorName.compare("total_power"))
        {
            sensorJson["@odata.type"] = "#Power.v1_0_0.PowerControl";
            // Put multiple "sensors" into a single PowerControl, so have
            // generic names for MemberId and Name. Follows Redfish mockup.
            sensorJson["MemberId"] = "0";
            sensorJson["Name"] = "Chassis Power Control";
            unit = "/PowerConsumedWatts"_json_pointer;
        }
        else if (sensorNameLower.find("input") != std::string::npos)
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

    if (sensorsAsyncResp->chassisSubNode == sensors::node::sensors)
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

    if (sensorsAsyncResp->chassisSubNode == sensors::node::sensors)
    {
        properties.emplace_back("xyz.openbmc_project.Sensor.Value", "MinValue",
                                "/ReadingRangeMin"_json_pointer);
        properties.emplace_back("xyz.openbmc_project.Sensor.Value", "MaxValue",
                                "/ReadingRangeMax"_json_pointer);
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
        auto interfaceProperties = interfacesDict.find(std::get<0>(p));
        if (interfaceProperties != interfacesDict.end())
        {
            auto thisValueIt = interfaceProperties->second.find(std::get<1>(p));
            if (thisValueIt != interfaceProperties->second.end())
            {
                const SensorVariant& valueVariant = thisValueIt->second;

                // The property we want to set may be nested json, so use
                // a json_pointer for easy indexing into the json structure.
                const nlohmann::json::json_pointer& key = std::get<2>(p);

                // Attempt to pull the int64 directly
                const int64_t* int64Value = std::get_if<int64_t>(&valueVariant);

                const double* doubleValue = std::get_if<double>(&valueVariant);
                const uint32_t* uValue = std::get_if<uint32_t>(&valueVariant);
                double temp = 0.0;
                if (int64Value != nullptr)
                {
                    temp = static_cast<double>(*int64Value);
                }
                else if (doubleValue != nullptr)
                {
                    temp = *doubleValue;
                }
                else if (uValue != nullptr)
                {
                    temp = *uValue;
                }
                else
                {
                    BMCWEB_LOG_ERROR
                        << "Got value interface that wasn't int or double";
                    continue;
                }
                temp = temp * std::pow(10, scaleMultiplier);
                if (forceToInt)
                {
                    sensorJson[key] = static_cast<int64_t>(temp);
                }
                else
                {
                    sensorJson[key] = temp;
                }
            }
        }
    }

    sensorsAsyncResp->addMetadata(sensorJson, unit.to_string(),
                                  "/xyz/openbmc_project/sensors/" + sensorType +
                                      "/" + sensorName);

    BMCWEB_LOG_DEBUG << "Added sensor " << sensorName;
}

inline void populateFanRedundancy(
    const std::shared_ptr<SensorsAsyncResp>& sensorsAsyncResp)
{
    crow::connections::systemBus->async_method_call(
        [sensorsAsyncResp](const boost::system::error_code ec,
                           const GetSubTreeType& resp) {
            if (ec)
            {
                return; // don't have to have this interface
            }
            for (const std::pair<std::string,
                                 std::vector<std::pair<
                                     std::string, std::vector<std::string>>>>&
                     pathPair : resp)
            {
                const std::string& path = pathPair.first;
                const std::vector<
                    std::pair<std::string, std::vector<std::string>>>& objDict =
                    pathPair.second;
                if (objDict.empty())
                {
                    continue; // this should be impossible
                }

                const std::string& owner = objDict.begin()->first;
                crow::connections::systemBus->async_method_call(
                    [path, owner,
                     sensorsAsyncResp](const boost::system::error_code e,
                                       std::variant<std::vector<std::string>>
                                           variantEndpoints) {
                        if (e)
                        {
                            return; // if they don't have an association we
                                    // can't tell what chassis is
                        }
                        // verify part of the right chassis
                        auto endpoints = std::get_if<std::vector<std::string>>(
                            &variantEndpoints);

                        if (endpoints == nullptr)
                        {
                            BMCWEB_LOG_ERROR << "Invalid association interface";
                            messages::internalError(sensorsAsyncResp->res);
                            return;
                        }

                        auto found = std::find_if(
                            endpoints->begin(), endpoints->end(),
                            [sensorsAsyncResp](const std::string& entry) {
                                return entry.find(
                                           sensorsAsyncResp->chassisId) !=
                                       std::string::npos;
                            });

                        if (found == endpoints->end())
                        {
                            return;
                        }
                        crow::connections::systemBus->async_method_call(
                            [path, sensorsAsyncResp](
                                const boost::system::error_code& err,
                                const boost::container::flat_map<
                                    std::string,
                                    std::variant<uint8_t,
                                                 std::vector<std::string>,
                                                 std::string>>& ret) {
                                if (err)
                                {
                                    return; // don't have to have this
                                            // interface
                                }
                                auto findFailures = ret.find("AllowedFailures");
                                auto findCollection = ret.find("Collection");
                                auto findStatus = ret.find("Status");

                                if (findFailures == ret.end() ||
                                    findCollection == ret.end() ||
                                    findStatus == ret.end())
                                {
                                    BMCWEB_LOG_ERROR
                                        << "Invalid redundancy interface";
                                    messages::internalError(
                                        sensorsAsyncResp->res);
                                    return;
                                }

                                auto allowedFailures = std::get_if<uint8_t>(
                                    &(findFailures->second));
                                auto collection =
                                    std::get_if<std::vector<std::string>>(
                                        &(findCollection->second));
                                auto status = std::get_if<std::string>(
                                    &(findStatus->second));

                                if (allowedFailures == nullptr ||
                                    collection == nullptr || status == nullptr)
                                {

                                    BMCWEB_LOG_ERROR
                                        << "Invalid redundancy interface "
                                           "types";
                                    messages::internalError(
                                        sensorsAsyncResp->res);
                                    return;
                                }
                                size_t lastSlash = path.rfind('/');
                                if (lastSlash == std::string::npos)
                                {
                                    // this should be impossible
                                    messages::internalError(
                                        sensorsAsyncResp->res);
                                    return;
                                }
                                std::string name = path.substr(lastSlash + 1);
                                std::replace(name.begin(), name.end(), '_',
                                             ' ');

                                std::string health;

                                if (boost::ends_with(*status, "Full"))
                                {
                                    health = "OK";
                                }
                                else if (boost::ends_with(*status, "Degraded"))
                                {
                                    health = "Warning";
                                }
                                else
                                {
                                    health = "Critical";
                                }
                                std::vector<nlohmann::json> redfishCollection;
                                const auto& fanRedfish =
                                    sensorsAsyncResp->res.jsonValue["Fans"];
                                for (const std::string& item : *collection)
                                {
                                    lastSlash = item.rfind('/');
                                    // make a copy as collection is const
                                    std::string itemName =
                                        item.substr(lastSlash + 1);
                                    /*
                                    todo(ed): merge patch that fixes the names
                                    std::replace(itemName.begin(),
                                                 itemName.end(), '_', ' ');*/
                                    auto schemaItem = std::find_if(
                                        fanRedfish.begin(), fanRedfish.end(),
                                        [itemName](const nlohmann::json& fan) {
                                            return fan["MemberId"] == itemName;
                                        });
                                    if (schemaItem != fanRedfish.end())
                                    {
                                        redfishCollection.push_back(
                                            {{"@odata.id",
                                              (*schemaItem)["@odata.id"]}});
                                    }
                                    else
                                    {
                                        BMCWEB_LOG_ERROR
                                            << "failed to find fan in schema";
                                        messages::internalError(
                                            sensorsAsyncResp->res);
                                        return;
                                    }
                                }

                                size_t minNumNeeded =
                                    collection->size() > 0
                                        ? collection->size() - *allowedFailures
                                        : 0;
                                nlohmann::json& jResp =
                                    sensorsAsyncResp->res
                                        .jsonValue["Redundancy"];
                                jResp.push_back(
                                    {{"@odata.id",
                                      "/redfish/v1/Chassis/" +
                                          sensorsAsyncResp->chassisId + "/" +
                                          sensorsAsyncResp->chassisSubNode +
                                          "#/Redundancy/" +
                                          std::to_string(jResp.size())},
                                     {"@odata.type",
                                      "#Redundancy.v1_3_2.Redundancy"},
                                     {"MinNumNeeded", minNumNeeded},
                                     {"MemberId", name},
                                     {"Mode", "N+m"},
                                     {"Name", name},
                                     {"RedundancySet", redfishCollection},
                                     {"Status",
                                      {{"Health", health},
                                       {"State", "Enabled"}}}});
                            },
                            owner, path, "org.freedesktop.DBus.Properties",
                            "GetAll",
                            "xyz.openbmc_project.Control.FanRedundancy");
                    },
                    "xyz.openbmc_project.ObjectMapper", path + "/chassis",
                    "org.freedesktop.DBus.Properties", "Get",
                    "xyz.openbmc_project.Association", "endpoints");
            }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/control", 2,
        std::array<const char*, 1>{
            "xyz.openbmc_project.Control.FanRedundancy"});
}

inline void
    sortJSONResponse(const std::shared_ptr<SensorsAsyncResp>& sensorsAsyncResp)
{
    nlohmann::json& response = sensorsAsyncResp->res.jsonValue;
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
                      [](nlohmann::json& c1, nlohmann::json& c2) {
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
                    *value += std::to_string(count);
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
    const boost::container::flat_map<
        std::string, boost::container::flat_map<std::string, SensorVariant>>&
        interfacesDict)
{
    // Get properties from Inventory.Item interface
    auto interfaceIt =
        interfacesDict.find("xyz.openbmc_project.Inventory.Item");
    if (interfaceIt != interfacesDict.end())
    {
        auto propertyIt = interfaceIt->second.find("Present");
        if (propertyIt != interfaceIt->second.end())
        {
            const bool* value = std::get_if<bool>(&propertyIt->second);
            if (value != nullptr)
            {
                inventoryItem.isPresent = *value;
            }
        }
    }

    // Check if Inventory.Item.PowerSupply interface is present
    interfaceIt =
        interfacesDict.find("xyz.openbmc_project.Inventory.Item.PowerSupply");
    if (interfaceIt != interfacesDict.end())
    {
        inventoryItem.isPowerSupply = true;
    }

    // Get properties from Inventory.Decorator.Asset interface
    interfaceIt =
        interfacesDict.find("xyz.openbmc_project.Inventory.Decorator.Asset");
    if (interfaceIt != interfacesDict.end())
    {
        auto propertyIt = interfaceIt->second.find("Manufacturer");
        if (propertyIt != interfaceIt->second.end())
        {
            const std::string* value =
                std::get_if<std::string>(&propertyIt->second);
            if (value != nullptr)
            {
                inventoryItem.manufacturer = *value;
            }
        }

        propertyIt = interfaceIt->second.find("Model");
        if (propertyIt != interfaceIt->second.end())
        {
            const std::string* value =
                std::get_if<std::string>(&propertyIt->second);
            if (value != nullptr)
            {
                inventoryItem.model = *value;
            }
        }

        propertyIt = interfaceIt->second.find("PartNumber");
        if (propertyIt != interfaceIt->second.end())
        {
            const std::string* value =
                std::get_if<std::string>(&propertyIt->second);
            if (value != nullptr)
            {
                inventoryItem.partNumber = *value;
            }
        }

        propertyIt = interfaceIt->second.find("SerialNumber");
        if (propertyIt != interfaceIt->second.end())
        {
            const std::string* value =
                std::get_if<std::string>(&propertyIt->second);
            if (value != nullptr)
            {
                inventoryItem.serialNumber = *value;
            }
        }
    }

    // Get properties from State.Decorator.OperationalStatus interface
    interfaceIt = interfacesDict.find(
        "xyz.openbmc_project.State.Decorator.OperationalStatus");
    if (interfaceIt != interfacesDict.end())
    {
        auto propertyIt = interfaceIt->second.find("Functional");
        if (propertyIt != interfaceIt->second.end())
        {
            const bool* value = std::get_if<bool>(&propertyIt->second);
            if (value != nullptr)
            {
                inventoryItem.isFunctional = *value;
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
 * @param objectMgrPaths Mappings from connection name to DBus object path that
 * implements ObjectManager.
 * @param callback Callback to invoke when inventory data has been obtained.
 * @param invConnectionsIndex Current index in invConnections.  Only specified
 * in recursive calls to this function.
 */
template <typename Callback>
static void getInventoryItemsData(
    std::shared_ptr<SensorsAsyncResp> sensorsAsyncResp,
    std::shared_ptr<std::vector<InventoryItem>> inventoryItems,
    std::shared_ptr<boost::container::flat_set<std::string>> invConnections,
    std::shared_ptr<boost::container::flat_map<std::string, std::string>>
        objectMgrPaths,
    Callback&& callback, size_t invConnectionsIndex = 0)
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
    auto it = invConnections->nth(invConnectionsIndex);
    if (it != invConnections->end())
    {
        const std::string& invConnection = *it;

        // Response handler for GetManagedObjects
        auto respHandler = [sensorsAsyncResp, inventoryItems, invConnections,
                            objectMgrPaths, callback{std::move(callback)},
                            invConnectionsIndex](
                               const boost::system::error_code ec,
                               ManagedObjectsVectorType& resp) {
            BMCWEB_LOG_DEBUG << "getInventoryItemsData respHandler enter";
            if (ec)
            {
                BMCWEB_LOG_ERROR
                    << "getInventoryItemsData respHandler DBus error " << ec;
                messages::internalError(sensorsAsyncResp->res);
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
                                  invConnections, objectMgrPaths,
                                  std::move(callback), invConnectionsIndex + 1);

            BMCWEB_LOG_DEBUG << "getInventoryItemsData respHandler exit";
        };

        // Find DBus object path that implements ObjectManager for the current
        // connection.  If no mapping found, default to "/".
        auto iter = objectMgrPaths->find(invConnection);
        const std::string& objectMgrPath =
            (iter != objectMgrPaths->end()) ? iter->second : "/";
        BMCWEB_LOG_DEBUG << "ObjectManager path for " << invConnection << " is "
                         << objectMgrPath;

        // Get all object paths and their interfaces for current connection
        crow::connections::systemBus->async_method_call(
            std::move(respHandler), invConnection, objectMgrPath,
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
 *   callback(std::shared_ptr<boost::container::flat_set<std::string>>
 *            invConnections)
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
    const std::array<std::string, 4> interfaces = {
        "xyz.openbmc_project.Inventory.Item",
        "xyz.openbmc_project.Inventory.Item.PowerSupply",
        "xyz.openbmc_project.Inventory.Decorator.Asset",
        "xyz.openbmc_project.State.Decorator.OperationalStatus"};

    // Response handler for parsing output from GetSubTree
    auto respHandler = [callback{std::move(callback)}, sensorsAsyncResp,
                        inventoryItems](const boost::system::error_code ec,
                                        const GetSubTreeType& subtree) {
        BMCWEB_LOG_DEBUG << "getInventoryItemsConnections respHandler enter";
        if (ec)
        {
            messages::internalError(sensorsAsyncResp->res);
            BMCWEB_LOG_ERROR
                << "getInventoryItemsConnections respHandler DBus error " << ec;
            return;
        }

        // Make unique list of connections for desired inventory items
        std::shared_ptr<boost::container::flat_set<std::string>>
            invConnections =
                std::make_shared<boost::container::flat_set<std::string>>();
        invConnections->reserve(8);

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
    };

    // Make call to ObjectMapper to find all inventory items
    crow::connections::systemBus->async_method_call(
        std::move(respHandler), "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree", path, 0, interfaces);
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
 * @param objectMgrPaths Mappings from connection name to DBus object path that
 * implements ObjectManager.
 * @param callback Callback to invoke when inventory items have been obtained.
 */
template <typename Callback>
static void getInventoryItemAssociations(
    const std::shared_ptr<SensorsAsyncResp>& sensorsAsyncResp,
    const std::shared_ptr<boost::container::flat_set<std::string>>& sensorNames,
    const std::shared_ptr<boost::container::flat_map<std::string, std::string>>&
        objectMgrPaths,
    Callback&& callback)
{
    BMCWEB_LOG_DEBUG << "getInventoryItemAssociations enter";

    // Response handler for GetManagedObjects
    auto respHandler = [callback{std::move(callback)}, sensorsAsyncResp,
                        sensorNames](const boost::system::error_code ec,
                                     dbus::utility::ManagedObjectType& resp) {
        BMCWEB_LOG_DEBUG << "getInventoryItemAssociations respHandler enter";
        if (ec)
        {
            BMCWEB_LOG_ERROR
                << "getInventoryItemAssociations respHandler DBus error " << ec;
            messages::internalError(sensorsAsyncResp->res);
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
            const boost::container::flat_map<
                std::string, boost::container::flat_map<
                                 std::string, dbus::utility::DbusVariantType>>&
                interfacesDict = objDictEntry.second;

            // If path is inventory association for one of the specified sensors
            for (const std::string& sensorName : *sensorNames)
            {
                sensorAssocPath = sensorName;
                sensorAssocPath += "/inventory";
                if (objPath == sensorAssocPath)
                {
                    // Get Association interface for object path
                    auto assocIt =
                        interfacesDict.find("xyz.openbmc_project.Association");
                    if (assocIt != interfacesDict.end())
                    {
                        // Get inventory item from end point
                        auto endpointsIt = assocIt->second.find("endpoints");
                        if (endpointsIt != assocIt->second.end())
                        {
                            const std::vector<std::string>* endpoints =
                                std::get_if<std::vector<std::string>>(
                                    &endpointsIt->second);
                            if ((endpoints != nullptr) && !endpoints->empty())
                            {
                                // Add inventory item to vector
                                const std::string& invItemPath =
                                    endpoints->front();
                                addInventoryItem(inventoryItems, invItemPath,
                                                 sensorName);
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
            const boost::container::flat_map<
                std::string, boost::container::flat_map<
                                 std::string, dbus::utility::DbusVariantType>>&
                interfacesDict = objDictEntry.second;

            for (InventoryItem& inventoryItem : *inventoryItems)
            {
                inventoryAssocPath = inventoryItem.objectPath;
                inventoryAssocPath += "/leds";
                if (objPath == inventoryAssocPath)
                {
                    // Get Association interface for object path
                    auto assocIt =
                        interfacesDict.find("xyz.openbmc_project.Association");
                    if (assocIt != interfacesDict.end())
                    {
                        // Get inventory item from end point
                        auto endpointsIt = assocIt->second.find("endpoints");
                        if (endpointsIt != assocIt->second.end())
                        {
                            const std::vector<std::string>* endpoints =
                                std::get_if<std::vector<std::string>>(
                                    &endpointsIt->second);
                            if ((endpoints != nullptr) && !endpoints->empty())
                            {
                                // Store LED path in inventory item
                                const std::string& ledPath = endpoints->front();
                                inventoryItem.ledObjectPath = ledPath;
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

    // Find DBus object path that implements ObjectManager for ObjectMapper
    std::string connection = "xyz.openbmc_project.ObjectMapper";
    auto iter = objectMgrPaths->find(connection);
    const std::string& objectMgrPath =
        (iter != objectMgrPaths->end()) ? iter->second : "/";
    BMCWEB_LOG_DEBUG << "ObjectManager path for " << connection << " is "
                     << objectMgrPath;

    // Call GetManagedObjects on the ObjectMapper to get all associations
    crow::connections::systemBus->async_method_call(
        std::move(respHandler), connection, objectMgrPath,
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
    std::shared_ptr<boost::container::flat_map<std::string, std::string>>
        ledConnections,
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
    auto it = ledConnections->nth(ledConnectionsIndex);
    if (it != ledConnections->end())
    {
        const std::string& ledPath = (*it).first;
        const std::string& ledConnection = (*it).second;
        // Response handler for Get State property
        auto respHandler =
            [sensorsAsyncResp, inventoryItems, ledConnections, ledPath,
             callback{std::move(callback)},
             ledConnectionsIndex](const boost::system::error_code ec,
                                  const std::variant<std::string>& ledState) {
                BMCWEB_LOG_DEBUG << "getInventoryLedData respHandler enter";
                if (ec)
                {
                    BMCWEB_LOG_ERROR
                        << "getInventoryLedData respHandler DBus error " << ec;
                    messages::internalError(sensorsAsyncResp->res);
                    return;
                }

                const std::string* state = std::get_if<std::string>(&ledState);
                if (state != nullptr)
                {
                    BMCWEB_LOG_DEBUG << "Led state: " << *state;
                    // Find inventory item with this LED object path
                    InventoryItem* inventoryItem =
                        findInventoryItemForLed(*inventoryItems, ledPath);
                    if (inventoryItem != nullptr)
                    {
                        // Store LED state in InventoryItem
                        if (boost::ends_with(*state, "On"))
                        {
                            inventoryItem->ledState = LedState::ON;
                        }
                        else if (boost::ends_with(*state, "Blink"))
                        {
                            inventoryItem->ledState = LedState::BLINK;
                        }
                        else if (boost::ends_with(*state, "Off"))
                        {
                            inventoryItem->ledState = LedState::OFF;
                        }
                        else
                        {
                            inventoryItem->ledState = LedState::UNKNOWN;
                        }
                    }
                }
                else
                {
                    BMCWEB_LOG_DEBUG << "Failed to find State data for LED: "
                                     << ledPath;
                }

                // Recurse to get LED data from next connection
                getInventoryLedData(sensorsAsyncResp, inventoryItems,
                                    ledConnections, std::move(callback),
                                    ledConnectionsIndex + 1);

                BMCWEB_LOG_DEBUG << "getInventoryLedData respHandler exit";
            };

        // Get the State property for the current LED
        crow::connections::systemBus->async_method_call(
            std::move(respHandler), ledConnection, ledPath,
            "org.freedesktop.DBus.Properties", "Get",
            "xyz.openbmc_project.Led.Physical", "State");
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
    const std::array<std::string, 1> interfaces = {
        "xyz.openbmc_project.Led.Physical"};

    // Response handler for parsing output from GetSubTree
    auto respHandler = [callback{std::move(callback)}, sensorsAsyncResp,
                        inventoryItems](const boost::system::error_code ec,
                                        const GetSubTreeType& subtree) {
        BMCWEB_LOG_DEBUG << "getInventoryLeds respHandler enter";
        if (ec)
        {
            messages::internalError(sensorsAsyncResp->res);
            BMCWEB_LOG_ERROR << "getInventoryLeds respHandler DBus error "
                             << ec;
            return;
        }

        // Build map of LED object paths to connections
        std::shared_ptr<boost::container::flat_map<std::string, std::string>>
            ledConnections = std::make_shared<
                boost::container::flat_map<std::string, std::string>>();

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
    };
    // Make call to ObjectMapper to find all inventory items
    crow::connections::systemBus->async_method_call(
        std::move(respHandler), "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree", path, 0, interfaces);
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
    const boost::container::flat_map<std::string, std::string>&
        psAttributesConnections,
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
    auto it = psAttributesConnections.nth(0);

    const std::string& psAttributesPath = (*it).first;
    const std::string& psAttributesConnection = (*it).second;

    // Response handler for Get DeratingFactor property
    auto respHandler = [sensorsAsyncResp, inventoryItems,
                        callback{std::move(callback)}](
                           const boost::system::error_code ec,
                           const std::variant<uint32_t>& deratingFactor) {
        BMCWEB_LOG_DEBUG << "getPowerSupplyAttributesData respHandler enter";
        if (ec)
        {
            BMCWEB_LOG_ERROR
                << "getPowerSupplyAttributesData respHandler DBus error " << ec;
            messages::internalError(sensorsAsyncResp->res);
            return;
        }

        const uint32_t* value = std::get_if<uint32_t>(&deratingFactor);
        if (value != nullptr)
        {
            BMCWEB_LOG_DEBUG << "PS EfficiencyPercent value: " << *value;
            // Store value in Power Supply Inventory Items
            for (InventoryItem& inventoryItem : *inventoryItems)
            {
                if (inventoryItem.isPowerSupply == true)
                {
                    inventoryItem.powerSupplyEfficiencyPercent =
                        static_cast<int>(*value);
                }
            }
        }
        else
        {
            BMCWEB_LOG_DEBUG
                << "Failed to find EfficiencyPercent value for PowerSupplies";
        }

        BMCWEB_LOG_DEBUG << "getPowerSupplyAttributesData respHandler exit";
        callback(inventoryItems);
    };

    // Get the DeratingFactor property for the PowerSupplyAttributes
    // Currently only property on the interface/only one we care about
    crow::connections::systemBus->async_method_call(
        std::move(respHandler), psAttributesConnection, psAttributesPath,
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Control.PowerSupplyAttributes", "DeratingFactor");

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

    const std::array<std::string, 1> interfaces = {
        "xyz.openbmc_project.Control.PowerSupplyAttributes"};

    // Response handler for parsing output from GetSubTree
    auto respHandler = [callback{std::move(callback)}, sensorsAsyncResp,
                        inventoryItems](const boost::system::error_code ec,
                                        const GetSubTreeType& subtree) {
        BMCWEB_LOG_DEBUG << "getPowerSupplyAttributes respHandler enter";
        if (ec)
        {
            messages::internalError(sensorsAsyncResp->res);
            BMCWEB_LOG_ERROR
                << "getPowerSupplyAttributes respHandler DBus error " << ec;
            return;
        }
        if (subtree.size() == 0)
        {
            BMCWEB_LOG_DEBUG << "Can't find Power Supply Attributes!";
            callback(inventoryItems);
            return;
        }

        // Currently we only support 1 power supply attribute, use this for
        // all the power supplies. Build map of object path to connection.
        // Assume just 1 connection and 1 path for now.
        boost::container::flat_map<std::string, std::string>
            psAttributesConnections;

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
    };
    // Make call to ObjectMapper to find the PowerSupplyAttributes service
    crow::connections::systemBus->async_method_call(
        std::move(respHandler), "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project", 0, interfaces);
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
 * @param objectMgrPaths Mappings from connection name to DBus object path that
 * implements ObjectManager.
 * @param callback Callback to invoke when inventory items have been obtained.
 */
template <typename Callback>
static void getInventoryItems(
    std::shared_ptr<SensorsAsyncResp> sensorsAsyncResp,
    const std::shared_ptr<boost::container::flat_set<std::string>> sensorNames,
    std::shared_ptr<boost::container::flat_map<std::string, std::string>>
        objectMgrPaths,
    Callback&& callback)
{
    BMCWEB_LOG_DEBUG << "getInventoryItems enter";
    auto getInventoryItemAssociationsCb =
        [sensorsAsyncResp, objectMgrPaths, callback{std::move(callback)}](
            std::shared_ptr<std::vector<InventoryItem>> inventoryItems) {
            BMCWEB_LOG_DEBUG << "getInventoryItemAssociationsCb enter";
            auto getInventoryItemsConnectionsCb =
                [sensorsAsyncResp, inventoryItems, objectMgrPaths,
                 callback{std::move(callback)}](
                    std::shared_ptr<boost::container::flat_set<std::string>>
                        invConnections) {
                    BMCWEB_LOG_DEBUG << "getInventoryItemsConnectionsCb enter";
                    auto getInventoryItemsDataCb =
                        [sensorsAsyncResp, inventoryItems,
                         callback{std::move(callback)}]() {
                            BMCWEB_LOG_DEBUG << "getInventoryItemsDataCb enter";

                            auto getInventoryLedsCb = [sensorsAsyncResp,
                                                       inventoryItems,
                                                       callback{std::move(
                                                           callback)}]() {
                                BMCWEB_LOG_DEBUG << "getInventoryLedsCb enter";
                                // Find Power Supply Attributes and get the data
                                getPowerSupplyAttributes(sensorsAsyncResp,
                                                         inventoryItems,
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
                                          invConnections, objectMgrPaths,
                                          std::move(getInventoryItemsDataCb));
                    BMCWEB_LOG_DEBUG << "getInventoryItemsConnectionsCb exit";
                };

            // Get connections that provide inventory item data
            getInventoryItemsConnections(
                sensorsAsyncResp, inventoryItems,
                std::move(getInventoryItemsConnectionsCb));
            BMCWEB_LOG_DEBUG << "getInventoryItemAssociationsCb exit";
        };

    // Get associations from sensors to inventory items
    getInventoryItemAssociations(sensorsAsyncResp, sensorNames, objectMgrPaths,
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
    powerSupply["@odata.id"] =
        "/redfish/v1/Chassis/" + chassisId + "/Power#/PowerSupplies/";
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
 * The objectMgrPaths map contains mappings from a connection name to the
 * corresponding DBus object path that implements ObjectManager.
 *
 * The InventoryItem vector contains D-Bus inventory items associated with the
 * sensors.  Inventory item data is needed for some Redfish sensor properties.
 *
 * @param SensorsAsyncResp Pointer to object holding response data.
 * @param sensorNames All requested sensors within the current chassis.
 * @param connections Connections that provide sensor values.
 * @param objectMgrPaths Mappings from connection name to DBus object path that
 * implements ObjectManager.
 * @param inventoryItems Inventory items associated with the sensors.
 */
inline void getSensorData(
    const std::shared_ptr<SensorsAsyncResp>& sensorsAsyncResp,
    const std::shared_ptr<boost::container::flat_set<std::string>>& sensorNames,
    const boost::container::flat_set<std::string>& connections,
    const std::shared_ptr<boost::container::flat_map<std::string, std::string>>&
        objectMgrPaths,
    const std::shared_ptr<std::vector<InventoryItem>>& inventoryItems)
{
    BMCWEB_LOG_DEBUG << "getSensorData enter";
    // Get managed objects from all services exposing sensors
    for (const std::string& connection : connections)
    {
        // Response handler to process managed objects
        auto getManagedObjectsCb = [sensorsAsyncResp, sensorNames,
                                    inventoryItems](
                                       const boost::system::error_code ec,
                                       ManagedObjectsVectorType& resp) {
            BMCWEB_LOG_DEBUG << "getManagedObjectsCb enter";
            if (ec)
            {
                BMCWEB_LOG_ERROR << "getManagedObjectsCb DBUS error: " << ec;
                messages::internalError(sensorsAsyncResp->res);
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
                boost::algorithm::split(split, objPath, boost::is_any_of("/"));
                if (split.size() < 6)
                {
                    BMCWEB_LOG_ERROR << "Got path that isn't long enough "
                                     << objPath;
                    continue;
                }
                // These indexes aren't intuitive, as boost::split puts an empty
                // string at the beginning
                const std::string& sensorType = split[4];
                const std::string& sensorName = split[5];
                BMCWEB_LOG_DEBUG << "sensorName " << sensorName
                                 << " sensorType " << sensorType;
                if (sensorNames->find(objPath) == sensorNames->end())
                {
                    BMCWEB_LOG_ERROR << sensorName << " not in sensor list ";
                    continue;
                }

                // Find inventory item (if any) associated with sensor
                InventoryItem* inventoryItem =
                    findInventoryItemForSensor(inventoryItems, objPath);

                const std::string& sensorSchema =
                    sensorsAsyncResp->chassisSubNode;

                nlohmann::json* sensorJson = nullptr;

                if (sensorSchema == sensors::node::sensors)
                {
                    sensorsAsyncResp->res.jsonValue["@odata.id"] =
                        "/redfish/v1/Chassis/" + sensorsAsyncResp->chassisId +
                        "/" + sensorsAsyncResp->chassisSubNode + "/" +
                        sensorName;
                    sensorJson = &(sensorsAsyncResp->res.jsonValue);
                }
                else
                {
                    std::string fieldName;
                    if (sensorType == "temperature")
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
                        if (!sensorName.compare("total_power"))
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
                        sensorsAsyncResp->res.jsonValue[fieldName];
                    if (fieldName == "PowerControl")
                    {
                        if (tempArray.empty())
                        {
                            // Put multiple "sensors" into a single
                            // PowerControl. Follows MemberId naming and
                            // naming in power.hpp.
                            tempArray.push_back(
                                {{"@odata.id",
                                  "/redfish/v1/Chassis/" +
                                      sensorsAsyncResp->chassisId + "/" +
                                      sensorsAsyncResp->chassisSubNode + "#/" +
                                      fieldName + "/0"}});
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
                    else
                    {
                        tempArray.push_back(
                            {{"@odata.id",
                              "/redfish/v1/Chassis/" +
                                  sensorsAsyncResp->chassisId + "/" +
                                  sensorsAsyncResp->chassisSubNode + "#/" +
                                  fieldName + "/"}});
                        sensorJson = &(tempArray.back());
                    }
                }

                if (sensorJson != nullptr)
                {
                    objectInterfacesToJson(
                        sensorName, sensorType, sensorsAsyncResp,
                        objDictEntry.second, *sensorJson, inventoryItem);
                }
            }
            if (sensorsAsyncResp.use_count() == 1)
            {
                sortJSONResponse(sensorsAsyncResp);
                if (sensorsAsyncResp->chassisSubNode == sensors::node::thermal)
                {
                    populateFanRedundancy(sensorsAsyncResp);
                }
            }
            BMCWEB_LOG_DEBUG << "getManagedObjectsCb exit";
        };

        // Find DBus object path that implements ObjectManager for the current
        // connection.  If no mapping found, default to "/".
        auto iter = objectMgrPaths->find(connection);
        const std::string& objectMgrPath =
            (iter != objectMgrPaths->end()) ? iter->second : "/";
        BMCWEB_LOG_DEBUG << "ObjectManager path for " << connection << " is "
                         << objectMgrPath;

        crow::connections::systemBus->async_method_call(
            getManagedObjectsCb, connection, objectMgrPath,
            "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
    }
    BMCWEB_LOG_DEBUG << "getSensorData exit";
}

inline void processSensorList(
    const std::shared_ptr<SensorsAsyncResp>& sensorsAsyncResp,
    const std::shared_ptr<boost::container::flat_set<std::string>>& sensorNames)
{
    auto getConnectionCb =
        [sensorsAsyncResp, sensorNames](
            const boost::container::flat_set<std::string>& connections) {
            BMCWEB_LOG_DEBUG << "getConnectionCb enter";
            auto getObjectManagerPathsCb =
                [sensorsAsyncResp, sensorNames,
                 connections](const std::shared_ptr<boost::container::flat_map<
                                  std::string, std::string>>& objectMgrPaths) {
                    BMCWEB_LOG_DEBUG << "getObjectManagerPathsCb enter";
                    auto getInventoryItemsCb =
                        [sensorsAsyncResp, sensorNames, connections,
                         objectMgrPaths](
                            const std::shared_ptr<std::vector<InventoryItem>>&
                                inventoryItems) {
                            BMCWEB_LOG_DEBUG << "getInventoryItemsCb enter";
                            // Get sensor data and store results in JSON
                            getSensorData(sensorsAsyncResp, sensorNames,
                                          connections, objectMgrPaths,
                                          inventoryItems);
                            BMCWEB_LOG_DEBUG << "getInventoryItemsCb exit";
                        };

                    // Get inventory items associated with sensors
                    getInventoryItems(sensorsAsyncResp, sensorNames,
                                      objectMgrPaths,
                                      std::move(getInventoryItemsCb));

                    BMCWEB_LOG_DEBUG << "getObjectManagerPathsCb exit";
                };

            // Get mapping from connection names to the DBus object
            // paths that implement the ObjectManager interface
            getObjectManagerPaths(sensorsAsyncResp,
                                  std::move(getObjectManagerPathsCb));
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
            const std::shared_ptr<boost::container::flat_set<std::string>>&
                sensorNames) {
            BMCWEB_LOG_DEBUG << "getChassisCb enter";
            processSensorList(sensorsAsyncResp, sensorNames);
            BMCWEB_LOG_DEBUG << "getChassisCb exit";
        };
    sensorsAsyncResp->res.jsonValue["Redundancy"] = nlohmann::json::array();

    // Get set of sensors in chassis
    getChassis(sensorsAsyncResp, std::move(getChassisCb));
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
inline bool findSensorNameUsingSensorPath(
    std::string_view sensorName,
    boost::container::flat_set<std::string>& sensorsList,
    boost::container::flat_set<std::string>& sensorsModified)
{
    for (std::string_view chassisSensor : sensorsList)
    {
        std::size_t pos = chassisSensor.rfind('/');
        if (pos >= (chassisSensor.size() - 1))
        {
            continue;
        }
        std::string_view thisSensorName = chassisSensor.substr(pos + 1);
        if (thisSensorName == sensorName)
        {
            sensorsModified.emplace(chassisSensor);
            return true;
        }
    }
    return false;
}

/**
 * @brief Entry point for overriding sensor values of given sensor
 *
 * @param res   response object
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

    const char* propertyValueName;
    std::unordered_map<std::string, std::pair<double, std::string>> overrideMap;
    std::string memberId;
    double value;
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
            if (!json_util::readJson(item, sensorAsyncResp->res, "MemberId",
                                     memberId, propertyValueName, value))
            {
                return;
            }
            overrideMap.emplace(memberId,
                                std::make_pair(value, collectionItems.first));
        }
    }

    auto getChassisSensorListCb = [sensorAsyncResp, overrideMap](
                                      const std::shared_ptr<
                                          boost::container::flat_set<
                                              std::string>>& sensorsList) {
        // Match sensor names in the PATCH request to those managed by the
        // chassis node
        const std::shared_ptr<boost::container::flat_set<std::string>>
            sensorNames =
                std::make_shared<boost::container::flat_set<std::string>>();
        for (const auto& item : overrideMap)
        {
            const auto& sensor = item.first;
            if (!findSensorNameUsingSensorPath(sensor, *sensorsList,
                                               *sensorNames))
            {
                BMCWEB_LOG_INFO << "Unable to find memberId " << item.first;
                messages::resourceNotFound(sensorAsyncResp->res,
                                           item.second.second, item.first);
                return;
            }
        }
        // Get the connection to which the memberId belongs
        auto getObjectsWithConnectionCb =
            [sensorAsyncResp, overrideMap](
                const boost::container::flat_set<std::string>& /*connections*/,
                const std::set<std::pair<std::string, std::string>>&
                    objectsWithConnection) {
                if (objectsWithConnection.size() != overrideMap.size())
                {
                    BMCWEB_LOG_INFO
                        << "Unable to find all objects with proper connection "
                        << objectsWithConnection.size() << " requested "
                        << overrideMap.size() << "\n";
                    messages::resourceNotFound(
                        sensorAsyncResp->res,
                        sensorAsyncResp->chassisSubNode ==
                                sensors::node::thermal
                            ? "Temperatures"
                            : "Voltages",
                        "Count");
                    return;
                }
                for (const auto& item : objectsWithConnection)
                {

                    auto lastPos = item.first.rfind('/');
                    if (lastPos == std::string::npos)
                    {
                        messages::internalError(sensorAsyncResp->res);
                        return;
                    }
                    std::string sensorName = item.first.substr(lastPos + 1);

                    const auto& iterator = overrideMap.find(sensorName);
                    if (iterator == overrideMap.end())
                    {
                        BMCWEB_LOG_INFO << "Unable to find sensor object"
                                        << item.first << "\n";
                        messages::internalError(sensorAsyncResp->res);
                        return;
                    }
                    crow::connections::systemBus->async_method_call(
                        [sensorAsyncResp](const boost::system::error_code ec) {
                            if (ec)
                            {
                                BMCWEB_LOG_DEBUG
                                    << "setOverrideValueStatus DBUS error: "
                                    << ec;
                                messages::internalError(sensorAsyncResp->res);
                                return;
                            }
                        },
                        item.second, item.first,
                        "org.freedesktop.DBus.Properties", "Set",
                        "xyz.openbmc_project.Sensor.Value", "Value",
                        std::variant<double>(iterator->second.first));
                }
            };
        // Get object with connection for the given sensor name
        getObjectsWithConnection(sensorAsyncResp, sensorNames,
                                 std::move(getObjectsWithConnectionCb));
    };
    // get full sensor list for the given chassisId and cross verify the sensor.
    getChassis(sensorAsyncResp, std::move(getChassisSensorListCb));
}

inline bool isOverridingAllowed(const std::string& manufacturingModeStatus)
{
    if (manufacturingModeStatus ==
        "xyz.openbmc_project.Control.Security.SpecialMode.Modes.Manufacturing")
    {
        return true;
    }

#ifdef BMCWEB_ENABLE_VALIDATION_UNSECURE_FEATURE
    if (manufacturingModeStatus == "xyz.openbmc_project.Control.Security."
                                   "SpecialMode.Modes.ValidationUnsecure")
    {
        return true;
    }

#endif

    return false;
}

/**
 * @brief Entry point for Checking the manufacturing mode before doing sensor
 * override values of given sensor
 *
 * @param res   response object
 * @param allCollections   Collections extract from sensors' request patch info
 * @param chassisSubNode   Chassis Node for which the query has to happen
 */
inline void checkAndDoSensorsOverride(
    const std::shared_ptr<SensorsAsyncResp>& sensorAsyncResp,
    std::unordered_map<std::string, std::vector<nlohmann::json>>&
        allCollections)
{
    BMCWEB_LOG_INFO << "checkAndDoSensorsOverride for subnode"
                    << sensorAsyncResp->chassisSubNode << "\n";

    const std::array<std::string, 1> interfaces = {
        "xyz.openbmc_project.Security.SpecialMode"};

    crow::connections::systemBus->async_method_call(
        [sensorAsyncResp, allCollections](const boost::system::error_code ec2,
                                          const GetSubTreeType& resp) mutable {
            if (ec2)
            {
                BMCWEB_LOG_DEBUG
                    << "Error in querying GetSubTree with Object Mapper. "
                    << ec2;
                messages::internalError(sensorAsyncResp->res);
                return;
            }
#ifdef BMCWEB_INSECURE_UNRESTRICTED_SENSOR_OVERRIDE
            // Proceed with sensor override
            setSensorsOverride(sensorAsyncResp, allCollections);
            return;
#endif

            if (resp.size() != 1)
            {
                BMCWEB_LOG_WARNING
                    << "Overriding sensor value is not allowed - Internal "
                       "error in querying SpecialMode property.";
                messages::internalError(sensorAsyncResp->res);
                return;
            }
            const std::string& path = resp[0].first;
            const std::string& serviceName = resp[0].second.begin()->first;

            if (path.empty() || serviceName.empty())
            {
                BMCWEB_LOG_DEBUG
                    << "Path or service name is returned as empty. ";
                messages::internalError(sensorAsyncResp->res);
                return;
            }

            // Sensor override is allowed only in manufacturing mode or
            // validation unsecure mode .
            crow::connections::systemBus->async_method_call(
                [sensorAsyncResp, allCollections,
                 path](const boost::system::error_code ec,
                       std::variant<std::string>& getManufactMode) mutable {
                    if (ec)
                    {
                        BMCWEB_LOG_DEBUG
                            << "Error in querying Special mode property " << ec;
                        messages::internalError(sensorAsyncResp->res);
                        return;
                    }

                    const std::string* manufacturingModeStatus =
                        std::get_if<std::string>(&getManufactMode);

                    if (nullptr == manufacturingModeStatus)
                    {
                        BMCWEB_LOG_DEBUG << "Sensor override mode is not "
                                            "Enabled. Returning ... ";
                        messages::internalError(sensorAsyncResp->res);
                        return;
                    }

                    if (isOverridingAllowed(*manufacturingModeStatus))
                    {
                        BMCWEB_LOG_INFO << "Manufacturing mode is Enabled. "
                                           "Proceeding further... ";
                        setSensorsOverride(sensorAsyncResp, allCollections);
                    }
                    else
                    {
                        BMCWEB_LOG_WARNING
                            << "Manufacturing mode is not Enabled...can't "
                               "Override the sensor value. ";

                        messages::actionNotSupported(
                            sensorAsyncResp->res,
                            "Overriding of Sensor Value for non "
                            "manufacturing mode");
                        return;
                    }
                },
                serviceName, path, "org.freedesktop.DBus.Properties", "Get",
                "xyz.openbmc_project.Security.SpecialMode", "SpecialMode");
        },

        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree", "/", 5, interfaces);
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
    auto typesIt = sensors::dbus::types.find(node);
    if (typesIt == sensors::dbus::types.end())
    {
        BMCWEB_LOG_ERROR << "Wrong node provided : " << node;
        mapComplete(boost::beast::http::status::bad_request, {});
        return;
    }

    auto respBuffer = std::make_shared<crow::Response>();
    auto callback =
        [respBuffer, mapCompleteCb{std::move(mapComplete)}](
            const boost::beast::http::status status,
            const boost::container::flat_map<std::string, std::string>&
                uriToDbus) { mapCompleteCb(status, uriToDbus); };

    auto resp = std::make_shared<SensorsAsyncResp>(
        *respBuffer, chassis, typesIt->second, node, std::move(callback));
    getChassisData(resp);
}

class SensorCollection : public Node
{
  public:
    SensorCollection(App& app) :
        Node(app, "/redfish/v1/Chassis/<str>/Sensors/", std::string())
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>& params) override
    {
        BMCWEB_LOG_DEBUG << "SensorCollection doGet enter";
        if (params.size() != 1)
        {
            BMCWEB_LOG_DEBUG << "SensorCollection doGet param size < 1";
            messages::internalError(res);
            res.end();
            return;
        }

        const std::string& chassisId = params[0];
        std::shared_ptr<SensorsAsyncResp> asyncResp =
            std::make_shared<SensorsAsyncResp>(
                res, chassisId, sensors::dbus::types.at(sensors::node::sensors),
                sensors::node::sensors);

        auto getChassisCb =
            [asyncResp](
                const std::shared_ptr<boost::container::flat_set<std::string>>&
                    sensorNames) {
                BMCWEB_LOG_DEBUG << "getChassisCb enter";

                nlohmann::json& entriesArray =
                    asyncResp->res.jsonValue["Members"];
                for (auto& sensor : *sensorNames)
                {
                    BMCWEB_LOG_DEBUG << "Adding sensor: " << sensor;

                    std::size_t lastPos = sensor.rfind('/');
                    if (lastPos == std::string::npos ||
                        lastPos + 1 >= sensor.size())
                    {
                        BMCWEB_LOG_ERROR << "Invalid sensor path: " << sensor;
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    std::string sensorName = sensor.substr(lastPos + 1);
                    entriesArray.push_back(
                        {{"@odata.id",
                          "/redfish/v1/Chassis/" + asyncResp->chassisId + "/" +
                              asyncResp->chassisSubNode + "/" + sensorName}});
                }

                asyncResp->res.jsonValue["Members@odata.count"] =
                    entriesArray.size();
                BMCWEB_LOG_DEBUG << "getChassisCb exit";
            };

        // Get set of sensors in chassis
        getChassis(asyncResp, std::move(getChassisCb));
        BMCWEB_LOG_DEBUG << "SensorCollection doGet exit";
    }
};

class Sensor : public Node
{
  public:
    Sensor(App& app) :
        Node(app, "/redfish/v1/Chassis/<str>/Sensors/<str>/", std::string(),
             std::string())
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>& params) override
    {
        BMCWEB_LOG_DEBUG << "Sensor doGet enter";
        if (params.size() != 2)
        {
            BMCWEB_LOG_DEBUG << "Sensor doGet param size < 2";
            messages::internalError(res);
            res.end();
            return;
        }
        const std::string& chassisId = params[0];
        std::shared_ptr<SensorsAsyncResp> asyncResp =
            std::make_shared<SensorsAsyncResp>(res, chassisId,
                                               std::vector<const char*>(),
                                               sensors::node::sensors);

        const std::string& sensorName = params[1];
        const std::array<const char*, 1> interfaces = {
            "xyz.openbmc_project.Sensor.Value"};

        // Get a list of all of the sensors that implement Sensor.Value
        // and get the path and service name associated with the sensor
        crow::connections::systemBus->async_method_call(
            [asyncResp, sensorName](const boost::system::error_code ec,
                                    const GetSubTreeType& subtree) {
                BMCWEB_LOG_DEBUG << "respHandler1 enter";
                if (ec)
                {
                    messages::internalError(asyncResp->res);
                    BMCWEB_LOG_ERROR << "Sensor getSensorPaths resp_handler: "
                                     << "Dbus error " << ec;
                    return;
                }

                GetSubTreeType::const_iterator it = std::find_if(
                    subtree.begin(), subtree.end(),
                    [sensorName](
                        const std::pair<
                            std::string,
                            std::vector<std::pair<std::string,
                                                  std::vector<std::string>>>>&
                            object) {
                        std::string_view sensor = object.first;
                        std::size_t lastPos = sensor.rfind('/');
                        if (lastPos == std::string::npos ||
                            lastPos + 1 >= sensor.size())
                        {
                            BMCWEB_LOG_ERROR << "Invalid sensor path: "
                                             << sensor;
                            return false;
                        }
                        std::string_view name = sensor.substr(lastPos + 1);

                        return name == sensorName;
                    });

                if (it == subtree.end())
                {
                    BMCWEB_LOG_ERROR << "Could not find path for sensor: "
                                     << sensorName;
                    messages::resourceNotFound(asyncResp->res, "Sensor",
                                               sensorName);
                    return;
                }
                std::string_view sensorPath = (*it).first;
                BMCWEB_LOG_DEBUG << "Found sensor path for sensor '"
                                 << sensorName << "': " << sensorPath;

                const std::shared_ptr<boost::container::flat_set<std::string>>
                    sensorList = std::make_shared<
                        boost::container::flat_set<std::string>>();

                sensorList->emplace(sensorPath);
                processSensorList(asyncResp, sensorList);
                BMCWEB_LOG_DEBUG << "respHandler1 exit";
            },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetSubTree",
            "/xyz/openbmc_project/sensors", 2, interfaces);
    }
};

} // namespace redfish
