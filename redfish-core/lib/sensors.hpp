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

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/range/algorithm/replace_copy_if.hpp>
#include <dbus_singleton.hpp>
#include <utils/json_utils.hpp>
#include <variant>

namespace redfish
{

constexpr const char* dbusSensorPrefix = "/xyz/openbmc_project/sensors/";

using GetSubTreeType = std::vector<
    std::pair<std::string,
              std::vector<std::pair<std::string, std::vector<std::string>>>>>;

using SensorVariant = std::variant<int64_t, double>;

using ManagedObjectsVectorType = std::vector<std::pair<
    sdbusplus::message::object_path,
    boost::container::flat_map<
        std::string, boost::container::flat_map<std::string, SensorVariant>>>>;

/**
 * SensorsAsyncResp
 * Gathers data needed for response processing after async calls are done
 */
class SensorsAsyncResp
{
  public:
    SensorsAsyncResp(crow::Response& response, const std::string& chassisId,
                     const std::initializer_list<const char*> types,
                     const std::string& subNode) :
        res(response),
        chassisId(chassisId), types(types), chassisSubNode(subNode)
    {
        res.jsonValue["@odata.id"] =
            "/redfish/v1/Chassis/" + chassisId + "/" + subNode;
    }

    ~SensorsAsyncResp()
    {
        if (res.result() == boost::beast::http::status::internal_server_error)
        {
            // Reset the json object to clear out any data that made it in
            // before the error happened todo(ed) handle error condition with
            // proper code
            res.jsonValue = nlohmann::json::object();
        }
        res.end();
    }

    crow::Response& res;
    std::string chassisId{};
    const std::vector<const char*> types;
    std::string chassisSubNode{};
};

/**
 * @brief Checks whether the specified sensor is one of the requested types.
 * @param objectPath DBus object path of the sensor.
 * @param SensorsAsyncResp Pointer to object holding response data and requested
 * sensor types.
 * @return true if sensor type is requested, false otherwise.
 */
inline bool
    isRequestedSensorType(const std::string& objectPath,
                          std::shared_ptr<SensorsAsyncResp> SensorsAsyncResp)
{
    for (const char* type : SensorsAsyncResp->types)
    {
        if (boost::starts_with(objectPath, type))
        {
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
    std::shared_ptr<SensorsAsyncResp> SensorsAsyncResp,
    const boost::container::flat_set<std::string>& sensorNames,
    Callback&& callback)
{
    BMCWEB_LOG_DEBUG << "getObjectsWithConnection enter";
    const std::string path = "/xyz/openbmc_project/sensors";
    const std::array<std::string, 1> interfaces = {
        "xyz.openbmc_project.Sensor.Value"};

    // Response handler for parsing objects subtree
    auto respHandler = [callback{std::move(callback)}, SensorsAsyncResp,
                        sensorNames](const boost::system::error_code ec,
                                     const GetSubTreeType& subtree) {
        BMCWEB_LOG_DEBUG << "getObjectsWithConnection resp_handler enter";
        if (ec)
        {
            messages::internalError(SensorsAsyncResp->res);
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

        BMCWEB_LOG_DEBUG << "sensorNames list count: " << sensorNames.size();
        for (const std::string& tsensor : sensorNames)
        {
            BMCWEB_LOG_DEBUG << "Sensor to find: " << tsensor;
        }

        for (const std::pair<
                 std::string,
                 std::vector<std::pair<std::string, std::vector<std::string>>>>&
                 object : subtree)
        {
            if (isRequestedSensorType(object.first, SensorsAsyncResp))
            {
                auto lastPos = object.first.rfind('/');
                if (lastPos != std::string::npos)
                {
                    std::string sensorName = object.first.substr(lastPos + 1);

                    if (sensorNames.find(sensorName) != sensorNames.end())
                    {
                        // For each Connection name
                        for (const std::pair<std::string,
                                             std::vector<std::string>>&
                                 objData : object.second)
                        {
                            BMCWEB_LOG_DEBUG << "Adding connection: "
                                             << objData.first;
                            connections.insert(objData.first);
                            objectsWithConnection.insert(
                                std::make_pair(object.first, objData.first));
                        }
                    }
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
void getConnections(std::shared_ptr<SensorsAsyncResp> SensorsAsyncResp,
                    const boost::container::flat_set<std::string>& sensorNames,
                    Callback&& callback)
{
    auto objectsWithConnectionCb =
        [callback](const boost::container::flat_set<std::string>& connections,
                   const std::set<std::pair<std::string, std::string>>&
                       objectsWithConnection) {
            callback(std::move(connections));
        };
    getObjectsWithConnection(SensorsAsyncResp, sensorNames,
                             std::move(objectsWithConnectionCb));
}

/**
 * @brief Gets all DBus sensor names.
 *
 * Finds the sensor names asynchronously.  Invokes callback when information has
 * been obtained.
 *
 * The callback must have the following signature:
 *   @code
 *   callback(boost::container::flat_set<std::string>& sensorNames)
 *   @endcode
 *
 * @param SensorsAsyncResp Pointer to object holding response data.
 * @param callback Callback to invoke when sensor names obtained.
 */
template <typename Callback>
void getAllSensors(std::shared_ptr<SensorsAsyncResp> SensorsAsyncResp,
                   Callback&& callback)
{
    BMCWEB_LOG_DEBUG << "getAllSensors enter";
    const std::array<std::string, 1> interfaces = {
        "xyz.openbmc_project.Sensor.Value"};

    // Response handler for GetSubTreePaths DBus method
    auto respHandler = [callback{std::move(callback)}, SensorsAsyncResp](
                           const boost::system::error_code ec,
                           const std::vector<std::string>& sensorList) {
        BMCWEB_LOG_DEBUG << "getAllSensors respHandler enter";
        if (ec)
        {
            messages::internalError(SensorsAsyncResp->res);
            BMCWEB_LOG_ERROR << "getAllSensors respHandler: DBus error " << ec;
            return;
        }
        boost::container::flat_set<std::string> sensorNames;
        for (const std::string& sensor : sensorList)
        {
            std::size_t lastPos = sensor.rfind("/");
            if (lastPos == std::string::npos)
            {
                BMCWEB_LOG_ERROR << "Failed to find '/' in " << sensor;
                continue;
            }
            std::string sensorName = sensor.substr(lastPos + 1);
            sensorNames.emplace(sensorName);
            BMCWEB_LOG_DEBUG << "Added sensor name " << sensorName;
        }
        callback(sensorNames);
        BMCWEB_LOG_DEBUG << "getAllSensors respHandler exit";
    };

    // Query mapper for all DBus sensor object paths
    crow::connections::systemBus->async_method_call(
        std::move(respHandler), "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
        "/xyz/openbmc_project/sensors", int32_t(0), interfaces);
    BMCWEB_LOG_DEBUG << "getAllSensors exit";
}

/**
 * @brief Retrieves requested chassis sensors and redundancy data from DBus .
 * @param SensorsAsyncResp   Pointer to object holding response data
 * @param callback  Callback for next step in gathered sensor processing
 */
template <typename Callback>
void getChassis(std::shared_ptr<SensorsAsyncResp> SensorsAsyncResp,
                Callback&& callback)
{
    BMCWEB_LOG_DEBUG << "getChassis enter";
    // Process response from EntityManager and extract chassis data
    auto respHandler = [callback{std::move(callback)},
                        SensorsAsyncResp](const boost::system::error_code ec,
                                          ManagedObjectsVectorType& resp) {
        BMCWEB_LOG_DEBUG << "getChassis respHandler enter";
        if (ec)
        {
            BMCWEB_LOG_ERROR << "getChassis respHandler DBUS error: " << ec;
            messages::internalError(SensorsAsyncResp->res);
            return;
        }
        boost::container::flat_set<std::string> sensorNames;

        //   SensorsAsyncResp->chassisId
        bool foundChassis = false;
        std::vector<std::string> split;
        // Reserve space for
        // /xyz/openbmc_project/inventory/<name>/<subname> + 3 subnames
        split.reserve(8);

        for (const auto& objDictEntry : resp)
        {
            const std::string& objectPath =
                static_cast<const std::string&>(objDictEntry.first);
            boost::algorithm::split(split, objectPath, boost::is_any_of("/"));
            if (split.size() < 2)
            {
                BMCWEB_LOG_ERROR << "Got path that isn't long enough "
                                 << objectPath;
                split.clear();
                continue;
            }
            const std::string& sensorName = split.end()[-1];
            const std::string& chassisName = split.end()[-2];

            if (chassisName != SensorsAsyncResp->chassisId)
            {
                split.clear();
                continue;
            }
            BMCWEB_LOG_DEBUG << "New sensor: " << sensorName;
            foundChassis = true;
            sensorNames.emplace(sensorName);
            split.clear();
        };
        BMCWEB_LOG_DEBUG << "Found " << sensorNames.size() << " Sensor names";

        if (!foundChassis)
        {
            BMCWEB_LOG_INFO << "Unable to find chassis named "
                            << SensorsAsyncResp->chassisId;
            messages::resourceNotFound(SensorsAsyncResp->res, "Chassis",
                                       SensorsAsyncResp->chassisId);
        }
        else
        {
            callback(sensorNames);
        }
        BMCWEB_LOG_DEBUG << "getChassis respHandler exit";
    };

    // Make call to EntityManager to find all chassis objects
    crow::connections::systemBus->async_method_call(
        respHandler, "xyz.openbmc_project.EntityManager", "/",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
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
 *   callback(const boost::container::flat_map<std::string,
 *            std::string>& objectMgrPaths)
 *   @endcode
 *
 * @param SensorsAsyncResp Pointer to object holding response data.
 * @param callback Callback to invoke when object paths obtained.
 */
template <typename Callback>
void getObjectManagerPaths(std::shared_ptr<SensorsAsyncResp> SensorsAsyncResp,
                           Callback&& callback)
{
    BMCWEB_LOG_DEBUG << "getObjectManagerPaths enter";
    const std::array<std::string, 1> interfaces = {
        "org.freedesktop.DBus.ObjectManager"};

    // Response handler for GetSubTree DBus method
    auto respHandler = [callback{std::move(callback)},
                        SensorsAsyncResp](const boost::system::error_code ec,
                                          const GetSubTreeType& subtree) {
        BMCWEB_LOG_DEBUG << "getObjectManagerPaths respHandler enter";
        if (ec)
        {
            messages::internalError(SensorsAsyncResp->res);
            BMCWEB_LOG_ERROR << "getObjectManagerPaths respHandler: DBus error "
                             << ec;
            return;
        }

        // Loop over returned object paths
        boost::container::flat_map<std::string, std::string> objectMgrPaths;
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
                objectMgrPaths[connection] = objectPath;
                BMCWEB_LOG_DEBUG << "Added mapping " << connection << " -> "
                                 << objectPath;
            }
        }
        callback(std::move(objectMgrPaths));
        BMCWEB_LOG_DEBUG << "getObjectManagerPaths respHandler exit";
    };

    // Query mapper for all DBus object paths that implement ObjectManager
    crow::connections::systemBus->async_method_call(
        std::move(respHandler), "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree", "/", int32_t(0),
        interfaces);
    BMCWEB_LOG_DEBUG << "getObjectManagerPaths exit";
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
    nlohmann::json& sensor_json)
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

    sensor_json["MemberId"] = sensorName;
    sensor_json["Name"] = sensorName;
    sensor_json["Status"]["State"] = "Enabled";
    sensor_json["Status"]["Health"] = "OK";

    // Parameter to set to override the type we get from dbus, and force it to
    // int, regardless of what is available.  This is used for schemas like fan,
    // that require integers, not floats.
    bool forceToInt = false;

    const char* unit = "Reading";
    if (sensorType == "temperature")
    {
        unit = "ReadingCelsius";
        sensor_json["@odata.type"] = "#Thermal.v1_3_0.Temperature";
        // TODO(ed) Documentation says that path should be type fan_tach,
        // implementation seems to implement fan
    }
    else if (sensorType == "fan" || sensorType == "fan_tach")
    {
        unit = "Reading";
        sensor_json["ReadingUnits"] = "RPM";
        sensor_json["@odata.type"] = "#Thermal.v1_3_0.Fan";
        forceToInt = true;
    }
    else if (sensorType == "fan_pwm")
    {
        unit = "Reading";
        sensor_json["ReadingUnits"] = "Percent";
        sensor_json["@odata.type"] = "#Thermal.v1_3_0.Fan";
        forceToInt = true;
    }
    else if (sensorType == "voltage")
    {
        unit = "ReadingVolts";
        sensor_json["@odata.type"] = "#Power.v1_0_0.Voltage";
    }
    else if (sensorType == "power")
    {
        unit = "LastPowerOutputWatts";
    }
    else
    {
        BMCWEB_LOG_ERROR << "Redfish cannot map object type for " << sensorName;
        return;
    }
    // Map of dbus interface name, dbus property name and redfish property_name
    std::vector<std::tuple<const char*, const char*, const char*>> properties;
    properties.reserve(7);

    properties.emplace_back("xyz.openbmc_project.Sensor.Value", "Value", unit);

    // If sensor type doesn't map to Redfish PowerSupply, add threshold props
    if ((sensorType != "current") && (sensorType != "power"))
    {
        properties.emplace_back("xyz.openbmc_project.Sensor.Threshold.Warning",
                                "WarningHigh", "UpperThresholdNonCritical");
        properties.emplace_back("xyz.openbmc_project.Sensor.Threshold.Warning",
                                "WarningLow", "LowerThresholdNonCritical");
        properties.emplace_back("xyz.openbmc_project.Sensor.Threshold.Critical",
                                "CriticalHigh", "UpperThresholdCritical");
        properties.emplace_back("xyz.openbmc_project.Sensor.Threshold.Critical",
                                "CriticalLow", "LowerThresholdCritical");
    }

    // TODO Need to get UpperThresholdFatal and LowerThresholdFatal

    if (sensorType == "temperature")
    {
        properties.emplace_back("xyz.openbmc_project.Sensor.Value", "MinValue",
                                "MinReadingRangeTemp");
        properties.emplace_back("xyz.openbmc_project.Sensor.Value", "MaxValue",
                                "MaxReadingRangeTemp");
    }
    else if ((sensorType != "current") && (sensorType != "power"))
    {
        // Sensor type doesn't map to Redfish PowerSupply; add min/max props
        properties.emplace_back("xyz.openbmc_project.Sensor.Value", "MinValue",
                                "MinReadingRange");
        properties.emplace_back("xyz.openbmc_project.Sensor.Value", "MaxValue",
                                "MaxReadingRange");
    }

    for (const std::tuple<const char*, const char*, const char*>& p :
         properties)
    {
        auto interfaceProperties = interfacesDict.find(std::get<0>(p));
        if (interfaceProperties != interfacesDict.end())
        {
            auto valueIt = interfaceProperties->second.find(std::get<1>(p));
            if (valueIt != interfaceProperties->second.end())
            {
                const SensorVariant& valueVariant = valueIt->second;
                nlohmann::json& valueIt = sensor_json[std::get<2>(p)];
                // Attempt to pull the int64 directly
                const int64_t* int64Value = std::get_if<int64_t>(&valueVariant);

                const double* doubleValue = std::get_if<double>(&valueVariant);
                double temp = 0.0;
                if (int64Value != nullptr)
                {
                    temp = *int64Value;
                }
                else if (doubleValue != nullptr)
                {
                    temp = *doubleValue;
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
                    valueIt = static_cast<int64_t>(temp);
                }
                else
                {
                    valueIt = temp;
                }
            }
        }
    }
    BMCWEB_LOG_DEBUG << "Added sensor " << sensorName;
}

static void
    populateFanRedundancy(std::shared_ptr<SensorsAsyncResp> sensorsAsyncResp)
{
    crow::connections::systemBus->async_method_call(
        [sensorsAsyncResp](const boost::system::error_code ec,
                           const GetSubTreeType& resp) {
            if (ec)
            {
                return; // don't have to have this interface
            }
            for (const auto& [path, objDict] : resp)
            {
                if (objDict.empty())
                {
                    continue; // this should be impossible
                }

                const std::string& owner = objDict.begin()->first;
                crow::connections::systemBus->async_method_call(
                    [path, owner,
                     sensorsAsyncResp](const boost::system::error_code ec,
                                       std::variant<std::vector<std::string>>
                                           variantEndpoints) {
                        if (ec)
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
                                const boost::system::error_code ec,
                                const boost::container::flat_map<
                                    std::string,
                                    std::variant<uint8_t,
                                                 std::vector<std::string>,
                                                 std::string>>& ret) {
                                if (ec)
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
                                size_t lastSlash = path.rfind("/");
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
                                    lastSlash = item.rfind("/");
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

                                auto& resp = sensorsAsyncResp->res
                                                 .jsonValue["Redundancy"];
                                resp.push_back(
                                    {{"@odata.id",
                                      "/refish/v1/Chassis/" +
                                          sensorsAsyncResp->chassisId + "/" +
                                          sensorsAsyncResp->chassisSubNode +
                                          "#/Redundancy/" +
                                          std::to_string(resp.size())},
                                     {"@odata.type",
                                      "#Redundancy.v1_3_2.Redundancy"},
                                     {"MinNumNeeded",
                                      collection->size() - *allowedFailures},
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
                    "xyz.openbmc_project.ObjectMapper", path + "/inventory",
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

/**
 * @brief Gets the values of the specified sensors.
 *
 * Stores the results as JSON in the SensorsAsyncResp.
 *
 * Gets the sensor values asynchronously.  Stores the results later when the
 * information has been obtained.
 *
 * The sensorNames set contains all sensors for the current chassis.
 * SensorsAsyncResp contains the requested sensor types.  Only sensors of a
 * requested type are included in the JSON output.
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
 * @param SensorsAsyncResp Pointer to object holding response data.
 * @param sensorNames All sensors within the current chassis.
 * @param connections Connections that provide sensor values.
 * @param objectMgrPaths Mappings from connection name to DBus object path that
 * implements ObjectManager.
 */
void getSensorData(
    std::shared_ptr<SensorsAsyncResp> SensorsAsyncResp,
    const boost::container::flat_set<std::string>& sensorNames,
    const boost::container::flat_set<std::string>& connections,
    const boost::container::flat_map<std::string, std::string>& objectMgrPaths)
{
    BMCWEB_LOG_DEBUG << "getSensorData enter";
    // Get managed objects from all services exposing sensors
    for (const std::string& connection : connections)
    {
        // Response handler to process managed objects
        auto getManagedObjectsCb = [&, SensorsAsyncResp, sensorNames](
                                       const boost::system::error_code ec,
                                       ManagedObjectsVectorType& resp) {
            BMCWEB_LOG_DEBUG << "getManagedObjectsCb enter";
            if (ec)
            {
                BMCWEB_LOG_ERROR << "getManagedObjectsCb DBUS error: " << ec;
                messages::internalError(SensorsAsyncResp->res);
                return;
            }
            // Go through all objects and update response with sensor data
            for (const auto& objDictEntry : resp)
            {
                const std::string& objPath =
                    static_cast<const std::string&>(objDictEntry.first);
                BMCWEB_LOG_DEBUG << "getManagedObjectsCb parsing object "
                                 << objPath;

                // Skip sensor if it is not one of the requested types
                if (!isRequestedSensorType(objPath, SensorsAsyncResp))
                {
                    continue;
                }

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
                if (sensorNames.find(sensorName) == sensorNames.end())
                {
                    BMCWEB_LOG_ERROR << sensorName << " not in sensor list ";
                    continue;
                }

                const char* fieldName = nullptr;
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
                else if (sensorType == "current")
                {
                    fieldName = "PowerSupplies";
                }
                else if (sensorType == "power")
                {
                    fieldName = "PowerSupplies";
                }
                else
                {
                    BMCWEB_LOG_ERROR << "Unsure how to handle sensorType "
                                     << sensorType;
                    continue;
                }

                nlohmann::json& tempArray =
                    SensorsAsyncResp->res.jsonValue[fieldName];

                tempArray.push_back(
                    {{"@odata.id",
                      "/redfish/v1/Chassis/" + SensorsAsyncResp->chassisId +
                          "/" + SensorsAsyncResp->chassisSubNode + "#/" +
                          fieldName + "/" + std::to_string(tempArray.size())}});
                nlohmann::json& sensorJson = tempArray.back();

                objectInterfacesToJson(sensorName, sensorType,
                                       objDictEntry.second, sensorJson);
                            }
                            if (SensorsAsyncResp.use_count() == 1 &&
                                SensorsAsyncResp->chassisSubNode == "Thermal")
                            {
                                populateFanRedundancy(SensorsAsyncResp);
            }
            BMCWEB_LOG_DEBUG << "getManagedObjectsCb exit";
        };

        // Find DBus object path that implements ObjectManager for the current
        // connection.  If no mapping found, default to "/".
        auto iter = objectMgrPaths.find(connection);
        const std::string& objectMgrPath =
            (iter != objectMgrPaths.end()) ? iter->second : "/";
        BMCWEB_LOG_DEBUG << "ObjectManager path for " << connection << " is "
                         << objectMgrPath;

        crow::connections::systemBus->async_method_call(
            getManagedObjectsCb, connection, objectMgrPath,
            "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
    };
    BMCWEB_LOG_DEBUG << "getSensorData exit";
}

/**
 * @brief Entry point for retrieving sensors data related to requested
 *        chassis.
 * @param SensorsAsyncResp   Pointer to object holding response data
 */
void getChassisData(std::shared_ptr<SensorsAsyncResp> SensorsAsyncResp)
{
    BMCWEB_LOG_DEBUG << "getChassisData enter";
    auto getChassisCb = [&, SensorsAsyncResp](
                            boost::container::flat_set<std::string>&
                                sensorNames) {
        BMCWEB_LOG_DEBUG << "getChassisCb enter";
        auto getConnectionCb =
            [&, SensorsAsyncResp, sensorNames](
                const boost::container::flat_set<std::string>& connections) {
                BMCWEB_LOG_DEBUG << "getConnectionCb enter";
                auto getObjectManagerPathsCb =
                    [SensorsAsyncResp, sensorNames,
                     connections](const boost::container::flat_map<
                                  std::string, std::string>& objectMgrPaths) {
                        BMCWEB_LOG_DEBUG << "getObjectManagerPathsCb enter";
                        // Get sensor data and store results in JSON response
                        getSensorData(SensorsAsyncResp, sensorNames,
                                      connections, objectMgrPaths);
                        BMCWEB_LOG_DEBUG << "getObjectManagerPathsCb exit";
                    };

                // Get mapping from connection names to the DBus object paths
                // that implement the ObjectManager interface
                getObjectManagerPaths(SensorsAsyncResp,
                                      std::move(getObjectManagerPathsCb));
                BMCWEB_LOG_DEBUG << "getConnectionCb exit";
            };

        // Get set of connections that provide sensor values
        getConnections(SensorsAsyncResp, sensorNames,
                       std::move(getConnectionCb));
        BMCWEB_LOG_DEBUG << "getChassisCb exit";
    };

#ifdef BMCWEB_ENABLE_REDFISH_ONE_CHASSIS
    // Get all sensor names
    getAllSensors(SensorsAsyncResp, std::move(getChassisCb));
#else
    // Get sensor names in chassis
    getChassis(SensorsAsyncResp, std::move(getChassisCb));
#endif
    BMCWEB_LOG_DEBUG << "getChassisData exit";
};

/**
 * @brief Entry point for overriding sensor values of given sensor
 *
 * @param res   response object
 * @param req   request object
 * @param params   parameter passed for CRUD
 * @param typeList   TypeList of sensors for the resource queried
 * @param chassisSubNode   Chassis Node for which the query has to happen
 */
void setSensorOverride(crow::Response& res, const crow::Request& req,
                       const std::vector<std::string>& params,
                       const std::initializer_list<const char*> typeList,
                       const std::string& chassisSubNode)
{

    // TODO: Need to figure out dynamic way to restrict patch (Set Sensor
    // override) based on another d-bus announcement to be more generic.
    if (params.size() != 1)
    {
        messages::internalError(res);
        res.end();
        return;
    }

    std::unordered_map<std::string, std::vector<nlohmann::json>> allCollections;
    std::optional<std::vector<nlohmann::json>> temperatureCollections;
    std::optional<std::vector<nlohmann::json>> fanCollections;
    std::vector<nlohmann::json> voltageCollections;
    BMCWEB_LOG_INFO << "setSensorOverride for subNode" << chassisSubNode
                    << "\n";

    if (chassisSubNode == "Thermal")
    {
        if (!json_util::readJson(req, res, "Temperatures",
                                 temperatureCollections, "Fans",
                                 fanCollections))
        {
            return;
        }
        if (!temperatureCollections && !fanCollections)
        {
            messages::resourceNotFound(res, "Thermal",
                                       "Temperatures / Voltages");
            res.end();
            return;
        }
        if (temperatureCollections)
        {
            allCollections.emplace("Temperatures",
                                   *std::move(temperatureCollections));
        }
        if (fanCollections)
        {
            allCollections.emplace("Fans", *std::move(fanCollections));
        }
    }
    else if (chassisSubNode == "Power")
    {
        if (!json_util::readJson(req, res, "Voltages", voltageCollections))
        {
            return;
        }
        allCollections.emplace("Voltages", std::move(voltageCollections));
    }
    else
    {
        res.result(boost::beast::http::status::not_found);
        res.end();
        return;
    }

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
            if (!json_util::readJson(item, res, "MemberId", memberId,
                                     propertyValueName, value))
            {
                return;
            }
            overrideMap.emplace(memberId,
                                std::make_pair(value, collectionItems.first));
        }
    }
    const std::string& chassisName = params[0];
    auto sensorAsyncResp = std::make_shared<SensorsAsyncResp>(
        res, chassisName, typeList, chassisSubNode);
    // first check for valid chassis id & sensor in requested chassis.
    auto getChassisSensorListCb = [sensorAsyncResp, overrideMap](
                                      const boost::container::flat_set<
                                          std::string>& sensorLists) {
        boost::container::flat_set<std::string> sensorNames;
        for (const auto& item : overrideMap)
        {
            const auto& sensor = item.first;
            if (sensorLists.find(item.first) == sensorLists.end())
            {
                BMCWEB_LOG_INFO << "Unable to find memberId " << item.first;
                messages::resourceNotFound(sensorAsyncResp->res,
                                           item.second.second, item.first);
                return;
            }
            sensorNames.emplace(sensor);
        }
        // Get the connection to which the memberId belongs
        auto getObjectsWithConnectionCb =
            [sensorAsyncResp, overrideMap](
                const boost::container::flat_set<std::string>& connections,
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
                        sensorAsyncResp->chassisSubNode == "Thermal"
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
                        sdbusplus::message::variant<double>(
                            iterator->second.first));
                }
            };
        // Get object with connection for the given sensor name
        getObjectsWithConnection(sensorAsyncResp, sensorNames,
                                 std::move(getObjectsWithConnectionCb));
    };
    // get full sensor list for the given chassisId and cross verify the sensor.
    getChassis(sensorAsyncResp, std::move(getChassisSensorListCb));
}

} // namespace redfish
