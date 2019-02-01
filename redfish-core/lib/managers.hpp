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

#include <boost/algorithm/string/replace.hpp>
#include <dbus_utility.hpp>
#include <variant>

namespace redfish
{

/**
 * ManagerActionsReset class supports handle POST method for Reset action.
 * The class retrieves and sends data directly to dbus.
 */
class ManagerActionsReset : public Node
{
  public:
    ManagerActionsReset(CrowApp& app) :
        Node(app, "/redfish/v1/Managers/bmc/Actions/Manager.Reset/")
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
    /**
     * Function handles POST method request.
     * Analyzes POST body message before sends Reset request data to dbus.
     * OpenBMC allows for ResetType is GracefulRestart only.
     */
    void doPost(crow::Response& res, const crow::Request& req,
                const std::vector<std::string>& params) override
    {
        std::string resetType;

        if (!json_util::readJson(req, res, "ResetType", resetType))
        {
            return;
        }

        if (resetType != "GracefulRestart")
        {
            res.result(boost::beast::http::status::bad_request);
            messages::actionParameterNotSupported(res, resetType, "ResetType");
            BMCWEB_LOG_ERROR << "Request incorrect action parameter: "
                             << resetType;
            res.end();
            return;
        }
        doBMCGracefulRestart(res, req, params);
    }

    /**
     * Function transceives data with dbus directly.
     * All BMC state properties will be retrieved before sending reset request.
     */
    void doBMCGracefulRestart(crow::Response& res, const crow::Request& req,
                              const std::vector<std::string>& params)
    {
        const char* processName = "xyz.openbmc_project.State.BMC";
        const char* objectPath = "/xyz/openbmc_project/state/bmc0";
        const char* interfaceName = "xyz.openbmc_project.State.BMC";
        const std::string& propertyValue =
            "xyz.openbmc_project.State.BMC.Transition.Reboot";
        const char* destProperty = "RequestedBMCTransition";

        // Create the D-Bus variant for D-Bus call.
        VariantType dbusPropertyValue(propertyValue);

        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);

        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec) {
                // Use "Set" method to set the property value.
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "[Set] Bad D-Bus request error: " << ec;
                    messages::internalError(asyncResp->res);
                    return;
                }

                messages::success(asyncResp->res);
            },
            processName, objectPath, "org.freedesktop.DBus.Properties", "Set",
            interfaceName, destProperty, dbusPropertyValue);
    }
};

static constexpr const char* objectManagerIface =
    "org.freedesktop.DBus.ObjectManager";
static constexpr const char* pidConfigurationIface =
    "xyz.openbmc_project.Configuration.Pid";
static constexpr const char* pidZoneConfigurationIface =
    "xyz.openbmc_project.Configuration.Pid.Zone";
static constexpr const char* stepwiseConfigurationIface =
    "xyz.openbmc_project.Configuration.Stepwise";

static void asyncPopulatePid(const std::string& connection,
                             const std::string& path,
                             std::shared_ptr<AsyncResp> asyncResp)
{

    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code ec,
                    const dbus::utility::ManagedObjectType& managedObj) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << ec;
                asyncResp->res.jsonValue.clear();
                messages::internalError(asyncResp->res);
                return;
            }
            nlohmann::json& configRoot =
                asyncResp->res.jsonValue["Oem"]["OpenBmc"]["Fan"];
            nlohmann::json& fans = configRoot["FanControllers"];
            fans["@odata.type"] = "#OemManager.FanControllers";
            fans["@odata.context"] =
                "/redfish/v1/$metadata#OemManager.FanControllers";
            fans["@odata.id"] = "/redfish/v1/Managers/bmc#/Oem/OpenBmc/"
                                "Fan/FanControllers";

            nlohmann::json& pids = configRoot["PidControllers"];
            pids["@odata.type"] = "#OemManager.PidControllers";
            pids["@odata.context"] =
                "/redfish/v1/$metadata#OemManager.PidControllers";
            pids["@odata.id"] =
                "/redfish/v1/Managers/bmc#/Oem/OpenBmc/Fan/PidControllers";

            nlohmann::json& stepwise = configRoot["StepwiseControllers"];
            stepwise["@odata.type"] = "#OemManager.StepwiseControllers";
            stepwise["@odata.context"] =
                "/redfish/v1/$metadata#OemManager.StepwiseControllers";
            stepwise["@odata.id"] =
                "/redfish/v1/Managers/bmc#/Oem/OpenBmc/Fan/StepwiseControllers";

            nlohmann::json& zones = configRoot["FanZones"];
            zones["@odata.id"] =
                "/redfish/v1/Managers/bmc#/Oem/OpenBmc/Fan/FanZones";
            zones["@odata.type"] = "#OemManager.FanZones";
            zones["@odata.context"] =
                "/redfish/v1/$metadata#OemManager.FanZones";
            configRoot["@odata.id"] =
                "/redfish/v1/Managers/bmc#/Oem/OpenBmc/Fan";
            configRoot["@odata.type"] = "#OemManager.Fan";
            configRoot["@odata.context"] =
                "/redfish/v1/$metadata#OemManager.Fan";

            bool propertyError = false;
            for (const auto& pathPair : managedObj)
            {
                for (const auto& intfPair : pathPair.second)
                {
                    if (intfPair.first != pidConfigurationIface &&
                        intfPair.first != pidZoneConfigurationIface &&
                        intfPair.first != stepwiseConfigurationIface)
                    {
                        continue;
                    }
                    auto findName = intfPair.second.find("Name");
                    if (findName == intfPair.second.end())
                    {
                        BMCWEB_LOG_ERROR << "Pid Field missing Name";
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    const std::string* namePtr =
                        std::get_if<std::string>(&findName->second);
                    if (namePtr == nullptr)
                    {
                        BMCWEB_LOG_ERROR << "Pid Name Field illegal";
                        messages::internalError(asyncResp->res);
                        return;
                    }

                    std::string name = *namePtr;
                    dbus::utility::escapePathForDbus(name);
                    nlohmann::json* config = nullptr;
                    if (intfPair.first == pidZoneConfigurationIface)
                    {
                        std::string chassis;
                        if (!dbus::utility::getNthStringFromPath(
                                pathPair.first.str, 5, chassis))
                        {
                            chassis = "#IllegalValue";
                        }
                        nlohmann::json& zone = zones[name];
                        zone["Chassis"] = {
                            {"@odata.id", "/redfish/v1/Chassis/" + chassis}};
                        zone["@odata.id"] = "/redfish/v1/Managers/bmc#/Oem/"
                                            "OpenBmc/Fan/FanZones/" +
                                            name;
                        zone["@odata.type"] = "#OemManager.FanZone";
                        zone["@odata.context"] =
                            "/redfish/v1/$metadata#OemManager.FanZone";
                        config = &zone;
                    }

                    else if (intfPair.first == stepwiseConfigurationIface)
                    {
                        nlohmann::json& controller = stepwise[name];
                        config = &controller;

                        controller["@odata.id"] =
                            "/redfish/v1/Managers/bmc#/Oem/"
                            "OpenBmc/Fan/StepwiseControllers/" +
                            std::string(name);
                        controller["@odata.type"] =
                            "#OemManager.StepwiseController";

                        controller["@odata.context"] =
                            "/redfish/v1/"
                            "$metadata#OemManager.StepwiseController";
                    }

                    // pid and fans are off the same configuration
                    else if (intfPair.first == pidConfigurationIface)
                    {
                        const std::string* classPtr = nullptr;
                        auto findClass = intfPair.second.find("Class");
                        if (findClass != intfPair.second.end())
                        {
                            classPtr =
                                std::get_if<std::string>(&findClass->second);
                        }
                        if (classPtr == nullptr)
                        {
                            BMCWEB_LOG_ERROR << "Pid Class Field illegal";
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        bool isFan = *classPtr == "fan";
                        nlohmann::json& element =
                            isFan ? fans[name] : pids[name];
                        config = &element;
                        if (isFan)
                        {
                            element["@odata.id"] =
                                "/redfish/v1/Managers/bmc#/Oem/"
                                "OpenBmc/Fan/FanControllers/" +
                                std::string(name);
                            element["@odata.type"] =
                                "#OemManager.FanController";

                            element["@odata.context"] =
                                "/redfish/v1/"
                                "$metadata#OemManager.FanController";
                        }
                        else
                        {
                            element["@odata.id"] =
                                "/redfish/v1/Managers/bmc#/Oem/"
                                "OpenBmc/Fan/PidControllers/" +
                                std::string(name);
                            element["@odata.type"] =
                                "#OemManager.PidController";
                            element["@odata.context"] =
                                "/redfish/v1/$metadata"
                                "#OemManager.PidController";
                        }
                    }
                    else
                    {
                        BMCWEB_LOG_ERROR << "Unexpected configuration";
                        messages::internalError(asyncResp->res);
                        return;
                    }

                    // used for making maps out of 2 vectors
                    const std::vector<double>* keys = nullptr;
                    const std::vector<double>* values = nullptr;

                    for (const auto& propertyPair : intfPair.second)
                    {
                        if (propertyPair.first == "Type" ||
                            propertyPair.first == "Class" ||
                            propertyPair.first == "Name")
                        {
                            continue;
                        }

                        // zones
                        if (intfPair.first == pidZoneConfigurationIface)
                        {
                            const double* ptr =
                                std::get_if<double>(&propertyPair.second);
                            if (ptr == nullptr)
                            {
                                BMCWEB_LOG_ERROR << "Field Illegal "
                                                 << propertyPair.first;
                                messages::internalError(asyncResp->res);
                                return;
                            }
                            (*config)[propertyPair.first] = *ptr;
                        }

                        if (intfPair.first == stepwiseConfigurationIface)
                        {
                            if (propertyPair.first == "Reading" ||
                                propertyPair.first == "Output")
                            {
                                const std::vector<double>* ptr =
                                    std::get_if<std::vector<double>>(
                                        &propertyPair.second);

                                if (ptr == nullptr)
                                {
                                    BMCWEB_LOG_ERROR << "Field Illegal "
                                                     << propertyPair.first;
                                    messages::internalError(asyncResp->res);
                                    return;
                                }

                                if (propertyPair.first == "Reading")
                                {
                                    keys = ptr;
                                }
                                else
                                {
                                    values = ptr;
                                }
                                if (keys && values)
                                {
                                    if (keys->size() != values->size())
                                    {
                                        BMCWEB_LOG_ERROR
                                            << "Reading and Output size don't "
                                               "match ";
                                        messages::internalError(asyncResp->res);
                                        return;
                                    }
                                    nlohmann::json& steps = (*config)["Steps"];
                                    steps = nlohmann::json::array();
                                    for (size_t ii = 0; ii < keys->size(); ii++)
                                    {
                                        steps.push_back(
                                            {{"Target", (*keys)[ii]},
                                             {"Output", (*values)[ii]}});
                                    }
                                }
                            }
                            if (propertyPair.first == "NegativeHysteresis" ||
                                propertyPair.first == "PositiveHysteresis")
                            {
                                const double* ptr =
                                    std::get_if<double>(&propertyPair.second);
                                if (ptr == nullptr)
                                {
                                    BMCWEB_LOG_ERROR << "Field Illegal "
                                                     << propertyPair.first;
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                                (*config)[propertyPair.first] = *ptr;
                            }
                        }

                        // pid and fans are off the same configuration
                        if (intfPair.first == pidConfigurationIface ||
                            intfPair.first == stepwiseConfigurationIface)
                        {

                            if (propertyPair.first == "Zones")
                            {
                                const std::vector<std::string>* inputs =
                                    std::get_if<std::vector<std::string>>(
                                        &propertyPair.second);

                                if (inputs == nullptr)
                                {
                                    BMCWEB_LOG_ERROR
                                        << "Zones Pid Field Illegal";
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                                auto& data = (*config)[propertyPair.first];
                                data = nlohmann::json::array();
                                for (std::string itemCopy : *inputs)
                                {
                                    dbus::utility::escapePathForDbus(itemCopy);
                                    data.push_back(
                                        {{"@odata.id",
                                          "/redfish/v1/Managers/bmc#/Oem/"
                                          "OpenBmc/Fan/FanZones/" +
                                              itemCopy}});
                                }
                            }
                            // todo(james): may never happen, but this
                            // assumes configuration data referenced in the
                            // PID config is provided by the same daemon, we
                            // could add another loop to cover all cases,
                            // but I'm okay kicking this can down the road a
                            // bit

                            else if (propertyPair.first == "Inputs" ||
                                     propertyPair.first == "Outputs")
                            {
                                auto& data = (*config)[propertyPair.first];
                                const std::vector<std::string>* inputs =
                                    std::get_if<std::vector<std::string>>(
                                        &propertyPair.second);

                                if (inputs == nullptr)
                                {
                                    BMCWEB_LOG_ERROR << "Field Illegal "
                                                     << propertyPair.first;
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                                data = *inputs;
                            } // doubles
                            else if (propertyPair.first ==
                                         "FFGainCoefficient" ||
                                     propertyPair.first == "FFOffCoefficient" ||
                                     propertyPair.first == "ICoefficient" ||
                                     propertyPair.first == "ILimitMax" ||
                                     propertyPair.first == "ILimitMin" ||
                                     propertyPair.first == "OutLimitMax" ||
                                     propertyPair.first == "OutLimitMin" ||
                                     propertyPair.first == "PCoefficient" ||
                                     propertyPair.first == "SetPoint" ||
                                     propertyPair.first == "SlewNeg" ||
                                     propertyPair.first == "SlewPos")
                            {
                                const double* ptr =
                                    std::get_if<double>(&propertyPair.second);
                                if (ptr == nullptr)
                                {
                                    BMCWEB_LOG_ERROR << "Field Illegal "
                                                     << propertyPair.first;
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                                (*config)[propertyPair.first] = *ptr;
                            }
                        }
                    }
                }
            }
        },
        connection, path, objectManagerIface, "GetManagedObjects");
}

enum class CreatePIDRet
{
    fail,
    del,
    patch
};

static bool getZonesFromJsonReq(const std::shared_ptr<AsyncResp>& response,
                                std::vector<nlohmann::json>& config,
                                std::vector<std::string>& zones)
{

    for (auto& odata : config)
    {
        std::string path;
        if (!redfish::json_util::readJson(odata, response->res, "@odata.id",
                                          path))
        {
            return false;
        }
        std::string input;
        if (!dbus::utility::getNthStringFromPath(path, 4, input))
        {
            BMCWEB_LOG_ERROR << "Got invalid path " << path;
            BMCWEB_LOG_ERROR << "Illegal Type Zones";
            messages::propertyValueFormatError(response->res, odata.dump(),
                                               "Zones");
            return false;
        }
        boost::replace_all(input, "_", " ");
        zones.emplace_back(std::move(input));
    }
    return true;
}

static CreatePIDRet createPidInterface(
    const std::shared_ptr<AsyncResp>& response, const std::string& type,
    nlohmann::json&& record, const std::string& path,
    const dbus::utility::ManagedObjectType& managedObj, bool createNewObject,
    boost::container::flat_map<std::string, dbus::utility::DbusVariantType>&
        output,
    std::string& chassis)
{

    // common deleter
    if (record == nullptr)
    {
        std::string iface;
        if (type == "PidControllers" || type == "FanControllers")
        {
            iface = pidConfigurationIface;
        }
        else if (type == "FanZones")
        {
            iface = pidZoneConfigurationIface;
        }
        else if (type == "StepwiseControllers")
        {
            iface = stepwiseConfigurationIface;
        }
        else
        {
            BMCWEB_LOG_ERROR << "Line:" << __LINE__ << ", Illegal Type "
                             << type;
            messages::propertyUnknown(response->res, type);
            return CreatePIDRet::fail;
        }
        // delete interface
        crow::connections::systemBus->async_method_call(
            [response, path](const boost::system::error_code ec) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "Error patching " << path << ": " << ec;
                    messages::internalError(response->res);
                }
            },
            "xyz.openbmc_project.EntityManager", path, iface, "Delete");
        return CreatePIDRet::del;
    }

    if (type == "PidControllers" || type == "FanControllers")
    {
        if (createNewObject)
        {
            output["Class"] = type == "PidControllers" ? std::string("temp")
                                                       : std::string("fan");
            output["Type"] = std::string("Pid");
        }

        std::optional<std::vector<nlohmann::json>> zones;
        std::optional<std::vector<std::string>> inputs;
        std::optional<std::vector<std::string>> outputs;
        std::map<std::string, std::optional<double>> doubles;
        if (!redfish::json_util::readJson(
                record, response->res, "Inputs", inputs, "Outputs", outputs,
                "Zones", zones, "FFGainCoefficient",
                doubles["FFGainCoefficient"], "FFOffCoefficient",
                doubles["FFOffCoefficient"], "ICoefficient",
                doubles["ICoefficient"], "ILimitMax", doubles["ILimitMax"],
                "ILimitMin", doubles["ILimitMin"], "OutLimitMax",
                doubles["OutLimitMax"], "OutLimitMin", doubles["OutLimitMin"],
                "PCoefficient", doubles["PCoefficient"], "SetPoint",
                doubles["SetPoint"], "SlewNeg", doubles["SlewNeg"], "SlewPos",
                doubles["SlewPos"]))
        {
            BMCWEB_LOG_ERROR << "Line:" << __LINE__ << ", Illegal Property "
                             << record.dump();
            return CreatePIDRet::fail;
        }
        if (zones)
        {
            std::vector<std::string> zonesStr;
            if (!getZonesFromJsonReq(response, *zones, zonesStr))
            {
                BMCWEB_LOG_ERROR << "Line:" << __LINE__ << ", Illegal Zones";
                return CreatePIDRet::fail;
            }
            output["Zones"] = std::move(zonesStr);
        }
        if (inputs || outputs)
        {
            std::array<std::optional<std::vector<std::string>>*, 2> containers =
                {&inputs, &outputs};
            size_t index = 0;
            for (const auto& containerPtr : containers)
            {
                std::optional<std::vector<std::string>>& container =
                    *containerPtr;
                if (!container)
                {
                    index++;
                    continue;
                }

                for (std::string& value : *container)
                {

                    // try to find the sensor in the
                    // configuration
                    if (chassis.empty())
                    {
                        std::string escaped =
                            boost::replace_all_copy(value, " ", "_");
                        std::find_if(
                            managedObj.begin(), managedObj.end(),
                            [&chassis, &escaped](const auto& obj) {
                                if (boost::algorithm::ends_with(obj.first.str,
                                                                escaped))
                                {
                                    return dbus::utility::getNthStringFromPath(
                                        obj.first.str, 5, chassis);
                                }
                                return false;
                            });
                    }
                    boost::replace_all(value, "_", " ");
                }
                std::string key;
                if (index == 0)
                {
                    key = "Inputs";
                }
                else
                {
                    key = "Outputs";
                }
                output[key] = *container;
                index++;
            }
        }

        // doubles
        for (const auto& pairs : doubles)
        {
            if (!pairs.second)
            {
                continue;
            }
            BMCWEB_LOG_DEBUG << pairs.first << " = " << *pairs.second;
            output[pairs.first] = *(pairs.second);
        }
    }

    else if (type == "FanZones")
    {
        output["Type"] = std::string("Pid.Zone");

        std::optional<nlohmann::json> chassisContainer;
        std::optional<double> failSafePercent;
        std::optional<double> minThermalRpm;
        if (!redfish::json_util::readJson(record, response->res, "Chassis",
                                          chassisContainer, "FailSafePercent",
                                          failSafePercent, "MinThermalRpm",
                                          minThermalRpm))
        {
            BMCWEB_LOG_ERROR << "Line:" << __LINE__ << ", Illegal Property "
                             << record.dump();
            return CreatePIDRet::fail;
        }

        if (chassisContainer)
        {

            std::string chassisId;
            if (!redfish::json_util::readJson(*chassisContainer, response->res,
                                              "@odata.id", chassisId))
            {
                BMCWEB_LOG_ERROR << "Line:" << __LINE__ << ", Illegal Property "
                                 << chassisContainer->dump();
                return CreatePIDRet::fail;
            }

            // /refish/v1/chassis/chassis_name/
            if (!dbus::utility::getNthStringFromPath(chassisId, 3, chassis))
            {
                BMCWEB_LOG_ERROR << "Got invalid path " << chassisId;
                messages::invalidObject(response->res, chassisId);
                return CreatePIDRet::fail;
            }
        }
        if (minThermalRpm)
        {
            output["MinThermalRpm"] = *minThermalRpm;
        }
        if (failSafePercent)
        {
            output["FailSafePercent"] = *failSafePercent;
        }
    }
    else if (type == "StepwiseControllers")
    {
        output["Type"] = std::string("Stepwise");

        std::optional<std::vector<nlohmann::json>> zones;
        std::optional<std::vector<nlohmann::json>> steps;
        std::optional<std::vector<std::string>> inputs;
        std::optional<double> positiveHysteresis;
        std::optional<double> negativeHysteresis;
        if (!redfish::json_util::readJson(
                record, response->res, "Zones", zones, "Steps", steps, "Inputs",
                inputs, "PositiveHysteresis", positiveHysteresis,
                "NegativeHysteresis", negativeHysteresis))
        {
            BMCWEB_LOG_ERROR << "Line:" << __LINE__ << ", Illegal Property "
                             << record.dump();
            return CreatePIDRet::fail;
        }

        if (zones)
        {
            std::vector<std::string> zoneStrs;
            if (!getZonesFromJsonReq(response, *zones, zoneStrs))
            {
                BMCWEB_LOG_ERROR << "Line:" << __LINE__ << ", Illegal Zones";
                return CreatePIDRet::fail;
            }
            output["Zones"] = std::move(zoneStrs);
        }
        if (steps)
        {
            std::vector<double> readings;
            std::vector<double> outputs;
            for (auto& step : *steps)
            {
                double target;
                double output;

                if (!redfish::json_util::readJson(step, response->res, "Target",
                                                  target, "Output", output))
                {
                    BMCWEB_LOG_ERROR << "Line:" << __LINE__
                                     << ", Illegal Property " << record.dump();
                    return CreatePIDRet::fail;
                }
                readings.emplace_back(target);
                outputs.emplace_back(output);
            }
            output["Reading"] = std::move(readings);
            output["Output"] = std::move(outputs);
        }
        if (inputs)
        {
            for (std::string& value : *inputs)
            {
                if (chassis.empty())
                {
                    std::string escaped =
                        boost::replace_all_copy(value, " ", "_");
                    std::find_if(
                        managedObj.begin(), managedObj.end(),
                        [&chassis, &escaped](const auto& obj) {
                            if (boost::algorithm::ends_with(obj.first.str,
                                                            escaped))
                            {
                                return dbus::utility::getNthStringFromPath(
                                    obj.first.str, 5, chassis);
                            }
                            return false;
                        });
                }
                boost::replace_all(value, "_", " ");
            }
            output["Inputs"] = std::move(*inputs);
        }
        if (negativeHysteresis)
        {
            output["NegativeHysteresis"] = *negativeHysteresis;
        }
        if (positiveHysteresis)
        {
            output["PositiveHysteresis"] = *positiveHysteresis;
        }
    }
    else
    {
        BMCWEB_LOG_ERROR << "Line:" << __LINE__ << ", Illegal Type " << type;
        messages::propertyUnknown(response->res, type);
        return CreatePIDRet::fail;
    }
    return CreatePIDRet::patch;
}

class Manager : public Node
{
  public:
    Manager(CrowApp& app) : Node(app, "/redfish/v1/Managers/bmc/")
    {
        uuid = app.template getMiddleware<crow::persistent_data::Middleware>()
                   .systemUuid;
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    void getPidValues(std::shared_ptr<AsyncResp> asyncResp)
    {
        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec,
                        const crow::openbmc_mapper::GetSubTreeType& subtree) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << ec;
                    messages::internalError(asyncResp->res);
                    return;
                }

                // create map of <connection, path to objMgr>>
                boost::container::flat_map<std::string, std::string>
                    objectMgrPaths;
                boost::container::flat_set<std::string> calledConnections;
                for (const auto& pathGroup : subtree)
                {
                    for (const auto& connectionGroup : pathGroup.second)
                    {
                        auto findConnection =
                            calledConnections.find(connectionGroup.first);
                        if (findConnection != calledConnections.end())
                        {
                            break;
                        }
                        for (const std::string& interface :
                             connectionGroup.second)
                        {
                            if (interface == objectManagerIface)
                            {
                                objectMgrPaths[connectionGroup.first] =
                                    pathGroup.first;
                            }
                            // this list is alphabetical, so we
                            // should have found the objMgr by now
                            if (interface == pidConfigurationIface ||
                                interface == pidZoneConfigurationIface ||
                                interface == stepwiseConfigurationIface)
                            {
                                auto findObjMgr =
                                    objectMgrPaths.find(connectionGroup.first);
                                if (findObjMgr == objectMgrPaths.end())
                                {
                                    BMCWEB_LOG_DEBUG << connectionGroup.first
                                                     << "Has no Object Manager";
                                    continue;
                                }

                                calledConnections.insert(connectionGroup.first);

                                asyncPopulatePid(findObjMgr->first,
                                                 findObjMgr->second, asyncResp);
                                break;
                            }
                        }
                    }
                }
            },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetSubTree", "/", 0,
            std::array<const char*, 4>{
                pidConfigurationIface, pidZoneConfigurationIface,
                objectManagerIface, stepwiseConfigurationIface});
    }

    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        res.jsonValue["@odata.id"] = "/redfish/v1/Managers/bmc";
        res.jsonValue["@odata.type"] = "#Manager.v1_3_0.Manager";
        res.jsonValue["@odata.context"] =
            "/redfish/v1/$metadata#Manager.Manager";
        res.jsonValue["Id"] = "bmc";
        res.jsonValue["Name"] = "OpenBmc Manager";
        res.jsonValue["Description"] = "Baseboard Management Controller";
        res.jsonValue["PowerState"] = "On";
        res.jsonValue["Status"] = {{"State", "Enabled"}, {"Health", "OK"}};
        res.jsonValue["ManagerType"] = "BMC";
        res.jsonValue["UUID"] = uuid;
        res.jsonValue["Model"] = "OpenBmc"; // TODO(ed), get model

        res.jsonValue["LogServices"] = {
            {"@odata.id", "/redfish/v1/Managers/bmc/LogServices"}};

        res.jsonValue["NetworkProtocol"] = {
            {"@odata.id", "/redfish/v1/Managers/bmc/NetworkProtocol"}};

        res.jsonValue["EthernetInterfaces"] = {
            {"@odata.id", "/redfish/v1/Managers/bmc/EthernetInterfaces"}};
        // default oem data
        nlohmann::json& oem = res.jsonValue["Oem"];
        nlohmann::json& oemOpenbmc = oem["OpenBmc"];
        oem["@odata.type"] = "#OemManager.Oem";
        oem["@odata.id"] = "/redfish/v1/Managers/bmc#/Oem";
        oem["@odata.context"] = "/redfish/v1/$metadata#OemManager.Oem";
        oemOpenbmc["@odata.type"] = "#OemManager.OpenBmc";
        oemOpenbmc["@odata.id"] = "/redfish/v1/Managers/bmc#/Oem/OpenBmc";
        oemOpenbmc["@odata.context"] =
            "/redfish/v1/$metadata#OemManager.OpenBmc";

        // Update Actions object.
        nlohmann::json& manager_reset =
            res.jsonValue["Actions"]["#Manager.Reset"];
        manager_reset["target"] =
            "/redfish/v1/Managers/bmc/Actions/Manager.Reset";
        manager_reset["ResetType@Redfish.AllowableValues"] = {
            "GracefulRestart"};

        res.jsonValue["DateTime"] = getDateTime();
        res.jsonValue["Links"] = {
            {"ManagerForServers@odata.count", 1},
            {"ManagerForServers",
             {{{"@odata.id", "/redfish/v1/Systems/system"}}}},
            {"ManagerForServers", nlohmann::json::array()}};
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);

        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec,
                        const dbus::utility::ManagedObjectType& resp) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "Error while getting Software Version";
                    messages::internalError(asyncResp->res);
                    return;
                }

                for (auto& objpath : resp)
                {
                    for (auto& interface : objpath.second)
                    {
                        // If interface is
                        // xyz.openbmc_project.Software.Version, this is
                        // what we're looking for.
                        if (interface.first ==
                            "xyz.openbmc_project.Software.Version")
                        {
                            // Cut out everyting until last "/", ...
                            const std::string& iface_id = objpath.first;
                            for (auto& property : interface.second)
                            {
                                if (property.first == "Version")
                                {
                                    const std::string* value =
                                        std::get_if<std::string>(
                                            &property.second);
                                    if (value == nullptr)
                                    {
                                        continue;
                                    }
                                    asyncResp->res
                                        .jsonValue["FirmwareVersion"] = *value;
                                }
                            }
                        }
                    }
                }
            },
            "xyz.openbmc_project.Software.BMC.Updater",
            "/xyz/openbmc_project/software",
            "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
        getPidValues(asyncResp);
    }
    void setPidValues(std::shared_ptr<AsyncResp> response, nlohmann::json& data)
    {

        // todo(james): might make sense to do a mapper call here if this
        // interface gets more traction
        crow::connections::systemBus->async_method_call(
            [response,
             data](const boost::system::error_code ec,
                   const dbus::utility::ManagedObjectType& managedObj) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "Error communicating to Entity Manager";
                    messages::internalError(response->res);
                    return;
                }

                // todo(james) mutable doesn't work with asio bindings
                nlohmann::json jsonData = data;

                std::optional<nlohmann::json> pidControllers;
                std::optional<nlohmann::json> fanControllers;
                std::optional<nlohmann::json> fanZones;
                std::optional<nlohmann::json> stepwiseControllers;
                if (!redfish::json_util::readJson(
                        jsonData, response->res, "PidControllers",
                        pidControllers, "FanControllers", fanControllers,
                        "FanZones", fanZones, "StepwiseControllers",
                        stepwiseControllers))
                {
                    BMCWEB_LOG_ERROR << "Line:" << __LINE__
                                     << ", Illegal Property "
                                     << jsonData.dump();
                    return;
                }
                std::array<
                    std::pair<const char*, std::optional<nlohmann::json>*>, 4>
                    sections = {
                        std::make_pair("PidControllers", &pidControllers),
                        std::make_pair("FanControllers", &fanControllers),
                        std::make_pair("FanZones", &fanZones),
                        std::make_pair("StepwiseControllers",
                                       &stepwiseControllers)};

                for (auto& containerPair : sections)
                {
                    auto& container = *(containerPair.second);
                    if (!container)
                    {
                        continue;
                    }
                    const char* type = containerPair.first;

                    for (auto& record : container->items())
                    {
                        const auto& name = record.key();
                        auto pathItr =
                            std::find_if(managedObj.begin(), managedObj.end(),
                                         [&name](const auto& obj) {
                                             return boost::algorithm::ends_with(
                                                 obj.first.str, name);
                                         });
                        boost::container::flat_map<
                            std::string, dbus::utility::DbusVariantType>
                            output;

                        output.reserve(16); // The pid interface length

                        // determines if we're patching entity-manager or
                        // creating a new object
                        bool createNewObject = (pathItr == managedObj.end());
                        std::string iface;
                        if (type == "PidControllers" ||
                            type == "FanControllers")
                        {
                            iface = pidConfigurationIface;
                            if (!createNewObject &&
                                pathItr->second.find(pidConfigurationIface) ==
                                    pathItr->second.end())
                            {
                                createNewObject = true;
                            }
                        }
                        else if (type == "FanZones")
                        {
                            iface = pidZoneConfigurationIface;
                            if (!createNewObject &&
                                pathItr->second.find(
                                    pidZoneConfigurationIface) ==
                                    pathItr->second.end())
                            {

                                createNewObject = true;
                            }
                        }
                        else if (type == "StepwiseControllers")
                        {
                            iface = stepwiseConfigurationIface;
                            if (!createNewObject &&
                                pathItr->second.find(
                                    stepwiseConfigurationIface) ==
                                    pathItr->second.end())
                            {
                                createNewObject = true;
                            }
                        }
                        output["Name"] =
                            boost::replace_all_copy(name, "_", " ");

                        std::string chassis;
                        CreatePIDRet ret = createPidInterface(
                            response, type, std::move(record.value()),
                            pathItr->first.str, managedObj, createNewObject,
                            output, chassis);
                        if (ret == CreatePIDRet::fail)
                        {
                            return;
                        }
                        else if (ret == CreatePIDRet::del)
                        {
                            continue;
                        }

                        if (!createNewObject)
                        {
                            for (const auto& property : output)
                            {
                                crow::connections::systemBus->async_method_call(
                                    [response,
                                     propertyName{std::string(property.first)}](
                                        const boost::system::error_code ec) {
                                        if (ec)
                                        {
                                            BMCWEB_LOG_ERROR
                                                << "Error patching "
                                                << propertyName << ": " << ec;
                                            messages::internalError(
                                                response->res);
                                        }
                                    },
                                    "xyz.openbmc_project.EntityManager",
                                    pathItr->first.str,
                                    "org.freedesktop.DBus.Properties", "Set",
                                    iface, property.first, property.second);
                            }
                        }
                        else
                        {
                            if (chassis.empty())
                            {
                                BMCWEB_LOG_ERROR
                                    << "Failed to get chassis from config";
                                messages::invalidObject(response->res, name);
                                return;
                            }

                            bool foundChassis = false;
                            for (const auto& obj : managedObj)
                            {
                                if (boost::algorithm::ends_with(obj.first.str,
                                                                chassis))
                                {
                                    chassis = obj.first.str;
                                    foundChassis = true;
                                    break;
                                }
                            }
                            if (!foundChassis)
                            {
                                BMCWEB_LOG_ERROR
                                    << "Failed to find chassis on dbus";
                                messages::resourceMissingAtURI(
                                    response->res,
                                    "/redfish/v1/Chassis/" + chassis);
                                return;
                            }

                            crow::connections::systemBus->async_method_call(
                                [response](const boost::system::error_code ec) {
                                    if (ec)
                                    {
                                        BMCWEB_LOG_ERROR
                                            << "Error Adding Pid Object " << ec;
                                        messages::internalError(response->res);
                                    }
                                },
                                "xyz.openbmc_project.EntityManager", chassis,
                                "xyz.openbmc_project.AddObject", "AddObject",
                                output);
                        }
                    }
                }
            },
            "xyz.openbmc_project.EntityManager", "/", objectManagerIface,
            "GetManagedObjects");
    }

    void doPatch(crow::Response& res, const crow::Request& req,
                 const std::vector<std::string>& params) override
    {
        std::optional<nlohmann::json> oem;

        if (!json_util::readJson(req, res, "Oem", oem))
        {
            return;
        }

        std::shared_ptr<AsyncResp> response = std::make_shared<AsyncResp>(res);

        if (oem)
        {
            for (const auto& oemLevel : oem->items())
            {
                std::optional<nlohmann::json> openbmc;
                if (!redfish::json_util::readJson(*oem, res, "OpenBmc",
                                                  openbmc))
                {
                    BMCWEB_LOG_ERROR << "Line:" << __LINE__
                                     << ", Illegal Property " << oem->dump();
                    return;
                }
                if (openbmc)
                {
                    std::optional<nlohmann::json> fan;
                    if (!redfish::json_util::readJson(*openbmc, res, "Fan",
                                                      fan))
                    {
                        BMCWEB_LOG_ERROR << "Line:" << __LINE__
                                         << ", Illegal Property "
                                         << openbmc->dump();
                        return;
                    }
                    if (fan)
                    {
                        setPidValues(response, *fan);
                    }
                }
            }
        }
    }

    std::string getDateTime() const
    {
        std::array<char, 128> dateTime;
        std::string redfishDateTime("0000-00-00T00:00:00Z00:00");
        std::time_t time = std::time(nullptr);

        if (std::strftime(dateTime.begin(), dateTime.size(), "%FT%T%z",
                          std::localtime(&time)))
        {
            // insert the colon required by the ISO 8601 standard
            redfishDateTime = std::string(dateTime.data());
            redfishDateTime.insert(redfishDateTime.end() - 2, ':');
        }

        return redfishDateTime;
    }

    std::string uuid;
};

class ManagerCollection : public Node
{
  public:
    ManagerCollection(CrowApp& app) : Node(app, "/redfish/v1/Managers/")
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
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        // Collections don't include the static data added by SubRoute
        // because it has a duplicate entry for members
        res.jsonValue["@odata.id"] = "/redfish/v1/Managers";
        res.jsonValue["@odata.type"] = "#ManagerCollection.ManagerCollection";
        res.jsonValue["@odata.context"] =
            "/redfish/v1/$metadata#ManagerCollection.ManagerCollection";
        res.jsonValue["Name"] = "Manager Collection";
        res.jsonValue["Members@odata.count"] = 1;
        res.jsonValue["Members"] = {
            {{"@odata.id", "/redfish/v1/Managers/bmc"}}};
        res.end();
    }
};
} // namespace redfish
