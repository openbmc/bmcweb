// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
// SPDX-FileCopyrightText: Copyright 2018 Intel Corporation
#pragma once

#include "bmcweb_config.h"

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_singleton.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/action_info.hpp"
#include "generated/enums/manager.hpp"
#include "generated/enums/resource.hpp"
#include "http_request.hpp"
#include "led.hpp"
#include "logging.hpp"
#include "persistent_data.hpp"
#include "query.hpp"
#include "redfish.hpp"
#include "redfish_util.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/dbus_utils.hpp"
#include "utils/etag_utils.hpp"
#include "utils/json_utils.hpp"
#include "utils/manager_utils.hpp"
#include "utils/sw_utils.hpp"
#include "utils/systemd_utils.hpp"
#include "utils/time_utils.hpp"

#include <systemd/sd-bus.h>

#include <boost/beast/http/status.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>
#include <boost/url/url.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/message/native_types.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>

namespace redfish
{

inline void handleSetLocationIndicatorActive(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    bool locationIndicatorActive, const std::string& managerId,
    const boost::system::error_code& ec, const std::string& managerPath,
    const dbus::utility::MapperServiceMap& /*serviceMap*/)
{
    if (ec)
    {
        if (ec == boost::system::errc::io_error)
        {
            // Not found
            BMCWEB_LOG_WARNING("Manager {} not found", managerId);
            messages::resourceNotFound(asyncResp->res, "Manager", managerId);
            return;
        }
        BMCWEB_LOG_ERROR("D-Bus response error {}", ec.value());
        messages::internalError(asyncResp->res);
        return;
    }
    if (managerPath.empty())
    {
        BMCWEB_LOG_WARNING("Manager {} not found", managerId);
        messages::resourceNotFound(asyncResp->res, "Manager", managerId);
        return;
    }

    setLocationIndicatorActive(asyncResp, managerPath, locationIndicatorActive);
}

/**
 * Set the locationIndicatorActive.
 *
 * @param[in,out]   asyncResp                   Async HTTP response.
 * @param[in]       locationIndicatorActive     Value of the property
 */
inline void setLocationIndicatorActiveState(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    bool locationIndicatorActive, const std::string& managerId)
{
    manager_utils::getValidManagerPath(
        asyncResp, managerId,
        std::bind_front(handleSetLocationIndicatorActive, asyncResp,
                        locationIndicatorActive, managerId));
}

inline std::string getBMCUpdateServiceName()
{
    if constexpr (BMCWEB_REDFISH_UPDATESERVICE_USE_DBUS)
    {
        return "xyz.openbmc_project.Software.Manager";
    }
    return "xyz.openbmc_project.Software.BMC.Updater";
}

inline std::string getBMCUpdateServicePath()
{
    if constexpr (BMCWEB_REDFISH_UPDATESERVICE_USE_DBUS)
    {
        return "/xyz/openbmc_project/software/bmc";
    }
    return "/xyz/openbmc_project/software";
}

/**
 * Function reboots the BMC.
 *
 * @param[in] asyncResp - Shared pointer for completing asynchronous calls
 */
inline void doBMCGracefulRestart(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    const char* processName = "xyz.openbmc_project.State.BMC";
    const char* objectPath = "/xyz/openbmc_project/state/bmc0";
    const char* interfaceName = "xyz.openbmc_project.State.BMC";
    const std::string& propertyValue =
        "xyz.openbmc_project.State.BMC.Transition.Reboot";
    const char* destProperty = "RequestedBMCTransition";

    // Create the D-Bus variant for D-Bus call.
    sdbusplus::asio::setProperty(
        *crow::connections::systemBus, processName, objectPath, interfaceName,
        destProperty, propertyValue,
        [asyncResp](const boost::system::error_code& ec) {
            // Use "Set" method to set the property value.
            if (ec)
            {
                BMCWEB_LOG_DEBUG("[Set] Bad D-Bus request error: {}", ec);
                messages::internalError(asyncResp->res);
                return;
            }

            messages::success(asyncResp->res);
        });
}

inline void doBMCForceRestart(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    const char* processName = "xyz.openbmc_project.State.BMC";
    const char* objectPath = "/xyz/openbmc_project/state/bmc0";
    const char* interfaceName = "xyz.openbmc_project.State.BMC";
    const std::string& propertyValue =
        "xyz.openbmc_project.State.BMC.Transition.HardReboot";
    const char* destProperty = "RequestedBMCTransition";

    // Create the D-Bus variant for D-Bus call.
    sdbusplus::asio::setProperty(
        *crow::connections::systemBus, processName, objectPath, interfaceName,
        destProperty, propertyValue,
        [asyncResp](const boost::system::error_code& ec) {
            // Use "Set" method to set the property value.
            if (ec)
            {
                BMCWEB_LOG_DEBUG("[Set] Bad D-Bus request error: {}", ec);
                messages::internalError(asyncResp->res);
                return;
            }

            messages::success(asyncResp->res);
        });
}

/**
 * ManagerResetAction handles POST method request.
 * Analyzes POST body before sending Reset (Reboot) request data to D-Bus.
 * OpenBMC supports ResetType "GracefulRestart" and "ForceRestart".
 */

inline void handleManagerResetAction(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& managerId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    if (managerId != BMCWEB_REDFISH_MANAGER_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "Manager", managerId);
        return;
    }

    BMCWEB_LOG_DEBUG("Post Manager Reset.");

    std::string resetType;

    if (!json_util::readJsonAction(req, asyncResp->res, "ResetType", resetType))
    {
        return;
    }

    if (resetType == "GracefulRestart")
    {
        BMCWEB_LOG_DEBUG("Proceeding with {}", resetType);
        doBMCGracefulRestart(asyncResp);
        return;
    }
    if (resetType == "ForceRestart")
    {
        BMCWEB_LOG_DEBUG("Proceeding with {}", resetType);
        doBMCForceRestart(asyncResp);
        return;
    }
    BMCWEB_LOG_DEBUG("Invalid property value for ResetType: {}", resetType);
    messages::actionParameterNotSupported(asyncResp->res, resetType,
                                          "ResetType");
}

/**
 * Function handles ResetToDefaults POST method request.
 *
 * Analyzes POST body message and factory resets BMC by calling
 * BMC code updater factory reset followed by a BMC reboot.
 *
 * BMC code updater factory reset wipes the whole BMC read-write
 * filesystem which includes things like the network settings.
 *
 * OpenBMC only supports ResetType "ResetAll".
 */

inline void handleManagerResetToDefaultsAction(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& managerId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    if (managerId != BMCWEB_REDFISH_MANAGER_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "Manager", managerId);
        return;
    }

    BMCWEB_LOG_DEBUG("Post ResetToDefaults.");

    std::optional<std::string> resetType;

    if (!json_util::readJsonAction( //
            req, asyncResp->res,    //
            "ResetType", resetType  //
            ))
    {
        BMCWEB_LOG_DEBUG("Missing property ResetType.");

        messages::actionParameterMissing(asyncResp->res, "ResetToDefaults",
                                         "ResetType");
        return;
    }

    if (resetType.value_or("") != "ResetAll")
    {
        BMCWEB_LOG_DEBUG("Invalid property value for ResetType: {}",
                         resetType.value_or(""));
        messages::actionParameterNotSupported(
            asyncResp->res, resetType.value_or(""), "ResetType");
        return;
    }

    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code& ec) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG("Failed to ResetToDefaults: {}", ec);
                messages::internalError(asyncResp->res);
                return;
            }
            // Factory Reset doesn't actually happen until a reboot
            // Can't erase what the BMC is running on
            doBMCGracefulRestart(asyncResp);
        },
        getBMCUpdateServiceName(), getBMCUpdateServicePath(),
        "xyz.openbmc_project.Common.FactoryReset", "Reset");
}

/**
 * ManagerResetActionInfo derived class for delivering Manager
 * ResetType AllowableValues using ResetInfo schema.
 */
inline void handleManagerResetActionInfo(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& managerId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    if (managerId != BMCWEB_REDFISH_MANAGER_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "Manager", managerId);
        return;
    }

    asyncResp->res.jsonValue["@odata.type"] = "#ActionInfo.v1_1_2.ActionInfo";
    asyncResp->res.jsonValue["@odata.id"] =
        boost::urls::format("/redfish/v1/Managers/{}/ResetActionInfo",
                            BMCWEB_REDFISH_MANAGER_URI_NAME);
    asyncResp->res.jsonValue["Name"] = "Reset Action Info";
    asyncResp->res.jsonValue["Id"] = "ResetActionInfo";
    nlohmann::json::object_t parameter;
    parameter["Name"] = "ResetType";
    parameter["Required"] = true;
    parameter["DataType"] = action_info::ParameterTypes::String;

    nlohmann::json::array_t allowableValues;
    allowableValues.emplace_back("GracefulRestart");
    allowableValues.emplace_back("ForceRestart");
    parameter["AllowableValues"] = std::move(allowableValues);

    nlohmann::json::array_t parameters;
    parameters.emplace_back(std::move(parameter));

    asyncResp->res.jsonValue["Parameters"] = std::move(parameters);
}

/**
 * @brief Retrieves BMC manager location data over DBus
 *
 * @param[in] asyncResp Shared pointer for completing asynchronous calls
 * @param[in] connectionName - service name
 * @param[in] path - object path
 * @return none
 */
inline void getLocation(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& connectionName,
                        const std::string& path)
{
    BMCWEB_LOG_DEBUG("Get BMC manager Location data.");

    dbus::utility::getProperty<std::string>(
        connectionName, path,
        "xyz.openbmc_project.Inventory.Decorator.LocationCode", "LocationCode",
        [asyncResp](const boost::system::error_code& ec,
                    const std::string& property) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG("DBUS response error for "
                                 "Location");
                messages::internalError(asyncResp->res);
                return;
            }

            asyncResp->res
                .jsonValue["Location"]["PartLocation"]["ServiceLabel"] =
                property;
        });
}
// avoid name collision systems.hpp
inline void managerGetLastResetTime(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    BMCWEB_LOG_DEBUG("Getting Manager Last Reset Time");

    dbus::utility::getProperty<uint64_t>(
        "xyz.openbmc_project.State.BMC", "/xyz/openbmc_project/state/bmc0",
        "xyz.openbmc_project.State.BMC", "LastRebootTime",
        [asyncResp](const boost::system::error_code& ec,
                    const uint64_t lastResetTime) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG("D-BUS response error {}", ec);
                return;
            }

            // LastRebootTime is epoch time, in milliseconds
            // https://github.com/openbmc/phosphor-dbus-interfaces/blob/7f9a128eb9296e926422ddc312c148b625890bb6/xyz/openbmc_project/State/BMC.interface.yaml#L19
            uint64_t lastResetTimeStamp = lastResetTime / 1000;

            // Convert to ISO 8601 standard
            asyncResp->res.jsonValue["LastResetTime"] =
                redfish::time_utils::getDateTimeUint(lastResetTimeStamp);
        });
}

/**
 * @brief Set the running firmware image
 *
 * @param[i,o] asyncResp - Async response object
 * @param[i] runningFirmwareTarget - Image to make the running image
 *
 * @return void
 */
inline void setActiveFirmwareImage(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& runningFirmwareTarget)
{
    // Get the Id from /redfish/v1/UpdateService/FirmwareInventory/<Id>
    std::string::size_type idPos = runningFirmwareTarget.rfind('/');
    if (idPos == std::string::npos)
    {
        messages::propertyValueNotInList(asyncResp->res, runningFirmwareTarget,
                                         "@odata.id");
        BMCWEB_LOG_DEBUG("Can't parse firmware ID!");
        return;
    }
    idPos++;
    if (idPos >= runningFirmwareTarget.size())
    {
        messages::propertyValueNotInList(asyncResp->res, runningFirmwareTarget,
                                         "@odata.id");
        BMCWEB_LOG_DEBUG("Invalid firmware ID.");
        return;
    }
    std::string firmwareId = runningFirmwareTarget.substr(idPos);

    // Make sure the image is valid before setting priority
    sdbusplus::message::object_path objPath("/xyz/openbmc_project/software");
    dbus::utility::getManagedObjects(
        getBMCUpdateServiceName(), objPath,
        [asyncResp, firmwareId, runningFirmwareTarget](
            const boost::system::error_code& ec,
            const dbus::utility::ManagedObjectType& subtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG("D-Bus response error getting objects.");
                messages::internalError(asyncResp->res);
                return;
            }

            if (subtree.empty())
            {
                BMCWEB_LOG_DEBUG("Can't find image!");
                messages::internalError(asyncResp->res);
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
                messages::propertyValueNotInList(
                    asyncResp->res, runningFirmwareTarget, "@odata.id");
                BMCWEB_LOG_DEBUG("Invalid firmware ID.");
                return;
            }

            BMCWEB_LOG_DEBUG("Setting firmware version {} to priority 0.",
                             firmwareId);

            // Only support Immediate
            // An addition could be a Redfish Setting like
            // ActiveSoftwareImageApplyTime and support OnReset
            sdbusplus::asio::setProperty(
                *crow::connections::systemBus, getBMCUpdateServiceName(),
                "/xyz/openbmc_project/software/" + firmwareId,
                "xyz.openbmc_project.Software.RedundancyPriority", "Priority",
                static_cast<uint8_t>(0),
                [asyncResp](const boost::system::error_code& ec2) {
                    if (ec2)
                    {
                        BMCWEB_LOG_DEBUG("D-Bus response error setting.");
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    doBMCGracefulRestart(asyncResp);
                });
        });
}

inline void afterSetDateTime(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec, const sdbusplus::message_t& msg)
{
    if (ec)
    {
        BMCWEB_LOG_DEBUG("Failed to set elapsed time. DBUS response error {}",
                         ec);
        const sd_bus_error* dbusError = msg.get_error();
        if (dbusError != nullptr)
        {
            std::string_view errorName(dbusError->name);
            if (errorName ==
                "org.freedesktop.timedate1.AutomaticTimeSyncEnabled")
            {
                BMCWEB_LOG_DEBUG("Setting conflict");
                messages::propertyValueConflict(
                    asyncResp->res, "DateTime",
                    "Managers/NetworkProtocol/NTPProcotolEnabled");
                return;
            }
        }
        messages::internalError(asyncResp->res);
        return;
    }
    asyncResp->res.result(boost::beast::http::status::no_content);
}

inline void setDateTime(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& datetime)
{
    BMCWEB_LOG_DEBUG("Set date time: {}", datetime);

    std::optional<redfish::time_utils::usSinceEpoch> us =
        redfish::time_utils::dateStringToEpoch(datetime);
    if (!us)
    {
        messages::propertyValueFormatError(asyncResp->res, datetime,
                                           "DateTime");
        return;
    }
    // Set the absolute datetime
    bool relative = false;
    bool interactive = false;
    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code& ec,
                    const sdbusplus::message_t& msg) {
            afterSetDateTime(asyncResp, ec, msg);
        },
        "org.freedesktop.timedate1", "/org/freedesktop/timedate1",
        "org.freedesktop.timedate1", "SetTime", us->count(), relative,
        interactive);
}

inline void checkForQuiesced(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    dbus::utility::getProperty<std::string>(
        "org.freedesktop.systemd1",
        "/org/freedesktop/systemd1/unit/obmc-bmc-service-quiesce@0.target",
        "org.freedesktop.systemd1.Unit", "ActiveState",
        [asyncResp](const boost::system::error_code& ec,
                    const std::string& val) {
            if (!ec)
            {
                if (val == "active")
                {
                    asyncResp->res.jsonValue["Status"]["Health"] =
                        resource::Health::Critical;
                    asyncResp->res.jsonValue["Status"]["State"] =
                        resource::State::Quiesced;
                    return;
                }
            }
            asyncResp->res.jsonValue["Status"]["Health"] = resource::Health::OK;
            asyncResp->res.jsonValue["Status"]["State"] =
                resource::State::Enabled;
        });
}

inline void getPhysicalAssets(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const dbus::utility::DBusPropertiesMap& propertiesList)
{
    if (ec)
    {
        BMCWEB_LOG_DEBUG("Can't get bmc asset!");
        return;
    }

    const std::string* partNumber = nullptr;
    const std::string* serialNumber = nullptr;
    const std::string* manufacturer = nullptr;
    const std::string* model = nullptr;
    const std::string* sparePartNumber = nullptr;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), propertiesList, "PartNumber",
        partNumber, "SerialNumber", serialNumber, "Manufacturer", manufacturer,
        "Model", model, "SparePartNumber", sparePartNumber);

    if (!success)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    if (partNumber != nullptr)
    {
        asyncResp->res.jsonValue["PartNumber"] = *partNumber;
    }

    if (serialNumber != nullptr)
    {
        asyncResp->res.jsonValue["SerialNumber"] = *serialNumber;
    }

    if (manufacturer != nullptr)
    {
        asyncResp->res.jsonValue["Manufacturer"] = *manufacturer;
    }

    if (model != nullptr)
    {
        asyncResp->res.jsonValue["Model"] = *model;
    }

    if (sparePartNumber != nullptr)
    {
        asyncResp->res.jsonValue["SparePartNumber"] = *sparePartNumber;
    }
}

inline void getManagerData(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const boost::system::error_code& ec,
                           const std::string& managerPath,
                           const dbus::utility::MapperServiceMap& serviceMap)

{
    if (ec)
    {
        BMCWEB_LOG_DEBUG("D-Bus response error on GetSubTree {}", ec);
        return;
    }
    if (managerPath.empty() || serviceMap.empty())
    {
        BMCWEB_LOG_DEBUG("Can't find bmc D-Bus object!");
        return;
    }

    for (const auto& [connectionName, interfaces] : serviceMap)
    {
        for (const auto& interfaceName : interfaces)
        {
            if (interfaceName ==
                "xyz.openbmc_project.Inventory.Decorator.Asset")
            {
                dbus::utility::getAllProperties(
                    *crow::connections::systemBus, connectionName, managerPath,
                    "xyz.openbmc_project.Inventory.Decorator.Asset",
                    std::bind_front(getPhysicalAssets, asyncResp));
            }
            else if (interfaceName ==
                     "xyz.openbmc_project.Inventory.Decorator.LocationCode")
            {
                getLocation(asyncResp, connectionName, managerPath);
            }
            else if (interfaceName ==
                     "xyz.openbmc_project.Association.Definitions")
            {
                getLocationIndicatorActive(asyncResp, managerPath);
            }
        }
    }
}

inline void getManagedChassis(
    const std::string& chassisId,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    asyncResp->res.jsonValue["Links"]["ManagerForChassis@odata.count"] = 1;
    nlohmann::json::array_t managerForChassis;
    nlohmann::json::object_t manager;
    manager["@odata.id"] =
        boost::urls::format("/redfish/v1/Chassis/{}", chassisId);
    managerForChassis.emplace_back(std::move(manager));
    asyncResp->res.jsonValue["Links"]["ManagerForChassis"] =
        std::move(managerForChassis);
}

inline void handleManagerGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& managerId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    if (managerId != BMCWEB_REDFISH_MANAGER_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "Manager", managerId);
        return;
    }

    std::string uuid = persistent_data::getConfig().systemUuid;

    asyncResp->res.jsonValue["@odata.id"] =
        boost::urls::format("/redfish/v1/Managers/{}", managerId);
    asyncResp->res.jsonValue["@odata.type"] = "#Manager.v1_15_0.Manager";
    asyncResp->res.jsonValue["Id"] = managerId;
    asyncResp->res.jsonValue["Name"] = "OpenBmc Manager";
    asyncResp->res.jsonValue["Description"] = "Baseboard Management Controller";
    asyncResp->res.jsonValue["PowerState"] = resource::PowerState::On;

    asyncResp->res.jsonValue["ManagerType"] = manager::ManagerType::BMC;
    asyncResp->res.jsonValue["UUID"] = systemd_utils::getUuid();
    asyncResp->res.jsonValue["ServiceEntryPointUUID"] = uuid;
    asyncResp->res.jsonValue["Model"] = "OpenBmc"; // TODO(ed), get model

    asyncResp->res.jsonValue["LogServices"]["@odata.id"] =
        boost::urls::format("/redfish/v1/Managers/{}/LogServices", managerId);
    asyncResp->res.jsonValue["NetworkProtocol"]["@odata.id"] =
        boost::urls::format("/redfish/v1/Managers/{}/NetworkProtocol",
                            managerId);
    asyncResp->res.jsonValue["EthernetInterfaces"]["@odata.id"] =
        boost::urls::format("/redfish/v1/Managers/{}/EthernetInterfaces",
                            managerId);

    manager_utils::getServiceIdentification(asyncResp, false);

    if constexpr (BMCWEB_VM_NBDPROXY)
    {
        asyncResp->res.jsonValue["VirtualMedia"]["@odata.id"] =
            boost::urls::format("/redfish/v1/Managers/{}/VirtualMedia",
                                managerId);
    }

    // Manager.Reset (an action) can be many values, OpenBMC only
    // supports BMC reboot.
    nlohmann::json& managerReset =
        asyncResp->res.jsonValue["Actions"]["#Manager.Reset"];
    managerReset["target"] = boost::urls::format(
        "/redfish/v1/Managers/{}/Actions/Manager.Reset", managerId);
    managerReset["@Redfish.ActionInfo"] = boost::urls::format(
        "/redfish/v1/Managers/{}/ResetActionInfo", managerId);

    // ResetToDefaults (Factory Reset) has values like
    // PreserveNetworkAndUsers and PreserveNetwork that aren't supported
    // on OpenBMC
    nlohmann::json& resetToDefaults =
        asyncResp->res.jsonValue["Actions"]["#Manager.ResetToDefaults"];
    resetToDefaults["target"] = boost::urls::format(
        "/redfish/v1/Managers/{}/Actions/Manager.ResetToDefaults", managerId);
    resetToDefaults["ResetType@Redfish.AllowableValues"] =
        nlohmann::json::array_t({"ResetAll"});

    std::pair<std::string, std::string> redfishDateTimeOffset =
        redfish::time_utils::getDateTimeOffsetNow();

    asyncResp->res.jsonValue["DateTime"] = redfishDateTimeOffset.first;
    asyncResp->res.jsonValue["DateTimeLocalOffset"] =
        redfishDateTimeOffset.second;

    if constexpr (BMCWEB_KVM)
    {
        // Fill in GraphicalConsole info
        asyncResp->res.jsonValue["GraphicalConsole"]["ServiceEnabled"] = true;
        asyncResp->res.jsonValue["GraphicalConsole"]["MaxConcurrentSessions"] =
            4;
        asyncResp->res.jsonValue["GraphicalConsole"]["ConnectTypesSupported"] =
            nlohmann::json::array_t({"KVMIP"});
    }
    if constexpr (!BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
        asyncResp->res.jsonValue["Links"]["ManagerForServers@odata.count"] = 1;

        nlohmann::json::array_t managerForServers;
        nlohmann::json::object_t manager;
        manager["@odata.id"] = boost::urls::format(
            "/redfish/v1/Systems/{}", BMCWEB_REDFISH_SYSTEM_URI_NAME);
        managerForServers.emplace_back(std::move(manager));

        asyncResp->res.jsonValue["Links"]["ManagerForServers"] =
            std::move(managerForServers);
    }

    sw_util::populateSoftwareInformation(asyncResp, sw_util::bmcPurpose,
                                         "FirmwareVersion", true);

    managerGetLastResetTime(asyncResp);

    // ManagerDiagnosticData is added for all BMCs.
    nlohmann::json& managerDiagnosticData =
        asyncResp->res.jsonValue["ManagerDiagnosticData"];
    managerDiagnosticData["@odata.id"] = boost::urls::format(
        "/redfish/v1/Managers/{}/ManagerDiagnosticData", managerId);

    getMainChassisId(asyncResp, std::bind_front(getManagedChassis));

    dbus::utility::getProperty<double>(
        "org.freedesktop.systemd1", "/org/freedesktop/systemd1",
        "org.freedesktop.systemd1.Manager", "Progress",
        [asyncResp](const boost::system::error_code& ec, double val) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("Error while getting progress");
                messages::internalError(asyncResp->res);
                return;
            }
            if (val < 1.0)
            {
                asyncResp->res.jsonValue["Status"]["Health"] =
                    resource::Health::OK;
                asyncResp->res.jsonValue["Status"]["State"] =
                    resource::State::Starting;
                return;
            }
            checkForQuiesced(asyncResp);
        });

    manager_utils::getValidManagerPath(
        asyncResp, managerId, std::bind_front(getManagerData, asyncResp));

    etag_utils::setEtagOmitDateTimeHandler(asyncResp);

    RedfishService::getInstance(app).handleSubRoute(req, asyncResp);
}

inline void handleManagerPatch(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& managerId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    if (managerId != BMCWEB_REDFISH_MANAGER_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "Manager", managerId);
        return;
    }

    std::optional<std::string> activeSoftwareImageOdataId;
    std::optional<std::string> datetime;
    std::optional<bool> locationIndicatorActive;
    std::optional<nlohmann::json::object_t> pidControllers;
    std::optional<nlohmann::json::object_t> fanControllers;
    std::optional<nlohmann::json::object_t> fanZones;
    std::optional<nlohmann::json::object_t> stepwiseControllers;
    std::optional<std::string> profile;
    std::optional<std::string> serviceIdentification;

    if (!json_util::readJsonPatch(                            //
            req, asyncResp->res,                              //
            "DateTime", datetime,                             //
            "Links/ActiveSoftwareImage/@odata.id",
            activeSoftwareImageOdataId,                       //
            "LocationIndicatorActive",
            locationIndicatorActive,                          //
            "Oem/OpenBmc/Fan/FanControllers", fanControllers, //
            "Oem/OpenBmc/Fan/FanZones", fanZones,             //
            "Oem/OpenBmc/Fan/PidControllers", pidControllers, //
            "Oem/OpenBmc/Fan/Profile", profile,               //
            "Oem/OpenBmc/Fan/StepwiseControllers",
            stepwiseControllers,                              //
            "ServiceIdentification", serviceIdentification    //
            ))
    {
        return;
    }

    if (activeSoftwareImageOdataId)
    {
        setActiveFirmwareImage(asyncResp, *activeSoftwareImageOdataId);
    }

    if (datetime)
    {
        setDateTime(asyncResp, *datetime);
    }

    if (locationIndicatorActive)
    {
        setLocationIndicatorActiveState(asyncResp, *locationIndicatorActive,
                                        managerId);
    }

    if (serviceIdentification)
    {
        manager_utils::setServiceIdentification(asyncResp,
                                                serviceIdentification.value());
    }

    RedfishService::getInstance(app).handleSubRoute(req, asyncResp);
}

inline void handleManagerCollectionGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
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
    bmc["@odata.id"] = boost::urls::format("/redfish/v1/Managers/{}",
                                           BMCWEB_REDFISH_MANAGER_URI_NAME);
    asyncResp->res.jsonValue["Members"] = std::move(members);
}

inline void requestRoutesManager(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Managers/<str>/")
        .privileges(redfish::privileges::getManager)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleManagerGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Managers/<str>/")
        .privileges(redfish::privileges::patchManager)
        .methods(boost::beast::http::verb::patch)(
            std::bind_front(handleManagerPatch, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Managers/")
        .privileges(redfish::privileges::getManagerCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleManagerCollectionGet, std::ref(app)));
}

inline void requestRoutesManagerResetAction(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Managers/<str>/Actions/Manager.Reset/")
        .privileges(redfish::privileges::postManager)
        .methods(boost::beast::http::verb::post)(
            std::bind_front(handleManagerResetAction, std::ref(app)));

    BMCWEB_ROUTE(app,
                 "/redfish/v1/Managers/<str>/Actions/Manager.ResetToDefaults/")
        .privileges(redfish::privileges::postManager)
        .methods(boost::beast::http::verb::post)(
            std::bind_front(handleManagerResetToDefaultsAction, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Managers/<str>/ResetActionInfo/")
        .privileges(redfish::privileges::getActionInfo)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleManagerResetActionInfo, std::ref(app)));
}

} // namespace redfish
