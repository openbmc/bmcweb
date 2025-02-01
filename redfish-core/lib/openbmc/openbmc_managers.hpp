
#pragma once

#include "bmcweb_config.h"

#include "async_resp.hpp"
#include "dbus_singleton.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "logging.hpp"
#include "utils/dbus_utils.hpp"
#include "utils/json_utils.hpp"

#include <boost/asio/post.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/url/format.hpp>
#include <boost/url/url.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/message/native_types.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace redfish
{

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

inline void asyncPopulatePid(
    const std::string& connection, const std::string& path,
    const std::string& currentProfile,
    const std::vector<std::string>& supportedProfiles,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    sdbusplus::message::object_path objPath(path);
    dbus::utility::getManagedObjects(
        connection, objPath,
        [asyncResp, currentProfile, supportedProfiles](
            const boost::system::error_code& ec,
            const dbus::utility::ManagedObjectType& managedObj) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("{}", ec);
                messages::internalError(asyncResp->res);
                return;
            }
            nlohmann::json& configRoot =
                asyncResp->res.jsonValue["Oem"]["OpenBmc"]["Fan"];
            nlohmann::json& fans = configRoot["FanControllers"];
            fans["@odata.type"] =
                "#OpenBMCManager.v1_0_0.Manager.FanControllers";
            fans["@odata.id"] = boost::urls::format(
                "/redfish/v1/Managers/{}#/Oem/OpenBmc/Fan/FanControllers",
                BMCWEB_REDFISH_MANAGER_URI_NAME);

            nlohmann::json& pids = configRoot["PidControllers"];
            pids["@odata.type"] =
                "#OpenBMCManager.v1_0_0.Manager.PidControllers";
            pids["@odata.id"] = boost::urls::format(
                "/redfish/v1/Managers/{}#/Oem/OpenBmc/Fan/PidControllers",
                BMCWEB_REDFISH_MANAGER_URI_NAME);

            nlohmann::json& stepwise = configRoot["StepwiseControllers"];
            stepwise["@odata.type"] =
                "#OpenBMCManager.v1_0_0.Manager.StepwiseControllers";
            stepwise["@odata.id"] = boost::urls::format(
                "/redfish/v1/Managers/{}#/Oem/OpenBmc/Fan/StepwiseControllers",
                BMCWEB_REDFISH_MANAGER_URI_NAME);

            nlohmann::json& zones = configRoot["FanZones"];
            zones["@odata.id"] = boost::urls::format(
                "/redfish/v1/Managers/{}#/Oem/OpenBmc/Fan/FanZones",
                BMCWEB_REDFISH_MANAGER_URI_NAME);
            zones["@odata.type"] = "#OpenBMCManager.v1_0_0.Manager.FanZones";
            configRoot["@odata.id"] =
                boost::urls::format("/redfish/v1/Managers/{}#/Oem/OpenBmc/Fan",
                                    BMCWEB_REDFISH_MANAGER_URI_NAME);
            configRoot["@odata.type"] = "#OpenBMCManager.v1_0_0.Manager.Fan";
            configRoot["Profile@Redfish.AllowableValues"] = supportedProfiles;

            if (!currentProfile.empty())
            {
                configRoot["Profile"] = currentProfile;
            }
            BMCWEB_LOG_DEBUG("profile = {} !", currentProfile);

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
                                         dbus::utility::DbusVariantType>&
                             propPair : intfPair.second)
                    {
                        if (propPair.first == "Name")
                        {
                            const std::string* namePtr =
                                std::get_if<std::string>(&propPair.second);
                            if (namePtr == nullptr)
                            {
                                BMCWEB_LOG_ERROR("Pid Name Field illegal");
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
                                BMCWEB_LOG_ERROR("Pid Profiles Field illegal");
                                messages::internalError(asyncResp->res);
                                return;
                            }
                            if (std::find(profiles->begin(), profiles->end(),
                                          currentProfile) == profiles->end())
                            {
                                BMCWEB_LOG_INFO(
                                    "{} not supported in current profile",
                                    name);
                                continue;
                            }
                        }
                    }
                    nlohmann::json* config = nullptr;
                    const std::string* classPtr = nullptr;

                    for (const std::pair<std::string,
                                         dbus::utility::DbusVariantType>&
                             propPair : intfPair.second)
                    {
                        if (propPair.first == "Class")
                        {
                            classPtr =
                                std::get_if<std::string>(&propPair.second);
                        }
                    }

                    boost::urls::url url(
                        boost::urls::format("/redfish/v1/Managers/{}",
                                            BMCWEB_REDFISH_MANAGER_URI_NAME));
                    if (intfPair.first == pidZoneConfigurationIface)
                    {
                        std::string chassis;
                        if (!dbus::utility::getNthStringFromPath(
                                pathPair.first.str, 5, chassis))
                        {
                            chassis = "#IllegalValue";
                        }
                        nlohmann::json& zone = zones[name];
                        zone["Chassis"]["@odata.id"] = boost::urls::format(
                            "/redfish/v1/Chassis/{}", chassis);
                        url.set_fragment(
                            ("/Oem/OpenBmc/Fan/FanZones"_json_pointer / name)
                                .to_string());
                        zone["@odata.id"] = std::move(url);
                        zone["@odata.type"] =
                            "#OpenBMCManager.v1_0_0.Manager.FanZone";
                        config = &zone;
                    }

                    else if (intfPair.first == stepwiseConfigurationIface)
                    {
                        if (classPtr == nullptr)
                        {
                            BMCWEB_LOG_ERROR("Pid Class Field illegal");
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
                            "#OpenBMCManager.v1_0_0.Manager.StepwiseController";

                        controller["Direction"] = *classPtr;
                    }

                    // pid and fans are off the same configuration
                    else if (intfPair.first == pidConfigurationIface)
                    {
                        if (classPtr == nullptr)
                        {
                            BMCWEB_LOG_ERROR("Pid Class Field illegal");
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        bool isFan = *classPtr == "fan";
                        nlohmann::json& element =
                            isFan ? fans[name] : pids[name];
                        config = &element;
                        if (isFan)
                        {
                            url.set_fragment(
                                ("/Oem/OpenBmc/Fan/FanControllers"_json_pointer /
                                 name)
                                    .to_string());
                            element["@odata.id"] = std::move(url);
                            element["@odata.type"] =
                                "#OpenBMCManager.v1_0_0.Manager.FanController";
                        }
                        else
                        {
                            url.set_fragment(
                                ("/Oem/OpenBmc/Fan/PidControllers"_json_pointer /
                                 name)
                                    .to_string());
                            element["@odata.id"] = std::move(url);
                            element["@odata.type"] =
                                "#OpenBMCManager.v1_0_0.Manager.PidController";
                        }
                    }
                    else
                    {
                        BMCWEB_LOG_ERROR("Unexpected configuration");
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
                                BMCWEB_LOG_ERROR("Field Illegal {}",
                                                 propertyPair.first);
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
                                    BMCWEB_LOG_ERROR("Field Illegal {}",
                                                     propertyPair.first);
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
                                        BMCWEB_LOG_ERROR(
                                            "Reading and Output size don't match ");
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
                                        steps.emplace_back(std::move(step));
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
                                    BMCWEB_LOG_ERROR("Field Illegal {}",
                                                     propertyPair.first);
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
                                    BMCWEB_LOG_ERROR("Zones Pid Field Illegal");
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
                                        boost::urls::format(
                                            "/redfish/v1/Managers/{}#{}",
                                            BMCWEB_REDFISH_MANAGER_URI_NAME,
                                            ("/Oem/OpenBmc/Fan/FanZones"_json_pointer /
                                             itemCopy)
                                                .to_string());
                                    input["@odata.id"] = std::move(managerUrl);
                                    data.emplace_back(std::move(input));
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
                                    BMCWEB_LOG_ERROR("Field Illegal {}",
                                                     propertyPair.first);
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                                data = *inputs;
                            }
                            else if (propertyPair.first == "SetPointOffset")
                            {
                                const std::string* ptr =
                                    std::get_if<std::string>(
                                        &propertyPair.second);

                                if (ptr == nullptr)
                                {
                                    BMCWEB_LOG_ERROR("Field Illegal {}",
                                                     propertyPair.first);
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
                                    BMCWEB_LOG_ERROR("Value Illegal {}", *ptr);
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                            }
                            // doubles
                            else if (propertyPair.first ==
                                         "FFGainCoefficient" ||
                                     propertyPair.first == "FFOffCoefficient" ||
                                     propertyPair.first == "ICoefficient" ||
                                     propertyPair.first == "ILimitMax" ||
                                     propertyPair.first == "ILimitMin" ||
                                     propertyPair.first ==
                                         "PositiveHysteresis" ||
                                     propertyPair.first ==
                                         "NegativeHysteresis" ||
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
                                    BMCWEB_LOG_ERROR("Field Illegal {}",
                                                     propertyPair.first);
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                                (*config)[propertyPair.first] = *ptr;
                            }
                        }
                    }
                }
            }
        });
}

enum class CreatePIDRet
{
    fail,
    del,
    patch
};

inline const dbus::utility::ManagedObjectType::value_type* findChassis(
    const dbus::utility::ManagedObjectType& managedObj, std::string_view value,
    std::string& chassis)
{
    BMCWEB_LOG_DEBUG("Find Chassis: {}", value);

    std::string escaped(value);
    std::replace(escaped.begin(), escaped.end(), ' ', '_');
    escaped = "/" + escaped;
    auto it = std::ranges::find_if(managedObj, [&escaped](const auto& obj) {
        if (obj.first.str.ends_with(escaped))
        {
            BMCWEB_LOG_DEBUG("Matched {}", obj.first.str);
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

inline bool getZonesFromJsonReq(
    const std::shared_ptr<bmcweb::AsyncResp>& response,
    std::vector<nlohmann::json::object_t>& config,
    std::vector<std::string>& zones)
{
    if (config.empty())
    {
        BMCWEB_LOG_ERROR("Empty Zones");
        messages::propertyValueFormatError(response->res, config, "Zones");
        return false;
    }
    for (auto& odata : config)
    {
        std::string path;
        if (!redfish::json_util::readJsonObject(odata, response->res,
                                                "@odata.id", path))
        {
            return false;
        }
        std::string input;

        // 8 below comes from
        // /redfish/v1/Managers/bmc#/Oem/OpenBmc/Fan/FanZones/Left
        //     0    1     2      3    4    5      6     7      8
        if (!dbus::utility::getNthStringFromPath(path, 8, input))
        {
            BMCWEB_LOG_ERROR("Got invalid path {}", path);
            BMCWEB_LOG_ERROR("Illegal Type Zones");
            messages::propertyValueFormatError(response->res, odata, "Zones");
            return false;
        }
        std::replace(input.begin(), input.end(), '_', ' ');
        zones.emplace_back(std::move(input));
    }
    return true;
}

inline CreatePIDRet createPidInterface(
    const std::shared_ptr<bmcweb::AsyncResp>& response, const std::string& type,
    std::string_view name, nlohmann::json& jsonValue, const std::string& path,
    const dbus::utility::ManagedObjectType& managedObj, bool createNewObject,
    dbus::utility::DBusPropertiesMap& output, std::string& chassis,
    const std::string& profile)
{
    // common deleter
    if (jsonValue == nullptr)
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
            BMCWEB_LOG_ERROR("Illegal Type {}", type);
            messages::propertyUnknown(response->res, type);
            return CreatePIDRet::fail;
        }

        BMCWEB_LOG_DEBUG("del {} {}", path, iface);
        // delete interface
        crow::connections::systemBus->async_method_call(
            [response, path](const boost::system::error_code& ec) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR("Error patching {}: {}", path, ec);
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
        managedItem = findChassis(managedObj, name, chassis);
        if (managedItem == nullptr)
        {
            BMCWEB_LOG_ERROR("Failed to get chassis from config patch");
            messages::invalidObject(
                response->res,
                boost::urls::format("/redfish/v1/Chassis/{}", chassis));
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
                                BMCWEB_LOG_ERROR(
                                    "Illegal profiles in managed object");
                                messages::internalError(response->res);
                                return CreatePIDRet::fail;
                            }
                            if (std::find(curProfiles->begin(),
                                          curProfiles->end(), profile) ==
                                curProfiles->end())
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
                BMCWEB_LOG_ERROR("Failed to find interface in managed object");
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

        std::optional<std::vector<nlohmann::json::object_t>> zones;
        std::optional<std::vector<std::string>> inputs;
        std::optional<std::vector<std::string>> outputs;
        std::map<std::string, std::optional<double>> doubles;
        std::optional<std::string> setpointOffset;
        if (!redfish::json_util::readJson(                           //
                jsonValue, response->res,                            //
                "FFGainCoefficient", doubles["FFGainCoefficient"],   //
                "FFOffCoefficient", doubles["FFOffCoefficient"],     //
                "ICoefficient", doubles["ICoefficient"],             //
                "ILimitMax", doubles["ILimitMax"],                   //
                "ILimitMin", doubles["ILimitMin"],                   //
                "Inputs", inputs,                                    //
                "NegativeHysteresis", doubles["NegativeHysteresis"], //
                "OutLimitMax", doubles["OutLimitMax"],               //
                "OutLimitMin", doubles["OutLimitMin"],               //
                "Outputs", outputs,                                  //
                "PCoefficient", doubles["PCoefficient"],             //
                "PositiveHysteresis", doubles["PositiveHysteresis"], //
                "SetPoint", doubles["SetPoint"],                     //
                "SetPointOffset", setpointOffset,                    //
                "SlewNeg", doubles["SlewNeg"],                       //
                "SlewPos", doubles["SlewPos"],                       //
                "Zones", zones                                       //
                ))
        {
            return CreatePIDRet::fail;
        }

        if (zones)
        {
            std::vector<std::string> zonesStr;
            if (!getZonesFromJsonReq(response, *zones, zonesStr))
            {
                BMCWEB_LOG_ERROR("Illegal Zones");
                return CreatePIDRet::fail;
            }
            if (chassis.empty() &&
                findChassis(managedObj, zonesStr[0], chassis) == nullptr)
            {
                BMCWEB_LOG_ERROR("Failed to get chassis from config patch");
                messages::invalidObject(
                    response->res,
                    boost::urls::format("/redfish/v1/Chassis/{}", chassis));
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
                BMCWEB_LOG_ERROR("Invalid setpointoffset {}", *setpointOffset);
                messages::propertyValueNotInList(response->res, name,
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
            BMCWEB_LOG_DEBUG("{} = {}", pairs.first, *pairs.second);
            output.emplace_back(pairs.first, *pairs.second);
        }
    }

    else if (type == "FanZones")
    {
        output.emplace_back("Type", "Pid.Zone");

        std::optional<std::string> chassisId;
        std::optional<double> failSafePercent;
        std::optional<double> minThermalOutput;
        if (!redfish::json_util::readJson(          //
                jsonValue, response->res,           //
                "Chassis/@odata.id", chassisId,     //
                "FailSafePercent", failSafePercent, //
                "MinThermalOutput", minThermalOutput))
        {
            return CreatePIDRet::fail;
        }

        if (chassisId)
        {
            // /redfish/v1/chassis/chassis_name/
            if (!dbus::utility::getNthStringFromPath(*chassisId, 3, chassis))
            {
                BMCWEB_LOG_ERROR("Got invalid path {}", *chassisId);
                messages::invalidObject(
                    response->res,
                    boost::urls::format("/redfish/v1/Chassis/{}", *chassisId));
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

        std::optional<std::vector<nlohmann::json::object_t>> zones;
        std::optional<std::vector<nlohmann::json::object_t>> steps;
        std::optional<std::vector<std::string>> inputs;
        std::optional<double> positiveHysteresis;
        std::optional<double> negativeHysteresis;
        std::optional<std::string> direction; // upper clipping curve vs lower
        if (!redfish::json_util::readJson(    //
                jsonValue, response->res,     //
                "Direction", direction,       //
                "Inputs", inputs,             //
                "NegativeHysteresis", negativeHysteresis, //
                "PositiveHysteresis", positiveHysteresis, //
                "Steps", steps,                           //
                "Zones", zones                            //
                ))
        {
            return CreatePIDRet::fail;
        }

        if (zones)
        {
            std::vector<std::string> zonesStrs;
            if (!getZonesFromJsonReq(response, *zones, zonesStrs))
            {
                BMCWEB_LOG_ERROR("Illegal Zones");
                return CreatePIDRet::fail;
            }
            if (chassis.empty() &&
                findChassis(managedObj, zonesStrs[0], chassis) == nullptr)
            {
                BMCWEB_LOG_ERROR("Failed to get chassis from config patch");
                messages::invalidObject(
                    response->res,
                    boost::urls::format("/redfish/v1/Chassis/{}", chassis));
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

                if (!redfish::json_util::readJsonObject( //
                        step, response->res,             //
                        "Output", out,                   //
                        "Target", target                 //
                        ))
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
            if (std::ranges::find(allowedDirections, *direction) ==
                allowedDirections.end())
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
        BMCWEB_LOG_ERROR("Illegal Type {}", type);
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
                    BMCWEB_LOG_ERROR("{}", ec);
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
                    BMCWEB_LOG_ERROR("GetPIDValues: Mapper Error");
                    messages::internalError(self->asyncResp->res);
                    return;
                }

                const std::string& path = subtreeLocal[0].first;
                const std::string& owner = subtreeLocal[0].second[0].first;

                dbus::utility::getAllProperties(
                    *crow::connections::systemBus, owner, path,
                    thermalModeIface,
                    [path, owner,
                     self](const boost::system::error_code& ec2,
                           const dbus::utility::DBusPropertiesMap& resp) {
                        if (ec2)
                        {
                            BMCWEB_LOG_ERROR(
                                "GetPIDValues: Can't get thermalModeIface {}",
                                path);
                            messages::internalError(self->asyncResp->res);
                            return;
                        }

                        const std::string* current = nullptr;
                        const std::vector<std::string>* supported = nullptr;

                        const bool success = sdbusplus::unpackPropertiesNoThrow(
                            dbus_utils::UnpackErrorPrinter(), resp, "Current",
                            current, "Supported", supported);

                        if (!success)
                        {
                            messages::internalError(self->asyncResp->res);
                            return;
                        }

                        if (current == nullptr || supported == nullptr)
                        {
                            BMCWEB_LOG_ERROR(
                                "GetPIDValues: thermal mode iface invalid {}",
                                path);
                            messages::internalError(self->asyncResp->res);
                            return;
                        }
                        self->complete.currentProfile = *current;
                        self->complete.supportedProfiles = *supported;
                    });
            });
    }

    static void processingComplete(
        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
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
                            BMCWEB_LOG_DEBUG("{}Has no Object Manager",
                                             connectionGroup.first);
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
    SetPIDValues(
        const std::shared_ptr<bmcweb::AsyncResp>& asyncRespIn,
        std::vector<
            std::pair<std::string, std::optional<nlohmann::json::object_t>>>&&
            configurationsIn,
        std::optional<std::string>& profileIn) :
        asyncResp(asyncRespIn), configuration(std::move(configurationsIn)),
        profile(std::move(profileIn))
    {}

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
        sdbusplus::message::object_path objPath(
            "/xyz/openbmc_project/inventory");
        dbus::utility::getManagedObjects(
            "xyz.openbmc_project.EntityManager", objPath,
            [self](const boost::system::error_code& ec,
                   const dbus::utility::ManagedObjectType& mObj) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR("Error communicating to Entity Manager");
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
                        if (std::ranges::find(configurations, interface) !=
                            configurations.end())
                        {
                            self->objectCount++;
                            break;
                        }
                    }
                }
                self->managedObj = mObj;
            });

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
                    BMCWEB_LOG_ERROR("SetPIDValues: Mapper Error");
                    messages::internalError(self->asyncResp->res);
                    return;
                }

                const std::string& path = subtree[0].first;
                const std::string& owner = subtree[0].second[0].first;
                dbus::utility::getAllProperties(
                    *crow::connections::systemBus, owner, path,
                    thermalModeIface,
                    [self, path,
                     owner](const boost::system::error_code& ec2,
                            const dbus::utility::DBusPropertiesMap& r) {
                        if (ec2)
                        {
                            BMCWEB_LOG_ERROR(
                                "SetPIDValues: Can't get thermalModeIface {}",
                                path);
                            messages::internalError(self->asyncResp->res);
                            return;
                        }
                        const std::string* current = nullptr;
                        const std::vector<std::string>* supported = nullptr;

                        const bool success = sdbusplus::unpackPropertiesNoThrow(
                            dbus_utils::UnpackErrorPrinter(), r, "Current",
                            current, "Supported", supported);

                        if (!success)
                        {
                            messages::internalError(self->asyncResp->res);
                            return;
                        }

                        if (current == nullptr || supported == nullptr)
                        {
                            BMCWEB_LOG_ERROR(
                                "SetPIDValues: thermal mode iface invalid {}",
                                path);
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
            if (std::ranges::find(supportedProfiles, *profile) ==
                supportedProfiles.end())
            {
                messages::actionParameterUnknown(response->res, "Profile",
                                                 *profile);
                return;
            }
            currentProfile = *profile;
            sdbusplus::asio::setProperty(
                *crow::connections::systemBus, profileConnection, profilePath,
                thermalModeIface, "Current", *profile,
                [response](const boost::system::error_code& ec) {
                    if (ec)
                    {
                        BMCWEB_LOG_ERROR("Error patching profile{}", ec);
                        messages::internalError(response->res);
                    }
                });
        }

        for (auto& containerPair : configuration)
        {
            auto& container = containerPair.second;
            if (!container)
            {
                continue;
            }

            const std::string& type = containerPair.first;

            for (auto& [name, value] : *container)
            {
                std::string dbusObjName = name;
                std::replace(dbusObjName.begin(), dbusObjName.end(), ' ', '_');
                BMCWEB_LOG_DEBUG("looking for {}", name);

                auto pathItr = std::ranges::find_if(
                    managedObj, [&dbusObjName](const auto& obj) {
                        return obj.first.filename() == dbusObjName;
                    });
                dbus::utility::DBusPropertiesMap output;

                output.reserve(16); // The pid interface length

                // determines if we're patching entity-manager or
                // creating a new object
                bool createNewObject = (pathItr == managedObj.end());
                BMCWEB_LOG_DEBUG("Found = {}", !createNewObject);

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
                                iface = pidZoneConfigurationIface;
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

                if (createNewObject && value == nullptr)
                {
                    // can't delete a non-existent object
                    messages::propertyValueNotInList(response->res, value,
                                                     name);
                    continue;
                }

                std::string path;
                if (pathItr != managedObj.end())
                {
                    path = pathItr->first.str;
                }

                BMCWEB_LOG_DEBUG("Create new = {}", createNewObject);

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
                    response, type, name, value, path, managedObj,
                    createNewObject, output, chassis, currentProfile);
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
                                    BMCWEB_LOG_ERROR("Error patching {}: {}",
                                                     propertyName, ec);
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
                        BMCWEB_LOG_ERROR("Failed to get chassis from config");
                        messages::internalError(response->res);
                        return;
                    }

                    bool foundChassis = false;
                    for (const auto& obj : managedObj)
                    {
                        if (obj.first.filename() == chassis)
                        {
                            chassis = obj.first.str;
                            foundChassis = true;
                            break;
                        }
                    }
                    if (!foundChassis)
                    {
                        BMCWEB_LOG_ERROR("Failed to find chassis on dbus");
                        messages::resourceMissingAtURI(
                            response->res,
                            boost::urls::format("/redfish/v1/Chassis/{}",
                                                chassis));
                        return;
                    }

                    crow::connections::systemBus->async_method_call(
                        [response](const boost::system::error_code& ec) {
                            if (ec)
                            {
                                BMCWEB_LOG_ERROR("Error Adding Pid Object {}",
                                                 ec);
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
            BMCWEB_LOG_CRITICAL("pidSetDone threw exception");
        }
    }

    std::shared_ptr<bmcweb::AsyncResp> asyncResp;
    std::vector<std::pair<std::string, std::optional<nlohmann::json::object_t>>>
        configuration;
    std::optional<std::string> profile;
    dbus::utility::ManagedObjectType managedObj;
    std::vector<std::string> supportedProfiles;
    std::string currentProfile;
    std::string profileConnection;
    std::string profilePath;
    size_t objectCount = 0;
};

inline void getHandleOemOpenBmc(
    const crow::Request& /*req*/,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& /*managerId*/)
{
    // Default OEM data
    nlohmann::json& oemOpenbmc = asyncResp->res.jsonValue["Oem"]["OpenBmc"];
    oemOpenbmc["@odata.type"] = "#OpenBMCManager.v1_0_0.Manager";
    oemOpenbmc["@odata.id"] =
        boost::urls::format("/redfish/v1/Managers/{}#/Oem/OpenBmc",
                            BMCWEB_REDFISH_MANAGER_URI_NAME);

    nlohmann::json::object_t certificates;
    certificates["@odata.id"] =
        boost::urls::format("/redfish/v1/Managers/{}/Truststore/Certificates",
                            BMCWEB_REDFISH_MANAGER_URI_NAME);
    oemOpenbmc["Certificates"] = std::move(certificates);

    if constexpr (BMCWEB_REDFISH_OEM_MANAGER_FAN_DATA)
    {
        auto pids = std::make_shared<GetPIDValues>(asyncResp);
        pids->run();
    }
}

} // namespace redfish
