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
        chassisId(chassisId),
        res(response), types(types), chassisSubNode(subNode)
    {
        res.jsonValue["@odata.id"] =
            "/redfish/v1/Chassis/" + chassisId + "/Thermal";
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
 * @brief Creates connections necessary for chassis sensors
 * @param SensorsAsyncResp Pointer to object holding response data
 * @param sensorNames Sensors retrieved from chassis
 * @param callback Callback for processing gathered connections
 */
template <typename Callback>
void getConnections(std::shared_ptr<SensorsAsyncResp> SensorsAsyncResp,
                    const boost::container::flat_set<std::string>& sensorNames,
                    Callback&& callback)
{
    BMCWEB_LOG_DEBUG << "getConnections enter";
    const std::string path = "/xyz/openbmc_project/sensors";
    const std::array<std::string, 1> interfaces = {
        "xyz.openbmc_project.Sensor.Value"};

    // Response handler for parsing objects subtree
    auto respHandler = [callback{std::move(callback)}, SensorsAsyncResp,
                        sensorNames](const boost::system::error_code ec,
                                     const GetSubTreeType& subtree) {
        BMCWEB_LOG_DEBUG << "getConnections resp_handler enter";
        if (ec)
        {
            messages::internalError(SensorsAsyncResp->res);
            BMCWEB_LOG_ERROR << "getConnections resp_handler: Dbus error "
                             << ec;
            return;
        }

        BMCWEB_LOG_DEBUG << "Found " << subtree.size() << " subtrees";

        // Make unique list of connections only for requested sensor types and
        // found in the chassis
        boost::container::flat_set<std::string> connections;
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
            for (const char* type : SensorsAsyncResp->types)
            {
                if (boost::starts_with(object.first, type))
                {
                    auto lastPos = object.first.rfind('/');
                    if (lastPos != std::string::npos)
                    {
                        std::string sensorName =
                            object.first.substr(lastPos + 1);

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
                            }
                        }
                    }
                    break;
                }
            }
        }
        BMCWEB_LOG_DEBUG << "Found " << connections.size() << " connections";
        callback(std::move(connections));
        BMCWEB_LOG_DEBUG << "getConnections resp_handler exit";
    };

    // Make call to ObjectMapper to find all sensors objects
    crow::connections::systemBus->async_method_call(
        std::move(respHandler), "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree", path, 2, interfaces);
    BMCWEB_LOG_DEBUG << "getConnections exit";
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
    properties.emplace_back("xyz.openbmc_project.Sensor.Threshold.Warning",
                            "WarningHigh", "UpperThresholdNonCritical");
    properties.emplace_back("xyz.openbmc_project.Sensor.Threshold.Warning",
                            "WarningLow", "LowerThresholdNonCritical");
    properties.emplace_back("xyz.openbmc_project.Sensor.Threshold.Critical",
                            "CriticalHigh", "UpperThresholdCritical");
    properties.emplace_back("xyz.openbmc_project.Sensor.Threshold.Critical",
                            "CriticalLow", "LowerThresholdCritical");

    // TODO Need to get UpperThresholdFatal and LowerThresholdFatal

    if (sensorType == "temperature")
    {
        properties.emplace_back("xyz.openbmc_project.Sensor.Value", "MinValue",
                                "MinReadingRangeTemp");
        properties.emplace_back("xyz.openbmc_project.Sensor.Value", "MaxValue",
                                "MaxReadingRangeTemp");
    }
    else
    {
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
                // Get managed objects from all services exposing sensors
                for (const std::string& connection : connections)
                {
                    // Response handler to process managed objects
                    auto getManagedObjectsCb =
                        [&, SensorsAsyncResp,
                         sensorNames](const boost::system::error_code ec,
                                      ManagedObjectsVectorType& resp) {
                            BMCWEB_LOG_DEBUG << "getManagedObjectsCb enter";
                            if (ec)
                            {
                                BMCWEB_LOG_ERROR
                                    << "getManagedObjectsCb DBUS error: " << ec;
                                messages::internalError(SensorsAsyncResp->res);
                                return;
                            }
                            // Go through all objects and update response with
                            // sensor data
                            for (const auto& objDictEntry : resp)
                            {
                                const std::string& objPath =
                                    static_cast<const std::string&>(
                                        objDictEntry.first);
                                BMCWEB_LOG_DEBUG
                                    << "getManagedObjectsCb parsing object "
                                    << objPath;

                                std::vector<std::string> split;
                                // Reserve space for
                                // /xyz/openbmc_project/sensors/<name>/<subname>
                                split.reserve(6);
                                boost::algorithm::split(split, objPath,
                                                        boost::is_any_of("/"));
                                if (split.size() < 6)
                                {
                                    BMCWEB_LOG_ERROR
                                        << "Got path that isn't long enough "
                                        << objPath;
                                    continue;
                                }
                                // These indexes aren't intuitive, as
                                // boost::split puts an empty string at the
                                // beggining
                                const std::string& sensorType = split[4];
                                const std::string& sensorName = split[5];
                                BMCWEB_LOG_DEBUG << "sensorName " << sensorName
                                                 << " sensorType "
                                                 << sensorType;
                                if (sensorNames.find(sensorName) ==
                                    sensorNames.end())
                                {
                                    BMCWEB_LOG_ERROR << sensorName
                                                     << " not in sensor list ";
                                    continue;
                                }

                                const char* fieldName = nullptr;
                                if (sensorType == "temperature")
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
                                else if (sensorType == "current")
                                {
                                    fieldName = "PowerSupply";
                                }
                                else if (sensorType == "power")
                                {
                                    fieldName = "PowerSupply";
                                }
                                else
                                {
                                    BMCWEB_LOG_ERROR
                                        << "Unsure how to handle sensorType "
                                        << sensorType;
                                    continue;
                                }

                                nlohmann::json& tempArray =
                                    SensorsAsyncResp->res.jsonValue[fieldName];

                                tempArray.push_back(
                                    {{"@odata.id",
                                      "/redfish/v1/Chassis/" +
                                          SensorsAsyncResp->chassisId + "/" +
                                          SensorsAsyncResp->chassisSubNode +
                                          "#/" + fieldName + "/" +
                                          std::to_string(tempArray.size())}});
                                nlohmann::json& sensorJson = tempArray.back();

                                objectInterfacesToJson(sensorName, sensorType,
                                                       objDictEntry.second,
                                                       sensorJson);
                            }
                            BMCWEB_LOG_DEBUG << "getManagedObjectsCb exit";
                        };
                    crow::connections::systemBus->async_method_call(
                        getManagedObjectsCb, connection, "/",
                        "org.freedesktop.DBus.ObjectManager",
                        "GetManagedObjects");
                };
                BMCWEB_LOG_DEBUG << "getConnectionCb exit";
            };
        // get connections and then pass it to get sensors
        getConnections(SensorsAsyncResp, sensorNames,
                       std::move(getConnectionCb));
        BMCWEB_LOG_DEBUG << "getChassisCb exit";
    };

    // get chassis information related to sensors
    getChassis(SensorsAsyncResp, std::move(getChassisCb));
    BMCWEB_LOG_DEBUG << "getChassisData exit";
};

} // namespace redfish
