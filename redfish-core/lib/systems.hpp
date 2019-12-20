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

#include "health.hpp"
#include "led.hpp"
#include "pcie.hpp"
#include "redfish_util.hpp"

#include <boost/container/flat_map.hpp>
#include <node.hpp>
#include <utils/fw_utils.hpp>
#include <utils/json_utils.hpp>
#include <variant>

namespace redfish
{

/**
 * @brief Updates the Functional State of DIMMs
 *
 * @param[in] aResp Shared pointer for completing asynchronous calls
 * @param[in] dimmState Dimm's Functional state, true/false
 *
 * @return None.
 */
void updateDimmProperties(std::shared_ptr<AsyncResp> aResp,
                          const std::variant<bool> &dimmState)
{
    const bool *isDimmFunctional = std::get_if<bool>(&dimmState);
    if (isDimmFunctional == nullptr)
    {
        messages::internalError(aResp->res);
        return;
    }
    BMCWEB_LOG_DEBUG << "Dimm Functional: " << *isDimmFunctional;

    // Set it as Enabled if atleast one DIMM is functional
    // Update STATE only if previous State was DISABLED and current Dimm is
    // ENABLED.
    nlohmann::json &prevMemSummary =
        aResp->res.jsonValue["MemorySummary"]["Status"]["State"];
    if (prevMemSummary == "Disabled")
    {
        if (*isDimmFunctional == true)
        {
            aResp->res.jsonValue["MemorySummary"]["Status"]["State"] =
                "Enabled";
        }
    }
}

/*
 * @brief Update "ProcessorSummary" "Count" based on Cpu PresenceState
 *
 * @param[in] aResp Shared pointer for completing asynchronous calls
 * @param[in] cpuPresenceState CPU present or not
 *
 * @return None.
 */
void modifyCpuPresenceState(std::shared_ptr<AsyncResp> aResp,
                            const std::variant<bool> &cpuPresenceState)
{
    const bool *isCpuPresent = std::get_if<bool>(&cpuPresenceState);

    if (isCpuPresent == nullptr)
    {
        messages::internalError(aResp->res);
        return;
    }
    BMCWEB_LOG_DEBUG << "Cpu Present: " << *isCpuPresent;

    if (*isCpuPresent == true)
    {
        nlohmann::json &procCount =
            aResp->res.jsonValue["ProcessorSummary"]["Count"];
        auto procCountPtr =
            procCount.get_ptr<nlohmann::json::number_integer_t *>();
        if (procCountPtr != nullptr)
        {
            // shouldn't be possible to be nullptr
            *procCountPtr += 1;
        }
    }
}

/*
 * @brief Update "ProcessorSummary" "Status" "State" based on
 *        CPU Functional State
 *
 * @param[in] aResp Shared pointer for completing asynchronous calls
 * @param[in] cpuFunctionalState is CPU functional true/false
 *
 * @return None.
 */
void modifyCpuFunctionalState(std::shared_ptr<AsyncResp> aResp,
                              const std::variant<bool> &cpuFunctionalState)
{
    const bool *isCpuFunctional = std::get_if<bool>(&cpuFunctionalState);

    if (isCpuFunctional == nullptr)
    {
        messages::internalError(aResp->res);
        return;
    }
    BMCWEB_LOG_DEBUG << "Cpu Functional: " << *isCpuFunctional;

    nlohmann::json &prevProcState =
        aResp->res.jsonValue["ProcessorSummary"]["Status"]["State"];

    // Set it as Enabled if atleast one CPU is functional
    // Update STATE only if previous State was Non_Functional and current CPU is
    // Functional.
    if (prevProcState == "Disabled")
    {
        if (*isCpuFunctional == true)
        {
            aResp->res.jsonValue["ProcessorSummary"]["Status"]["State"] =
                "Enabled";
        }
    }
}

/*
 * @brief Retrieves computer system properties over dbus
 *
 * @param[in] aResp Shared pointer for completing asynchronous calls
 * @param[in] name  Computer system name from request
 *
 * @return None.
 */
void getComputerSystem(std::shared_ptr<AsyncResp> aResp,
                       std::shared_ptr<HealthPopulate> systemHealth)
{
    BMCWEB_LOG_DEBUG << "Get available system components.";

    crow::connections::systemBus->async_method_call(
        [aResp, systemHealth](
            const boost::system::error_code ec,
            const std::vector<std::pair<
                std::string,
                std::vector<std::pair<std::string, std::vector<std::string>>>>>
                &subtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(aResp->res);
                return;
            }
            // Iterate over all retrieved ObjectPaths.
            for (const std::pair<std::string,
                                 std::vector<std::pair<
                                     std::string, std::vector<std::string>>>>
                     &object : subtree)
            {
                const std::string &path = object.first;
                BMCWEB_LOG_DEBUG << "Got path: " << path;
                const std::vector<
                    std::pair<std::string, std::vector<std::string>>>
                    &connectionNames = object.second;
                if (connectionNames.size() < 1)
                {
                    continue;
                }

                auto memoryHealth = std::make_shared<HealthPopulate>(
                    aResp, aResp->res.jsonValue["MemorySummary"]["Status"]);

                auto cpuHealth = std::make_shared<HealthPopulate>(
                    aResp, aResp->res.jsonValue["ProcessorSummary"]["Status"]);

                systemHealth->children.emplace_back(memoryHealth);
                systemHealth->children.emplace_back(cpuHealth);

                // This is not system, so check if it's cpu, dimm, UUID or
                // BiosVer
                for (const auto &connection : connectionNames)
                {
                    for (const auto &interfaceName : connection.second)
                    {
                        if (interfaceName ==
                            "xyz.openbmc_project.Inventory.Item.Dimm")
                        {
                            BMCWEB_LOG_DEBUG
                                << "Found Dimm, now get its properties.";

                            crow::connections::systemBus->async_method_call(
                                [aResp, service{connection.first},
                                 path(std::move(path))](
                                    const boost::system::error_code ec,
                                    const std::vector<
                                        std::pair<std::string, VariantType>>
                                        &properties) {
                                    if (ec)
                                    {
                                        BMCWEB_LOG_ERROR
                                            << "DBUS response error " << ec;
                                        messages::internalError(aResp->res);
                                        return;
                                    }
                                    BMCWEB_LOG_DEBUG << "Got "
                                                     << properties.size()
                                                     << " Dimm properties.";

                                    if (properties.size() > 0)
                                    {
                                        for (const std::pair<std::string,
                                                             VariantType>
                                                 &property : properties)
                                        {
                                            if (property.first !=
                                                "MemorySizeInKB")
                                            {
                                                continue;
                                            }
                                            const uint32_t *value =
                                                sdbusplus::message::variant_ns::
                                                    get_if<uint32_t>(
                                                        &property.second);
                                            if (value == nullptr)
                                            {
                                                BMCWEB_LOG_DEBUG
                                                    << "Find incorrect type of "
                                                       "MemorySize";
                                                continue;
                                            }
                                            nlohmann::json &totalMemory =
                                                aResp->res
                                                    .jsonValue["MemorySummar"
                                                               "y"]
                                                              ["TotalSystemMe"
                                                               "moryGiB"];
                                            uint64_t *preValue =
                                                totalMemory
                                                    .get_ptr<uint64_t *>();
                                            if (preValue == nullptr)
                                            {
                                                continue;
                                            }
                                            aResp->res
                                                .jsonValue["MemorySummary"]
                                                          ["TotalSystemMemoryGi"
                                                           "B"] =
                                                *value / (1024 * 1024) +
                                                *preValue;
                                            aResp->res
                                                .jsonValue["MemorySummary"]
                                                          ["Status"]["State"] =
                                                "Enabled";
                                        }
                                    }
                                    else
                                    {
                                        auto getDimmProperties =
                                            [aResp](
                                                const boost::system::error_code
                                                    ec,
                                                const std::variant<bool>
                                                    &dimmState) {
                                                if (ec)
                                                {
                                                    BMCWEB_LOG_ERROR
                                                        << "DBUS response "
                                                           "error "
                                                        << ec;
                                                    return;
                                                }
                                                updateDimmProperties(aResp,
                                                                     dimmState);
                                            };
                                        crow::connections::systemBus
                                            ->async_method_call(
                                                std::move(getDimmProperties),
                                                service, path,
                                                "org.freedesktop.DBus."
                                                "Properties",
                                                "Get",
                                                "xyz.openbmc_project.State."
                                                "Decorator.OperationalStatus",
                                                "Functional");
                                    }
                                },
                                connection.first, path,
                                "org.freedesktop.DBus.Properties", "GetAll",
                                "xyz.openbmc_project.Inventory.Item.Dimm");

                            memoryHealth->inventory.emplace_back(path);
                        }
                        else if (interfaceName ==
                                 "xyz.openbmc_project.Inventory.Item.Cpu")
                        {
                            BMCWEB_LOG_DEBUG
                                << "Found Cpu, now get its properties.";

                            crow::connections::systemBus->async_method_call(
                                [aResp, service{connection.first},
                                 path(std::move(path))](
                                    const boost::system::error_code ec,
                                    const std::vector<
                                        std::pair<std::string, VariantType>>
                                        &properties) {
                                    if (ec)
                                    {
                                        BMCWEB_LOG_ERROR
                                            << "DBUS response error " << ec;
                                        messages::internalError(aResp->res);
                                        return;
                                    }
                                    BMCWEB_LOG_DEBUG << "Got "
                                                     << properties.size()
                                                     << " Cpu properties.";

                                    if (properties.size() > 0)
                                    {
                                        for (const auto &property : properties)
                                        {
                                            if (property.first ==
                                                "ProcessorFamily")
                                            {
                                                const std::string *value =
                                                    sdbusplus::message::
                                                        variant_ns::get_if<
                                                            std::string>(
                                                            &property.second);
                                                if (value != nullptr)
                                                {
                                                    nlohmann::json
                                                        &procSummary =
                                                            aResp->res.jsonValue
                                                                ["ProcessorSumm"
                                                                 "ary"];
                                                    nlohmann::json &procCount =
                                                        procSummary["Count"];

                                                    auto procCountPtr =
                                                        procCount.get_ptr<
                                                            nlohmann::json::
                                                                number_integer_t
                                                                    *>();
                                                    if (procCountPtr != nullptr)
                                                    {
                                                        // shouldn't be possible
                                                        // to be nullptr
                                                        *procCountPtr += 1;
                                                    }
                                                    procSummary["Status"]
                                                               ["State"] =
                                                                   "Enabled";
                                                    procSummary["Model"] =
                                                        *value;
                                                }
                                            }
                                        }
                                    }
                                    else
                                    {
                                        auto getCpuPresenceState =
                                            [aResp](
                                                const boost::system::error_code
                                                    ec,
                                                const std::variant<bool>
                                                    &cpuPresenceCheck) {
                                                if (ec)
                                                {
                                                    BMCWEB_LOG_ERROR
                                                        << "DBUS response "
                                                           "error "
                                                        << ec;
                                                    return;
                                                }
                                                modifyCpuPresenceState(
                                                    aResp, cpuPresenceCheck);
                                            };

                                        auto getCpuFunctionalState =
                                            [aResp](
                                                const boost::system::error_code
                                                    ec,
                                                const std::variant<bool>
                                                    &cpuFunctionalCheck) {
                                                if (ec)
                                                {
                                                    BMCWEB_LOG_ERROR
                                                        << "DBUS response "
                                                           "error "
                                                        << ec;
                                                    return;
                                                }
                                                modifyCpuFunctionalState(
                                                    aResp, cpuFunctionalCheck);
                                            };
                                        // Get the Presence of CPU
                                        crow::connections::systemBus
                                            ->async_method_call(
                                                std::move(getCpuPresenceState),
                                                service, path,
                                                "org.freedesktop.DBus."
                                                "Properties",
                                                "Get",
                                                "xyz.openbmc_project.Inventory."
                                                "Item",
                                                "Present");

                                        // Get the Functional State
                                        crow::connections::systemBus
                                            ->async_method_call(
                                                std::move(
                                                    getCpuFunctionalState),
                                                service, path,
                                                "org.freedesktop.DBus."
                                                "Properties",
                                                "Get",
                                                "xyz.openbmc_project.State."
                                                "Decorator."
                                                "OperationalStatus",
                                                "Functional");

                                        // Get the MODEL from
                                        // xyz.openbmc_project.Inventory.Decorator.Asset
                                        // support it later as Model  is Empty
                                        // currently.
                                    }
                                },
                                connection.first, path,
                                "org.freedesktop.DBus.Properties", "GetAll",
                                "xyz.openbmc_project.Inventory.Item.Cpu");

                            cpuHealth->inventory.emplace_back(path);
                        }
                        else if (interfaceName ==
                                 "xyz.openbmc_project.Common.UUID")
                        {
                            BMCWEB_LOG_DEBUG
                                << "Found UUID, now get its properties.";
                            crow::connections::systemBus->async_method_call(
                                [aResp](const boost::system::error_code ec,
                                        const std::vector<
                                            std::pair<std::string, VariantType>>
                                            &properties) {
                                    if (ec)
                                    {
                                        BMCWEB_LOG_DEBUG
                                            << "DBUS response error " << ec;
                                        messages::internalError(aResp->res);
                                        return;
                                    }
                                    BMCWEB_LOG_DEBUG << "Got "
                                                     << properties.size()
                                                     << " UUID properties.";
                                    for (const std::pair<std::string,
                                                         VariantType>
                                             &property : properties)
                                    {
                                        if (property.first == "UUID")
                                        {
                                            const std::string *value =
                                                sdbusplus::message::variant_ns::
                                                    get_if<std::string>(
                                                        &property.second);

                                            if (value != nullptr)
                                            {
                                                std::string valueStr = *value;
                                                if (valueStr.size() == 32)
                                                {
                                                    valueStr.insert(8, 1, '-');
                                                    valueStr.insert(13, 1, '-');
                                                    valueStr.insert(18, 1, '-');
                                                    valueStr.insert(23, 1, '-');
                                                }
                                                BMCWEB_LOG_DEBUG << "UUID = "
                                                                 << valueStr;
                                                aResp->res.jsonValue["UUID"] =
                                                    valueStr;
                                            }
                                        }
                                    }
                                },
                                connection.first, path,
                                "org.freedesktop.DBus.Properties", "GetAll",
                                "xyz.openbmc_project.Common.UUID");
                        }
                        else if (interfaceName ==
                                 "xyz.openbmc_project.Inventory.Item.System")
                        {
                            crow::connections::systemBus->async_method_call(
                                [aResp](const boost::system::error_code ec,
                                        const std::vector<
                                            std::pair<std::string, VariantType>>
                                            &propertiesList) {
                                    if (ec)
                                    {
                                        // doesn't have to include this
                                        // interface
                                        return;
                                    }
                                    BMCWEB_LOG_DEBUG
                                        << "Got " << propertiesList.size()
                                        << " properties for system";
                                    for (const std::pair<std::string,
                                                         VariantType>
                                             &property : propertiesList)
                                    {
                                        const std::string &propertyName =
                                            property.first;
                                        if ((propertyName == "PartNumber") ||
                                            (propertyName == "SerialNumber") ||
                                            (propertyName == "Manufacturer") ||
                                            (propertyName == "Model"))
                                        {
                                            const std::string *value =
                                                std::get_if<std::string>(
                                                    &property.second);
                                            if (value != nullptr)
                                            {
                                                aResp->res
                                                    .jsonValue[propertyName] =
                                                    *value;
                                            }
                                        }
                                    }
                                    aResp->res.jsonValue["Name"] = "system";
                                    aResp->res.jsonValue["Id"] =
                                        aResp->res.jsonValue["SerialNumber"];
                                    // Grab the bios version
                                    fw_util::getActiveFwVersion(
                                        aResp, fw_util::biosPurpose,
                                        "BiosVersion");
                                },
                                connection.first, path,
                                "org.freedesktop.DBus.Properties", "GetAll",
                                "xyz.openbmc_project.Inventory.Decorator."
                                "Asset");

                            crow::connections::systemBus->async_method_call(
                                [aResp](
                                    const boost::system::error_code ec,
                                    const std::variant<std::string> &property) {
                                    if (ec)
                                    {
                                        // doesn't have to include this
                                        // interface
                                        return;
                                    }

                                    const std::string *value =
                                        std::get_if<std::string>(&property);
                                    if (value != nullptr)
                                    {
                                        aResp->res.jsonValue["AssetTag"] =
                                            *value;
                                    }
                                },
                                connection.first, path,
                                "org.freedesktop.DBus.Properties", "Get",
                                "xyz.openbmc_project.Inventory.Decorator."
                                "AssetTag",
                                "AssetTag");
                        }
                    }
                }
            }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", int32_t(0),
        std::array<const char *, 5>{
            "xyz.openbmc_project.Inventory.Decorator.Asset",
            "xyz.openbmc_project.Inventory.Item.Cpu",
            "xyz.openbmc_project.Inventory.Item.Dimm",
            "xyz.openbmc_project.Inventory.Item.System",
            "xyz.openbmc_project.Common.UUID",
        });
}

/**
 * @brief Retrieves host state properties over dbus
 *
 * @param[in] aResp     Shared pointer for completing asynchronous calls.
 *
 * @return None.
 */
void getHostState(std::shared_ptr<AsyncResp> aResp)
{
    BMCWEB_LOG_DEBUG << "Get host information.";
    crow::connections::systemBus->async_method_call(
        [aResp](const boost::system::error_code ec,
                const std::variant<std::string> &hostState) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                messages::internalError(aResp->res);
                return;
            }

            const std::string *s = std::get_if<std::string>(&hostState);
            BMCWEB_LOG_DEBUG << "Host state: " << *s;
            if (s != nullptr)
            {
                // Verify Host State
                if (*s == "xyz.openbmc_project.State.Host.HostState.Running")
                {
                    aResp->res.jsonValue["PowerState"] = "On";
                    aResp->res.jsonValue["Status"]["State"] = "Enabled";
                }
                else
                {
                    aResp->res.jsonValue["PowerState"] = "Off";
                    aResp->res.jsonValue["Status"]["State"] = "Disabled";
                }
            }
        },
        "xyz.openbmc_project.State.Host", "/xyz/openbmc_project/state/host0",
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.State.Host", "CurrentHostState");
}

/**
 * @brief Traslates boot source DBUS property value to redfish.
 *
 * @param[in] dbusSource    The boot source in DBUS speak.
 *
 * @return Returns as a string, the boot source in Redfish terms. If translation
 * cannot be done, returns an empty string.
 */
static std::string dbusToRfBootSource(const std::string &dbusSource)
{
    if (dbusSource == "xyz.openbmc_project.Control.Boot.Source.Sources.Default")
    {
        return "None";
    }
    else if (dbusSource ==
             "xyz.openbmc_project.Control.Boot.Source.Sources.Disk")
    {
        return "Hdd";
    }
    else if (dbusSource ==
             "xyz.openbmc_project.Control.Boot.Source.Sources.ExternalMedia")
    {
        return "Cd";
    }
    else if (dbusSource ==
             "xyz.openbmc_project.Control.Boot.Source.Sources.Network")
    {
        return "Pxe";
    }
    else if (dbusSource ==
             "xyz.openbmc_project.Control.Boot.Source.Sources.RemovableMedia")
    {
        return "Usb";
    }
    else
    {
        return "";
    }
}

/**
 * @brief Traslates boot mode DBUS property value to redfish.
 *
 * @param[in] dbusMode    The boot mode in DBUS speak.
 *
 * @return Returns as a string, the boot mode in Redfish terms. If translation
 * cannot be done, returns an empty string.
 */
static std::string dbusToRfBootMode(const std::string &dbusMode)
{
    if (dbusMode == "xyz.openbmc_project.Control.Boot.Mode.Modes.Regular")
    {
        return "None";
    }
    else if (dbusMode == "xyz.openbmc_project.Control.Boot.Mode.Modes.Safe")
    {
        return "Diags";
    }
    else if (dbusMode == "xyz.openbmc_project.Control.Boot.Mode.Modes.Setup")
    {
        return "BiosSetup";
    }
    else
    {
        return "";
    }
}

/**
 * @brief Traslates boot source from Redfish to the DBus boot paths.
 *
 * @param[in] rfSource    The boot source in Redfish.
 * @param[out] bootSource The DBus source
 * @param[out] bootMode   the DBus boot mode
 *
 * @return Integer error code.
 */
static int assignBootParameters(std::shared_ptr<AsyncResp> aResp,
                                const std::string &rfSource,
                                std::string &bootSource, std::string &bootMode)
{
    // The caller has initialized the bootSource and bootMode to:
    // bootMode = "xyz.openbmc_project.Control.Boot.Mode.Modes.Regular";
    // bootSource = "xyz.openbmc_project.Control.Boot.Source.Sources.Default";
    // Only modify the bootSource/bootMode variable needed to achieve the
    // desired boot action.

    if (rfSource == "None")
    {
        return 0;
    }
    else if (rfSource == "Pxe")
    {
        bootSource = "xyz.openbmc_project.Control.Boot.Source.Sources.Network";
    }
    else if (rfSource == "Hdd")
    {
        bootSource = "xyz.openbmc_project.Control.Boot.Source.Sources.Disk";
    }
    else if (rfSource == "Diags")
    {
        bootMode = "xyz.openbmc_project.Control.Boot.Mode.Modes.Safe";
    }
    else if (rfSource == "Cd")
    {
        bootSource =
            "xyz.openbmc_project.Control.Boot.Source.Sources.ExternalMedia";
    }
    else if (rfSource == "BiosSetup")
    {
        bootMode = "xyz.openbmc_project.Control.Boot.Mode.Modes.Setup";
    }
    else if (rfSource == "Usb")
    {
        bootSource =
            "xyz.openbmc_project.Control.Boot.Source.Sources.RemovableMedia";
    }
    else
    {
        BMCWEB_LOG_DEBUG << "Invalid property value for "
                            "BootSourceOverrideTarget: "
                         << bootSource;
        messages::propertyValueNotInList(aResp->res, rfSource,
                                         "BootSourceTargetOverride");
        return -1;
    }
    return 0;
}

/**
 * @brief Retrieves boot mode over DBUS and fills out the response
 *
 * @param[in] aResp         Shared pointer for generating response message.
 * @param[in] bootDbusObj   The dbus object to query for boot properties.
 *
 * @return None.
 */
static void getBootMode(std::shared_ptr<AsyncResp> aResp,
                        std::string bootDbusObj)
{
    crow::connections::systemBus->async_method_call(
        [aResp](const boost::system::error_code ec,
                const std::variant<std::string> &bootMode) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                messages::internalError(aResp->res);
                return;
            }

            const std::string *bootModeStr =
                std::get_if<std::string>(&bootMode);

            if (!bootModeStr)
            {
                messages::internalError(aResp->res);
                return;
            }

            BMCWEB_LOG_DEBUG << "Boot mode: " << *bootModeStr;

            // TODO (Santosh): Do we need to support override mode?
            aResp->res.jsonValue["Boot"]["BootSourceOverrideMode"] = "Legacy";
            aResp->res.jsonValue["Boot"]["BootSourceOverrideTarget@Redfish."
                                         "AllowableValues"] = {
                "None", "Pxe", "Hdd", "Cd", "Diags", "BiosSetup", "Usb"};

            if (*bootModeStr !=
                "xyz.openbmc_project.Control.Boot.Mode.Modes.Regular")
            {
                auto rfMode = dbusToRfBootMode(*bootModeStr);
                if (!rfMode.empty())
                {
                    aResp->res.jsonValue["Boot"]["BootSourceOverrideTarget"] =
                        rfMode;
                }
            }

            // If the BootSourceOverrideTarget is still "None" at the end,
            // reset the BootSourceOverrideEnabled to indicate that
            // overrides are disabled
            if (aResp->res.jsonValue["Boot"]["BootSourceOverrideTarget"] ==
                "None")
            {
                aResp->res.jsonValue["Boot"]["BootSourceOverrideEnabled"] =
                    "Disabled";
            }
        },
        "xyz.openbmc_project.Settings", bootDbusObj,
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Control.Boot.Mode", "BootMode");
}

/**
 * @brief Retrieves boot source over DBUS
 *
 * @param[in] aResp         Shared pointer for generating response message.
 * @param[in] oneTimeEnable Boolean to indicate boot properties are one-time.
 *
 * @return None.
 */
static void getBootSource(std::shared_ptr<AsyncResp> aResp, bool oneTimeEnabled)
{
    std::string bootDbusObj =
        oneTimeEnabled ? "/xyz/openbmc_project/control/host0/boot/one_time"
                       : "/xyz/openbmc_project/control/host0/boot";

    BMCWEB_LOG_DEBUG << "Is one time: " << oneTimeEnabled;
    aResp->res.jsonValue["Boot"]["BootSourceOverrideEnabled"] =
        (oneTimeEnabled) ? "Once" : "Continuous";

    crow::connections::systemBus->async_method_call(
        [aResp, bootDbusObj](const boost::system::error_code ec,
                             const std::variant<std::string> &bootSource) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                messages::internalError(aResp->res);
                return;
            }

            const std::string *bootSourceStr =
                std::get_if<std::string>(&bootSource);

            if (!bootSourceStr)
            {
                messages::internalError(aResp->res);
                return;
            }
            BMCWEB_LOG_DEBUG << "Boot source: " << *bootSourceStr;

            auto rfSource = dbusToRfBootSource(*bootSourceStr);
            if (!rfSource.empty())
            {
                aResp->res.jsonValue["Boot"]["BootSourceOverrideTarget"] =
                    rfSource;
            }
        },
        "xyz.openbmc_project.Settings", bootDbusObj,
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Control.Boot.Source", "BootSource");
    getBootMode(std::move(aResp), std::move(bootDbusObj));
}

/**
 * @brief Retrieves "One time" enabled setting over DBUS and calls function to
 * get boot source and boot mode.
 *
 * @param[in] aResp     Shared pointer for generating response message.
 *
 * @return None.
 */
static void getBootProperties(std::shared_ptr<AsyncResp> aResp)
{
    BMCWEB_LOG_DEBUG << "Get boot information.";

    crow::connections::systemBus->async_method_call(
        [aResp](const boost::system::error_code ec,
                const sdbusplus::message::variant<bool> &oneTime) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                // not an error, don't have to have the interface
                return;
            }

            const bool *oneTimePtr = std::get_if<bool>(&oneTime);

            if (!oneTimePtr)
            {
                messages::internalError(aResp->res);
                return;
            }
            getBootSource(aResp, *oneTimePtr);
        },
        "xyz.openbmc_project.Settings",
        "/xyz/openbmc_project/control/host0/boot/one_time",
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Object.Enable", "Enabled");
}

/**
 * @brief Sets boot properties into DBUS object(s).
 *
 * @param[in] aResp           Shared pointer for generating response message.
 * @param[in] oneTimeEnabled  Is "one-time" setting already enabled.
 * @param[in] bootSource      The boot source to set.
 * @param[in] bootEnable      The source override "enable" to set.
 *
 * @return Integer error code.
 */
static void setBootModeOrSource(std::shared_ptr<AsyncResp> aResp,
                                bool oneTimeEnabled,
                                std::optional<std::string> bootSource,
                                std::optional<std::string> bootEnable)
{
    std::string bootSourceStr =
        "xyz.openbmc_project.Control.Boot.Source.Sources.Default";
    std::string bootModeStr =
        "xyz.openbmc_project.Control.Boot.Mode.Modes.Regular";
    bool oneTimeSetting = oneTimeEnabled;
    bool useBootSource = true;

    // Validate incoming parameters
    if (bootEnable)
    {
        if (*bootEnable == "Once")
        {
            oneTimeSetting = true;
        }
        else if (*bootEnable == "Continuous")
        {
            oneTimeSetting = false;
        }
        else if (*bootEnable == "Disabled")
        {
            BMCWEB_LOG_DEBUG << "Boot source override will be disabled";
            oneTimeSetting = false;
            useBootSource = false;
        }
        else
        {
            BMCWEB_LOG_DEBUG << "Unsupported value for "
                                "BootSourceOverrideEnabled: "
                             << *bootEnable;
            messages::propertyValueNotInList(aResp->res, *bootEnable,
                                             "BootSourceOverrideEnabled");
            return;
        }
    }

    if (bootSource && useBootSource)
    {
        // Source target specified
        BMCWEB_LOG_DEBUG << "Boot source: " << *bootSource;
        // Figure out which DBUS interface and property to use
        if (assignBootParameters(aResp, *bootSource, bootSourceStr,
                                 bootModeStr))
        {
            BMCWEB_LOG_DEBUG
                << "Invalid property value for BootSourceOverrideTarget: "
                << *bootSource;
            messages::propertyValueNotInList(aResp->res, *bootSource,
                                             "BootSourceTargetOverride");
            return;
        }
    }

    // Act on validated parameters
    BMCWEB_LOG_DEBUG << "DBUS boot source: " << bootSourceStr;
    BMCWEB_LOG_DEBUG << "DBUS boot mode: " << bootModeStr;
    const char *bootObj =
        oneTimeSetting ? "/xyz/openbmc_project/control/host0/boot/one_time"
                       : "/xyz/openbmc_project/control/host0/boot";

    crow::connections::systemBus->async_method_call(
        [aResp](const boost::system::error_code ec) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                messages::internalError(aResp->res);
                return;
            }
            BMCWEB_LOG_DEBUG << "Boot source update done.";
        },
        "xyz.openbmc_project.Settings", bootObj,
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Control.Boot.Source", "BootSource",
        std::variant<std::string>(bootSourceStr));

    crow::connections::systemBus->async_method_call(
        [aResp](const boost::system::error_code ec) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                messages::internalError(aResp->res);
                return;
            }
            BMCWEB_LOG_DEBUG << "Boot mode update done.";
        },
        "xyz.openbmc_project.Settings", bootObj,
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Control.Boot.Mode", "BootMode",
        std::variant<std::string>(bootModeStr));

    crow::connections::systemBus->async_method_call(
        [aResp{std::move(aResp)}](const boost::system::error_code ec) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                messages::internalError(aResp->res);
                return;
            }
            BMCWEB_LOG_DEBUG << "Boot enable update done.";
        },
        "xyz.openbmc_project.Settings",
        "/xyz/openbmc_project/control/host0/boot/one_time",
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Object.Enable", "Enabled",
        std::variant<bool>(oneTimeSetting));
}

/**
 * @brief Retrieves "One time" enabled setting over DBUS and calls function to
 * set boot source/boot mode properties.
 *
 * @param[in] aResp      Shared pointer for generating response message.
 * @param[in] bootSource The boot source from incoming RF request.
 * @param[in] bootEnable The boot override enable from incoming RF request.
 *
 * @return Integer error code.
 */
static void setBootProperties(std::shared_ptr<AsyncResp> aResp,
                              std::optional<std::string> bootSource,
                              std::optional<std::string> bootEnable)
{
    BMCWEB_LOG_DEBUG << "Set boot information.";

    crow::connections::systemBus->async_method_call(
        [aResp, bootSource{std::move(bootSource)},
         bootEnable{std::move(bootEnable)}](
            const boost::system::error_code ec,
            const sdbusplus::message::variant<bool> &oneTime) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                messages::internalError(aResp->res);
                return;
            }

            const bool *oneTimePtr = std::get_if<bool>(&oneTime);

            if (!oneTimePtr)
            {
                messages::internalError(aResp->res);
                return;
            }

            BMCWEB_LOG_DEBUG << "Got one time: " << *oneTimePtr;

            setBootModeOrSource(aResp, *oneTimePtr, std::move(bootSource),
                                std::move(bootEnable));
        },
        "xyz.openbmc_project.Settings",
        "/xyz/openbmc_project/control/host0/boot/one_time",
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Object.Enable", "Enabled");
}

#ifdef BMCWEB_ENABLE_REDFISH_PROVISIONING_FEATURE
/**
 * @brief Retrieves provisioning status
 *
 * @param[in] aResp     Shared pointer for completing asynchronous calls.
 *
 * @return None.
 */
void getProvisioningStatus(std::shared_ptr<AsyncResp> aResp)
{
    BMCWEB_LOG_DEBUG << "Get OEM information.";
    crow::connections::systemBus->async_method_call(
        [aResp](const boost::system::error_code ec,
                const std::vector<std::pair<std::string, VariantType>>
                    &propertiesList) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                messages::internalError(aResp->res);
                return;
            }

            const bool *provState = nullptr;
            const bool *lockState = nullptr;
            for (const std::pair<std::string, VariantType> &property :
                 propertiesList)
            {
                if (property.first == "UfmProvisioned")
                {
                    provState = std::get_if<bool>(&property.second);
                }
                else if (property.first == "UfmLocked")
                {
                    lockState = std::get_if<bool>(&property.second);
                }
            }

            if ((provState == nullptr) || (lockState == nullptr))
            {
                BMCWEB_LOG_DEBUG << "Unable to get PFR attributes.";
                messages::internalError(aResp->res);
                return;
            }

            nlohmann::json &oemPFR =
                aResp->res.jsonValue["Oem"]["OpenBmc"]["FirmwareProvisioning"];
            if (*provState == true)
            {
                if (*lockState == true)
                {
                    oemPFR["ProvisioningStatus"] = "ProvisionedAndLocked";
                }
                else
                {
                    oemPFR["ProvisioningStatus"] = "ProvisionedButNotLocked";
                }
            }
            else
            {
                oemPFR["ProvisioningStatus"] = "NotProvisioned";
            }
        },
        "xyz.openbmc_project.PFR.Manager", "/xyz/openbmc_project/pfr",
        "org.freedesktop.DBus.Properties", "GetAll",
        "xyz.openbmc_project.PFR.Attributes");
}
#endif

/**
 * @brief Translates watchdog timeout action DBUS property value to redfish.
 *
 * @param[in] dbusAction    The watchdog timeout action in D-BUS.
 *
 * @return Returns as a string, the timeout action in Redfish terms. If
 * translation cannot be done, returns an empty string.
 */
static std::string dbusToRfWatchdogAction(const std::string &dbusAction)
{
    if (dbusAction == "xyz.openbmc_project.State.Watchdog.Action.None")
    {
        return "None";
    }
    else if (dbusAction ==
             "xyz.openbmc_project.State.Watchdog.Action.HardReset")
    {
        return "ResetSystem";
    }
    else if (dbusAction == "xyz.openbmc_project.State.Watchdog.Action.PowerOff")
    {
        return "PowerDown";
    }
    else if (dbusAction ==
             "xyz.openbmc_project.State.Watchdog.Action.PowerCycle")
    {
        return "PowerCycle";
    }

    return "";
}

/**
 *@brief Translates timeout action from Redfish to DBUS property value.
 *
 *@param[in] rfAction The timeout action in Redfish.
 *
 *@return Returns as a string, the time_out action as expected by DBUS.
 *If translation cannot be done, returns an empty string.
 */

static std::string rfToDbusWDTTimeOutAct(const std::string &rfAction)
{
    if (rfAction == "None")
    {
        return "xyz.openbmc_project.State.Watchdog.Action.None";
    }
    else if (rfAction == "PowerCycle")
    {
        return "xyz.openbmc_project.State.Watchdog.Action.PowerCycle";
    }
    else if (rfAction == "PowerDown")
    {
        return "xyz.openbmc_project.State.Watchdog.Action.PowerOff";
    }
    else if (rfAction == "ResetSystem")
    {
        return "xyz.openbmc_project.State.Watchdog.Action.HardReset";
    }

    return "";
}

/**
 * @brief Retrieves host watchdog timer properties over DBUS
 *
 * @param[in] aResp     Shared pointer for completing asynchronous calls.
 *
 * @return None.
 */
void getHostWatchdogTimer(std::shared_ptr<AsyncResp> aResp)
{
    BMCWEB_LOG_DEBUG << "Get host watchodg";
    crow::connections::systemBus->async_method_call(
        [aResp](const boost::system::error_code ec,
                PropertiesType &properties) {
            if (ec)
            {
                // watchdog service is stopped
                BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                return;
            }

            BMCWEB_LOG_DEBUG << "Got " << properties.size() << " wdt prop.";

            nlohmann::json &hostWatchdogTimer =
                aResp->res.jsonValue["HostWatchdogTimer"];

            // watchdog service is running/enabled
            hostWatchdogTimer["Status"]["State"] = "Enabled";

            for (const auto &property : properties)
            {
                BMCWEB_LOG_DEBUG << "prop=" << property.first;
                if (property.first == "Enabled")
                {
                    const bool *state = std::get_if<bool>(&property.second);

                    if (!state)
                    {
                        messages::internalError(aResp->res);
                        continue;
                    }

                    hostWatchdogTimer["FunctionEnabled"] = *state;
                }
                else if (property.first == "ExpireAction")
                {
                    const std::string *s =
                        std::get_if<std::string>(&property.second);
                    if (!s)
                    {
                        messages::internalError(aResp->res);
                        continue;
                    }

                    std::string action = dbusToRfWatchdogAction(*s);
                    if (action.empty())
                    {
                        messages::internalError(aResp->res);
                        continue;
                    }
                    hostWatchdogTimer["TimeoutAction"] = action;
                }
            }
        },
        "xyz.openbmc_project.Watchdog", "/xyz/openbmc_project/watchdog/host0",
        "org.freedesktop.DBus.Properties", "GetAll",
        "xyz.openbmc_project.State.Watchdog");
}

/**
 * @brief Sets Host WatchDog Timer properties.
 *
 * @param[in] aResp      Shared pointer for generating response message.
 * @param[in] wdtEnable  The WDTimer Enable value (true/false) from incoming
 *                       RF request.
 * @param[in] wdtTimeOutAction The WDT Timeout action, from incoming RF request.
 *
 * @return None.
 */
static void setWDTProperties(std::shared_ptr<AsyncResp> aResp,
                             const std::optional<bool> wdtEnable,
                             const std::optional<std::string> &wdtTimeOutAction)
{
    BMCWEB_LOG_DEBUG << "Set host watchdog";

    if (wdtTimeOutAction)
    {
        std::string wdtTimeOutActStr = rfToDbusWDTTimeOutAct(*wdtTimeOutAction);
        // check if TimeOut Action is Valid
        if (wdtTimeOutActStr.empty())
        {
            BMCWEB_LOG_DEBUG << "Unsupported value for TimeoutAction: "
                             << *wdtTimeOutAction;
            messages::propertyValueNotInList(aResp->res, *wdtTimeOutAction,
                                             "TimeoutAction");
            return;
        }

        crow::connections::systemBus->async_method_call(
            [aResp](const boost::system::error_code ec) {
                if (ec)
                {
                    BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                    messages::internalError(aResp->res);
                    return;
                }
            },
            "xyz.openbmc_project.Watchdog",
            "/xyz/openbmc_project/watchdog/host0",
            "org.freedesktop.DBus.Properties", "Set",
            "xyz.openbmc_project.State.Watchdog", "ExpireAction",
            std::variant<std::string>(wdtTimeOutActStr));
    }

    if (wdtEnable)
    {
        crow::connections::systemBus->async_method_call(
            [aResp](const boost::system::error_code ec) {
                if (ec)
                {
                    BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                    messages::internalError(aResp->res);
                    return;
                }
            },
            "xyz.openbmc_project.Watchdog",
            "/xyz/openbmc_project/watchdog/host0",
            "org.freedesktop.DBus.Properties", "Set",
            "xyz.openbmc_project.State.Watchdog", "Enabled",
            std::variant<bool>(*wdtEnable));
    }
}

/**
 * SystemsCollection derived class for delivering ComputerSystems Collection
 * Schema
 */
class SystemsCollection : public Node
{
  public:
    SystemsCollection(CrowApp &app) : Node(app, "/redfish/v1/Systems/")
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::put, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::post, {{"ConfigureComponents"}}}};
    }

  private:
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        res.jsonValue["@odata.type"] =
            "#ComputerSystemCollection.ComputerSystemCollection";
        res.jsonValue["@odata.id"] = "/redfish/v1/Systems";
        res.jsonValue["@odata.context"] =
            "/redfish/v1/"
            "$metadata#ComputerSystemCollection.ComputerSystemCollection";
        res.jsonValue["Name"] = "Computer System Collection";
        res.jsonValue["Members"] = {
            {{"@odata.id", "/redfish/v1/Systems/system"}}};
        res.jsonValue["Members@odata.count"] = 1;
        res.end();
    }
};

/**
 * SystemActionsReset class supports handle POST method for Reset action.
 * The class retrieves and sends data directly to D-Bus.
 */
class SystemActionsReset : public Node
{
  public:
    SystemActionsReset(CrowApp &app) :
        Node(app, "/redfish/v1/Systems/system/Actions/ComputerSystem.Reset/")
    {
        entityPrivileges = {
            {boost::beast::http::verb::post, {{"ConfigureComponents"}}}};
    }

  private:
    /**
     * Function handles POST method request.
     * Analyzes POST body message before sends Reset request data to D-Bus.
     */
    void doPost(crow::Response &res, const crow::Request &req,
                const std::vector<std::string> &params) override
    {
        auto asyncResp = std::make_shared<AsyncResp>(res);

        std::string resetType;
        if (!json_util::readJson(req, res, "ResetType", resetType))
        {
            return;
        }

        // Get the command and host vs. chassis
        std::string command;
        bool hostCommand;
        if (resetType == "On")
        {
            command = "xyz.openbmc_project.State.Host.Transition.On";
            hostCommand = true;
        }
        else if (resetType == "ForceOff")
        {
            command = "xyz.openbmc_project.State.Chassis.Transition.Off";
            hostCommand = false;
        }
        else if (resetType == "ForceOn")
        {
            command = "xyz.openbmc_project.State.Host.Transition.On";
            hostCommand = true;
        }
        else if (resetType == "ForceRestart")
        {
            command = "xyz.openbmc_project.State.Chassis.Transition.Reset";
            hostCommand = false;
        }
        else if (resetType == "GracefulShutdown")
        {
            command = "xyz.openbmc_project.State.Host.Transition.Off";
            hostCommand = true;
        }
        else if (resetType == "GracefulRestart")
        {
            command = "xyz.openbmc_project.State.Host.Transition.Reboot";
            hostCommand = true;
        }
        else if (resetType == "PowerCycle")
        {
            command = "xyz.openbmc_project.State.Chassis.Transition.PowerCycle";
            hostCommand = false;
        }
        else if (resetType == "Nmi")
        {
            doNMI(asyncResp);
            return;
        }
        else
        {
            messages::actionParameterUnknown(res, "Reset", resetType);
            return;
        }

        if (hostCommand)
        {
            crow::connections::systemBus->async_method_call(
                [asyncResp, resetType](const boost::system::error_code ec) {
                    if (ec)
                    {
                        BMCWEB_LOG_ERROR << "D-Bus responses error: " << ec;
                        if (ec.value() == boost::asio::error::invalid_argument)
                        {
                            messages::actionParameterNotSupported(
                                asyncResp->res, resetType, "Reset");
                        }
                        else
                        {
                            messages::internalError(asyncResp->res);
                        }
                        return;
                    }
                    messages::success(asyncResp->res);
                },
                "xyz.openbmc_project.State.Host",
                "/xyz/openbmc_project/state/host0",
                "org.freedesktop.DBus.Properties", "Set",
                "xyz.openbmc_project.State.Host", "RequestedHostTransition",
                std::variant<std::string>{command});
        }
        else
        {
            crow::connections::systemBus->async_method_call(
                [asyncResp, resetType](const boost::system::error_code ec) {
                    if (ec)
                    {
                        BMCWEB_LOG_ERROR << "D-Bus responses error: " << ec;
                        if (ec.value() == boost::asio::error::invalid_argument)
                        {
                            messages::actionParameterNotSupported(
                                asyncResp->res, resetType, "Reset");
                        }
                        else
                        {
                            messages::internalError(asyncResp->res);
                        }
                        return;
                    }
                    messages::success(asyncResp->res);
                },
                "xyz.openbmc_project.State.Chassis",
                "/xyz/openbmc_project/state/chassis0",
                "org.freedesktop.DBus.Properties", "Set",
                "xyz.openbmc_project.State.Chassis", "RequestedPowerTransition",
                std::variant<std::string>{command});
        }
    }
    /**
     * Function transceives data with dbus directly.
     */
    void doNMI(const std::shared_ptr<AsyncResp> &asyncResp)
    {
        constexpr char const *serviceName =
            "xyz.openbmc_project.Control.Host.NMI";
        constexpr char const *objectPath =
            "/xyz/openbmc_project/control/host0/nmi";
        constexpr char const *interfaceName =
            "xyz.openbmc_project.Control.Host.NMI";
        constexpr char const *method = "NMI";

        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << " Bad D-Bus request error: " << ec;
                    messages::internalError(asyncResp->res);
                    return;
                }
                messages::success(asyncResp->res);
            },
            serviceName, objectPath, interfaceName, method);
    }
};

/**
 * Systems derived class for delivering Computer Systems Schema.
 */
class Systems : public Node
{
  public:
    /*
     * Default Constructor
     */
    Systems(CrowApp &app) : Node(app, "/redfish/v1/Systems/system/")
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::put, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::post, {{"ConfigureComponents"}}}};
    }

  private:
    /**
     * Functions triggers appropriate requests on DBus
     */
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        res.jsonValue["@odata.type"] = "#ComputerSystem.v1_6_0.ComputerSystem";
        res.jsonValue["@odata.context"] =
            "/redfish/v1/$metadata#ComputerSystem.ComputerSystem";
        res.jsonValue["Name"] = "Computer System";
        res.jsonValue["Id"] = "system";
        res.jsonValue["SystemType"] = "Physical";
        res.jsonValue["Description"] = "Computer System";
        res.jsonValue["ProcessorSummary"]["Count"] = 0;
        res.jsonValue["ProcessorSummary"]["Status"]["State"] = "Disabled";
        res.jsonValue["MemorySummary"]["TotalSystemMemoryGiB"] = uint64_t(0);
        res.jsonValue["MemorySummary"]["Status"]["State"] = "Disabled";
        res.jsonValue["@odata.id"] = "/redfish/v1/Systems/system";

        res.jsonValue["Processors"] = {
            {"@odata.id", "/redfish/v1/Systems/system/Processors"}};
        res.jsonValue["Memory"] = {
            {"@odata.id", "/redfish/v1/Systems/system/Memory"}};
        res.jsonValue["Storage"] = {
            {"@odata.id", "/redfish/v1/Systems/system/Storage"}};

        // TODO Need to support ForceRestart.
        res.jsonValue["Actions"]["#ComputerSystem.Reset"] = {
            {"target",
             "/redfish/v1/Systems/system/Actions/ComputerSystem.Reset"},
            {"ResetType@Redfish.AllowableValues",
             {"On", "ForceOff", "ForceOn", "ForceRestart", "GracefulRestart",
              "GracefulShutdown", "PowerCycle", "Nmi"}}};

        res.jsonValue["LogServices"] = {
            {"@odata.id", "/redfish/v1/Systems/system/LogServices"}};

        res.jsonValue["Bios"] = {
            {"@odata.id", "/redfish/v1/Systems/system/Bios"}};

        res.jsonValue["Links"]["ManagedBy"] = {
            {{"@odata.id", "/redfish/v1/Managers/bmc"}}};

        res.jsonValue["Status"] = {
            {"Health", "OK"},
            {"State", "Enabled"},
        };
        auto asyncResp = std::make_shared<AsyncResp>(res);

        constexpr const std::array<const char *, 4> inventoryForSystems = {
            "xyz.openbmc_project.Inventory.Item.Dimm",
            "xyz.openbmc_project.Inventory.Item.Cpu",
            "xyz.openbmc_project.Inventory.Item.Drive",
            "xyz.openbmc_project.Inventory.Item.StorageController"};

        auto health = std::make_shared<HealthPopulate>(asyncResp);
        crow::connections::systemBus->async_method_call(
            [health](const boost::system::error_code ec,
                     std::vector<std::string> &resp) {
                if (ec)
                {
                    // no inventory
                    return;
                }

                health->inventory = std::move(resp);
            },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths", "/",
            int32_t(0), inventoryForSystems);

        health->populate();

        getMainChassisId(asyncResp, [](const std::string &chassisId,
                                       std::shared_ptr<AsyncResp> aRsp) {
            aRsp->res.jsonValue["Links"]["Chassis"] = {
                {{"@odata.id", "/redfish/v1/Chassis/" + chassisId}}};
        });

        getIndicatorLedState(asyncResp);
        getComputerSystem(asyncResp, health);
        getHostState(asyncResp);
        getBootProperties(asyncResp);
        getPCIeDeviceList(asyncResp, "PCIeDevices");
        getHostWatchdogTimer(asyncResp);
#ifdef BMCWEB_ENABLE_REDFISH_PROVISIONING_FEATURE
        getProvisioningStatus(asyncResp);
#endif
    }

    void doPatch(crow::Response &res, const crow::Request &req,
                 const std::vector<std::string> &params) override
    {
        std::optional<std::string> indicatorLed;
        std::optional<nlohmann::json> bootProps;
        std::optional<nlohmann::json> wdtTimerProps;
        auto asyncResp = std::make_shared<AsyncResp>(res);

        if (!json_util::readJson(req, res, "IndicatorLED", indicatorLed, "Boot",
                                 bootProps, "WatchdogTimer", wdtTimerProps))
        {
            return;
        }

        res.result(boost::beast::http::status::no_content);

        if (wdtTimerProps)
        {
            std::optional<bool> wdtEnable;
            std::optional<std::string> wdtTimeOutAction;

            if (!json_util::readJson(*wdtTimerProps, asyncResp->res,
                                     "FunctionEnabled", wdtEnable,
                                     "TimeoutAction", wdtTimeOutAction))
            {
                return;
            }
            setWDTProperties(asyncResp, std::move(wdtEnable),
                             std::move(wdtTimeOutAction));
        }

        if (bootProps)
        {
            std::optional<std::string> bootSource;
            std::optional<std::string> bootEnable;

            if (!json_util::readJson(*bootProps, asyncResp->res,
                                     "BootSourceOverrideTarget", bootSource,
                                     "BootSourceOverrideEnabled", bootEnable))
            {
                return;
            }
            setBootProperties(asyncResp, std::move(bootSource),
                              std::move(bootEnable));
        }

        if (indicatorLed)
        {
            setIndicatorLedState(asyncResp, std::move(*indicatorLed));
        }
    }
};
} // namespace redfish
