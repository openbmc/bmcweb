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

#include "dbus_singleton.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/container/flat_map.hpp>
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
    SensorsAsyncResp(crow::Response& response,
                     const std::initializer_list<const char*> types,
                     const std::string& subNode) :
        res(response),
        types(types), chassisSubNode(subNode)
    {
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
    std::string nodeId{};
    const std::vector<const char*> types;
    std::string chassisSubNode{};
    std::string sensorPath{};
};

/**
 * @brief Creates a sensor entry collecting the sensors current state
 * @param sensorName  The name of the sensor
 * @param sensorType  The type of the sensor (temperature, fan_tach, etc)
 * @param sensorData  A collection of all of the sensor readings provided by the
 * system
 * @param sensor_json The Redfish response data
 */
void addSensorValuesToRFResponse(
    const std::string& sensorName, const std::string& sensorType,
    const boost::container::flat_map<
        std::string, boost::container::flat_map<std::string, SensorVariant>>&
        sensorData,
    nlohmann::json& sensor_json)
{
    // We need a value interface before we can do anything with it
    auto valueIt = sensorData.find("xyz.openbmc_project.Sensor.Value");
    if (valueIt == sensorData.end())
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
        auto interfaceProperties = sensorData.find(std::get<0>(p));
        if (interfaceProperties != sensorData.end())
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

                        std::string nodeName = sensorsAsyncResp->nodeId.empty()
                                                   ? sensorsAsyncResp->chassisId
                                                   : sensorsAsyncResp->nodeId;
                        auto found =
                            std::find_if(endpoints->begin(), endpoints->end(),
                                         [nodeName](const std::string& entry) {
                                             return entry.find(nodeName) !=
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
                                if (sensorsAsyncResp->nodeId.empty())
                                {
                                    resp.push_back(
                                        {{"@odata.id",
                                          "/refish/v1/Chassis/" +
                                              sensorsAsyncResp->chassisId +
                                              "/" +
                                              sensorsAsyncResp->chassisSubNode +
                                              "#/Redundancy/" +
                                              std::to_string(resp.size())},
                                         {"@odata.type",
                                          "#Redundancy.v1_3_2.Redundancy"},
                                         {"MinNumNeeded", collection->size() -
                                                              *allowedFailures},
                                         {"MemberId", name},
                                         {"Mode", "N+m"},
                                         {"Name", name},
                                         {"RedundancySet", redfishCollection},
                                         {"Status",
                                          {{"Health", health},
                                           {"State", "Enabled"}}}});
                                }
                                else
                                {
                                    resp.push_back(
                                        {{"@odata.id",
                                          "/refish/v1/Chassis/" +
                                              sensorsAsyncResp->chassisId +
                                              "/" + sensorsAsyncResp->nodeId +
                                              "/" +
                                              sensorsAsyncResp->chassisSubNode +
                                              "#/Redundancy/" +
                                              std::to_string(resp.size())},
                                         {"@odata.type",
                                          "#Redundancy.v1_3_2.Redundancy"},
                                         {"MinNumNeeded", collection->size() -
                                                              *allowedFailures},
                                         {"MemberId", name},
                                         {"Mode", "N+m"},
                                         {"Name", name},
                                         {"RedundancySet", redfishCollection},
                                         {"Status",
                                          {{"Health", health},
                                           {"State", "Enabled"}}}});
                                }
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

void addSensorToRedfishResponse(
    std::shared_ptr<SensorsAsyncResp> SensorsAsyncResp,
    const std::string& objPath,
    const boost::container::flat_map<
        std::string, boost::container::flat_map<std::string, SensorVariant>>&
        sensorData)
{
    BMCWEB_LOG_DEBUG << __func__ << " parsing object " << objPath;

    std::vector<std::string> split;
    // Reserve space for /xyz/openbmc_project/sensors/<name>/<subname>
    split.reserve(6);
    boost::algorithm::split(split, objPath, boost::is_any_of("/"));
    if (split.size() < 6)
    {
        BMCWEB_LOG_ERROR << "Got path that isn't long enough " << objPath;
        return;
    }
    // These indexes aren't intuitive, as boost::split puts an empty string at
    // the beginning
    const std::string& sensorType = split[4];
    const std::string& sensorName = split[5];
    BMCWEB_LOG_DEBUG << "sensorName " << sensorName << " sensorType "
                     << sensorType;

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
        BMCWEB_LOG_ERROR << "Unsure how to handle sensorType " << sensorType;
        return;
    }

    nlohmann::json& tempArray = SensorsAsyncResp->res.jsonValue[fieldName];

    if (SensorsAsyncResp->nodeId.empty())
    {
        tempArray.push_back(
            {{"@odata.id",
              "/redfish/v1/Chassis/" + SensorsAsyncResp->chassisId + "/" +
                  SensorsAsyncResp->chassisSubNode + "#/" + fieldName + "/"}});
    }
    else
    {
        tempArray.push_back(
            {{"@odata.id",
              "/redfish/v1/Chassis/" + SensorsAsyncResp->chassisId + "/" +
                  SensorsAsyncResp->nodeId + "/" +
                  SensorsAsyncResp->chassisSubNode + "#/" + fieldName + "/"}});
    }
    nlohmann::json& sensorJson = tempArray.back();

    addSensorValuesToRFResponse(sensorName, sensorType, sensorData, sensorJson);
}

void overrideSensor(
    std::shared_ptr<SensorsAsyncResp> SensorsAsyncResp,
    const std::shared_ptr<boost::container::flat_map<std::string, std::string>>
        sensorData,
    std::shared_ptr<
        boost::container::flat_map<std::string, std::pair<double, std::string>>>
        overrideMap)
{
    BMCWEB_LOG_DEBUG << __func__ << " enter";
    for (auto entry : *overrideMap)
    {
        bool sensorFound = false;
        for (auto sname : *sensorData)
        {
            auto lastPos = sname.first.rfind('/');
            if (lastPos == std::string::npos)
            {
                messages::internalError(SensorsAsyncResp->res);
                return;
            }
            const std::string& sensorName = sname.first.substr(lastPos + 1);
            if (sensorName == entry.first)
            {
                sensorFound = true;
                crow::connections::systemBus->async_method_call(
                    [SensorsAsyncResp](const boost::system::error_code ec) {
                        if (ec)
                        {
                            BMCWEB_LOG_DEBUG
                                << "setOverrideValueStatus DBUS error: " << ec;
                            messages::internalError(SensorsAsyncResp->res);
                            return;
                        }
                    },
                    sname.second, sname.first,
                    "org.freedesktop.DBus.Properties", "Set",
                    "xyz.openbmc_project.Sensor.Value", "Value",
                    sdbusplus::message::variant<double>(entry.second.first));
            }
        }
        if (!sensorFound)
        {
            messages::resourceNotFound(SensorsAsyncResp->res,
                                       SensorsAsyncResp->chassisSubNode,
                                       entry.first);
        }
    }
    BMCWEB_LOG_DEBUG << __func__ << " exit";
}

void sortJSONResponse(std::shared_ptr<SensorsAsyncResp> SensorsAsyncResp)
{
    nlohmann::json& response = SensorsAsyncResp->res.jsonValue;
    std::array<std::string, 2> sensorHeaders{"Temperatures", "Fans"};
    if (SensorsAsyncResp->chassisSubNode == "Power")
    {
        sensorHeaders = {"Voltages", "Powersupplies"};
    }
    for (auto sensorGroup : sensorHeaders)
    {
        auto entry = response.find(sensorGroup);
        if (entry != response.end())
        {
            std::sort(entry->begin(), entry->end(),
                      [](nlohmann::json c1, nlohmann::json c2) {
                          return c1["Name"] < c2["Name"];
                      });
            int count = 0;
            for (auto it = entry->begin(); it != entry->end(); it++)
            {
                nlohmann::json::string_t* value =
                    it->at("@odata.id").get_ptr<nlohmann::json::string_t*>();
                if (value)
                {
                    value->append(std::to_string(count));
                    count++;
                }
            }
        }
    }
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
    const std::shared_ptr<boost::container::flat_map<std::string, std::string>>
        sensorData,
    const std::shared_ptr<boost::container::flat_set<std::string>> connections)
{
    BMCWEB_LOG_DEBUG << __func__ << " enter";
    // Get managed objects from all services exposing sensors
    for (const std::string& connection : *connections)
    {
        // Response handler to process managed objects
        crow::connections::systemBus->async_method_call(
            [SensorsAsyncResp, sensorData](const boost::system::error_code ec,
                                           ManagedObjectsVectorType& resp) {
                if (ec)
                {
                    messages::internalError(SensorsAsyncResp->res);
                    return;
                }
                // Go through all sensors and match to the entries in the
                // dictionary of all sensors in the system
                for (auto sData : *sensorData)
                {
                    for (const auto& objDictEntry : resp)
                    {
                        const std::string& objPath =
                            static_cast<const std::string&>(objDictEntry.first);
                        if (sData.first == objPath)
                        {
                            addSensorToRedfishResponse(
                                SensorsAsyncResp, objPath, objDictEntry.second);
                        }
                    }
                }
                if (SensorsAsyncResp.use_count() == 1)
                {
                    sortJSONResponse(SensorsAsyncResp);
                    if (SensorsAsyncResp->chassisSubNode == "Thermal")
                    {
                        populateFanRedundancy(SensorsAsyncResp);
                    }
                }
            },
            connection, "/", "org.freedesktop.DBus.ObjectManager",
            "GetManagedObjects");
    };
    BMCWEB_LOG_DEBUG << __func__ << " exit";
}

/**
 * @brief Shrinks the list of sensors for processing
 * @param SensorsAysncResp  The class holding the Redfish response
 * @param allSensors  A list of all the sensors associated to the
 * chassis element (i.e. baseboard, front panel, etc...)
 * @param activeSensors A list that is a reduction of the incoming
 * allSensor list.  Eliminate Thermal sensors when a Power request is
 * made, and eliminate Power sensors when a Thermal request is made.
 */
void reduceSensorList(
    std::shared_ptr<SensorsAsyncResp> SensorsAsyncResp,
    const std::vector<std::string>* allSensors,
    std::shared_ptr<boost::container::flat_set<std::string>> activeSensors)
{
    if ((SensorsAsyncResp == nullptr) || (allSensors == nullptr) ||
        (activeSensors == nullptr))
    {
        if (SensorsAsyncResp)
        {
            messages::resourceNotFound(
                SensorsAsyncResp->res, SensorsAsyncResp->chassisSubNode,
                SensorsAsyncResp->chassisSubNode == "Thermal" ? "Temperatures"
                                                              : "Voltages");
        }
        return;
    }
    std::vector<std::string> split;
    split.reserve(6);
    for (const std::string& sensor : *allSensors)
    {
        boost::algorithm::split(split, sensor, boost::is_any_of("/"));
        if (split.size() < 6)
        {
            continue;
        }
        const std::string& sensorType = split[4];
        if ((SensorsAsyncResp->chassisSubNode == "Thermal") &&
            ((sensorType == "fan_pwm") || (sensorType == "fan_tach") ||
             (sensorType == "fan") || sensorType == "temperature"))
        {
            activeSensors->emplace(sensor);
        }
        else if ((SensorsAsyncResp->chassisSubNode == "Power") &&
                 ((sensorType == "voltage") || (sensorType == "current") ||
                  (sensorType == "power")))
        {
            activeSensors->emplace(sensor);
        }
    }
}

/**
 * @brief Creates two lists. One is the containing sensor group
 * (i.e. xyz.openbmc_project.CPUSensor) that needs to be queried for
 * the current sensor data.  The other list matches the sensor name to
 * the object path.
 * @param SensorsAysncResp  The class holding the Redfish response
 * @param allSensors A list of all sensors in the system
 * @param culledSensorList A list of the sensors for which to return
 * data.  These will be all of one "class" (i.e. Thermal type, or
 * Power type).
 * @param nodeSensors A list containind the sensor path, and the
 * sensor group.
 * @param sensorGroupList A list containing only the sensor group
 * containing all the sensors in the incoming list (Example: if there
 * are 7 sensors in the xyz.openbmc_project.CPUSensors, this list will
 * contain only a single instance of xyz.openbmc_project.CPUSensors.
 */
void getSensorsForChassisElement(
    std::shared_ptr<SensorsAsyncResp> SensorsAsyncResp,
    const std::shared_ptr<boost::container::flat_set<std::string>>
        culledSensorList,
    const std::vector<std::pair<
        std::string,
        std::vector<std::pair<std::string, std::vector<std::string>>>>>&
        allSensors,
    const std::shared_ptr<boost::container::flat_map<std::string, std::string>>
        nodeSensors,
    const std::shared_ptr<boost::container::flat_set<std::string>>
        sensorGroupList)
{
    if ((SensorsAsyncResp == nullptr) || (culledSensorList == nullptr) ||
        (sensorGroupList == nullptr) || (nodeSensors == nullptr))
    {
        messages::resourceNotFound(
            SensorsAsyncResp->res, SensorsAsyncResp->chassisSubNode,
            SensorsAsyncResp->chassisSubNode == "Thermal" ? "Temperatures"
                                                          : "Voltages");
        return;
    }

    for (auto sensor : *culledSensorList)
    {
        for (auto sensorEntry : allSensors)
        {
            if (sensorEntry.first == sensor)
            {
                std::pair sdata =
                    std::make_pair(sensor, sensorEntry.second[0].first);
                nodeSensors->emplace(sdata);
                sensorGroupList->emplace(sensorEntry.second[0].first);
                break;
            }
        }
    }
}

/**
 * @brief Acquire the list of sensors associated to this chassis
 * element.
 * @param SensorsAysncResp  The class holding the Redfish response
 * @param overrideMap A list of sensors to override, in the event of a
 * PATCH request.
 */
void getSensorAssociations(
    std::shared_ptr<SensorsAsyncResp> SensorsAsyncResp,
    std::shared_ptr<
        boost::container::flat_map<std::string, std::pair<double, std::string>>>
        overrideMap)
{
    // Acquire a list of sensors provided by this chassis element
    std::string sensorPath = SensorsAsyncResp->sensorPath + "/sensors";
    crow::connections::systemBus->async_method_call(
        [SensorsAsyncResp, overrideMap](
            const boost::system::error_code ec,
            const std::variant<std::vector<std::string>>& variantEndpoints) {
            if (ec)
            {
                messages::internalError(SensorsAsyncResp->res);
                return;
            }
            const std::vector<std::string>* nodeSensorList =
                std::get_if<std::vector<std::string>>(&(variantEndpoints));
            if (nodeSensorList == nullptr)
            {
                messages::resourceNotFound(
                    SensorsAsyncResp->res, SensorsAsyncResp->chassisSubNode,
                    SensorsAsyncResp->chassisSubNode == "Thermal"
                        ? "Temperatures"
                        : "Voltages");
                return;
            }
            const std::shared_ptr<boost::container::flat_set<std::string>>
                culledSensorList =
                    std::make_shared<boost::container::flat_set<std::string>>();
            reduceSensorList(SensorsAsyncResp, nodeSensorList,
                             culledSensorList);
            // Get a list of all the sensors in the system
            crow::connections::systemBus->async_method_call(
                [SensorsAsyncResp, culledSensorList, overrideMap](
                    const boost::system::error_code ec,
                    const std::vector<
                        std::pair<std::string,
                                  std::vector<std::pair<
                                      std::string, std::vector<std::string>>>>>&
                        allSensors) {
                    if (ec)
                    {
                        messages::internalError(SensorsAsyncResp->res);
                        return;
                    }
                    const std::shared_ptr<
                        boost::container::flat_set<std::string>>
                        sensorGroupList = std::make_shared<
                            boost::container::flat_set<std::string>>();
                    const std::shared_ptr<
                        boost::container::flat_map<std::string, std::string>>
                        nodeSensorsList =
                            std::make_shared<boost::container::flat_map<
                                std::string, std::string>>();
                    // match the sensors in the chassis element to the
                    // list of all sensors in the system
                    getSensorsForChassisElement(
                        SensorsAsyncResp, culledSensorList, allSensors,
                        nodeSensorsList, sensorGroupList);
                    if (overrideMap == nullptr)
                    {
                        getSensorData(SensorsAsyncResp, nodeSensorsList,
                                      sensorGroupList);
                    }
                    else
                    {
                        overrideSensor(SensorsAsyncResp, nodeSensorsList,
                                       overrideMap);
                    }
                },
                "xyz.openbmc_project.ObjectMapper",
                "/xyz/openbmc_project/object_mapper",
                "xyz.openbmc_project.ObjectMapper", "GetSubTree",
                "/xyz/openbmc_project/sensors", 2,
                std::array<const char*, 1>{"xyz.openbmc_project.Sensor.Value"});
        },
        "xyz.openbmc_project.ObjectMapper", sensorPath,
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Association", "endpoints");
}

/**
 * @brief Entry point for retrieving sensors data related to requested
 *        chassis.
 * @param SensorsAsyncResp   Pointer to object holding response data
 */
void getChassisData(std::shared_ptr<SensorsAsyncResp> SensorsAsyncResp)
{
    BMCWEB_LOG_DEBUG << __func__ << " enter";
    getSensorAssociations(SensorsAsyncResp, nullptr);
    BMCWEB_LOG_DEBUG << __func__ << " exit";
}

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

    std::unordered_map<std::string, std::vector<nlohmann::json>> allCollections;
    std::optional<std::vector<nlohmann::json>> temperatureCollections;
    std::optional<std::vector<nlohmann::json>> fanCollections;
    std::vector<nlohmann::json> voltageCollections;
    BMCWEB_LOG_INFO << __func__ << " for subNode " << chassisSubNode << "\n";

    auto sensorAsyncResp =
        std::make_shared<SensorsAsyncResp>(res, typeList, chassisSubNode);
    int paramCount = params.size();
    switch (paramCount)
    {
        case 1:
            sensorAsyncResp->chassisId = params[paramCount - 1];
            break;
        case 2:
            sensorAsyncResp->chassisId = params[paramCount - 2];
            sensorAsyncResp->nodeId = params[paramCount - 1];
            break;
        default:
            // this should not be possible
            messages::internalError(sensorAsyncResp->res);
            sensorAsyncResp->res.end();
            return;
    }

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
    std::shared_ptr<
        boost::container::flat_map<std::string, std::pair<double, std::string>>>
        overrideMap = std::make_shared<boost::container::flat_map<
            std::string, std::pair<double, std::string>>>();
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
            overrideMap->emplace(memberId,
                                 std::make_pair(value, collectionItems.first));
        }
    }
    getSensorAssociations(sensorAsyncResp, overrideMap);
}

} // namespace redfish
