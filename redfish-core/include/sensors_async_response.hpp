#pragma once

#include "bmcweb_config.h"

#include "generated/enums/redundancy.hpp"
#include "utils/chassis_utils.hpp"
#include "utils/sensor_utils.hpp"

#include <boost/beast/http/status.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>
#include <boost/url/url.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/message/native_types.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <array>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace redfish
{

namespace sensors
{

// clang-format off
namespace dbus
{
constexpr auto powerPaths = std::to_array<std::string_view>({
    "/xyz/openbmc_project/sensors/voltage",
    "/xyz/openbmc_project/sensors/power"
});

constexpr auto getSensorPaths(){
    if constexpr(BMCWEB_REDFISH_NEW_POWERSUBSYSTEM_THERMALSUBSYSTEM){
    return std::to_array<std::string_view>({
        "/xyz/openbmc_project/sensors/power",
        "/xyz/openbmc_project/sensors/current",
        "/xyz/openbmc_project/sensors/airflow",
        "/xyz/openbmc_project/sensors/humidity",
        "/xyz/openbmc_project/sensors/voltage",
        "/xyz/openbmc_project/sensors/fan_tach",
        "/xyz/openbmc_project/sensors/temperature",
        "/xyz/openbmc_project/sensors/fan_pwm",
        "/xyz/openbmc_project/sensors/altitude",
        "/xyz/openbmc_project/sensors/energy",
        "/xyz/openbmc_project/sensors/liquidflow",
        "/xyz/openbmc_project/sensors/pressure",
        "/xyz/openbmc_project/sensors/utilization"});
    } else {
      return  std::to_array<std::string_view>({"/xyz/openbmc_project/sensors/power",
        "/xyz/openbmc_project/sensors/current",
        "/xyz/openbmc_project/sensors/airflow",
        "/xyz/openbmc_project/sensors/humidity",
        "/xyz/openbmc_project/sensors/utilization"});
}
}

constexpr auto sensorPaths = getSensorPaths();

constexpr auto thermalPaths = std::to_array<std::string_view>({
    "/xyz/openbmc_project/sensors/fan_tach",
    "/xyz/openbmc_project/sensors/temperature",
    "/xyz/openbmc_project/sensors/fan_pwm"
});

} // namespace dbus
// clang-format on

constexpr std::string_view powerNodeStr = sensor_utils::chassisSubNodeToString(
    sensor_utils::ChassisSubNode::powerNode);
constexpr std::string_view sensorsNodeStr =
    sensor_utils::chassisSubNodeToString(
        sensor_utils::ChassisSubNode::sensorsNode);
constexpr std::string_view thermalNodeStr =
    sensor_utils::chassisSubNodeToString(
        sensor_utils::ChassisSubNode::thermalNode);

using sensorPair =
    std::pair<std::string_view, std::span<const std::string_view>>;
static constexpr std::array<sensorPair, 3> paths = {
    {{sensors::powerNodeStr, dbus::powerPaths},
     {sensors::sensorsNodeStr, dbus::sensorPaths},
     {sensors::thermalNodeStr, dbus::thermalPaths}}};

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
        std::string name;
        std::string uri;
        std::string dbusPath;
    };

    SensorsAsyncResp(const std::shared_ptr<bmcweb::AsyncResp>& asyncRespIn,
                     const std::string& chassisIdIn,
                     std::span<const std::string_view> typesIn,
                     std::string_view subNode) :
        asyncResp(asyncRespIn), chassisId(chassisIdIn), types(typesIn),
        chassisSubNode(subNode), efficientExpand(false)
    {}

    // Store extra data about sensor mapping and return it in callback
    SensorsAsyncResp(const std::shared_ptr<bmcweb::AsyncResp>& asyncRespIn,
                     const std::string& chassisIdIn,
                     std::span<const std::string_view> typesIn,
                     std::string_view subNode,
                     DataCompleteCb&& creationComplete) :
        asyncResp(asyncRespIn), chassisId(chassisIdIn), types(typesIn),
        chassisSubNode(subNode), efficientExpand(false),
        metadata{std::vector<SensorData>()},
        dataComplete{std::move(creationComplete)}
    {}

    // sensor collections expand
    SensorsAsyncResp(const std::shared_ptr<bmcweb::AsyncResp>& asyncRespIn,
                     const std::string& chassisIdIn,
                     std::span<const std::string_view> typesIn,
                     const std::string_view& subNode, bool efficientExpandIn) :
        asyncResp(asyncRespIn), chassisId(chassisIdIn), types(typesIn),
        chassisSubNode(subNode), efficientExpand(efficientExpandIn)
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
        if (!metadata)
        {
            return;
        }
        const auto nameIt = sensorObject.find("Name");
        if (nameIt == sensorObject.end())
        {
            return;
        }
        const auto idIt = sensorObject.find("@odata.id");
        if (idIt == sensorObject.end())
        {
            return;
        }
        const std::string* name = nameIt->get_ptr<const std::string*>();
        if (name == nullptr)
        {
            return;
        }
        const std::string* id = idIt->get_ptr<const std::string*>();
        if (id == nullptr)
        {
            return;
        }
        metadata->emplace_back(SensorData{*name, *id, dbusPath});
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
 * @brief Find the requested sensorName in the list of all sensors supplied by
 * the chassis node
 *
 * @param sensorName   The sensor name supplied in the PATCH request
 * @param sensorsList  The list of sensors managed by the chassis node
 * @param sensorsModified  The list of sensors that were found as a result of
 *                         repeated calls to this function
 */
inline bool findSensorNameUsingSensorPath(
    std::string_view sensorName, const std::set<std::string>& sensorsList,
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
    BMCWEB_LOG_DEBUG("getObjectsWithConnection enter");
    const std::string path = "/xyz/openbmc_project/sensors";
    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Sensor.Value"};

    // Make call to ObjectMapper to find all sensors objects
    dbus::utility::getSubTree(
        path, 2, interfaces,
        [callback = std::forward<Callback>(callback), sensorsAsyncResp,
         sensorNames](const boost::system::error_code& ec,
                      const dbus::utility::MapperGetSubTreeResponse& subtree) {
            // Response handler for parsing objects subtree
            BMCWEB_LOG_DEBUG("getObjectsWithConnection resp_handler enter");
            if (ec)
            {
                messages::internalError(sensorsAsyncResp->asyncResp->res);
                BMCWEB_LOG_ERROR(
                    "getObjectsWithConnection resp_handler: Dbus error {}", ec);
                return;
            }

            BMCWEB_LOG_DEBUG("Found {} subtrees", subtree.size());

            // Make unique list of connections only for requested sensor types
            // and found in the chassis
            std::set<std::string> connections;
            std::set<std::pair<std::string, std::string>> objectsWithConnection;

            BMCWEB_LOG_DEBUG("sensorNames list count: {}", sensorNames->size());
            for (const std::string& tsensor : *sensorNames)
            {
                BMCWEB_LOG_DEBUG("Sensor to find: {}", tsensor);
            }

            for (const std::pair<std::string,
                                 std::vector<std::pair<
                                     std::string, std::vector<std::string>>>>&
                     object : subtree)
            {
                if (sensorNames->contains(object.first))
                {
                    for (const std::pair<std::string, std::vector<std::string>>&
                             objData : object.second)
                    {
                        BMCWEB_LOG_DEBUG("Adding connection: {}",
                                         objData.first);
                        connections.insert(objData.first);
                        objectsWithConnection.insert(
                            std::make_pair(object.first, objData.first));
                    }
                }
            }
            BMCWEB_LOG_DEBUG("Found {} connections", connections.size());
            callback(std::move(connections), std::move(objectsWithConnection));
            BMCWEB_LOG_DEBUG("getObjectsWithConnection resp_handler exit");
        });
    BMCWEB_LOG_DEBUG("getObjectsWithConnection exit");
}

/*
 *Populates the top level collection for a given subnode.  Populates
 *SensorCollection, Power, or Thermal schemas.
 *
 * */
inline void populateChassisNode(nlohmann::json& jsonValue,
                                std::string_view chassisSubNode)
{
    if (chassisSubNode == sensors::powerNodeStr)
    {
        jsonValue["@odata.type"] = "#Power.v1_5_2.Power";
    }
    else if (chassisSubNode == sensors::thermalNodeStr)
    {
        jsonValue["@odata.type"] = "#Thermal.v1_4_0.Thermal";
        jsonValue["Fans"] = nlohmann::json::array();
        jsonValue["Temperatures"] = nlohmann::json::array();
    }
    else if (chassisSubNode == sensors::sensorsNodeStr)
    {
        jsonValue["@odata.type"] = "#SensorCollection.SensorCollection";
        jsonValue["Description"] = "Collection of Sensors for this Chassis";
        jsonValue["Members"] = nlohmann::json::array();
        jsonValue["Members@odata.count"] = 0;
    }

    if (chassisSubNode != sensors::sensorsNodeStr)
    {
        jsonValue["Id"] = chassisSubNode;
    }
    jsonValue["Name"] = chassisSubNode;
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
                                   chassisSubNode == sensors::thermalNodeStr
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
inline void getInventoryItems(
    const std::shared_ptr<SensorsAsyncResp>& sensorsAsyncResp,
    const std::shared_ptr<std::set<std::string>>& sensorNames,
    Callback&& callback)
{
    BMCWEB_LOG_DEBUG("getInventoryItems enter");
    auto getInventoryItemAssociationsCb =
        [sensorsAsyncResp, callback = std::forward<Callback>(callback)](
            const std::shared_ptr<std::vector<sensor_utils::InventoryItem>>&
                inventoryItems) mutable {
            BMCWEB_LOG_DEBUG("getInventoryItemAssociationsCb enter");
            auto getInventoryItemsConnectionsCb =
                [sensorsAsyncResp, inventoryItems,
                 callback = std::forward<Callback>(callback)](
                    const std::shared_ptr<std::set<std::string>>&
                        invConnections) mutable {
                    BMCWEB_LOG_DEBUG("getInventoryItemsConnectionsCb enter");
                    auto getInventoryItemsDataCb =
                        [sensorsAsyncResp, inventoryItems,
                         callback =
                             std::forward<Callback>(callback)]() mutable {
                            BMCWEB_LOG_DEBUG("getInventoryItemsDataCb enter");

                            auto getInventoryLedsCb =
                                [sensorsAsyncResp, inventoryItems,
                                 callback = std::forward<Callback>(
                                     callback)]() mutable {
                                    BMCWEB_LOG_DEBUG(
                                        "getInventoryLedsCb enter");
                                    // Find Power Supply Attributes and get the
                                    // data
                                    getPowerSupplyAttributes(
                                        sensorsAsyncResp, inventoryItems,
                                        std::move(callback));
                                    BMCWEB_LOG_DEBUG("getInventoryLedsCb exit");
                                };

                            // Find led connections and get the data
                            getInventoryLeds(sensorsAsyncResp, inventoryItems,
                                             std::move(getInventoryLedsCb));
                            BMCWEB_LOG_DEBUG("getInventoryItemsDataCb exit");
                        };

                    // Get inventory item data from connections
                    getInventoryItemsData(sensorsAsyncResp, inventoryItems,
                                          invConnections,
                                          std::move(getInventoryItemsDataCb));
                    BMCWEB_LOG_DEBUG("getInventoryItemsConnectionsCb exit");
                };

            // Get connections that provide inventory item data
            getInventoryItemsConnections(
                sensorsAsyncResp, inventoryItems,
                std::move(getInventoryItemsConnectionsCb));
            BMCWEB_LOG_DEBUG("getInventoryItemAssociationsCb exit");
        };

    // Get associations from sensors to inventory items
    getInventoryItemAssociations(sensorsAsyncResp, sensorNames,
                                 std::move(getInventoryItemAssociationsCb));
    BMCWEB_LOG_DEBUG("getInventoryItems exit");
}

/**
 * @brief Finds the inventory item associated with the specified sensor.
 * @param inventoryItems D-Bus inventory items associated with sensors.
 * @param sensorObjPath D-Bus object path of sensor.
 * @return Inventory item within vector, or nullptr if no match found.
 */
inline sensor_utils::InventoryItem* findInventoryItemForSensor(
    const std::shared_ptr<std::vector<sensor_utils::InventoryItem>>&
        inventoryItems,
    const std::string& sensorObjPath)
{
    for (sensor_utils::InventoryItem& inventoryItem : *inventoryItems)
    {
        if (inventoryItem.sensors.contains(sensorObjPath))
        {
            return &inventoryItem;
        }
    }
    return nullptr;
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
inline nlohmann::json& getPowerSupply(
    nlohmann::json& powerSupplyArray,
    const sensor_utils::InventoryItem& inventoryItem,
    const std::string& chassisId)
{
    std::string nameS;
    nameS.resize(inventoryItem.name.size());
    std::ranges::replace_copy(inventoryItem.name, nameS.begin(), '_', ' ');
    // Check if matching PowerSupply object already exists in JSON array
    for (nlohmann::json& powerSupply : powerSupplyArray)
    {
        nlohmann::json::iterator nameIt = powerSupply.find("Name");
        if (nameIt == powerSupply.end())
        {
            continue;
        }
        const std::string* name = nameIt->get_ptr<std::string*>();
        if (name == nullptr)
        {
            continue;
        }
        if (nameS == *name)
        {
            return powerSupply;
        }
    }

    // Add new PowerSupply object to JSON array
    powerSupplyArray.push_back({});
    nlohmann::json& powerSupply = powerSupplyArray.back();
    boost::urls::url url =
        boost::urls::format("/redfish/v1/Chassis/{}/Power", chassisId);
    url.set_fragment(("/PowerSupplies"_json_pointer).to_string());
    powerSupply["@odata.id"] = std::move(url);
    std::string escaped;
    escaped.resize(inventoryItem.name.size());
    std::ranges::replace_copy(inventoryItem.name, escaped.begin(), '_', ' ');
    powerSupply["Name"] = std::move(escaped);
    powerSupply["Manufacturer"] = inventoryItem.manufacturer;
    powerSupply["Model"] = inventoryItem.model;
    powerSupply["PartNumber"] = inventoryItem.partNumber;
    powerSupply["SerialNumber"] = inventoryItem.serialNumber;
    sensor_utils::setLedState(powerSupply, &inventoryItem);

    if (inventoryItem.powerSupplyEfficiencyPercent >= 0)
    {
        powerSupply["EfficiencyPercent"] =
            inventoryItem.powerSupplyEfficiencyPercent;
    }

    powerSupply["Status"]["State"] =
        sensor_utils::getState(&inventoryItem, true);
    const char* health = inventoryItem.isFunctional ? "OK" : "Critical";
    powerSupply["Status"]["Health"] = health;

    return powerSupply;
}

inline void sortJSONResponse(
    const std::shared_ptr<SensorsAsyncResp>& sensorsAsyncResp)
{
    nlohmann::json& response = sensorsAsyncResp->asyncResp->res.jsonValue;
    std::array<std::string, 2> sensorHeaders{"Temperatures", "Fans"};
    if (sensorsAsyncResp->chassisSubNode == sensors::powerNodeStr)
    {
        sensorHeaders = {"Voltages", "PowerSupplies"};
    }
    for (const std::string& sensorGroup : sensorHeaders)
    {
        nlohmann::json::iterator entry = response.find(sensorGroup);
        if (entry == response.end())
        {
            continue;
        }
        nlohmann::json::array_t* arr =
            entry->get_ptr<nlohmann::json::array_t*>();
        if (arr == nullptr)
        {
            continue;
        }
        json_util::sortJsonArrayByKey(*arr, "Name");

        // add the index counts to the end of each entry
        size_t count = 0;
        for (nlohmann::json& sensorJson : *entry)
        {
            nlohmann::json::iterator odata = sensorJson.find("@odata.id");
            if (odata == sensorJson.end())
            {
                continue;
            }
            const auto nameIt = sensorJson.find("Name");
            if (nameIt == sensorJson.end())
            {
                continue;
            }
            std::string* value = odata->get_ptr<std::string*>();
            if (value == nullptr)
            {
                continue;
            }
            const std::string* name = nameIt->get_ptr<const std::string*>();
            if (name == nullptr)
            {
                continue;
            }
            *value += "/" + std::to_string(count);
            sensorJson["MemberId"] = std::to_string(count);
            count++;
            sensorsAsyncResp->updateUri(*name, *value);
        }
    }
}

/**
 * @brief Builds a json sensor representation of a sensor.
 * @param sensorName  The name of the sensor to be built
 * @param sensorType  The type (temperature, fan_tach, etc) of the sensor to
 * build
 * @param chassisSubNode The subnode (thermal, sensor, etc) of the sensor
 * @param interfacesDict  A dictionary of the interfaces and properties of said
 * interfaces to be built from
 * @param sensorJson  The json object to fill
 * @param inventoryItem D-Bus inventory item associated with the sensor.  Will
 * be nullptr if no associated inventory item was found.
 */
inline void objectInterfacesToJson(
    const std::string& sensorName, const std::string& sensorType,
    const sensor_utils::ChassisSubNode chassisSubNode,
    const dbus::utility::DBusInterfacesMap& interfacesDict,
    nlohmann::json& sensorJson, sensor_utils::InventoryItem* inventoryItem)
{
    for (const auto& [interface, valuesDict] : interfacesDict)
    {
        sensor_utils::objectPropertiesToJson(
            sensorName, sensorType, chassisSubNode, valuesDict, sensorJson,
            inventoryItem);
    }
    BMCWEB_LOG_DEBUG("Added sensor {}", sensorName);
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
            for (const std::pair<std::string, dbus::utility::MapperServiceMap>&
                     pathPair : resp)
            {
                const std::string& path = pathPair.first;
                const dbus::utility::MapperServiceMap& objDict =
                    pathPair.second;
                if (objDict.empty())
                {
                    continue; // this should be impossible
                }

                const std::string& owner = objDict.begin()->first;
                dbus::utility::getAssociationEndPoints(
                    path + "/chassis",
                    [path, owner, sensorsAsyncResp](
                        const boost::system::error_code& ec2,
                        const dbus::utility::MapperEndPoints& endpoints) {
                        if (ec2)
                        {
                            return; // if they don't have an association we
                                    // can't tell what chassis is
                        }
                        auto found = std::ranges::find_if(
                            endpoints,
                            [sensorsAsyncResp](const std::string& entry) {
                                return entry.find(
                                           sensorsAsyncResp->chassisId) !=
                                       std::string::npos;
                            });

                        if (found == endpoints.end())
                        {
                            return;
                        }
                        dbus::utility::getAllProperties(
                            *crow::connections::systemBus, owner, path,
                            "xyz.openbmc_project.Control.FanRedundancy",
                            [path, sensorsAsyncResp](
                                const boost::system::error_code& ec3,
                                const dbus::utility::DBusPropertiesMap& ret) {
                                if (ec3)
                                {
                                    return; // don't have to have this
                                            // interface
                                }

                                const uint8_t* allowedFailures = nullptr;
                                const std::vector<std::string>* collection =
                                    nullptr;
                                const std::string* status = nullptr;

                                const bool success =
                                    sdbusplus::unpackPropertiesNoThrow(
                                        dbus_utils::UnpackErrorPrinter(), ret,
                                        "AllowedFailures", allowedFailures,
                                        "Collection", collection, "Status",
                                        status);

                                if (!success)
                                {
                                    messages::internalError(
                                        sensorsAsyncResp->asyncResp->res);
                                    return;
                                }

                                if (allowedFailures == nullptr ||
                                    collection == nullptr || status == nullptr)
                                {
                                    BMCWEB_LOG_ERROR(
                                        "Invalid redundancy interface");
                                    messages::internalError(
                                        sensorsAsyncResp->asyncResp->res);
                                    return;
                                }

                                sdbusplus::message::object_path objectPath(
                                    path);
                                std::string name = objectPath.filename();
                                if (name.empty())
                                {
                                    // this should be impossible
                                    messages::internalError(
                                        sensorsAsyncResp->asyncResp->res);
                                    return;
                                }
                                std::ranges::replace(name, '_', ' ');

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
                                    sensorsAsyncResp->asyncResp->res
                                        .jsonValue["Fans"];
                                for (const std::string& item : *collection)
                                {
                                    sdbusplus::message::object_path itemPath(
                                        item);
                                    std::string itemName = itemPath.filename();
                                    if (itemName.empty())
                                    {
                                        continue;
                                    }
                                    /*
                                    todo(ed): merge patch that fixes the names
                                    std::replace(itemName.begin(),
                                                 itemName.end(), '_', ' ');*/
                                    auto schemaItem = std::ranges::find_if(
                                        fanRedfish,
                                        [itemName](const nlohmann::json& fan) {
                                            return fan["Name"] == itemName;
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
                                        BMCWEB_LOG_ERROR(
                                            "failed to find fan in schema");
                                        messages::internalError(
                                            sensorsAsyncResp->asyncResp->res);
                                        return;
                                    }
                                }

                                size_t minNumNeeded =
                                    collection->empty()
                                        ? 0
                                        : collection->size() - *allowedFailures;
                                nlohmann::json& jResp =
                                    sensorsAsyncResp->asyncResp->res
                                        .jsonValue["Redundancy"];

                                nlohmann::json::object_t redundancy;
                                boost::urls::url url = boost::urls::format(
                                    "/redfish/v1/Chassis/{}/{}",
                                    sensorsAsyncResp->chassisId,
                                    sensorsAsyncResp->chassisSubNode);
                                url.set_fragment(
                                    ("/Redundancy"_json_pointer / jResp.size())
                                        .to_string());
                                redundancy["@odata.id"] = std::move(url);
                                redundancy["@odata.type"] =
                                    "#Redundancy.v1_3_2.Redundancy";
                                redundancy["MinNumNeeded"] = minNumNeeded;
                                redundancy["Mode"] =
                                    redundancy::RedundancyType::NPlusM;
                                redundancy["Name"] = name;
                                redundancy["RedundancySet"] = redfishCollection;
                                redundancy["Status"]["Health"] = health;
                                redundancy["Status"]["State"] =
                                    resource::State::Enabled;

                                jResp.emplace_back(std::move(redundancy));
                            });
                    });
            }
        });
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
    const std::shared_ptr<std::vector<sensor_utils::InventoryItem>>&
        inventoryItems)
{
    BMCWEB_LOG_DEBUG("getSensorData enter");
    // Get managed objects from all services exposing sensors
    for (const std::string& connection : connections)
    {
        sdbusplus::message::object_path sensorPath(
            "/xyz/openbmc_project/sensors");
        dbus::utility::getManagedObjects(
            connection, sensorPath,
            [sensorsAsyncResp, sensorNames,
             inventoryItems](const boost::system::error_code& ec,
                             const dbus::utility::ManagedObjectType& resp) {
                BMCWEB_LOG_DEBUG("getManagedObjectsCb enter");
                if (ec)
                {
                    BMCWEB_LOG_ERROR("getManagedObjectsCb DBUS error: {}", ec);
                    messages::internalError(sensorsAsyncResp->asyncResp->res);
                    return;
                }
                auto chassisSubNode = sensor_utils::chassisSubNodeFromString(
                    sensorsAsyncResp->chassisSubNode);
                // Go through all objects and update response with sensor data
                for (const auto& objDictEntry : resp)
                {
                    const std::string& objPath =
                        static_cast<const std::string&>(objDictEntry.first);
                    BMCWEB_LOG_DEBUG("getManagedObjectsCb parsing object {}",
                                     objPath);

                    std::vector<std::string> split;
                    // Reserve space for
                    // /xyz/openbmc_project/sensors/<name>/<subname>
                    split.reserve(6);
                    // NOLINTNEXTLINE
                    bmcweb::split(split, objPath, '/');
                    if (split.size() < 6)
                    {
                        BMCWEB_LOG_ERROR("Got path that isn't long enough {}",
                                         objPath);
                        continue;
                    }
                    // These indexes aren't intuitive, as split puts an empty
                    // string at the beginning
                    const std::string& sensorType = split[4];
                    const std::string& sensorName = split[5];
                    BMCWEB_LOG_DEBUG("sensorName {} sensorType {}", sensorName,
                                     sensorType);
                    if (!sensorNames->contains(objPath))
                    {
                        BMCWEB_LOG_DEBUG("{} not in sensor list ", sensorName);
                        continue;
                    }

                    // Find inventory item (if any) associated with sensor
                    sensor_utils::InventoryItem* inventoryItem =
                        findInventoryItemForSensor(inventoryItems, objPath);

                    const std::string& sensorSchema =
                        sensorsAsyncResp->chassisSubNode;

                    nlohmann::json* sensorJson = nullptr;

                    if (sensorSchema == sensors::sensorsNodeStr &&
                        !sensorsAsyncResp->efficientExpand)
                    {
                        std::string sensorId =
                            redfish::sensor_utils::getSensorId(sensorName,
                                                               sensorType);

                        sensorsAsyncResp->asyncResp->res
                            .jsonValue["@odata.id"] = boost::urls::format(
                            "/redfish/v1/Chassis/{}/{}/{}",
                            sensorsAsyncResp->chassisId,
                            sensorsAsyncResp->chassisSubNode, sensorId);
                        sensorJson =
                            &(sensorsAsyncResp->asyncResp->res.jsonValue);
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
                        else if (sensorType == "fan" ||
                                 sensorType == "fan_tach" ||
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
                            BMCWEB_LOG_ERROR(
                                "Unsure how to handle sensorType {}",
                                sensorType);
                            continue;
                        }

                        nlohmann::json& tempArray =
                            sensorsAsyncResp->asyncResp->res
                                .jsonValue[fieldName];
                        if (fieldName == "PowerControl")
                        {
                            if (tempArray.empty())
                            {
                                // Put multiple "sensors" into a single
                                // PowerControl. Follows MemberId naming and
                                // naming in power.hpp.
                                nlohmann::json::object_t power;
                                boost::urls::url url = boost::urls::format(
                                    "/redfish/v1/Chassis/{}/{}",
                                    sensorsAsyncResp->chassisId,
                                    sensorsAsyncResp->chassisSubNode);
                                url.set_fragment(
                                    (""_json_pointer / fieldName / "0")
                                        .to_string());
                                power["@odata.id"] = std::move(url);
                                tempArray.emplace_back(std::move(power));
                            }
                            sensorJson = &(tempArray.back());
                        }
                        else if (fieldName == "PowerSupplies")
                        {
                            if (inventoryItem != nullptr)
                            {
                                sensorJson = &(getPowerSupply(
                                    tempArray, *inventoryItem,
                                    sensorsAsyncResp->chassisId));
                            }
                        }
                        else if (fieldName == "Members")
                        {
                            std::string sensorId =
                                redfish::sensor_utils::getSensorId(sensorName,
                                                                   sensorType);

                            nlohmann::json::object_t member;
                            member["@odata.id"] = boost::urls::format(
                                "/redfish/v1/Chassis/{}/{}/{}",
                                sensorsAsyncResp->chassisId,
                                sensorsAsyncResp->chassisSubNode, sensorId);
                            tempArray.emplace_back(std::move(member));
                            sensorJson = &(tempArray.back());
                        }
                        else
                        {
                            nlohmann::json::object_t member;
                            boost::urls::url url = boost::urls::format(
                                "/redfish/v1/Chassis/{}/{}",
                                sensorsAsyncResp->chassisId,
                                sensorsAsyncResp->chassisSubNode);
                            url.set_fragment(
                                (""_json_pointer / fieldName).to_string());
                            member["@odata.id"] = std::move(url);
                            tempArray.emplace_back(std::move(member));
                            sensorJson = &(tempArray.back());
                        }
                    }

                    if (sensorJson != nullptr)
                    {
                        objectInterfacesToJson(
                            sensorName, sensorType, chassisSubNode,
                            objDictEntry.second, *sensorJson, inventoryItem);

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
                    if (chassisSubNode ==
                            sensor_utils::ChassisSubNode::sensorsNode &&
                        sensorsAsyncResp->efficientExpand)
                    {
                        sensorsAsyncResp->asyncResp->res
                            .jsonValue["Members@odata.count"] =
                            sensorsAsyncResp->asyncResp->res
                                .jsonValue["Members"]
                                .size();
                    }
                    else if (chassisSubNode ==
                             sensor_utils::ChassisSubNode::thermalNode)
                    {
                        populateFanRedundancy(sensorsAsyncResp);
                    }
                }
                BMCWEB_LOG_DEBUG("getManagedObjectsCb exit");
            });
    }
    BMCWEB_LOG_DEBUG("getSensorData exit");
}

/**
 * @brief Create connections necessary for sensors
 * @param SensorsAsyncResp Pointer to object holding response data
 * @param sensorNames Sensors retrieved from chassis
 * @param callback Callback for processing gathered connections
 */
template <typename Callback>
void getConnections(const std::shared_ptr<SensorsAsyncResp>& sensorsAsyncResp,
                    const std::shared_ptr<std::set<std::string>>& sensorNames,
                    Callback&& callback)
{
    auto objectsWithConnectionCb =
        [callback = std::forward<Callback>(callback)](
            const std::set<std::string>& connections,
            const std::set<std::pair<std::string, std::string>>&
            /*objectsWithConnection*/) { callback(connections); };
    getObjectsWithConnection(sensorsAsyncResp, sensorNames,
                             std::move(objectsWithConnectionCb));
}

inline void processSensorList(
    const std::shared_ptr<SensorsAsyncResp>& sensorsAsyncResp,
    const std::shared_ptr<std::set<std::string>>& sensorNames)
{
    auto getConnectionCb = [sensorsAsyncResp, sensorNames](
                               const std::set<std::string>& connections) {
        BMCWEB_LOG_DEBUG("getConnectionCb enter");
        auto getInventoryItemsCb =
            [sensorsAsyncResp, sensorNames, connections](
                const std::shared_ptr<std::vector<sensor_utils::InventoryItem>>&
                    inventoryItems) mutable {
                BMCWEB_LOG_DEBUG("getInventoryItemsCb enter");
                // Get sensor data and store results in JSON
                getSensorData(sensorsAsyncResp, sensorNames, connections,
                              inventoryItems);
                BMCWEB_LOG_DEBUG("getInventoryItemsCb exit");
            };

        // Get inventory items associated with sensors
        getInventoryItems(sensorsAsyncResp, sensorNames,
                          std::move(getInventoryItemsCb));

        BMCWEB_LOG_DEBUG("getConnectionCb exit");
    };

    // Get set of connections that provide sensor values
    getConnections(sensorsAsyncResp, sensorNames, std::move(getConnectionCb));
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
    BMCWEB_LOG_DEBUG("getChassis enter");

    // Get the Chassis Collection
    dbus::utility::getSubTreePaths(
        "/xyz/openbmc_project/inventory", 0, chassisInterfaces,
        [callback = std::forward<Callback>(callback), asyncResp,
         chassisIdStr{std::string(chassisId)},
         chassisSubNode{std::string(chassisSubNode)},
         sensorTypes](const boost::system::error_code& ec,
                      const dbus::utility::MapperGetSubTreePathsResponse&
                          chassisPaths) mutable {
            BMCWEB_LOG_DEBUG("getChassis respHandler enter");
            if (ec)
            {
                BMCWEB_LOG_ERROR("getChassis respHandler DBUS error: {}", ec);
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
                    BMCWEB_LOG_ERROR("Failed to find '/' in {}", chassis);
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
                messages::resourceNotFound(asyncResp->res, "Chassis",
                                           chassisIdStr);
                return;
            }
            populateChassisNode(asyncResp->res.jsonValue, chassisSubNode);

            asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
                "/redfish/v1/Chassis/{}/{}", chassisIdStr, chassisSubNode);

            // Get the list of all sensors for this Chassis element
            std::string sensorPath = *chassisPath + "/all_sensors";
            dbus::utility::getAssociationEndPoints(
                sensorPath, [asyncResp, chassisSubNode, sensorTypes,
                             callback = std::forward<Callback>(callback)](
                                const boost::system::error_code& ec2,
                                const dbus::utility::MapperEndPoints&
                                    nodeSensorList) mutable {
                    if (ec2)
                    {
                        if (ec2.value() != EBADR)
                        {
                            messages::internalError(asyncResp->res);
                            return;
                        }
                    }
                    const std::shared_ptr<std::set<std::string>>
                        culledSensorList =
                            std::make_shared<std::set<std::string>>();
                    reduceSensorList(asyncResp->res, chassisSubNode,
                                     sensorTypes, &nodeSensorList,
                                     culledSensorList);
                    BMCWEB_LOG_DEBUG("Finishing with {}",
                                     culledSensorList->size());
                    callback(culledSensorList);
                });
        });
    BMCWEB_LOG_DEBUG("getChassis exit");
}

/**
 * @brief Entry point for retrieving sensors data related to requested
 *        chassis.
 * @param SensorsAsyncResp   Pointer to object holding response data
 */
static void getChassisData(
    const std::shared_ptr<SensorsAsyncResp>& sensorsAsyncResp)
{
    BMCWEB_LOG_DEBUG("getChassisData enter");
    auto getChassisCb =
        [sensorsAsyncResp](
            const std::shared_ptr<std::set<std::string>>& sensorNames) {
            BMCWEB_LOG_DEBUG("getChassisCb enter");
            processSensorList(sensorsAsyncResp, sensorNames);
            BMCWEB_LOG_DEBUG("getChassisCb exit");
        };
    // SensorCollection doesn't contain the Redundancy property
    if (sensorsAsyncResp->chassisSubNode != sensors::sensorsNodeStr)
    {
        sensorsAsyncResp->asyncResp->res.jsonValue["Redundancy"] =
            nlohmann::json::array();
    }
    // Get set of sensors in chassis
    getChassis(sensorsAsyncResp->asyncResp, sensorsAsyncResp->chassisId,
               sensorsAsyncResp->chassisSubNode, sensorsAsyncResp->types,
               std::move(getChassisCb));
    BMCWEB_LOG_DEBUG("getChassisData exit");
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
    std::unordered_map<std::string, std::vector<nlohmann::json::object_t>>&
        allCollections)
{
    BMCWEB_LOG_INFO("setSensorsOverride for subNode{}",
                    sensorAsyncResp->chassisSubNode);

    std::string_view propertyValueName;
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
            if (!json_util::readJsonObject(                //
                    item, sensorAsyncResp->asyncResp->res, //
                    "MemberId", memberId,                  //
                    propertyValueName, value               //
                    ))
            {
                return;
            }
            overrideMap.emplace(memberId,
                                std::make_pair(value, collectionItems.first));
        }
    }

    auto getChassisSensorListCb = [sensorAsyncResp, overrideMap,
                                   propertyValueNameStr =
                                       std::string(propertyValueName)](
                                      const std::shared_ptr<
                                          std::set<std::string>>& sensorsList) {
        // Match sensor names in the PATCH request to those managed by the
        // chassis node
        const std::shared_ptr<std::set<std::string>> sensorNames =
            std::make_shared<std::set<std::string>>();
        for (const auto& item : overrideMap)
        {
            const auto& sensor = item.first;
            std::pair<std::string, std::string> sensorNameType =
                redfish::sensor_utils::splitSensorNameAndType(sensor);
            if (!findSensorNameUsingSensorPath(sensorNameType.second,
                                               *sensorsList, *sensorNames))
            {
                BMCWEB_LOG_INFO("Unable to find memberId {}", item.first);
                messages::resourceNotFound(sensorAsyncResp->asyncResp->res,
                                           item.second.second, item.first);
                return;
            }
        }
        // Get the connection to which the memberId belongs
        auto getObjectsWithConnectionCb = [sensorAsyncResp, overrideMap,
                                           propertyValueNameStr](
                                              const std::set<
                                                  std::string>& /*connections*/,
                                              const std::set<std::pair<
                                                  std::string, std::string>>&
                                                  objectsWithConnection) {
            if (objectsWithConnection.size() != overrideMap.size())
            {
                BMCWEB_LOG_INFO(
                    "Unable to find all objects with proper connection {} requested {}",
                    objectsWithConnection.size(), overrideMap.size());
                messages::resourceNotFound(
                    sensorAsyncResp->asyncResp->res,
                    sensorAsyncResp->chassisSubNode == sensors::thermalNodeStr
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
                std::string id = redfish::sensor_utils::getSensorId(
                    sensorName, path.parent_path().filename());

                const auto& iterator = overrideMap.find(id);
                if (iterator == overrideMap.end())
                {
                    BMCWEB_LOG_INFO("Unable to find sensor object{}",
                                    item.first);
                    messages::internalError(sensorAsyncResp->asyncResp->res);
                    return;
                }
                setDbusProperty(sensorAsyncResp->asyncResp,
                                propertyValueNameStr, item.second, item.first,
                                "xyz.openbmc_project.Sensor.Value", "Value",
                                iterator->second.first);
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

} // namespace redfish
