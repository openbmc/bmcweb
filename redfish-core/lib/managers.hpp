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
#include "dbus_utility.hpp"
#include "health.hpp"
#include "query.hpp"
#include "redfish_util.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/dbus_utils.hpp"
#include "utils/json_utils.hpp"
#include "utils/sw_utils.hpp"
#include "utils/systemd_utils.hpp"
#include "utils/time_utils.hpp"

#include <boost/system/error_code.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <sstream>
#include <string_view>
#include <variant>

namespace redfish
{

/**
 * Function reboots the BMC.
 *
 * @param[in] asyncResp - Shared pointer for completing asynchronous calls
 */
inline void
    doBMCGracefulRestart(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    const char* processName = "xyz.openbmc_project.State.BMC";
    const char* objectPath = "/xyz/openbmc_project/state/bmc0";
    const char* interfaceName = "xyz.openbmc_project.State.BMC";
    const std::string& propertyValue =
        "xyz.openbmc_project.State.BMC.Transition.Reboot";
    const char* destProperty = "RequestedBMCTransition";

    // Create the D-Bus variant for D-Bus call.
    dbus::utility::DbusVariantType dbusPropertyValue(propertyValue);

    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code& ec) {
        // Use "Set" method to set the property value.
        if (ec)
        {
            BMCWEB_LOG_DEBUG << "[Set] Bad D-Bus request error: " << ec;
            messages::internalError(asyncResp->res);
            return;
        }

        messages::success(asyncResp->res);
        },
        processName, objectPath, "org.freedesktop.DBus.Properties", "Set",
        interfaceName, destProperty, dbusPropertyValue);
}

inline void
    doBMCForceRestart(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    const char* processName = "xyz.openbmc_project.State.BMC";
    const char* objectPath = "/xyz/openbmc_project/state/bmc0";
    const char* interfaceName = "xyz.openbmc_project.State.BMC";
    const std::string& propertyValue =
        "xyz.openbmc_project.State.BMC.Transition.HardReboot";
    const char* destProperty = "RequestedBMCTransition";

    // Create the D-Bus variant for D-Bus call.
    dbus::utility::DbusVariantType dbusPropertyValue(propertyValue);

    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code& ec) {
        // Use "Set" method to set the property value.
        if (ec)
        {
            BMCWEB_LOG_DEBUG << "[Set] Bad D-Bus request error: " << ec;
            messages::internalError(asyncResp->res);
            return;
        }

        messages::success(asyncResp->res);
        },
        processName, objectPath, "org.freedesktop.DBus.Properties", "Set",
        interfaceName, destProperty, dbusPropertyValue);
}

/**
 * ManagerResetAction class supports the POST method for the Reset (reboot)
 * action.
 */
inline void requestRoutesManagerResetAction(App& app)
{
    /**
     * Function handles POST method request.
     * Analyzes POST body before sending Reset (Reboot) request data to D-Bus.
     * OpenBMC supports ResetType "GracefulRestart" and "ForceRestart".
     */

    BMCWEB_ROUTE(app, "/redfish/v1/Managers/bmc/Actions/Manager.Reset/")
        .privileges(redfish::privileges::postManager)
        .methods(boost::beast::http::verb::post)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        BMCWEB_LOG_DEBUG << "Post Manager Reset.";

        std::string resetType;

        if (!json_util::readJsonAction(req, asyncResp->res, "ResetType",
                                       resetType))
        {
            return;
        }

        if (resetType == "GracefulRestart")
        {
            BMCWEB_LOG_DEBUG << "Proceeding with " << resetType;
            doBMCGracefulRestart(asyncResp);
            return;
        }
        if (resetType == "ForceRestart")
        {
            BMCWEB_LOG_DEBUG << "Proceeding with " << resetType;
            doBMCForceRestart(asyncResp);
            return;
        }
        BMCWEB_LOG_DEBUG << "Invalid property value for ResetType: "
                         << resetType;
        messages::actionParameterNotSupported(asyncResp->res, resetType,
                                              "ResetType");

        return;
        });
}

/**
 * ManagerResetToDefaultsAction class supports POST method for factory reset
 * action.
 */
inline void requestRoutesManagerResetToDefaultsAction(App& app)
{

    /**
     * Function handles ResetToDefaults POST method request.
     *
     * Analyzes POST body message and factory resets BMC by calling
     * BMC code updater factory reset followed by a BMC reboot.
     *
     * BMC code updater factory reset wipes the whole BMC read-write
     * filesystem which includes things like the network settings.
     *
     * OpenBMC only supports ResetToDefaultsType "ResetAll".
     */

    BMCWEB_ROUTE(app,
                 "/redfish/v1/Managers/bmc/Actions/Manager.ResetToDefaults/")
        .privileges(redfish::privileges::postManager)
        .methods(boost::beast::http::verb::post)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        BMCWEB_LOG_DEBUG << "Post ResetToDefaults.";

        std::string resetType;

        if (!json_util::readJsonAction(req, asyncResp->res,
                                       "ResetToDefaultsType", resetType))
        {
            BMCWEB_LOG_DEBUG << "Missing property ResetToDefaultsType.";

            messages::actionParameterMissing(asyncResp->res, "ResetToDefaults",
                                             "ResetToDefaultsType");
            return;
        }

        if (resetType != "ResetAll")
        {
            BMCWEB_LOG_DEBUG
                << "Invalid property value for ResetToDefaultsType: "
                << resetType;
            messages::actionParameterNotSupported(asyncResp->res, resetType,
                                                  "ResetToDefaultsType");
            return;
        }

        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code& ec) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "Failed to ResetToDefaults: " << ec;
                messages::internalError(asyncResp->res);
                return;
            }
            // Factory Reset doesn't actually happen until a reboot
            // Can't erase what the BMC is running on
            doBMCGracefulRestart(asyncResp);
            },
            "xyz.openbmc_project.Software.BMC.Updater",
            "/xyz/openbmc_project/software",
            "xyz.openbmc_project.Common.FactoryReset", "Reset");
        });
}

/**
 * ManagerResetActionInfo derived class for delivering Manager
 * ResetType AllowableValues using ResetInfo schema.
 */
inline void requestRoutesManagerResetActionInfo(App& app)
{
    /**
     * Functions triggers appropriate requests on DBus
     */

    BMCWEB_ROUTE(app, "/redfish/v1/Managers/bmc/ResetActionInfo/")
        .privileges(redfish::privileges::getActionInfo)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }

        asyncResp->res.jsonValue["@odata.type"] =
            "#ActionInfo.v1_1_2.ActionInfo";
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Managers/bmc/ResetActionInfo";
        asyncResp->res.jsonValue["Name"] = "Reset Action Info";
        asyncResp->res.jsonValue["Id"] = "ResetActionInfo";
        nlohmann::json::object_t parameter;
        parameter["Name"] = "ResetType";
        parameter["Required"] = true;
        parameter["DataType"] = "String";

        nlohmann::json::array_t allowableValues;
        allowableValues.push_back("GracefulRestart");
        allowableValues.push_back("ForceRestart");
        parameter["AllowableValues"] = std::move(allowableValues);

        nlohmann::json::array_t parameters;
        parameters.push_back(std::move(parameter));

        asyncResp->res.jsonValue["Parameters"] = std::move(parameters);
        });
}

static constexpr const char* objectManagerIface =
    "org.freedesktop.DBus.ObjectManager";
static constexpr const char* pidConfigurationIface =
    "xyz.openbmc_project.Configuration.Pid";
static constexpr const char* pidZoneConfigurationIface =
    "xyz.openbmc_project.Configuration.Pid.Zone";
static constexpr const char* stepwiseConfigurationIface =
    "xyz.openbmc_project.Configuration.Stepwise";
static constexpr const char* thermalModeIface =
    "xyz.openbmc_project.Control.ThermalMode";

inline void
    asyncPopulatePid(const std::string& connection, const std::string& path,
                     const std::string& currentProfile,
                     const std::vector<std::string>& supportedProfiles,
                     const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{

    crow::connections::systemBus->async_method_call(
        [asyncResp, currentProfile, supportedProfiles](
            const boost::system::error_code& ec,
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
        fans["@odata.id"] =
            "/redfish/v1/Managers/bmc#/Oem/OpenBmc/Fan/FanControllers";

        nlohmann::json& pids = configRoot["PidControllers"];
        pids["@odata.type"] = "#OemManager.PidControllers";
        pids["@odata.id"] =
            "/redfish/v1/Managers/bmc#/Oem/OpenBmc/Fan/PidControllers";

        nlohmann::json& stepwise = configRoot["StepwiseControllers"];
        stepwise["@odata.type"] = "#OemManager.StepwiseControllers";
        stepwise["@odata.id"] =
            "/redfish/v1/Managers/bmc#/Oem/OpenBmc/Fan/StepwiseControllers";

        nlohmann::json& zones = configRoot["FanZones"];
        zones["@odata.id"] =
            "/redfish/v1/Managers/bmc#/Oem/OpenBmc/Fan/FanZones";
        zones["@odata.type"] = "#OemManager.FanZones";
        configRoot["@odata.id"] = "/redfish/v1/Managers/bmc#/Oem/OpenBmc/Fan";
        configRoot["@odata.type"] = "#OemManager.Fan";
        configRoot["Profile@Redfish.AllowableValues"] = supportedProfiles;

        if (!currentProfile.empty())
        {
            configRoot["Profile"] = currentProfile;
        }
        BMCWEB_LOG_ERROR << "profile = " << currentProfile << " !";

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

                std::string name;

                for (const std::pair<std::string,
                                     dbus::utility::DbusVariantType>& propPair :
                     intfPair.second)
                {
                    if (propPair.first == "Name")
                    {
                        const std::string* namePtr =
                            std::get_if<std::string>(&propPair.second);
                        if (namePtr == nullptr)
                        {
                            BMCWEB_LOG_ERROR << "Pid Name Field illegal";
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        name = *namePtr;
                        dbus::utility::escapePathForDbus(name);
                    }
                    else if (propPair.first == "Profiles")
                    {
                        const std::vector<std::string>* profiles =
                            std::get_if<std::vector<std::string>>(
                                &propPair.second);
                        if (profiles == nullptr)
                        {
                            BMCWEB_LOG_ERROR << "Pid Profiles Field illegal";
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        if (std::find(profiles->begin(), profiles->end(),
                                      currentProfile) == profiles->end())
                        {
                            BMCWEB_LOG_INFO
                                << name << " not supported in current profile";
                            continue;
                        }
                    }
                }
                nlohmann::json* config = nullptr;
                const std::string* classPtr = nullptr;

                for (const std::pair<std::string,
                                     dbus::utility::DbusVariantType>& propPair :
                     intfPair.second)
                {
                    if (propPair.first == "Class")
                    {
                        classPtr = std::get_if<std::string>(&propPair.second);
                    }
                }

                boost::urls::url url = crow::utility::urlFromPieces(
                    "redfish", "v1", "Managers", "bmc");
                if (intfPair.first == pidZoneConfigurationIface)
                {
                    std::string chassis;
                    if (!dbus::utility::getNthStringFromPath(pathPair.first.str,
                                                             5, chassis))
                    {
                        chassis = "#IllegalValue";
                    }
                    nlohmann::json& zone = zones[name];
                    zone["Chassis"]["@odata.id"] = crow::utility::urlFromPieces(
                        "redfish", "v1", "Chassis", chassis);
                    url.set_fragment(
                        ("/Oem/OpenBmc/Fan/FanZones"_json_pointer / name)
                            .to_string());
                    zone["@odata.id"] = std::move(url);
                    zone["@odata.type"] = "#OemManager.FanZone";
                    config = &zone;
                }

                else if (intfPair.first == stepwiseConfigurationIface)
                {
                    if (classPtr == nullptr)
                    {
                        BMCWEB_LOG_ERROR << "Pid Class Field illegal";
                        messages::internalError(asyncResp->res);
                        return;
                    }

                    nlohmann::json& controller = stepwise[name];
                    config = &controller;
                    url.set_fragment(
                        ("/Oem/OpenBmc/Fan/StepwiseControllers"_json_pointer /
                         name)
                            .to_string());
                    controller["@odata.id"] = std::move(url);
                    controller["@odata.type"] =
                        "#OemManager.StepwiseController";

                    controller["Direction"] = *classPtr;
                }

                // pid and fans are off the same configuration
                else if (intfPair.first == pidConfigurationIface)
                {

                    if (classPtr == nullptr)
                    {
                        BMCWEB_LOG_ERROR << "Pid Class Field illegal";
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    bool isFan = *classPtr == "fan";
                    nlohmann::json& element = isFan ? fans[name] : pids[name];
                    config = &element;
                    if (isFan)
                    {
                        url.set_fragment(
                            ("/Oem/OpenBmc/Fan/FanControllers"_json_pointer /
                             name)
                                .to_string());
                        element["@odata.id"] = std::move(url);
                        element["@odata.type"] = "#OemManager.FanController";
                    }
                    else
                    {
                        url.set_fragment(
                            ("/Oem/OpenBmc/Fan/PidControllers"_json_pointer /
                             name)
                                .to_string());
                        element["@odata.id"] = std::move(url);
                        element["@odata.type"] = "#OemManager.PidController";
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
                            if (keys != nullptr && values != nullptr)
                            {
                                if (keys->size() != values->size())
                                {
                                    BMCWEB_LOG_ERROR
                                        << "Reading and Output size don't match ";
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                                nlohmann::json& steps = (*config)["Steps"];
                                steps = nlohmann::json::array();
                                for (size_t ii = 0; ii < keys->size(); ii++)
                                {
                                    nlohmann::json::object_t step;
                                    step["Target"] = (*keys)[ii];
                                    step["Output"] = (*values)[ii];
                                    steps.push_back(std::move(step));
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
                                BMCWEB_LOG_ERROR << "Zones Pid Field Illegal";
                                messages::internalError(asyncResp->res);
                                return;
                            }
                            auto& data = (*config)[propertyPair.first];
                            data = nlohmann::json::array();
                            for (std::string itemCopy : *inputs)
                            {
                                dbus::utility::escapePathForDbus(itemCopy);
                                nlohmann::json::object_t input;
                                boost::urls::url managerUrl =
                                    crow::utility::urlFromPieces(
                                        "redfish", "v1", "Managers", "bmc");
                                managerUrl.set_fragment(
                                    ("/Oem/OpenBmc/Fan/FanZones"_json_pointer /
                                     itemCopy)
                                        .to_string());
                                input["@odata.id"] = std::move(managerUrl);
                                data.push_back(std::move(input));
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
                        }
                        else if (propertyPair.first == "SetPointOffset")
                        {
                            const std::string* ptr =
                                std::get_if<std::string>(&propertyPair.second);

                            if (ptr == nullptr)
                            {
                                BMCWEB_LOG_ERROR << "Field Illegal "
                                                 << propertyPair.first;
                                messages::internalError(asyncResp->res);
                                return;
                            }
                            // translate from dbus to redfish
                            if (*ptr == "WarningHigh")
                            {
                                (*config)["SetPointOffset"] =
                                    "UpperThresholdNonCritical";
                            }
                            else if (*ptr == "WarningLow")
                            {
                                (*config)["SetPointOffset"] =
                                    "LowerThresholdNonCritical";
                            }
                            else if (*ptr == "CriticalHigh")
                            {
                                (*config)["SetPointOffset"] =
                                    "UpperThresholdCritical";
                            }
                            else if (*ptr == "CriticalLow")
                            {
                                (*config)["SetPointOffset"] =
                                    "LowerThresholdCritical";
                            }
                            else
                            {
                                BMCWEB_LOG_ERROR << "Value Illegal " << *ptr;
                                messages::internalError(asyncResp->res);
                                return;
                            }
                        }
                        // doubles
                        else if (propertyPair.first == "FFGainCoefficient" ||
                                 propertyPair.first == "FFOffCoefficient" ||
                                 propertyPair.first == "ICoefficient" ||
                                 propertyPair.first == "ILimitMax" ||
                                 propertyPair.first == "ILimitMin" ||
                                 propertyPair.first == "PositiveHysteresis" ||
                                 propertyPair.first == "NegativeHysteresis" ||
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

inline bool
    getZonesFromJsonReq(const std::shared_ptr<bmcweb::AsyncResp>& response,
                        std::vector<nlohmann::json>& config,
                        std::vector<std::string>& zones)
{
    if (config.empty())
    {
        BMCWEB_LOG_ERROR << "Empty Zones";
        messages::propertyValueFormatError(response->res, "[]", "Zones");
        return false;
    }
    for (auto& odata : config)
    {
        std::string path;
        if (!redfish::json_util::readJson(odata, response->res, "@odata.id",
                                          path))
        {
            return false;
        }
        std::string input;

        // 8 below comes from
        // /redfish/v1/Managers/bmc#/Oem/OpenBmc/Fan/FanZones/Left
        //     0    1     2      3    4    5      6     7      8
        if (!dbus::utility::getNthStringFromPath(path, 8, input))
        {
            BMCWEB_LOG_ERROR << "Got invalid path " << path;
            BMCWEB_LOG_ERROR << "Illegal Type Zones";
            messages::propertyValueFormatError(response->res, odata.dump(),
                                               "Zones");
            return false;
        }
        std::replace(input.begin(), input.end(), '_', ' ');
        zones.emplace_back(std::move(input));
    }
    return true;
}

inline const dbus::utility::ManagedObjectType::value_type*
    findChassis(const dbus::utility::ManagedObjectType& managedObj,
                const std::string& value, std::string& chassis)
{
    BMCWEB_LOG_DEBUG << "Find Chassis: " << value << "\n";

    std::string escaped = value;
    std::replace(escaped.begin(), escaped.end(), ' ', '_');
    escaped = "/" + escaped;
    auto it = std::find_if(managedObj.begin(), managedObj.end(),
                           [&escaped](const auto& obj) {
        if (boost::algorithm::ends_with(obj.first.str, escaped))
        {
            BMCWEB_LOG_DEBUG << "Matched " << obj.first.str << "\n";
            return true;
        }
        return false;
    });

    if (it == managedObj.end())
    {
        return nullptr;
    }
    // 5 comes from <chassis-name> being the 5th element
    // /xyz/openbmc_project/inventory/system/chassis/<chassis-name>
    if (dbus::utility::getNthStringFromPath(it->first.str, 5, chassis))
    {
        return &(*it);
    }

    return nullptr;
}

inline CreatePIDRet createPidInterface(
    const std::shared_ptr<bmcweb::AsyncResp>& response, const std::string& type,
    const nlohmann::json::iterator& it, const std::string& path,
    const dbus::utility::ManagedObjectType& managedObj, bool createNewObject,
    dbus::utility::DBusPropertiesMap& output, std::string& chassis,
    const std::string& profile)
{

    // common deleter
    if (it.value() == nullptr)
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
            BMCWEB_LOG_ERROR << "Illegal Type " << type;
            messages::propertyUnknown(response->res, type);
            return CreatePIDRet::fail;
        }

        BMCWEB_LOG_DEBUG << "del " << path << " " << iface << "\n";
        // delete interface
        crow::connections::systemBus->async_method_call(
            [response, path](const boost::system::error_code& ec) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "Error patching " << path << ": " << ec;
                messages::internalError(response->res);
                return;
            }
            messages::success(response->res);
            },
            "xyz.openbmc_project.EntityManager", path, iface, "Delete");
        return CreatePIDRet::del;
    }

    const dbus::utility::ManagedObjectType::value_type* managedItem = nullptr;
    if (!createNewObject)
    {
        // if we aren't creating a new object, we should be able to find it on
        // d-bus
        managedItem = findChassis(managedObj, it.key(), chassis);
        if (managedItem == nullptr)
        {
            BMCWEB_LOG_ERROR << "Failed to get chassis from config patch";
            messages::invalidObject(response->res,
                                    crow::utility::urlFromPieces(
                                        "redfish", "v1", "Chassis", chassis));
            return CreatePIDRet::fail;
        }
    }

    if (!profile.empty() &&
        (type == "PidControllers" || type == "FanControllers" ||
         type == "StepwiseControllers"))
    {
        if (managedItem == nullptr)
        {
            output.emplace_back("Profiles", std::vector<std::string>{profile});
        }
        else
        {
            std::string interface;
            if (type == "StepwiseControllers")
            {
                interface = stepwiseConfigurationIface;
            }
            else
            {
                interface = pidConfigurationIface;
            }
            bool ifaceFound = false;
            for (const auto& iface : managedItem->second)
            {
                if (iface.first == interface)
                {
                    ifaceFound = true;
                    for (const auto& prop : iface.second)
                    {
                        if (prop.first == "Profiles")
                        {
                            const std::vector<std::string>* curProfiles =
                                std::get_if<std::vector<std::string>>(
                                    &(prop.second));
                            if (curProfiles == nullptr)
                            {
                                BMCWEB_LOG_ERROR
                                    << "Illegal profiles in managed object";
                                messages::internalError(response->res);
                                return CreatePIDRet::fail;
                            }
                            if (std::find(curProfiles->begin(),
                                          curProfiles->end(),
                                          profile) == curProfiles->end())
                            {
                                std::vector<std::string> newProfiles =
                                    *curProfiles;
                                newProfiles.push_back(profile);
                                output.emplace_back("Profiles", newProfiles);
                            }
                        }
                    }
                }
            }

            if (!ifaceFound)
            {
                BMCWEB_LOG_ERROR
                    << "Failed to find interface in managed object";
                messages::internalError(response->res);
                return CreatePIDRet::fail;
            }
        }
    }

    if (type == "PidControllers" || type == "FanControllers")
    {
        if (createNewObject)
        {
            output.emplace_back("Class",
                                type == "PidControllers" ? "temp" : "fan");
            output.emplace_back("Type", "Pid");
        }

        std::optional<std::vector<nlohmann::json>> zones;
        std::optional<std::vector<std::string>> inputs;
        std::optional<std::vector<std::string>> outputs;
        std::map<std::string, std::optional<double>> doubles;
        std::optional<std::string> setpointOffset;
        if (!redfish::json_util::readJson(
                it.value(), response->res, "Inputs", inputs, "Outputs", outputs,
                "Zones", zones, "FFGainCoefficient",
                doubles["FFGainCoefficient"], "FFOffCoefficient",
                doubles["FFOffCoefficient"], "ICoefficient",
                doubles["ICoefficient"], "ILimitMax", doubles["ILimitMax"],
                "ILimitMin", doubles["ILimitMin"], "OutLimitMax",
                doubles["OutLimitMax"], "OutLimitMin", doubles["OutLimitMin"],
                "PCoefficient", doubles["PCoefficient"], "SetPoint",
                doubles["SetPoint"], "SetPointOffset", setpointOffset,
                "SlewNeg", doubles["SlewNeg"], "SlewPos", doubles["SlewPos"],
                "PositiveHysteresis", doubles["PositiveHysteresis"],
                "NegativeHysteresis", doubles["NegativeHysteresis"]))
        {
            return CreatePIDRet::fail;
        }
        if (zones)
        {
            std::vector<std::string> zonesStr;
            if (!getZonesFromJsonReq(response, *zones, zonesStr))
            {
                BMCWEB_LOG_ERROR << "Illegal Zones";
                return CreatePIDRet::fail;
            }
            if (chassis.empty() &&
                findChassis(managedObj, zonesStr[0], chassis) == nullptr)
            {
                BMCWEB_LOG_ERROR << "Failed to get chassis from config patch";
                messages::invalidObject(
                    response->res, crow::utility::urlFromPieces(
                                       "redfish", "v1", "Chassis", chassis));
                return CreatePIDRet::fail;
            }
            output.emplace_back("Zones", std::move(zonesStr));
        }

        if (inputs)
        {
            for (std::string& value : *inputs)
            {
                std::replace(value.begin(), value.end(), '_', ' ');
            }
            output.emplace_back("Inputs", *inputs);
        }

        if (outputs)
        {
            for (std::string& value : *outputs)
            {
                std::replace(value.begin(), value.end(), '_', ' ');
            }
            output.emplace_back("Outputs", *outputs);
        }

        if (setpointOffset)
        {
            // translate between redfish and dbus names
            if (*setpointOffset == "UpperThresholdNonCritical")
            {
                output.emplace_back("SetPointOffset", "WarningLow");
            }
            else if (*setpointOffset == "LowerThresholdNonCritical")
            {
                output.emplace_back("SetPointOffset", "WarningHigh");
            }
            else if (*setpointOffset == "LowerThresholdCritical")
            {
                output.emplace_back("SetPointOffset", "CriticalLow");
            }
            else if (*setpointOffset == "UpperThresholdCritical")
            {
                output.emplace_back("SetPointOffset", "CriticalHigh");
            }
            else
            {
                BMCWEB_LOG_ERROR << "Invalid setpointoffset "
                                 << *setpointOffset;
                messages::propertyValueNotInList(response->res, it.key(),
                                                 "SetPointOffset");
                return CreatePIDRet::fail;
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
            output.emplace_back(pairs.first, *pairs.second);
        }
    }

    else if (type == "FanZones")
    {
        output.emplace_back("Type", "Pid.Zone");

        std::optional<nlohmann::json> chassisContainer;
        std::optional<double> failSafePercent;
        std::optional<double> minThermalOutput;
        if (!redfish::json_util::readJson(it.value(), response->res, "Chassis",
                                          chassisContainer, "FailSafePercent",
                                          failSafePercent, "MinThermalOutput",
                                          minThermalOutput))
        {
            return CreatePIDRet::fail;
        }

        if (chassisContainer)
        {

            std::string chassisId;
            if (!redfish::json_util::readJson(*chassisContainer, response->res,
                                              "@odata.id", chassisId))
            {
                return CreatePIDRet::fail;
            }

            // /redfish/v1/chassis/chassis_name/
            if (!dbus::utility::getNthStringFromPath(chassisId, 3, chassis))
            {
                BMCWEB_LOG_ERROR << "Got invalid path " << chassisId;
                messages::invalidObject(
                    response->res, crow::utility::urlFromPieces(
                                       "redfish", "v1", "Chassis", chassisId));
                return CreatePIDRet::fail;
            }
        }
        if (minThermalOutput)
        {
            output.emplace_back("MinThermalOutput", *minThermalOutput);
        }
        if (failSafePercent)
        {
            output.emplace_back("FailSafePercent", *failSafePercent);
        }
    }
    else if (type == "StepwiseControllers")
    {
        output.emplace_back("Type", "Stepwise");

        std::optional<std::vector<nlohmann::json>> zones;
        std::optional<std::vector<nlohmann::json>> steps;
        std::optional<std::vector<std::string>> inputs;
        std::optional<double> positiveHysteresis;
        std::optional<double> negativeHysteresis;
        std::optional<std::string> direction; // upper clipping curve vs lower
        if (!redfish::json_util::readJson(
                it.value(), response->res, "Zones", zones, "Steps", steps,
                "Inputs", inputs, "PositiveHysteresis", positiveHysteresis,
                "NegativeHysteresis", negativeHysteresis, "Direction",
                direction))
        {
            return CreatePIDRet::fail;
        }

        if (zones)
        {
            std::vector<std::string> zonesStrs;
            if (!getZonesFromJsonReq(response, *zones, zonesStrs))
            {
                BMCWEB_LOG_ERROR << "Illegal Zones";
                return CreatePIDRet::fail;
            }
            if (chassis.empty() &&
                findChassis(managedObj, zonesStrs[0], chassis) == nullptr)
            {
                BMCWEB_LOG_ERROR << "Failed to get chassis from config patch";
                messages::invalidObject(
                    response->res, crow::utility::urlFromPieces(
                                       "redfish", "v1", "Chassis", chassis));
                return CreatePIDRet::fail;
            }
            output.emplace_back("Zones", std::move(zonesStrs));
        }
        if (steps)
        {
            std::vector<double> readings;
            std::vector<double> outputs;
            for (auto& step : *steps)
            {
                double target = 0.0;
                double out = 0.0;

                if (!redfish::json_util::readJson(step, response->res, "Target",
                                                  target, "Output", out))
                {
                    return CreatePIDRet::fail;
                }
                readings.emplace_back(target);
                outputs.emplace_back(out);
            }
            output.emplace_back("Reading", std::move(readings));
            output.emplace_back("Output", std::move(outputs));
        }
        if (inputs)
        {
            for (std::string& value : *inputs)
            {

                std::replace(value.begin(), value.end(), '_', ' ');
            }
            output.emplace_back("Inputs", std::move(*inputs));
        }
        if (negativeHysteresis)
        {
            output.emplace_back("NegativeHysteresis", *negativeHysteresis);
        }
        if (positiveHysteresis)
        {
            output.emplace_back("PositiveHysteresis", *positiveHysteresis);
        }
        if (direction)
        {
            constexpr const std::array<const char*, 2> allowedDirections = {
                "Ceiling", "Floor"};
            if (std::find(allowedDirections.begin(), allowedDirections.end(),
                          *direction) == allowedDirections.end())
            {
                messages::propertyValueTypeError(response->res, "Direction",
                                                 *direction);
                return CreatePIDRet::fail;
            }
            output.emplace_back("Class", *direction);
        }
    }
    else
    {
        BMCWEB_LOG_ERROR << "Illegal Type " << type;
        messages::propertyUnknown(response->res, type);
        return CreatePIDRet::fail;
    }
    return CreatePIDRet::patch;
}
struct GetPIDValues : std::enable_shared_from_this<GetPIDValues>
{
    struct CompletionValues
    {
        std::vector<std::string> supportedProfiles;
        std::string currentProfile;
        dbus::utility::MapperGetSubTreeResponse subtree;
    };

    explicit GetPIDValues(
        const std::shared_ptr<bmcweb::AsyncResp>& asyncRespIn) :
        asyncResp(asyncRespIn)

    {}

    void run()
    {
        std::shared_ptr<GetPIDValues> self = shared_from_this();

        // get all configurations
        constexpr std::array<std::string_view, 4> interfaces = {
            pidConfigurationIface, pidZoneConfigurationIface,
            objectManagerIface, stepwiseConfigurationIface};
        dbus::utility::getSubTree(
            "/", 0, interfaces,
            [self](
                const boost::system::error_code& ec,
                const dbus::utility::MapperGetSubTreeResponse& subtreeLocal) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << ec;
                messages::internalError(self->asyncResp->res);
                return;
            }
            self->complete.subtree = subtreeLocal;
            });

        // at the same time get the selected profile
        constexpr std::array<std::string_view, 1> thermalModeIfaces = {
            thermalModeIface};
        dbus::utility::getSubTree(
            "/", 0, thermalModeIfaces,
            [self](
                const boost::system::error_code& ec,
                const dbus::utility::MapperGetSubTreeResponse& subtreeLocal) {
            if (ec || subtreeLocal.empty())
            {
                return;
            }
            if (subtreeLocal[0].second.size() != 1)
            {
                // invalid mapper response, should never happen
                BMCWEB_LOG_ERROR << "GetPIDValues: Mapper Error";
                messages::internalError(self->asyncResp->res);
                return;
            }

            const std::string& path = subtreeLocal[0].first;
            const std::string& owner = subtreeLocal[0].second[0].first;

            sdbusplus::asio::getAllProperties(
                *crow::connections::systemBus, owner, path, thermalModeIface,
                [path, owner,
                 self](const boost::system::error_code& ec2,
                       const dbus::utility::DBusPropertiesMap& resp) {
                if (ec2)
                {
                    BMCWEB_LOG_ERROR
                        << "GetPIDValues: Can't get thermalModeIface " << path;
                    messages::internalError(self->asyncResp->res);
                    return;
                }

                const std::string* current = nullptr;
                const std::vector<std::string>* supported = nullptr;

                const bool success = sdbusplus::unpackPropertiesNoThrow(
                    dbus_utils::UnpackErrorPrinter(), resp, "Current", current,
                    "Supported", supported);

                if (!success)
                {
                    messages::internalError(self->asyncResp->res);
                    return;
                }

                if (current == nullptr || supported == nullptr)
                {
                    BMCWEB_LOG_ERROR
                        << "GetPIDValues: thermal mode iface invalid " << path;
                    messages::internalError(self->asyncResp->res);
                    return;
                }
                self->complete.currentProfile = *current;
                self->complete.supportedProfiles = *supported;
                });
            });
    }

    static void
        processingComplete(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const CompletionValues& completion)
    {
        if (asyncResp->res.result() != boost::beast::http::status::ok)
        {
            return;
        }
        // create map of <connection, path to objMgr>>
        boost::container::flat_map<
            std::string, std::string, std::less<>,
            std::vector<std::pair<std::string, std::string>>>
            objectMgrPaths;
        boost::container::flat_set<std::string, std::less<>,
                                   std::vector<std::string>>
            calledConnections;
        for (const auto& pathGroup : completion.subtree)
        {
            for (const auto& connectionGroup : pathGroup.second)
            {
                auto findConnection =
                    calledConnections.find(connectionGroup.first);
                if (findConnection != calledConnections.end())
                {
                    break;
                }
                for (const std::string& interface : connectionGroup.second)
                {
                    if (interface == objectManagerIface)
                    {
                        objectMgrPaths[connectionGroup.first] = pathGroup.first;
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

                        asyncPopulatePid(findObjMgr->first, findObjMgr->second,
                                         completion.currentProfile,
                                         completion.supportedProfiles,
                                         asyncResp);
                        break;
                    }
                }
            }
        }
    }

    ~GetPIDValues()
    {
        boost::asio::post(crow::connections::systemBus->get_io_context(),
                          std::bind_front(&processingComplete, asyncResp,
                                          std::move(complete)));
    }

    GetPIDValues(const GetPIDValues&) = delete;
    GetPIDValues(GetPIDValues&&) = delete;
    GetPIDValues& operator=(const GetPIDValues&) = delete;
    GetPIDValues& operator=(GetPIDValues&&) = delete;

    std::shared_ptr<bmcweb::AsyncResp> asyncResp;
    CompletionValues complete;
};

struct SetPIDValues : std::enable_shared_from_this<SetPIDValues>
{

    SetPIDValues(const std::shared_ptr<bmcweb::AsyncResp>& asyncRespIn,
                 nlohmann::json& data) :
        asyncResp(asyncRespIn)
    {

        std::optional<nlohmann::json> pidControllers;
        std::optional<nlohmann::json> fanControllers;
        std::optional<nlohmann::json> fanZones;
        std::optional<nlohmann::json> stepwiseControllers;

        if (!redfish::json_util::readJson(
                data, asyncResp->res, "PidControllers", pidControllers,
                "FanControllers", fanControllers, "FanZones", fanZones,
                "StepwiseControllers", stepwiseControllers, "Profile", profile))
        {
            return;
        }
        configuration.emplace_back("PidControllers", std::move(pidControllers));
        configuration.emplace_back("FanControllers", std::move(fanControllers));
        configuration.emplace_back("FanZones", std::move(fanZones));
        configuration.emplace_back("StepwiseControllers",
                                   std::move(stepwiseControllers));
    }

    SetPIDValues(const SetPIDValues&) = delete;
    SetPIDValues(SetPIDValues&&) = delete;
    SetPIDValues& operator=(const SetPIDValues&) = delete;
    SetPIDValues& operator=(SetPIDValues&&) = delete;

    void run()
    {
        if (asyncResp->res.result() != boost::beast::http::status::ok)
        {
            return;
        }

        std::shared_ptr<SetPIDValues> self = shared_from_this();

        // todo(james): might make sense to do a mapper call here if this
        // interface gets more traction
        crow::connections::systemBus->async_method_call(
            [self](const boost::system::error_code& ec,
                   const dbus::utility::ManagedObjectType& mObj) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "Error communicating to Entity Manager";
                messages::internalError(self->asyncResp->res);
                return;
            }
            const std::array<const char*, 3> configurations = {
                pidConfigurationIface, pidZoneConfigurationIface,
                stepwiseConfigurationIface};

            for (const auto& [path, object] : mObj)
            {
                for (const auto& [interface, _] : object)
                {
                    if (std::find(configurations.begin(), configurations.end(),
                                  interface) != configurations.end())
                    {
                        self->objectCount++;
                        break;
                    }
                }
            }
            self->managedObj = mObj;
            },
            "xyz.openbmc_project.EntityManager",
            "/xyz/openbmc_project/inventory", objectManagerIface,
            "GetManagedObjects");

        // at the same time get the profile information
        constexpr std::array<std::string_view, 1> thermalModeIfaces = {
            thermalModeIface};
        dbus::utility::getSubTree(
            "/", 0, thermalModeIfaces,
            [self](const boost::system::error_code& ec,
                   const dbus::utility::MapperGetSubTreeResponse& subtree) {
            if (ec || subtree.empty())
            {
                return;
            }
            if (subtree[0].second.empty())
            {
                // invalid mapper response, should never happen
                BMCWEB_LOG_ERROR << "SetPIDValues: Mapper Error";
                messages::internalError(self->asyncResp->res);
                return;
            }

            const std::string& path = subtree[0].first;
            const std::string& owner = subtree[0].second[0].first;
            sdbusplus::asio::getAllProperties(
                *crow::connections::systemBus, owner, path, thermalModeIface,
                [self, path, owner](const boost::system::error_code& ec2,
                                    const dbus::utility::DBusPropertiesMap& r) {
                if (ec2)
                {
                    BMCWEB_LOG_ERROR
                        << "SetPIDValues: Can't get thermalModeIface " << path;
                    messages::internalError(self->asyncResp->res);
                    return;
                }
                const std::string* current = nullptr;
                const std::vector<std::string>* supported = nullptr;

                const bool success = sdbusplus::unpackPropertiesNoThrow(
                    dbus_utils::UnpackErrorPrinter(), r, "Current", current,
                    "Supported", supported);

                if (!success)
                {
                    messages::internalError(self->asyncResp->res);
                    return;
                }

                if (current == nullptr || supported == nullptr)
                {
                    BMCWEB_LOG_ERROR
                        << "SetPIDValues: thermal mode iface invalid " << path;
                    messages::internalError(self->asyncResp->res);
                    return;
                }
                self->currentProfile = *current;
                self->supportedProfiles = *supported;
                self->profileConnection = owner;
                self->profilePath = path;
                });
            });
    }
    void pidSetDone()
    {
        if (asyncResp->res.result() != boost::beast::http::status::ok)
        {
            return;
        }
        std::shared_ptr<bmcweb::AsyncResp> response = asyncResp;
        if (profile)
        {
            if (std::find(supportedProfiles.begin(), supportedProfiles.end(),
                          *profile) == supportedProfiles.end())
            {
                messages::actionParameterUnknown(response->res, "Profile",
                                                 *profile);
                return;
            }
            currentProfile = *profile;
            crow::connections::systemBus->async_method_call(
                [response](const boost::system::error_code& ec) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "Error patching profile" << ec;
                    messages::internalError(response->res);
                }
                },
                profileConnection, profilePath,
                "org.freedesktop.DBus.Properties", "Set", thermalModeIface,
                "Current", dbus::utility::DbusVariantType(*profile));
        }

        for (auto& containerPair : configuration)
        {
            auto& container = containerPair.second;
            if (!container)
            {
                continue;
            }
            BMCWEB_LOG_DEBUG << *container;

            const std::string& type = containerPair.first;

            for (nlohmann::json::iterator it = container->begin();
                 it != container->end(); ++it)
            {
                const auto& name = it.key();
                BMCWEB_LOG_DEBUG << "looking for " << name;

                auto pathItr =
                    std::find_if(managedObj.begin(), managedObj.end(),
                                 [&name](const auto& obj) {
                    return boost::algorithm::ends_with(obj.first.str,
                                                       "/" + name);
                    });
                dbus::utility::DBusPropertiesMap output;

                output.reserve(16); // The pid interface length

                // determines if we're patching entity-manager or
                // creating a new object
                bool createNewObject = (pathItr == managedObj.end());
                BMCWEB_LOG_DEBUG << "Found = " << !createNewObject;

                std::string iface;
                if (!createNewObject)
                {
                    bool findInterface = false;
                    for (const auto& interface : pathItr->second)
                    {
                        if (interface.first == pidConfigurationIface)
                        {
                            if (type == "PidControllers" ||
                                type == "FanControllers")
                            {
                                iface = pidConfigurationIface;
                                findInterface = true;
                                break;
                            }
                        }
                        else if (interface.first == pidZoneConfigurationIface)
                        {
                            if (type == "FanZones")
                            {
                                iface = pidConfigurationIface;
                                findInterface = true;
                                break;
                            }
                        }
                        else if (interface.first == stepwiseConfigurationIface)
                        {
                            if (type == "StepwiseControllers")
                            {
                                iface = stepwiseConfigurationIface;
                                findInterface = true;
                                break;
                            }
                        }
                    }

                    // create new object if interface not found
                    if (!findInterface)
                    {
                        createNewObject = true;
                    }
                }

                if (createNewObject && it.value() == nullptr)
                {
                    // can't delete a non-existent object
                    messages::propertyValueNotInList(response->res,
                                                     it.value().dump(), name);
                    continue;
                }

                std::string path;
                if (pathItr != managedObj.end())
                {
                    path = pathItr->first.str;
                }

                BMCWEB_LOG_DEBUG << "Create new = " << createNewObject << "\n";

                // arbitrary limit to avoid attacks
                constexpr const size_t controllerLimit = 500;
                if (createNewObject && objectCount >= controllerLimit)
                {
                    messages::resourceExhaustion(response->res, type);
                    continue;
                }
                std::string escaped = name;
                std::replace(escaped.begin(), escaped.end(), '_', ' ');
                output.emplace_back("Name", escaped);

                std::string chassis;
                CreatePIDRet ret = createPidInterface(
                    response, type, it, path, managedObj, createNewObject,
                    output, chassis, currentProfile);
                if (ret == CreatePIDRet::fail)
                {
                    return;
                }
                if (ret == CreatePIDRet::del)
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
                                const boost::system::error_code& ec) {
                            if (ec)
                            {
                                BMCWEB_LOG_ERROR << "Error patching "
                                                 << propertyName << ": " << ec;
                                messages::internalError(response->res);
                                return;
                            }
                            messages::success(response->res);
                            },
                            "xyz.openbmc_project.EntityManager", path,
                            "org.freedesktop.DBus.Properties", "Set", iface,
                            property.first, property.second);
                    }
                }
                else
                {
                    if (chassis.empty())
                    {
                        BMCWEB_LOG_ERROR << "Failed to get chassis from config";
                        messages::internalError(response->res);
                        return;
                    }

                    bool foundChassis = false;
                    for (const auto& obj : managedObj)
                    {
                        if (boost::algorithm::ends_with(obj.first.str, chassis))
                        {
                            chassis = obj.first.str;
                            foundChassis = true;
                            break;
                        }
                    }
                    if (!foundChassis)
                    {
                        BMCWEB_LOG_ERROR << "Failed to find chassis on dbus";
                        messages::resourceMissingAtURI(
                            response->res,
                            crow::utility::urlFromPieces("redfish", "v1",
                                                         "Chassis", chassis));
                        return;
                    }

                    crow::connections::systemBus->async_method_call(
                        [response](const boost::system::error_code& ec) {
                        if (ec)
                        {
                            BMCWEB_LOG_ERROR << "Error Adding Pid Object "
                                             << ec;
                            messages::internalError(response->res);
                            return;
                        }
                        messages::success(response->res);
                        },
                        "xyz.openbmc_project.EntityManager", chassis,
                        "xyz.openbmc_project.AddObject", "AddObject", output);
                }
            }
        }
    }

    ~SetPIDValues()
    {
        try
        {
            pidSetDone();
        }
        catch (...)
        {
            BMCWEB_LOG_CRITICAL << "pidSetDone threw exception";
        }
    }

    std::shared_ptr<bmcweb::AsyncResp> asyncResp;
    std::vector<std::pair<std::string, std::optional<nlohmann::json>>>
        configuration;
    std::optional<std::string> profile;
    dbus::utility::ManagedObjectType managedObj;
    std::vector<std::string> supportedProfiles;
    std::string currentProfile;
    std::string profileConnection;
    std::string profilePath;
    size_t objectCount = 0;
};

/**
 * @brief Retrieves BMC manager location data over DBus
 *
 * @param[in] aResp Shared pointer for completing asynchronous calls
 * @param[in] connectionName - service name
 * @param[in] path - object path
 * @return none
 */
inline void getLocation(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                        const std::string& connectionName,
                        const std::string& path)
{
    BMCWEB_LOG_DEBUG << "Get BMC manager Location data.";

    sdbusplus::asio::getProperty<std::string>(
        *crow::connections::systemBus, connectionName, path,
        "xyz.openbmc_project.Inventory.Decorator.LocationCode", "LocationCode",
        [aResp](const boost::system::error_code& ec,
                const std::string& property) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG << "DBUS response error for "
                                "Location";
            messages::internalError(aResp->res);
            return;
        }

        aResp->res.jsonValue["Location"]["PartLocation"]["ServiceLabel"] =
            property;
        });
}
// avoid name collision systems.hpp
inline void
    managerGetLastResetTime(const std::shared_ptr<bmcweb::AsyncResp>& aResp)
{
    BMCWEB_LOG_DEBUG << "Getting Manager Last Reset Time";

    sdbusplus::asio::getProperty<uint64_t>(
        *crow::connections::systemBus, "xyz.openbmc_project.State.BMC",
        "/xyz/openbmc_project/state/bmc0", "xyz.openbmc_project.State.BMC",
        "LastRebootTime",
        [aResp](const boost::system::error_code& ec,
                const uint64_t lastResetTime) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG << "D-BUS response error " << ec;
            return;
        }

        // LastRebootTime is epoch time, in milliseconds
        // https://github.com/openbmc/phosphor-dbus-interfaces/blob/7f9a128eb9296e926422ddc312c148b625890bb6/xyz/openbmc_project/State/BMC.interface.yaml#L19
        uint64_t lastResetTimeStamp = lastResetTime / 1000;

        // Convert to ISO 8601 standard
        aResp->res.jsonValue["LastResetTime"] =
            redfish::time_utils::getDateTimeUint(lastResetTimeStamp);
        });
}

/**
 * @brief Set the running firmware image
 *
 * @param[i,o] aResp - Async response object
 * @param[i] runningFirmwareTarget - Image to make the running image
 *
 * @return void
 */
inline void
    setActiveFirmwareImage(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                           const std::string& runningFirmwareTarget)
{
    // Get the Id from /redfish/v1/UpdateService/FirmwareInventory/<Id>
    std::string::size_type idPos = runningFirmwareTarget.rfind('/');
    if (idPos == std::string::npos)
    {
        messages::propertyValueNotInList(aResp->res, runningFirmwareTarget,
                                         "@odata.id");
        BMCWEB_LOG_DEBUG << "Can't parse firmware ID!";
        return;
    }
    idPos++;
    if (idPos >= runningFirmwareTarget.size())
    {
        messages::propertyValueNotInList(aResp->res, runningFirmwareTarget,
                                         "@odata.id");
        BMCWEB_LOG_DEBUG << "Invalid firmware ID.";
        return;
    }
    std::string firmwareId = runningFirmwareTarget.substr(idPos);

    // Make sure the image is valid before setting priority
    crow::connections::systemBus->async_method_call(
        [aResp, firmwareId,
         runningFirmwareTarget](const boost::system::error_code& ec,
                                dbus::utility::ManagedObjectType& subtree) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG << "D-Bus response error getting objects.";
            messages::internalError(aResp->res);
            return;
        }

        if (subtree.empty())
        {
            BMCWEB_LOG_DEBUG << "Can't find image!";
            messages::internalError(aResp->res);
            return;
        }

        bool foundImage = false;
        for (const auto& object : subtree)
        {
            const std::string& path =
                static_cast<const std::string&>(object.first);
            std::size_t idPos2 = path.rfind('/');

            if (idPos2 == std::string::npos)
            {
                continue;
            }

            idPos2++;
            if (idPos2 >= path.size())
            {
                continue;
            }

            if (path.substr(idPos2) == firmwareId)
            {
                foundImage = true;
                break;
            }
        }

        if (!foundImage)
        {
            messages::propertyValueNotInList(aResp->res, runningFirmwareTarget,
                                             "@odata.id");
            BMCWEB_LOG_DEBUG << "Invalid firmware ID.";
            return;
        }

        BMCWEB_LOG_DEBUG << "Setting firmware version " << firmwareId
                         << " to priority 0.";

        // Only support Immediate
        // An addition could be a Redfish Setting like
        // ActiveSoftwareImageApplyTime and support OnReset
        crow::connections::systemBus->async_method_call(
            [aResp](const boost::system::error_code& ec2) {
            if (ec2)
            {
                BMCWEB_LOG_DEBUG << "D-Bus response error setting.";
                messages::internalError(aResp->res);
                return;
            }
            doBMCGracefulRestart(aResp);
            },

            "xyz.openbmc_project.Software.BMC.Updater",
            "/xyz/openbmc_project/software/" + firmwareId,
            "org.freedesktop.DBus.Properties", "Set",
            "xyz.openbmc_project.Software.RedundancyPriority", "Priority",
            dbus::utility::DbusVariantType(static_cast<uint8_t>(0)));
        },
        "xyz.openbmc_project.Software.BMC.Updater",
        "/xyz/openbmc_project/software", "org.freedesktop.DBus.ObjectManager",
        "GetManagedObjects");
}

inline void setDateTime(std::shared_ptr<bmcweb::AsyncResp> aResp,
                        std::string datetime)
{
    BMCWEB_LOG_DEBUG << "Set date time: " << datetime;

    std::optional<redfish::time_utils::usSinceEpoch> us =
        redfish::time_utils::dateStringToEpoch(datetime);
    if (!us)
    {
        messages::propertyValueFormatError(aResp->res, datetime, "DateTime");
        return;
    }
    crow::connections::systemBus->async_method_call(
        [aResp{std::move(aResp)},
         datetime{std::move(datetime)}](const boost::system::error_code& ec) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG << "Failed to set elapsed time. "
                                "DBUS response error "
                             << ec;
            messages::internalError(aResp->res);
            return;
        }
        aResp->res.jsonValue["DateTime"] = datetime;
        },
        "xyz.openbmc_project.Time.Manager", "/xyz/openbmc_project/time/bmc",
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Time.EpochTime", "Elapsed",
        dbus::utility::DbusVariantType(us->count()));
}

inline void requestRoutesManager(App& app)
{
    std::string uuid = persistent_data::getConfig().systemUuid;

    BMCWEB_ROUTE(app, "/redfish/v1/Managers/bmc/")
        .privileges(redfish::privileges::getManager)
        .methods(boost::beast::http::verb::get)(
            [&app, uuid](const crow::Request& req,
                         const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        asyncResp->res.jsonValue["@odata.id"] = "/redfish/v1/Managers/bmc";
        asyncResp->res.jsonValue["@odata.type"] = "#Manager.v1_14_0.Manager";
        asyncResp->res.jsonValue["Id"] = "bmc";
        asyncResp->res.jsonValue["Name"] = "OpenBmc Manager";
        asyncResp->res.jsonValue["Description"] =
            "Baseboard Management Controller";
        asyncResp->res.jsonValue["PowerState"] = "On";
        asyncResp->res.jsonValue["Status"]["State"] = "Enabled";
        asyncResp->res.jsonValue["Status"]["Health"] = "OK";

        asyncResp->res.jsonValue["ManagerType"] = "BMC";
        asyncResp->res.jsonValue["UUID"] = systemd_utils::getUuid();
        asyncResp->res.jsonValue["ServiceEntryPointUUID"] = uuid;
        asyncResp->res.jsonValue["Model"] = "OpenBmc"; // TODO(ed), get model

        asyncResp->res.jsonValue["LogServices"]["@odata.id"] =
            "/redfish/v1/Managers/bmc/LogServices";
        asyncResp->res.jsonValue["NetworkProtocol"]["@odata.id"] =
            "/redfish/v1/Managers/bmc/NetworkProtocol";
        asyncResp->res.jsonValue["EthernetInterfaces"]["@odata.id"] =
            "/redfish/v1/Managers/bmc/EthernetInterfaces";

#ifdef BMCWEB_ENABLE_VM_NBDPROXY
        asyncResp->res.jsonValue["VirtualMedia"]["@odata.id"] =
            "/redfish/v1/Managers/bmc/VirtualMedia";
#endif // BMCWEB_ENABLE_VM_NBDPROXY

        // default oem data
        nlohmann::json& oem = asyncResp->res.jsonValue["Oem"];
        nlohmann::json& oemOpenbmc = oem["OpenBmc"];
        oem["@odata.type"] = "#OemManager.Oem";
        oem["@odata.id"] = "/redfish/v1/Managers/bmc#/Oem";
        oemOpenbmc["@odata.type"] = "#OemManager.OpenBmc";
        oemOpenbmc["@odata.id"] = "/redfish/v1/Managers/bmc#/Oem/OpenBmc";

        nlohmann::json::object_t certificates;
        certificates["@odata.id"] =
            "/redfish/v1/Managers/bmc/Truststore/Certificates";
        oemOpenbmc["Certificates"] = std::move(certificates);

        // Manager.Reset (an action) can be many values, OpenBMC only
        // supports BMC reboot.
        nlohmann::json& managerReset =
            asyncResp->res.jsonValue["Actions"]["#Manager.Reset"];
        managerReset["target"] =
            "/redfish/v1/Managers/bmc/Actions/Manager.Reset";
        managerReset["@Redfish.ActionInfo"] =
            "/redfish/v1/Managers/bmc/ResetActionInfo";

        // ResetToDefaults (Factory Reset) has values like
        // PreserveNetworkAndUsers and PreserveNetwork that aren't supported
        // on OpenBMC
        nlohmann::json& resetToDefaults =
            asyncResp->res.jsonValue["Actions"]["#Manager.ResetToDefaults"];
        resetToDefaults["target"] =
            "/redfish/v1/Managers/bmc/Actions/Manager.ResetToDefaults";
        resetToDefaults["ResetType@Redfish.AllowableValues"] =
            nlohmann::json::array_t({"ResetAll"});

        std::pair<std::string, std::string> redfishDateTimeOffset =
            redfish::time_utils::getDateTimeOffsetNow();

        asyncResp->res.jsonValue["DateTime"] = redfishDateTimeOffset.first;
        asyncResp->res.jsonValue["DateTimeLocalOffset"] =
            redfishDateTimeOffset.second;

        // TODO (Gunnar): Remove these one day since moved to ComputerSystem
        // Still used by OCP profiles
        // https://github.com/opencomputeproject/OCP-Profiles/issues/23
        // Fill in SerialConsole info
        asyncResp->res.jsonValue["SerialConsole"]["ServiceEnabled"] = true;
        asyncResp->res.jsonValue["SerialConsole"]["MaxConcurrentSessions"] = 15;
        asyncResp->res.jsonValue["SerialConsole"]["ConnectTypesSupported"] =
            nlohmann::json::array_t({"IPMI", "SSH"});
#ifdef BMCWEB_ENABLE_KVM
        // Fill in GraphicalConsole info
        asyncResp->res.jsonValue["GraphicalConsole"]["ServiceEnabled"] = true;
        asyncResp->res.jsonValue["GraphicalConsole"]["MaxConcurrentSessions"] =
            4;
        asyncResp->res.jsonValue["GraphicalConsole"]["ConnectTypesSupported"] =
            nlohmann::json::array_t({"KVMIP"});
#endif // BMCWEB_ENABLE_KVM

        asyncResp->res.jsonValue["Links"]["ManagerForServers@odata.count"] = 1;

        nlohmann::json::array_t managerForServers;
        nlohmann::json::object_t manager;
        manager["@odata.id"] = "/redfish/v1/Systems/system";
        managerForServers.push_back(std::move(manager));

        asyncResp->res.jsonValue["Links"]["ManagerForServers"] =
            std::move(managerForServers);

        auto health = std::make_shared<HealthPopulate>(asyncResp);
        health->isManagersHealth = true;
        health->populate();

        sw_util::populateSoftwareInformation(asyncResp, sw_util::bmcPurpose,
                                             "FirmwareVersion", true);

        managerGetLastResetTime(asyncResp);

        // ManagerDiagnosticData is added for all BMCs.
        nlohmann::json& managerDiagnosticData =
            asyncResp->res.jsonValue["ManagerDiagnosticData"];
        managerDiagnosticData["@odata.id"] =
            "/redfish/v1/Managers/bmc/ManagerDiagnosticData";

#ifdef BMCWEB_ENABLE_REDFISH_OEM_MANAGER_FAN_DATA
        auto pids = std::make_shared<GetPIDValues>(asyncResp);
        pids->run();
#endif

        getMainChassisId(asyncResp,
                         [](const std::string& chassisId,
                            const std::shared_ptr<bmcweb::AsyncResp>& aRsp) {
            aRsp->res.jsonValue["Links"]["ManagerForChassis@odata.count"] = 1;
            nlohmann::json::array_t managerForChassis;
            nlohmann::json::object_t managerObj;
            boost::urls::url chassiUrl = crow::utility::urlFromPieces(
                "redfish", "v1", "Chassis", chassisId);
            managerObj["@odata.id"] = chassiUrl;
            managerForChassis.push_back(std::move(managerObj));
            aRsp->res.jsonValue["Links"]["ManagerForChassis"] =
                std::move(managerForChassis);
            aRsp->res.jsonValue["Links"]["ManagerInChassis"]["@odata.id"] =
                chassiUrl;
        });

        static bool started = false;

        if (!started)
        {
            sdbusplus::asio::getProperty<double>(
                *crow::connections::systemBus, "org.freedesktop.systemd1",
                "/org/freedesktop/systemd1", "org.freedesktop.systemd1.Manager",
                "Progress",
                [asyncResp](const boost::system::error_code& ec,
                            const double& val) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "Error while getting progress";
                    messages::internalError(asyncResp->res);
                    return;
                }
                if (val < 1.0)
                {
                    asyncResp->res.jsonValue["Status"]["State"] = "Starting";
                    started = true;
                }
                });
        }

        constexpr std::array<std::string_view, 1> interfaces = {
            "xyz.openbmc_project.Inventory.Item.Bmc"};
        dbus::utility::getSubTree(
            "/xyz/openbmc_project/inventory", 0, interfaces,
            [asyncResp](
                const boost::system::error_code& ec,
                const dbus::utility::MapperGetSubTreeResponse& subtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "D-Bus response error on GetSubTree " << ec;
                return;
            }
            if (subtree.empty())
            {
                BMCWEB_LOG_DEBUG << "Can't find bmc D-Bus object!";
                return;
            }
            // Assume only 1 bmc D-Bus object
            // Throw an error if there is more than 1
            if (subtree.size() > 1)
            {
                BMCWEB_LOG_DEBUG << "Found more than 1 bmc D-Bus object!";
                messages::internalError(asyncResp->res);
                return;
            }

            if (subtree[0].first.empty() || subtree[0].second.size() != 1)
            {
                BMCWEB_LOG_DEBUG << "Error getting bmc D-Bus object!";
                messages::internalError(asyncResp->res);
                return;
            }

            const std::string& path = subtree[0].first;
            const std::string& connectionName = subtree[0].second[0].first;

            for (const auto& interfaceName : subtree[0].second[0].second)
            {
                if (interfaceName ==
                    "xyz.openbmc_project.Inventory.Decorator.Asset")
                {

                    sdbusplus::asio::getAllProperties(
                        *crow::connections::systemBus, connectionName, path,
                        "xyz.openbmc_project.Inventory.Decorator.Asset",
                        [asyncResp](const boost::system::error_code& ec2,
                                    const dbus::utility::DBusPropertiesMap&
                                        propertiesList) {
                        if (ec2)
                        {
                            BMCWEB_LOG_DEBUG << "Can't get bmc asset!";
                            return;
                        }

                        const std::string* partNumber = nullptr;
                        const std::string* serialNumber = nullptr;
                        const std::string* manufacturer = nullptr;
                        const std::string* model = nullptr;
                        const std::string* sparePartNumber = nullptr;

                        const bool success = sdbusplus::unpackPropertiesNoThrow(
                            dbus_utils::UnpackErrorPrinter(), propertiesList,
                            "PartNumber", partNumber, "SerialNumber",
                            serialNumber, "Manufacturer", manufacturer, "Model",
                            model, "SparePartNumber", sparePartNumber);

                        if (!success)
                        {
                            messages::internalError(asyncResp->res);
                            return;
                        }

                        if (partNumber != nullptr)
                        {
                            asyncResp->res.jsonValue["PartNumber"] =
                                *partNumber;
                        }

                        if (serialNumber != nullptr)
                        {
                            asyncResp->res.jsonValue["SerialNumber"] =
                                *serialNumber;
                        }

                        if (manufacturer != nullptr)
                        {
                            asyncResp->res.jsonValue["Manufacturer"] =
                                *manufacturer;
                        }

                        if (model != nullptr)
                        {
                            asyncResp->res.jsonValue["Model"] = *model;
                        }

                        if (sparePartNumber != nullptr)
                        {
                            asyncResp->res.jsonValue["SparePartNumber"] =
                                *sparePartNumber;
                        }
                        });
                }
                else if (interfaceName ==
                         "xyz.openbmc_project.Inventory.Decorator.LocationCode")
                {
                    getLocation(asyncResp, connectionName, path);
                }
            }
            });
        });

    BMCWEB_ROUTE(app, "/redfish/v1/Managers/bmc/")
        .privileges(redfish::privileges::patchManager)
        .methods(boost::beast::http::verb::patch)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        std::optional<nlohmann::json> oem;
        std::optional<nlohmann::json> links;
        std::optional<std::string> datetime;

        if (!json_util::readJsonPatch(req, asyncResp->res, "Oem", oem,
                                      "DateTime", datetime, "Links", links))
        {
            return;
        }

        if (oem)
        {
#ifdef BMCWEB_ENABLE_REDFISH_OEM_MANAGER_FAN_DATA
            std::optional<nlohmann::json> openbmc;
            if (!redfish::json_util::readJson(*oem, asyncResp->res, "OpenBmc",
                                              openbmc))
            {
                return;
            }
            if (openbmc)
            {
                std::optional<nlohmann::json> fan;
                if (!redfish::json_util::readJson(*openbmc, asyncResp->res,
                                                  "Fan", fan))
                {
                    return;
                }
                if (fan)
                {
                    auto pid = std::make_shared<SetPIDValues>(asyncResp, *fan);
                    pid->run();
                }
            }
#else
            messages::propertyUnknown(asyncResp->res, "Oem");
            return;
#endif
        }
        if (links)
        {
            std::optional<nlohmann::json> activeSoftwareImage;
            if (!redfish::json_util::readJson(*links, asyncResp->res,
                                              "ActiveSoftwareImage",
                                              activeSoftwareImage))
            {
                return;
            }
            if (activeSoftwareImage)
            {
                std::optional<std::string> odataId;
                if (!json_util::readJson(*activeSoftwareImage, asyncResp->res,
                                         "@odata.id", odataId))
                {
                    return;
                }

                if (odataId)
                {
                    setActiveFirmwareImage(asyncResp, *odataId);
                }
            }
        }
        if (datetime)
        {
            setDateTime(asyncResp, std::move(*datetime));
        }
        });
}

inline void requestRoutesManagerCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Managers/")
        .privileges(redfish::privileges::getManagerCollection)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        // Collections don't include the static data added by SubRoute
        // because it has a duplicate entry for members
        asyncResp->res.jsonValue["@odata.id"] = "/redfish/v1/Managers";
        asyncResp->res.jsonValue["@odata.type"] =
            "#ManagerCollection.ManagerCollection";
        asyncResp->res.jsonValue["Name"] = "Manager Collection";
        asyncResp->res.jsonValue["Members@odata.count"] = 1;
        nlohmann::json::array_t members;
        nlohmann::json& bmc = members.emplace_back();
        bmc["@odata.id"] = "/redfish/v1/Managers/bmc";
        asyncResp->res.jsonValue["Members"] = std::move(members);
        });
}
} // namespace redfish
