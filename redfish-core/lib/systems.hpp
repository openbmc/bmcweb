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

#include <app.hpp>
#include <boost/container/flat_map.hpp>
#include <registries/privilege_registry.hpp>
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
inline void
    updateDimmProperties(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                         const std::variant<bool>& dimmState)
{
    const bool* isDimmFunctional = std::get_if<bool>(&dimmState);
    if (isDimmFunctional == nullptr)
    {
        messages::internalError(aResp->res);
        return;
    }
    BMCWEB_LOG_DEBUG << "Dimm Functional: " << *isDimmFunctional;

    // Set it as Enabled if at least one DIMM is functional
    // Update STATE only if previous State was DISABLED and current Dimm is
    // ENABLED.
    nlohmann::json& prevMemSummary =
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
inline void
    modifyCpuPresenceState(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                           const std::variant<bool>& cpuPresenceState)
{
    const bool* isCpuPresent = std::get_if<bool>(&cpuPresenceState);

    if (isCpuPresent == nullptr)
    {
        messages::internalError(aResp->res);
        return;
    }
    BMCWEB_LOG_DEBUG << "Cpu Present: " << *isCpuPresent;

    if (*isCpuPresent == true)
    {
        nlohmann::json& procCount =
            aResp->res.jsonValue["ProcessorSummary"]["Count"];
        auto procCountPtr =
            procCount.get_ptr<nlohmann::json::number_integer_t*>();
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
inline void
    modifyCpuFunctionalState(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                             const std::variant<bool>& cpuFunctionalState)
{
    const bool* isCpuFunctional = std::get_if<bool>(&cpuFunctionalState);

    if (isCpuFunctional == nullptr)
    {
        messages::internalError(aResp->res);
        return;
    }
    BMCWEB_LOG_DEBUG << "Cpu Functional: " << *isCpuFunctional;

    nlohmann::json& prevProcState =
        aResp->res.jsonValue["ProcessorSummary"]["Status"]["State"];

    // Set it as Enabled if at least one CPU is functional
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

inline void getProcessorProperties(
    const std::shared_ptr<bmcweb::AsyncResp>& aResp, const std::string& service,
    const std::string& path,
    const std::vector<std::pair<
        std::string, std::variant<std::string, uint64_t, uint32_t, uint16_t>>>&
        properties)
{

    BMCWEB_LOG_DEBUG << "Got " << properties.size() << " Cpu properties.";

    auto getCpuPresenceState =
        [aResp](const boost::system::error_code ec3,
                const std::variant<bool>& cpuPresenceCheck) {
            if (ec3)
            {
                BMCWEB_LOG_ERROR << "DBUS response error " << ec3;
                return;
            }
            modifyCpuPresenceState(aResp, cpuPresenceCheck);
        };

    auto getCpuFunctionalState =
        [aResp](const boost::system::error_code ec3,
                const std::variant<bool>& cpuFunctionalCheck) {
            if (ec3)
            {
                BMCWEB_LOG_ERROR << "DBUS response error " << ec3;
                return;
            }
            modifyCpuFunctionalState(aResp, cpuFunctionalCheck);
        };

    // Get the Presence of CPU
    crow::connections::systemBus->async_method_call(
        std::move(getCpuPresenceState), service, path,
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Inventory.Item", "Present");

    // Get the Functional State
    crow::connections::systemBus->async_method_call(
        std::move(getCpuFunctionalState), service, path,
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.State.Decorator.OperationalStatus", "Functional");

    for (const auto& property : properties)
    {

        // TODO: Get Model

        // Get CoreCount
        if (property.first == "CoreCount")
        {

            // Get CPU CoreCount and add it to the total
            const uint16_t* coreCountVal =
                std::get_if<uint16_t>(&property.second);

            if (!coreCountVal)
            {
                messages::internalError(aResp->res);
                return;
            }

            nlohmann::json& coreCount =
                aResp->res.jsonValue["ProcessorSummary"]["CoreCount"];
            uint64_t* coreCountPtr = coreCount.get_ptr<uint64_t*>();

            if (coreCountPtr == nullptr)
            {
                coreCount = 0;
            }
            else
            {
                *coreCountPtr += *coreCountVal;
            }
        }
    }
}

/*
 * @brief Get ProcessorSummary fields
 *
 * @param[in] aResp Shared pointer for completing asynchronous calls
 * @param[in] service dbus service for Cpu Information
 * @param[in] path dbus path for Cpu
 *
 * @return None.
 */
inline void getProcessorSummary(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                                const std::string& service,
                                const std::string& path)
{

    crow::connections::systemBus->async_method_call(
        [aResp, service,
         path](const boost::system::error_code ec2,
               const std::vector<std::pair<
                   std::string, std::variant<std::string, uint64_t, uint32_t,
                                             uint16_t>>>& properties) {
            if (ec2)
            {
                BMCWEB_LOG_ERROR << "DBUS response error " << ec2;
                messages::internalError(aResp->res);
                return;
            }
            getProcessorProperties(aResp, service, path, properties);
        },
        service, path, "org.freedesktop.DBus.Properties", "GetAll",
        "xyz.openbmc_project.Inventory.Item.Cpu");
}

/*
 * @brief Retrieves computer system properties over dbus
 *
 * @param[in] aResp Shared pointer for completing asynchronous calls
 * @param[in] systemHealth  Shared HealthPopulate pointer
 *
 * @return None.
 */
inline void
    getComputerSystem(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                      const std::shared_ptr<HealthPopulate>& systemHealth)
{
    BMCWEB_LOG_DEBUG << "Get available system components.";

    crow::connections::systemBus->async_method_call(
        [aResp, systemHealth](
            const boost::system::error_code ec,
            const std::vector<std::pair<
                std::string,
                std::vector<std::pair<std::string, std::vector<std::string>>>>>&
                subtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(aResp->res);
                return;
            }
            // Iterate over all retrieved ObjectPaths.
            for (const std::pair<std::string,
                                 std::vector<std::pair<
                                     std::string, std::vector<std::string>>>>&
                     object : subtree)
            {
                const std::string& path = object.first;
                BMCWEB_LOG_DEBUG << "Got path: " << path;
                const std::vector<
                    std::pair<std::string, std::vector<std::string>>>&
                    connectionNames = object.second;
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
                for (const auto& connection : connectionNames)
                {
                    for (const auto& interfaceName : connection.second)
                    {
                        if (interfaceName ==
                            "xyz.openbmc_project.Inventory.Item.Dimm")
                        {
                            BMCWEB_LOG_DEBUG
                                << "Found Dimm, now get its properties.";

                            crow::connections::systemBus->async_method_call(
                                [aResp, service{connection.first},
                                 path](const boost::system::error_code ec2,
                                       const std::vector<
                                           std::pair<std::string, VariantType>>&
                                           properties) {
                                    if (ec2)
                                    {
                                        BMCWEB_LOG_ERROR
                                            << "DBUS response error " << ec2;
                                        messages::internalError(aResp->res);
                                        return;
                                    }
                                    BMCWEB_LOG_DEBUG << "Got "
                                                     << properties.size()
                                                     << " Dimm properties.";

                                    if (properties.size() > 0)
                                    {
                                        for (const std::pair<std::string,
                                                             VariantType>&
                                                 property : properties)
                                        {
                                            if (property.first !=
                                                "MemorySizeInKB")
                                            {
                                                continue;
                                            }
                                            const uint32_t* value =
                                                std::get_if<uint32_t>(
                                                    &property.second);
                                            if (value == nullptr)
                                            {
                                                BMCWEB_LOG_DEBUG
                                                    << "Find incorrect type of MemorySize";
                                                continue;
                                            }
                                            nlohmann::json& totalMemory =
                                                aResp->res.jsonValue
                                                    ["MemorySummary"]
                                                    ["TotalSystemMemoryGiB"];
                                            uint64_t* preValue =
                                                totalMemory
                                                    .get_ptr<uint64_t*>();
                                            if (preValue == nullptr)
                                            {
                                                continue;
                                            }
                                            aResp->res.jsonValue
                                                ["MemorySummary"]
                                                ["TotalSystemMemoryGiB"] =
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
                                                    ec3,
                                                const std::variant<bool>&
                                                    dimmState) {
                                                if (ec3)
                                                {
                                                    BMCWEB_LOG_ERROR
                                                        << "DBUS response error "
                                                        << ec3;
                                                    return;
                                                }
                                                updateDimmProperties(aResp,
                                                                     dimmState);
                                            };
                                        crow::connections::systemBus
                                            ->async_method_call(
                                                std::move(getDimmProperties),
                                                service, path,
                                                "org.freedesktop.DBus.Properties",
                                                "Get",
                                                "xyz.openbmc_project.State.Decorator.OperationalStatus",
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

                            getProcessorSummary(aResp, connection.first, path);

                            cpuHealth->inventory.emplace_back(path);
                        }
                        else if (interfaceName ==
                                 "xyz.openbmc_project.Common.UUID")
                        {
                            BMCWEB_LOG_DEBUG
                                << "Found UUID, now get its properties.";
                            crow::connections::systemBus->async_method_call(
                                [aResp](
                                    const boost::system::error_code ec3,
                                    const std::vector<
                                        std::pair<std::string, VariantType>>&
                                        properties) {
                                    if (ec3)
                                    {
                                        BMCWEB_LOG_DEBUG
                                            << "DBUS response error " << ec3;
                                        messages::internalError(aResp->res);
                                        return;
                                    }
                                    BMCWEB_LOG_DEBUG << "Got "
                                                     << properties.size()
                                                     << " UUID properties.";
                                    for (const std::pair<std::string,
                                                         VariantType>&
                                             property : properties)
                                    {
                                        if (property.first == "UUID")
                                        {
                                            const std::string* value =
                                                std::get_if<std::string>(
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
                                [aResp](
                                    const boost::system::error_code ec2,
                                    const std::vector<
                                        std::pair<std::string, VariantType>>&
                                        propertiesList) {
                                    if (ec2)
                                    {
                                        // doesn't have to include this
                                        // interface
                                        return;
                                    }
                                    BMCWEB_LOG_DEBUG
                                        << "Got " << propertiesList.size()
                                        << " properties for system";
                                    for (const std::pair<std::string,
                                                         VariantType>&
                                             property : propertiesList)
                                    {
                                        const std::string& propertyName =
                                            property.first;
                                        if ((propertyName == "PartNumber") ||
                                            (propertyName == "SerialNumber") ||
                                            (propertyName == "Manufacturer") ||
                                            (propertyName == "Model") ||
                                            (propertyName == "SubModel"))
                                        {
                                            const std::string* value =
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

                                    // Grab the bios version
                                    fw_util::populateFirmwareInformation(
                                        aResp, fw_util::biosPurpose,
                                        "BiosVersion", false);
                                },
                                connection.first, path,
                                "org.freedesktop.DBus.Properties", "GetAll",
                                "xyz.openbmc_project.Inventory.Decorator.Asset");

                            crow::connections::systemBus->async_method_call(
                                [aResp](
                                    const boost::system::error_code ec2,
                                    const std::variant<std::string>& property) {
                                    if (ec2)
                                    {
                                        // doesn't have to include this
                                        // interface
                                        return;
                                    }

                                    const std::string* value =
                                        std::get_if<std::string>(&property);
                                    if (value != nullptr)
                                    {
                                        aResp->res.jsonValue["AssetTag"] =
                                            *value;
                                    }
                                },
                                connection.first, path,
                                "org.freedesktop.DBus.Properties", "Get",
                                "xyz.openbmc_project.Inventory.Decorator.AssetTag",
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
        std::array<const char*, 5>{
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
inline void getHostState(const std::shared_ptr<bmcweb::AsyncResp>& aResp)
{
    BMCWEB_LOG_DEBUG << "Get host information.";
    crow::connections::systemBus->async_method_call(
        [aResp](const boost::system::error_code ec,
                const std::variant<std::string>& hostState) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                messages::internalError(aResp->res);
                return;
            }

            const std::string* s = std::get_if<std::string>(&hostState);
            BMCWEB_LOG_DEBUG << "Host state: " << *s;
            if (s != nullptr)
            {
                // Verify Host State
                if (*s == "xyz.openbmc_project.State.Host.HostState.Running")
                {
                    aResp->res.jsonValue["PowerState"] = "On";
                    aResp->res.jsonValue["Status"]["State"] = "Enabled";
                }
                else if (*s ==
                         "xyz.openbmc_project.State.Host.HostState.Quiesced")
                {
                    aResp->res.jsonValue["PowerState"] = "On";
                    aResp->res.jsonValue["Status"]["State"] = "Quiesced";
                }
                else if (
                    *s ==
                    "xyz.openbmc_project.State.Host.HostState.DiagnosticMode")
                {
                    aResp->res.jsonValue["PowerState"] = "On";
                    aResp->res.jsonValue["Status"]["State"] = "InTest";
                }
                else if (
                    *s ==
                    "xyz.openbmc_project.State.Host.HostState.TransitioningToRunning")
                {
                    aResp->res.jsonValue["PowerState"] = "PoweringOn";
                    aResp->res.jsonValue["Status"]["State"] = "Starting";
                }
                else if (
                    *s ==
                    "xyz.openbmc_project.State.Host.HostState.TransitioningToOff")
                {
                    aResp->res.jsonValue["PowerState"] = "PoweringOff";
                    aResp->res.jsonValue["Status"]["State"] = "Disabled";
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
 * @brief Translates boot source DBUS property value to redfish.
 *
 * @param[in] dbusSource    The boot source in DBUS speak.
 *
 * @return Returns as a string, the boot source in Redfish terms. If translation
 * cannot be done, returns an empty string.
 */
inline std::string dbusToRfBootSource(const std::string& dbusSource)
{
    if (dbusSource == "xyz.openbmc_project.Control.Boot.Source.Sources.Default")
    {
        return "None";
    }
    if (dbusSource == "xyz.openbmc_project.Control.Boot.Source.Sources.Disk")
    {
        return "Hdd";
    }
    if (dbusSource ==
        "xyz.openbmc_project.Control.Boot.Source.Sources.ExternalMedia")
    {
        return "Cd";
    }
    if (dbusSource == "xyz.openbmc_project.Control.Boot.Source.Sources.Network")
    {
        return "Pxe";
    }
    if (dbusSource ==
        "xyz.openbmc_project.Control.Boot.Source.Sources.RemovableMedia")
    {
        return "Usb";
    }
    return "";
}

/**
 * @brief Translates boot type DBUS property value to redfish.
 *
 * @param[in] dbusType    The boot type in DBUS speak.
 *
 * @return Returns as a string, the boot type in Redfish terms. If translation
 * cannot be done, returns an empty string.
 */
inline std::string dbusToRfBootType(const std::string& dbusType)
{
    if (dbusType == "xyz.openbmc_project.Control.Boot.Type.Types.Legacy")
    {
        return "Legacy";
    }
    if (dbusType == "xyz.openbmc_project.Control.Boot.Type.Types.EFI")
    {
        return "UEFI";
    }
    return "";
}

/**
 * @brief Translates boot mode DBUS property value to redfish.
 *
 * @param[in] dbusMode    The boot mode in DBUS speak.
 *
 * @return Returns as a string, the boot mode in Redfish terms. If translation
 * cannot be done, returns an empty string.
 */
inline std::string dbusToRfBootMode(const std::string& dbusMode)
{
    if (dbusMode == "xyz.openbmc_project.Control.Boot.Mode.Modes.Regular")
    {
        return "None";
    }
    if (dbusMode == "xyz.openbmc_project.Control.Boot.Mode.Modes.Safe")
    {
        return "Diags";
    }
    if (dbusMode == "xyz.openbmc_project.Control.Boot.Mode.Modes.Setup")
    {
        return "BiosSetup";
    }
    return "";
}

/**
 * @brief Translates boot source from Redfish to the DBus boot paths.
 *
 * @param[in] rfSource    The boot source in Redfish.
 * @param[out] bootSource The DBus source
 * @param[out] bootMode   the DBus boot mode
 *
 * @return Integer error code.
 */
inline int assignBootParameters(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                                const std::string& rfSource,
                                std::string& bootSource, std::string& bootMode)
{
    bootSource = "xyz.openbmc_project.Control.Boot.Source.Sources.Default";
    bootMode = "xyz.openbmc_project.Control.Boot.Mode.Modes.Regular";

    if (rfSource == "None")
    {
        return 0;
    }
    if (rfSource == "Pxe")
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
        BMCWEB_LOG_DEBUG
            << "Invalid property value for BootSourceOverrideTarget: "
            << bootSource;
        messages::propertyValueNotInList(aResp->res, rfSource,
                                         "BootSourceTargetOverride");
        return -1;
    }
    return 0;
}

/**
 * @brief Retrieves boot progress of the system
 *
 * @param[in] aResp  Shared pointer for generating response message.
 *
 * @return None.
 */
inline void getBootProgress(const std::shared_ptr<bmcweb::AsyncResp>& aResp)
{
    crow::connections::systemBus->async_method_call(
        [aResp](const boost::system::error_code ec,
                const std::variant<std::string>& bootProgress) {
            if (ec)
            {
                // BootProgress is an optional object so just do nothing if
                // not found
                return;
            }

            const std::string* bootProgressStr =
                std::get_if<std::string>(&bootProgress);

            if (!bootProgressStr)
            {
                // Interface implemented but property not found, return error
                // for that
                messages::internalError(aResp->res);
                return;
            }

            BMCWEB_LOG_DEBUG << "Boot Progress: " << *bootProgressStr;

            // Now convert the D-Bus BootProgress to the appropriate Redfish
            // enum
            std::string rfBpLastState = "None";
            if (*bootProgressStr ==
                "xyz.openbmc_project.State.Boot.Progress.ProgressStages.Unspecified")
            {
                rfBpLastState = "None";
            }
            else if (
                *bootProgressStr ==
                "xyz.openbmc_project.State.Boot.Progress.ProgressStages.PrimaryProcInit")
            {
                rfBpLastState = "PrimaryProcessorInitializationStarted";
            }
            else if (
                *bootProgressStr ==
                "xyz.openbmc_project.State.Boot.Progress.ProgressStages.BusInit")
            {
                rfBpLastState = "BusInitializationStarted";
            }
            else if (
                *bootProgressStr ==
                "xyz.openbmc_project.State.Boot.Progress.ProgressStages.MemoryInit")
            {
                rfBpLastState = "MemoryInitializationStarted";
            }
            else if (
                *bootProgressStr ==
                "xyz.openbmc_project.State.Boot.Progress.ProgressStages.SecondaryProcInit")
            {
                rfBpLastState = "SecondaryProcessorInitializationStarted";
            }
            else if (
                *bootProgressStr ==
                "xyz.openbmc_project.State.Boot.Progress.ProgressStages.PCIInit")
            {
                rfBpLastState = "PCIResourceConfigStarted";
            }
            else if (
                *bootProgressStr ==
                "xyz.openbmc_project.State.Boot.Progress.ProgressStages.SystemInitComplete")
            {
                rfBpLastState = "SystemHardwareInitializationComplete";
            }
            else if (
                *bootProgressStr ==
                "xyz.openbmc_project.State.Boot.Progress.ProgressStages.OSStart")
            {
                rfBpLastState = "OSBootStarted";
            }
            else if (
                *bootProgressStr ==
                "xyz.openbmc_project.State.Boot.Progress.ProgressStages.OSRunning")
            {
                rfBpLastState = "OSRunning";
            }
            else
            {
                BMCWEB_LOG_DEBUG << "Unsupported D-Bus BootProgress "
                                 << *bootProgressStr;
                // Just return the default
            }

            aResp->res.jsonValue["BootProgress"]["LastState"] = rfBpLastState;
        },
        "xyz.openbmc_project.State.Host", "/xyz/openbmc_project/state/host0",
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.State.Boot.Progress", "BootProgress");
}

/**
 * @brief Retrieves boot override type over DBUS and fills out the response
 *
 * @param[in] aResp         Shared pointer for generating response message.
 *
 * @return None.
 */

inline void getBootOverrideType(const std::shared_ptr<bmcweb::AsyncResp>& aResp)
{
    crow::connections::systemBus->async_method_call(
        [aResp](const boost::system::error_code ec,
                const std::variant<std::string>& bootType) {
            if (ec)
            {
                // not an error, don't have to have the interface
                return;
            }

            const std::string* bootTypeStr =
                std::get_if<std::string>(&bootType);

            if (!bootTypeStr)
            {
                messages::internalError(aResp->res);
                return;
            }

            BMCWEB_LOG_DEBUG << "Boot type: " << *bootTypeStr;

            aResp->res
                .jsonValue["Boot"]
                          ["BootSourceOverrideMode@Redfish.AllowableValues"] = {
                "Legacy", "UEFI"};

            auto rfType = dbusToRfBootType(*bootTypeStr);
            if (rfType.empty())
            {
                messages::internalError(aResp->res);
                return;
            }

            aResp->res.jsonValue["Boot"]["BootSourceOverrideMode"] = rfType;
        },
        "xyz.openbmc_project.Settings",
        "/xyz/openbmc_project/control/host0/boot",
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Control.Boot.Type", "BootType");
}

/**
 * @brief Retrieves boot override mode over DBUS and fills out the response
 *
 * @param[in] aResp         Shared pointer for generating response message.
 *
 * @return None.
 */

inline void getBootOverrideMode(const std::shared_ptr<bmcweb::AsyncResp>& aResp)
{
    crow::connections::systemBus->async_method_call(
        [aResp](const boost::system::error_code ec,
                const std::variant<std::string>& bootMode) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                messages::internalError(aResp->res);
                return;
            }

            const std::string* bootModeStr =
                std::get_if<std::string>(&bootMode);

            if (!bootModeStr)
            {
                messages::internalError(aResp->res);
                return;
            }

            BMCWEB_LOG_DEBUG << "Boot mode: " << *bootModeStr;

            aResp->res
                .jsonValue["Boot"]
                          ["BootSourceOverrideTarget@Redfish.AllowableValues"] =
                {"None", "Pxe", "Hdd", "Cd", "Diags", "BiosSetup", "Usb"};

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
        },
        "xyz.openbmc_project.Settings",
        "/xyz/openbmc_project/control/host0/boot",
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Control.Boot.Mode", "BootMode");
}

/**
 * @brief Retrieves boot override source over DBUS
 *
 * @param[in] aResp         Shared pointer for generating response message.
 *
 * @return None.
 */

inline void
    getBootOverrideSource(const std::shared_ptr<bmcweb::AsyncResp>& aResp)
{
    crow::connections::systemBus->async_method_call(
        [aResp](const boost::system::error_code ec,
                const std::variant<std::string>& bootSource) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                messages::internalError(aResp->res);
                return;
            }

            const std::string* bootSourceStr =
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

            // Get BootMode as BootSourceOverrideTarget is constructed
            // from both BootSource and BootMode
            getBootOverrideMode(aResp);
        },
        "xyz.openbmc_project.Settings",
        "/xyz/openbmc_project/control/host0/boot",
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Control.Boot.Source", "BootSource");
}

/**
 * @brief This functions abstracts all the logic behind getting a
 * "BootSourceOverrideEnabled" property from an overall boot override enable
 * state
 *
 * @param[in] aResp     Shared pointer for generating response message.
 *
 * @return None.
 */

inline void
    processBootOverrideEnable(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                              const bool bootOverrideEnableSetting)
{
    if (!bootOverrideEnableSetting)
    {
        aResp->res.jsonValue["Boot"]["BootSourceOverrideEnabled"] = "Disabled";
        return;
    }

    // If boot source override is enabled, we need to check 'one_time'
    // property to set a correct value for the "BootSourceOverrideEnabled"
    crow::connections::systemBus->async_method_call(
        [aResp](const boost::system::error_code ec,
                const std::variant<bool>& oneTime) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                messages::internalError(aResp->res);
                return;
            }

            const bool* oneTimePtr = std::get_if<bool>(&oneTime);

            if (!oneTimePtr)
            {
                messages::internalError(aResp->res);
                return;
            }

            bool oneTimeSetting = *oneTimePtr;

            if (oneTimeSetting)
            {
                aResp->res.jsonValue["Boot"]["BootSourceOverrideEnabled"] =
                    "Once";
            }
            else
            {
                aResp->res.jsonValue["Boot"]["BootSourceOverrideEnabled"] =
                    "Continuous";
            }
        },
        "xyz.openbmc_project.Settings",
        "/xyz/openbmc_project/control/host0/boot/one_time",
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Object.Enable", "Enabled");
}

/**
 * @brief Retrieves boot override enable over DBUS
 *
 * @param[in] aResp     Shared pointer for generating response message.
 *
 * @return None.
 */

inline void
    getBootOverrideEnable(const std::shared_ptr<bmcweb::AsyncResp>& aResp)
{
    crow::connections::systemBus->async_method_call(
        [aResp](const boost::system::error_code ec,
                const std::variant<bool>& bootOverrideEnable) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                messages::internalError(aResp->res);
                return;
            }

            const bool* bootOverrideEnablePtr =
                std::get_if<bool>(&bootOverrideEnable);

            if (!bootOverrideEnablePtr)
            {
                messages::internalError(aResp->res);
                return;
            }

            processBootOverrideEnable(aResp, *bootOverrideEnablePtr);
        },
        "xyz.openbmc_project.Settings",
        "/xyz/openbmc_project/control/host0/boot",
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Object.Enable", "Enabled");
}

/**
 * @brief Retrieves boot source override properties
 *
 * @param[in] aResp     Shared pointer for generating response message.
 *
 * @return None.
 */
inline void getBootProperties(const std::shared_ptr<bmcweb::AsyncResp>& aResp)
{
    BMCWEB_LOG_DEBUG << "Get boot information.";

    getBootOverrideSource(aResp);
    getBootOverrideType(aResp);
    getBootOverrideEnable(aResp);
}

/**
 * @brief Retrieves the Last Reset Time
 *
 * "Reset" is an overloaded term in Redfish, "Reset" includes power on
 * and power off. Even though this is the "system" Redfish object look at the
 * chassis D-Bus interface for the LastStateChangeTime since this has the
 * last power operation time.
 *
 * @param[in] aResp     Shared pointer for generating response message.
 *
 * @return None.
 */
inline void getLastResetTime(const std::shared_ptr<bmcweb::AsyncResp>& aResp)
{
    BMCWEB_LOG_DEBUG << "Getting System Last Reset Time";

    crow::connections::systemBus->async_method_call(
        [aResp](const boost::system::error_code ec,
                std::variant<uint64_t>& lastResetTime) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "D-BUS response error " << ec;
                return;
            }

            const uint64_t* lastResetTimePtr =
                std::get_if<uint64_t>(&lastResetTime);

            if (!lastResetTimePtr)
            {
                messages::internalError(aResp->res);
                return;
            }
            // LastStateChangeTime is epoch time, in milliseconds
            // https://github.com/openbmc/phosphor-dbus-interfaces/blob/33e8e1dd64da53a66e888d33dc82001305cd0bf9/xyz/openbmc_project/State/Chassis.interface.yaml#L19
            uint64_t lastResetTimeStamp = *lastResetTimePtr / 1000;

            // Convert to ISO 8601 standard
            aResp->res.jsonValue["LastResetTime"] =
                crow::utility::getDateTimeUint(lastResetTimeStamp);
        },
        "xyz.openbmc_project.State.Chassis",
        "/xyz/openbmc_project/state/chassis0",
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.State.Chassis", "LastStateChangeTime");
}

/**
 * @brief Retrieves Automatic Retry properties. Known on D-Bus as AutoReboot.
 *
 * @param[in] aResp     Shared pointer for generating response message.
 *
 * @return None.
 */
inline void getAutomaticRetry(const std::shared_ptr<bmcweb::AsyncResp>& aResp)
{
    BMCWEB_LOG_DEBUG << "Get Automatic Retry policy";

    crow::connections::systemBus->async_method_call(
        [aResp](const boost::system::error_code ec,
                std::variant<bool>& autoRebootEnabled) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "D-BUS response error " << ec;
                return;
            }

            const bool* autoRebootEnabledPtr =
                std::get_if<bool>(&autoRebootEnabled);

            if (!autoRebootEnabledPtr)
            {
                messages::internalError(aResp->res);
                return;
            }

            BMCWEB_LOG_DEBUG << "Auto Reboot: " << *autoRebootEnabledPtr;
            if (*autoRebootEnabledPtr == true)
            {
                aResp->res.jsonValue["Boot"]["AutomaticRetryConfig"] =
                    "RetryAttempts";
                // If AutomaticRetry (AutoReboot) is enabled see how many
                // attempts are left
                crow::connections::systemBus->async_method_call(
                    [aResp](const boost::system::error_code ec2,
                            std::variant<uint32_t>& autoRebootAttemptsLeft) {
                        if (ec2)
                        {
                            BMCWEB_LOG_DEBUG << "D-BUS response error " << ec2;
                            return;
                        }

                        const uint32_t* autoRebootAttemptsLeftPtr =
                            std::get_if<uint32_t>(&autoRebootAttemptsLeft);

                        if (!autoRebootAttemptsLeftPtr)
                        {
                            messages::internalError(aResp->res);
                            return;
                        }

                        BMCWEB_LOG_DEBUG << "Auto Reboot Attempts Left: "
                                         << *autoRebootAttemptsLeftPtr;

                        aResp->res
                            .jsonValue["Boot"]
                                      ["RemainingAutomaticRetryAttempts"] =
                            *autoRebootAttemptsLeftPtr;
                    },
                    "xyz.openbmc_project.State.Host",
                    "/xyz/openbmc_project/state/host0",
                    "org.freedesktop.DBus.Properties", "Get",
                    "xyz.openbmc_project.Control.Boot.RebootAttempts",
                    "AttemptsLeft");
            }
            else
            {
                aResp->res.jsonValue["Boot"]["AutomaticRetryConfig"] =
                    "Disabled";
            }

            // Not on D-Bus. Hardcoded here:
            // https://github.com/openbmc/phosphor-state-manager/blob/1dbbef42675e94fb1f78edb87d6b11380260535a/meson_options.txt#L71
            aResp->res.jsonValue["Boot"]["AutomaticRetryAttempts"] = 3;

            // "AutomaticRetryConfig" can be 3 values, Disabled, RetryAlways,
            // and RetryAttempts. OpenBMC only supports Disabled and
            // RetryAttempts.
            aResp->res
                .jsonValue["Boot"]
                          ["AutomaticRetryConfig@Redfish.AllowableValues"] = {
                "Disabled", "RetryAttempts"};
        },
        "xyz.openbmc_project.Settings",
        "/xyz/openbmc_project/control/host0/auto_reboot",
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Control.Boot.RebootPolicy", "AutoReboot");
}

/**
 * @brief Retrieves power restore policy over DBUS.
 *
 * @param[in] aResp     Shared pointer for generating response message.
 *
 * @return None.
 */
inline void
    getPowerRestorePolicy(const std::shared_ptr<bmcweb::AsyncResp>& aResp)
{
    BMCWEB_LOG_DEBUG << "Get power restore policy";

    crow::connections::systemBus->async_method_call(
        [aResp](const boost::system::error_code ec,
                std::variant<std::string>& policy) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                return;
            }

            const boost::container::flat_map<std::string, std::string> policyMaps = {
                {"xyz.openbmc_project.Control.Power.RestorePolicy.Policy.AlwaysOn",
                 "AlwaysOn"},
                {"xyz.openbmc_project.Control.Power.RestorePolicy.Policy.AlwaysOff",
                 "AlwaysOff"},
                {"xyz.openbmc_project.Control.Power.RestorePolicy.Policy.Restore",
                 "LastState"}};

            const std::string* policyPtr = std::get_if<std::string>(&policy);

            if (!policyPtr)
            {
                messages::internalError(aResp->res);
                return;
            }

            auto policyMapsIt = policyMaps.find(*policyPtr);
            if (policyMapsIt == policyMaps.end())
            {
                messages::internalError(aResp->res);
                return;
            }

            aResp->res.jsonValue["PowerRestorePolicy"] = policyMapsIt->second;
        },
        "xyz.openbmc_project.Settings",
        "/xyz/openbmc_project/control/host0/power_restore_policy",
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Control.Power.RestorePolicy",
        "PowerRestorePolicy");
}

/**
 * @brief Get TrustedModuleRequiredToBoot property. Determines whether or not
 * TPM is required for booting the host.
 *
 * @param[in] aResp     Shared pointer for generating response message.
 *
 * @return None.
 */
inline void getTrustedModuleRequiredToBoot(
    const std::shared_ptr<bmcweb::AsyncResp>& aResp)
{
    BMCWEB_LOG_DEBUG << "Get TPM required to boot.";

    crow::connections::systemBus->async_method_call(
        [aResp](
            const boost::system::error_code ec,
            std::vector<std::pair<
                std::string,
                std::vector<std::pair<std::string, std::vector<std::string>>>>>&
                subtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG
                    << "DBUS response error on TPM.Policy GetSubTree" << ec;
                // This is an optional D-Bus object so just return if
                // error occurs
                return;
            }
            if (subtree.size() == 0)
            {
                // As noted above, this is an optional interface so just return
                // if there is no instance found
                return;
            }

            /* When there is more than one TPMEnable object... */
            if (subtree.size() > 1)
            {
                BMCWEB_LOG_DEBUG
                    << "DBUS response has more than 1 TPM Enable object:"
                    << subtree.size();
                // Throw an internal Error and return
                messages::internalError(aResp->res);
                return;
            }

            // Make sure the Dbus response map has a service and objectPath
            // field
            if (subtree[0].first.empty() || subtree[0].second.size() != 1)
            {
                BMCWEB_LOG_DEBUG << "TPM.Policy mapper error!";
                messages::internalError(aResp->res);
                return;
            }

            const std::string& path = subtree[0].first;
            const std::string& serv = subtree[0].second.begin()->first;

            // Valid TPM Enable object found, now reading the current value
            crow::connections::systemBus->async_method_call(
                [aResp](const boost::system::error_code ec,
                        std::variant<bool>& tpmRequired) {
                    if (ec)
                    {
                        BMCWEB_LOG_DEBUG
                            << "D-BUS response error on TPM.Policy Get" << ec;
                        messages::internalError(aResp->res);
                        return;
                    }

                    const bool* tpmRequiredVal =
                        std::get_if<bool>(&tpmRequired);

                    if (!tpmRequiredVal)
                    {
                        messages::internalError(aResp->res);
                        return;
                    }

                    if (*tpmRequiredVal == true)
                    {
                        aResp->res
                            .jsonValue["Boot"]["TrustedModuleRequiredToBoot"] =
                            "Required";
                    }
                    else
                    {
                        aResp->res
                            .jsonValue["Boot"]["TrustedModuleRequiredToBoot"] =
                            "Disabled";
                    }
                },
                serv, path, "org.freedesktop.DBus.Properties", "Get",
                "xyz.openbmc_project.Control.TPM.Policy", "TPMEnable");
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree", "/", int32_t(0),
        std::array<const char*, 1>{"xyz.openbmc_project.Control.TPM.Policy"});
}

/**
 * @brief Set TrustedModuleRequiredToBoot property. Determines whether or not
 * TPM is required for booting the host.
 *
 * @param[in] aResp         Shared pointer for generating response message.
 * @param[in] tpmRequired   Value to set TPM Required To Boot property to.
 *
 * @return None.
 */
inline void setTrustedModuleRequiredToBoot(
    const std::shared_ptr<bmcweb::AsyncResp>& aResp, const bool tpmRequired)
{
    BMCWEB_LOG_DEBUG << "Set TrustedModuleRequiredToBoot.";

    crow::connections::systemBus->async_method_call(
        [aResp, tpmRequired](
            const boost::system::error_code ec,
            std::vector<std::pair<
                std::string,
                std::vector<std::pair<std::string, std::vector<std::string>>>>>&
                subtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG
                    << "DBUS response error on TPM.Policy GetSubTree" << ec;
                messages::internalError(aResp->res);
                return;
            }
            if (subtree.size() == 0)
            {
                messages::propertyValueNotInList(aResp->res, "ComputerSystem",
                                                 "TrustedModuleRequiredToBoot");
                return;
            }

            /* When there is more than one TPMEnable object... */
            if (subtree.size() > 1)
            {
                BMCWEB_LOG_DEBUG
                    << "DBUS response has more than 1 TPM Enable object:"
                    << subtree.size();
                // Throw an internal Error and return
                messages::internalError(aResp->res);
                return;
            }

            // Make sure the Dbus response map has a service and objectPath
            // field
            if (subtree[0].first.empty() || subtree[0].second.size() != 1)
            {
                BMCWEB_LOG_DEBUG << "TPM.Policy mapper error!";
                messages::internalError(aResp->res);
                return;
            }

            const std::string& path = subtree[0].first;
            const std::string& serv = subtree[0].second.begin()->first;

            if (serv.empty())
            {
                BMCWEB_LOG_DEBUG << "TPM.Policy service mapper error!";
                messages::internalError(aResp->res);
                return;
            }

            // Valid TPM Enable object found, now setting the value
            crow::connections::systemBus->async_method_call(
                [aResp](const boost::system::error_code ec) {
                    if (ec)
                    {
                        BMCWEB_LOG_DEBUG
                            << "DBUS response error: Set TrustedModuleRequiredToBoot"
                            << ec;
                        messages::internalError(aResp->res);
                        return;
                    }
                    BMCWEB_LOG_DEBUG << "Set TrustedModuleRequiredToBoot done.";
                },
                serv, path, "org.freedesktop.DBus.Properties", "Set",
                "xyz.openbmc_project.Control.TPM.Policy", "TPMEnable",
                std::variant<bool>(tpmRequired));
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree", "/", int32_t(0),
        std::array<const char*, 1>{"xyz.openbmc_project.Control.TPM.Policy"});
}

/**
 * @brief Sets boot properties into DBUS object(s).
 *
 * @param[in] aResp           Shared pointer for generating response message.
 * @param[in] bootType        The boot type to set.
 * @return Integer error code.
 */
inline void setBootType(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                        const std::optional<std::string>& bootType)
{
    std::string bootTypeStr;

    if (!bootType)
    {
        return;
    }

    // Source target specified
    BMCWEB_LOG_DEBUG << "Boot type: " << *bootType;
    // Figure out which DBUS interface and property to use
    if (*bootType == "Legacy")
    {
        bootTypeStr = "xyz.openbmc_project.Control.Boot.Type.Types.Legacy";
    }
    else if (*bootType == "UEFI")
    {
        bootTypeStr = "xyz.openbmc_project.Control.Boot.Type.Types.EFI";
    }
    else
    {
        BMCWEB_LOG_DEBUG << "Invalid property value for "
                            "BootSourceOverrideMode: "
                         << *bootType;
        messages::propertyValueNotInList(aResp->res, *bootType,
                                         "BootSourceOverrideMode");
        return;
    }

    // Act on validated parameters
    BMCWEB_LOG_DEBUG << "DBUS boot type: " << bootTypeStr;

    crow::connections::systemBus->async_method_call(
        [aResp](const boost::system::error_code ec) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                if (ec.value() == boost::asio::error::host_unreachable)
                {
                    messages::resourceNotFound(aResp->res, "Set", "BootType");
                    return;
                }
                messages::internalError(aResp->res);
                return;
            }
            BMCWEB_LOG_DEBUG << "Boot type update done.";
        },
        "xyz.openbmc_project.Settings",
        "/xyz/openbmc_project/control/host0/boot",
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Control.Boot.Type", "BootType",
        std::variant<std::string>(bootTypeStr));
}

/**
 * @brief Sets boot properties into DBUS object(s).
 *
 * @param[in] aResp           Shared pointer for generating response message.
 * @param[in] bootType        The boot type to set.
 * @return Integer error code.
 */
inline void setBootEnable(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                          const std::optional<std::string>& bootEnable)
{
    if (!bootEnable)
    {
        return;
    }
    // Source target specified
    BMCWEB_LOG_DEBUG << "Boot enable: " << *bootEnable;

    bool bootOverrideEnable = false;
    bool bootOverridePersistent = false;
    // Figure out which DBUS interface and property to use
    if (*bootEnable == "Disabled")
    {
        bootOverrideEnable = false;
    }
    else if (*bootEnable == "Once")
    {
        bootOverrideEnable = true;
        bootOverridePersistent = false;
    }
    else if (*bootEnable == "Continuous")
    {
        bootOverrideEnable = true;
        bootOverridePersistent = true;
    }
    else
    {
        BMCWEB_LOG_DEBUG
            << "Invalid property value for BootSourceOverrideEnabled: "
            << *bootEnable;
        messages::propertyValueNotInList(aResp->res, *bootEnable,
                                         "BootSourceOverrideEnabled");
        return;
    }

    // Act on validated parameters
    BMCWEB_LOG_DEBUG << "DBUS boot override enable: " << bootOverrideEnable;

    crow::connections::systemBus->async_method_call(
        [aResp](const boost::system::error_code ec) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                messages::internalError(aResp->res);
                return;
            }
            BMCWEB_LOG_DEBUG << "Boot override enable update done.";
        },
        "xyz.openbmc_project.Settings",
        "/xyz/openbmc_project/control/host0/boot",
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Object.Enable", "Enabled",
        std::variant<bool>(bootOverrideEnable));

    if (!bootOverrideEnable)
    {
        return;
    }

    // In case boot override is enabled we need to set correct value for the
    // 'one_time' enable DBus interface
    BMCWEB_LOG_DEBUG << "DBUS boot override persistent: "
                     << bootOverridePersistent;

    crow::connections::systemBus->async_method_call(
        [aResp](const boost::system::error_code ec) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                messages::internalError(aResp->res);
                return;
            }
            BMCWEB_LOG_DEBUG << "Boot one_time update done.";
        },
        "xyz.openbmc_project.Settings",
        "/xyz/openbmc_project/control/host0/boot/one_time",
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Object.Enable", "Enabled",
        std::variant<bool>(!bootOverridePersistent));
}

/**
 * @brief Sets boot properties into DBUS object(s).
 *
 * @param[in] aResp           Shared pointer for generating response message.
 * @param[in] bootSource      The boot source to set.
 *
 * @return Integer error code.
 */
inline void setBootModeOrSource(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                                const std::optional<std::string>& bootSource)
{
    std::string bootSourceStr;
    std::string bootModeStr;

    if (!bootSource)
    {
        return;
    }

    // Source target specified
    BMCWEB_LOG_DEBUG << "Boot source: " << *bootSource;
    // Figure out which DBUS interface and property to use
    if (assignBootParameters(aResp, *bootSource, bootSourceStr, bootModeStr))
    {
        BMCWEB_LOG_DEBUG
            << "Invalid property value for BootSourceOverrideTarget: "
            << *bootSource;
        messages::propertyValueNotInList(aResp->res, *bootSource,
                                         "BootSourceTargetOverride");
        return;
    }

    // Act on validated parameters
    BMCWEB_LOG_DEBUG << "DBUS boot source: " << bootSourceStr;
    BMCWEB_LOG_DEBUG << "DBUS boot mode: " << bootModeStr;

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
        "xyz.openbmc_project.Settings",
        "/xyz/openbmc_project/control/host0/boot",
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
        "xyz.openbmc_project.Settings",
        "/xyz/openbmc_project/control/host0/boot",
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Control.Boot.Mode", "BootMode",
        std::variant<std::string>(bootModeStr));
}

/**
 * @brief Sets Boot source override properties.
 *
 * @param[in] aResp      Shared pointer for generating response message.
 * @param[in] bootSource The boot source from incoming RF request.
 * @param[in] bootType   The boot type from incoming RF request.
 * @param[in] bootEnable The boot override enable from incoming RF request.
 *
 * @return Integer error code.
 */

inline void setBootProperties(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                              const std::optional<std::string>& bootSource,
                              const std::optional<std::string>& bootType,
                              const std::optional<std::string>& bootEnable)
{
    BMCWEB_LOG_DEBUG << "Set boot information.";

    setBootModeOrSource(aResp, bootSource);
    setBootType(aResp, bootType);
    setBootEnable(aResp, bootEnable);
}

/**
 * @brief Sets AssetTag
 *
 * @param[in] aResp   Shared pointer for generating response message.
 * @param[in] assetTag  "AssetTag" from request.
 *
 * @return None.
 */
inline void setAssetTag(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                        const std::string& assetTag)
{
    crow::connections::systemBus->async_method_call(
        [aResp, assetTag](
            const boost::system::error_code ec,
            const std::vector<std::pair<
                std::string,
                std::vector<std::pair<std::string, std::vector<std::string>>>>>&
                subtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "D-Bus response error on GetSubTree " << ec;
                messages::internalError(aResp->res);
                return;
            }
            if (subtree.size() == 0)
            {
                BMCWEB_LOG_DEBUG << "Can't find system D-Bus object!";
                messages::internalError(aResp->res);
                return;
            }
            // Assume only 1 system D-Bus object
            // Throw an error if there is more than 1
            if (subtree.size() > 1)
            {
                BMCWEB_LOG_DEBUG << "Found more than 1 system D-Bus object!";
                messages::internalError(aResp->res);
                return;
            }
            if (subtree[0].first.empty() || subtree[0].second.size() != 1)
            {
                BMCWEB_LOG_DEBUG << "Asset Tag Set mapper error!";
                messages::internalError(aResp->res);
                return;
            }

            const std::string& path = subtree[0].first;
            const std::string& service = subtree[0].second.begin()->first;

            if (service.empty())
            {
                BMCWEB_LOG_DEBUG << "Asset Tag Set service mapper error!";
                messages::internalError(aResp->res);
                return;
            }

            crow::connections::systemBus->async_method_call(
                [aResp](const boost::system::error_code ec2) {
                    if (ec2)
                    {
                        BMCWEB_LOG_DEBUG
                            << "D-Bus response error on AssetTag Set " << ec2;
                        messages::internalError(aResp->res);
                        return;
                    }
                },
                service, path, "org.freedesktop.DBus.Properties", "Set",
                "xyz.openbmc_project.Inventory.Decorator.AssetTag", "AssetTag",
                std::variant<std::string>(assetTag));
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", int32_t(0),
        std::array<const char*, 1>{
            "xyz.openbmc_project.Inventory.Item.System"});
}

/**
 * @brief Sets automaticRetry (Auto Reboot)
 *
 * @param[in] aResp   Shared pointer for generating response message.
 * @param[in] automaticRetryConfig  "AutomaticRetryConfig" from request.
 *
 * @return None.
 */
inline void setAutomaticRetry(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                              const std::string& automaticRetryConfig)
{
    BMCWEB_LOG_DEBUG << "Set Automatic Retry.";

    // OpenBMC only supports "Disabled" and "RetryAttempts".
    bool autoRebootEnabled;

    if (automaticRetryConfig == "Disabled")
    {
        autoRebootEnabled = false;
    }
    else if (automaticRetryConfig == "RetryAttempts")
    {
        autoRebootEnabled = true;
    }
    else
    {
        BMCWEB_LOG_DEBUG << "Invalid property value for AutomaticRetryConfig: "
                         << automaticRetryConfig;
        messages::propertyValueNotInList(aResp->res, automaticRetryConfig,
                                         "AutomaticRetryConfig");
        return;
    }

    crow::connections::systemBus->async_method_call(
        [aResp](const boost::system::error_code ec) {
            if (ec)
            {
                messages::internalError(aResp->res);
                return;
            }
        },
        "xyz.openbmc_project.Settings",
        "/xyz/openbmc_project/control/host0/auto_reboot",
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Control.Boot.RebootPolicy", "AutoReboot",
        std::variant<bool>(autoRebootEnabled));
}

/**
 * @brief Sets power restore policy properties.
 *
 * @param[in] aResp   Shared pointer for generating response message.
 * @param[in] policy  power restore policy properties from request.
 *
 * @return None.
 */
inline void
    setPowerRestorePolicy(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                          const std::string& policy)
{
    BMCWEB_LOG_DEBUG << "Set power restore policy.";

    const boost::container::flat_map<std::string, std::string> policyMaps = {
        {"AlwaysOn",
         "xyz.openbmc_project.Control.Power.RestorePolicy.Policy.AlwaysOn"},
        {"AlwaysOff",
         "xyz.openbmc_project.Control.Power.RestorePolicy.Policy.AlwaysOff"},
        {"LastState",
         "xyz.openbmc_project.Control.Power.RestorePolicy.Policy.Restore"}};

    std::string powerRestorPolicy;

    auto policyMapsIt = policyMaps.find(policy);
    if (policyMapsIt == policyMaps.end())
    {
        messages::propertyValueNotInList(aResp->res, policy,
                                         "PowerRestorePolicy");
        return;
    }

    powerRestorPolicy = policyMapsIt->second;

    crow::connections::systemBus->async_method_call(
        [aResp](const boost::system::error_code ec) {
            if (ec)
            {
                messages::internalError(aResp->res);
                return;
            }
        },
        "xyz.openbmc_project.Settings",
        "/xyz/openbmc_project/control/host0/power_restore_policy",
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Control.Power.RestorePolicy", "PowerRestorePolicy",
        std::variant<std::string>(powerRestorPolicy));
}

#ifdef BMCWEB_ENABLE_REDFISH_PROVISIONING_FEATURE
/**
 * @brief Retrieves provisioning status
 *
 * @param[in] aResp     Shared pointer for completing asynchronous calls.
 *
 * @return None.
 */
inline void getProvisioningStatus(std::shared_ptr<bmcweb::AsyncResp> aResp)
{
    BMCWEB_LOG_DEBUG << "Get OEM information.";
    crow::connections::systemBus->async_method_call(
        [aResp](const boost::system::error_code ec,
                const std::vector<std::pair<std::string, VariantType>>&
                    propertiesList) {
            nlohmann::json& oemPFR =
                aResp->res.jsonValue["Oem"]["OpenBmc"]["FirmwareProvisioning"];
            aResp->res.jsonValue["Oem"]["OpenBmc"]["@odata.type"] =
                "#OemComputerSystem.OpenBmc";
            oemPFR["@odata.type"] = "#OemComputerSystem.FirmwareProvisioning";

            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                // not an error, don't have to have the interface
                oemPFR["ProvisioningStatus"] = "NotProvisioned";
                return;
            }

            const bool* provState = nullptr;
            const bool* lockState = nullptr;
            for (const std::pair<std::string, VariantType>& property :
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
 * @brief Retrieves provisioned platform state
 *
 * @param[in] aResp     Shared pointer for completing asynchronous calls.
 *
 * @return None.
 */
inline void getPlatformState(std::shared_ptr<bmcweb::AsyncResp> aResp)
{
    BMCWEB_LOG_DEBUG << "Get OEM information.";
    crow::connections::systemBus->async_method_call(
        [aResp](const boost::system::error_code ec,
                const std::vector<std::pair<std::string, VariantType>>&
                    propertiesList) {
            nlohmann::json& oemPFR =
                aResp->res.jsonValue["Oem"]["OpenBmc"]["FirmwareProvisioning"]["Status"];

            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                // not an error, don't have to have the interface
                return;
            }

            const uint8_t* postcode = nullptr;
            const std::string* platformState = nullptr;
            for (const std::pair<std::string, VariantType>& property :
                 propertiesList)
            {
                if (property.first == "Data")
                {
                    postcode = std::get_if<uint8_t>(&property.second);
                }
                else if (property.first == "PlatformState")
                {
                    platformState = std::get_if<std::string>(&property.second);
                }
            }

            if ((postcode == nullptr) || (platformState == nullptr))
            {
                BMCWEB_LOG_DEBUG << "Unable to get PFR platform state.";
                messages::internalError(aResp->res);
                return;
            }
            oemPFR["Data"] = *postcode;
            oemPFR["PlatformState"] = *platformState;
        },
        "xyz.openbmc_project.PFR.Manager", "/xyz/openbmc_project/pfr",
        "org.freedesktop.DBus.Properties", "GetAll",
        "xyz.openbmc_project.State.Boot.Platform");
}

/**
 * @brief Translate the PowerMode to a response message.
 *
 * @param[in] aResp  Shared pointer for generating response message.
 * @param[in] modeValue  PowerMode value to be translated
 *
 * @return None.
 */
inline void translatePowerMode(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                               const std::string& modeValue)
{
    std::string modeString;

    if (modeValue == "xyz.openbmc_project.Control.Power.Mode.PowerMode.Static")
    {
        aResp->res.jsonValue["PowerMode"] = "Static";
    }
    else if (
        modeValue ==
        "xyz.openbmc_project.Control.Power.Mode.PowerMode.MaximumPerformance")
    {
        aResp->res.jsonValue["PowerMode"] = "MaximumPerformance";
    }
    else if (modeValue ==
             "xyz.openbmc_project.Control.Power.Mode.PowerMode.PowerSaving")
    {
        aResp->res.jsonValue["PowerMode"] = "PowerSaving";
    }
    else if (modeValue ==
             "xyz.openbmc_project.Control.Power.Mode.PowerMode.OEM")
    {
        aResp->res.jsonValue["PowerMode"] = "OEM";
    }
    else
    {
        // Any other values would be invalid
        BMCWEB_LOG_DEBUG << "PowerMode value was not valid: " << modeValue;
        messages::internalError(aResp->res);
    }
}

/**
 * @brief Retrieves system power mode
 *
 * @param[in] aResp  Shared pointer for generating response message.
 *
 * @return None.
 */
inline void getPowerMode(const std::shared_ptr<bmcweb::AsyncResp>& aResp)
{
    BMCWEB_LOG_DEBUG << "Get power mode.";

    // Get Power Mode object path:
    crow::connections::systemBus->async_method_call(
        [aResp](
            const boost::system::error_code ec,
            const std::vector<std::pair<
                std::string,
                std::vector<std::pair<std::string, std::vector<std::string>>>>>&
                subtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG
                    << "DBUS response error on Power.Mode GetSubTree " << ec;
                // This is an optional D-Bus object so just return if
                // error occurs
                return;
            }
            if (subtree.empty())
            {
                // As noted above, this is an optional interface so just return
                // if there is no instance found
                return;
            }
            if (subtree.size() > 1)
            {
                // More then one PowerMode object is not supported and is an
                // error
                BMCWEB_LOG_DEBUG
                    << "Found more than 1 system D-Bus Power.Mode objects: "
                    << subtree.size();
                messages::internalError(aResp->res);
                return;
            }
            if ((subtree[0].first.empty()) || (subtree[0].second.size() != 1))
            {
                BMCWEB_LOG_DEBUG << "Power.Mode mapper error!";
                messages::internalError(aResp->res);
                return;
            }
            const std::string& path = subtree[0].first;
            const std::string& service = subtree[0].second.begin()->first;
            if (service.empty())
            {
                BMCWEB_LOG_DEBUG << "Power.Mode service mapper error!";
                messages::internalError(aResp->res);
                return;
            }
            // Valid Power Mode object found, now read the current value
            crow::connections::systemBus->async_method_call(
                [aResp](const boost::system::error_code ec,
                        const std::variant<std::string>& pmode) {
                    if (ec)
                    {
                        BMCWEB_LOG_DEBUG
                            << "DBUS response error on PowerMode Get: " << ec;
                        messages::internalError(aResp->res);
                        return;
                    }

                    const std::string* s = std::get_if<std::string>(&pmode);
                    if (s == nullptr)
                    {
                        BMCWEB_LOG_DEBUG << "Unable to get PowerMode value";
                        messages::internalError(aResp->res);
                        return;
                    }

                    aResp->res.jsonValue["PowerMode@Redfish.AllowableValues"] =
                        {"Static", "MaximumPerformance", "PowerSaving"};

                    BMCWEB_LOG_DEBUG << "Current power mode: " << *s;
                    translatePowerMode(aResp, *s);
                },
                service, path, "org.freedesktop.DBus.Properties", "Get",
                "xyz.openbmc_project.Control.Power.Mode", "PowerMode");
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree", "/", int32_t(0),
        std::array<const char*, 1>{"xyz.openbmc_project.Control.Power.Mode"});
}

/**
 * @brief Validate the specified mode is valid and return the PowerMode
 * name associated with that string
 *
 * @param[in] aResp   Shared pointer for generating response message.
 * @param[in] modeString  String representing the desired PowerMode
 *
 * @return PowerMode value or empty string if mode is not valid
 */
inline std::string
    validatePowerMode(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                      const std::string& modeString)
{
    std::string mode;

    if (modeString == "Static")
    {
        mode = "xyz.openbmc_project.Control.Power.Mode.PowerMode.Static";
    }
    else if (modeString == "MaximumPerformance")
    {
        mode =
            "xyz.openbmc_project.Control.Power.Mode.PowerMode.MaximumPerformance";
    }
    else if (modeString == "PowerSaving")
    {
        mode = "xyz.openbmc_project.Control.Power.Mode.PowerMode.PowerSaving";
    }
    else
    {
        messages::propertyValueNotInList(aResp->res, modeString, "PowerMode");
    }
    return mode;
}

/**
 * @brief Sets system power mode.
 *
 * @param[in] aResp   Shared pointer for generating response message.
 * @param[in] pmode   System power mode from request.
 *
 * @return None.
 */
inline void setPowerMode(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                         const std::string& pmode)
{
    BMCWEB_LOG_DEBUG << "Set power mode.";

    std::string powerMode = validatePowerMode(aResp, pmode);
    if (powerMode.empty())
    {
        return;
    }

    // Get Power Mode object path:
    crow::connections::systemBus->async_method_call(
        [aResp, powerMode](
            const boost::system::error_code ec,
            const std::vector<std::pair<
                std::string,
                std::vector<std::pair<std::string, std::vector<std::string>>>>>&
                subtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG
                    << "DBUS response error on Power.Mode GetSubTree " << ec;
                // This is an optional D-Bus object, but user attempted to patch
                messages::internalError(aResp->res);
                return;
            }
            if (subtree.empty())
            {
                // This is an optional D-Bus object, but user attempted to patch
                messages::resourceNotFound(aResp->res, "ComputerSystem",
                                           "PowerMode");
                return;
            }
            if (subtree.size() > 1)
            {
                // More then one PowerMode object is not supported and is an
                // error
                BMCWEB_LOG_DEBUG
                    << "Found more than 1 system D-Bus Power.Mode objects: "
                    << subtree.size();
                messages::internalError(aResp->res);
                return;
            }
            if ((subtree[0].first.empty()) || (subtree[0].second.size() != 1))
            {
                BMCWEB_LOG_DEBUG << "Power.Mode mapper error!";
                messages::internalError(aResp->res);
                return;
            }
            const std::string& path = subtree[0].first;
            const std::string& service = subtree[0].second.begin()->first;
            if (service.empty())
            {
                BMCWEB_LOG_DEBUG << "Power.Mode service mapper error!";
                messages::internalError(aResp->res);
                return;
            }

            BMCWEB_LOG_DEBUG << "Setting power mode(" << powerMode << ") -> "
                             << path;

            // Set the Power Mode property
            crow::connections::systemBus->async_method_call(
                [aResp](const boost::system::error_code ec) {
                    if (ec)
                    {
                        messages::internalError(aResp->res);
                        return;
                    }
                },
                service, path, "org.freedesktop.DBus.Properties", "Set",
                "xyz.openbmc_project.Control.Power.Mode", "PowerMode",
                std::variant<std::string>(powerMode));
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree", "/", int32_t(0),
        std::array<const char*, 1>{"xyz.openbmc_project.Control.Power.Mode"});
}

/**
 * @brief Translates watchdog timeout action DBUS property value to redfish.
 *
 * @param[in] dbusAction    The watchdog timeout action in D-BUS.
 *
 * @return Returns as a string, the timeout action in Redfish terms. If
 * translation cannot be done, returns an empty string.
 */
inline std::string dbusToRfWatchdogAction(const std::string& dbusAction)
{
    if (dbusAction == "xyz.openbmc_project.State.Watchdog.Action.None")
    {
        return "None";
    }
    if (dbusAction == "xyz.openbmc_project.State.Watchdog.Action.HardReset")
    {
        return "ResetSystem";
    }
    if (dbusAction == "xyz.openbmc_project.State.Watchdog.Action.PowerOff")
    {
        return "PowerDown";
    }
    if (dbusAction == "xyz.openbmc_project.State.Watchdog.Action.PowerCycle")
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

inline std::string rfToDbusWDTTimeOutAct(const std::string& rfAction)
{
    if (rfAction == "None")
    {
        return "xyz.openbmc_project.State.Watchdog.Action.None";
    }
    if (rfAction == "PowerCycle")
    {
        return "xyz.openbmc_project.State.Watchdog.Action.PowerCycle";
    }
    if (rfAction == "PowerDown")
    {
        return "xyz.openbmc_project.State.Watchdog.Action.PowerOff";
    }
    if (rfAction == "ResetSystem")
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
inline void
    getHostWatchdogTimer(const std::shared_ptr<bmcweb::AsyncResp>& aResp)
{
    BMCWEB_LOG_DEBUG << "Get host watchodg";
    crow::connections::systemBus->async_method_call(
        [aResp](const boost::system::error_code ec,
                PropertiesType& properties) {
            if (ec)
            {
                // watchdog service is stopped
                BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                return;
            }

            BMCWEB_LOG_DEBUG << "Got " << properties.size() << " wdt prop.";

            nlohmann::json& hostWatchdogTimer =
                aResp->res.jsonValue["HostWatchdogTimer"];

            // watchdog service is running/enabled
            hostWatchdogTimer["Status"]["State"] = "Enabled";

            for (const auto& property : properties)
            {
                BMCWEB_LOG_DEBUG << "prop=" << property.first;
                if (property.first == "Enabled")
                {
                    const bool* state = std::get_if<bool>(&property.second);

                    if (!state)
                    {
                        messages::internalError(aResp->res);
                        return;
                    }

                    hostWatchdogTimer["FunctionEnabled"] = *state;
                }
                else if (property.first == "ExpireAction")
                {
                    const std::string* s =
                        std::get_if<std::string>(&property.second);
                    if (!s)
                    {
                        messages::internalError(aResp->res);
                        return;
                    }

                    std::string action = dbusToRfWatchdogAction(*s);
                    if (action.empty())
                    {
                        messages::internalError(aResp->res);
                        return;
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
inline void setWDTProperties(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                             const std::optional<bool> wdtEnable,
                             const std::optional<std::string>& wdtTimeOutAction)
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

using ipsPropertiesType =
    std::vector<std::pair<std::string, dbus::utility::DbusVariantType>>;
/**
 * @brief Parse the Idle Power Saver properties into json
 *
 * @param[in] aResp     Shared pointer for completing asynchronous calls.
 * @param[in] properties  IPS property data from DBus.
 *
 * @return true if successful
 */
inline bool parseIpsProperties(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                               ipsPropertiesType& properties)
{
    for (const auto& property : properties)
    {
        if (property.first == "Enabled")
        {
            const bool* state = std::get_if<bool>(&property.second);
            if (!state)
            {
                return false;
            }
            aResp->res.jsonValue["IdlePowerSaver"][property.first] = *state;
        }
        else if (property.first == "EnterUtilizationPercent")
        {
            const uint8_t* util = std::get_if<uint8_t>(&property.second);
            if (!util)
            {
                return false;
            }
            aResp->res.jsonValue["IdlePowerSaver"][property.first] = *util;
        }
        else if (property.first == "EnterDwellTime")
        {
            // Convert Dbus time from milliseconds to seconds
            const uint64_t* timeMilliseconds =
                std::get_if<uint64_t>(&property.second);
            if (!timeMilliseconds)
            {
                return false;
            }
            const std::chrono::duration<uint64_t, std::milli> ms(
                *timeMilliseconds);
            aResp->res.jsonValue["IdlePowerSaver"]["EnterDwellTimeSeconds"] =
                std::chrono::duration_cast<std::chrono::duration<uint64_t>>(ms)
                    .count();
        }
        else if (property.first == "ExitUtilizationPercent")
        {
            const uint8_t* util = std::get_if<uint8_t>(&property.second);
            if (!util)
            {
                return false;
            }
            aResp->res.jsonValue["IdlePowerSaver"][property.first] = *util;
        }
        else if (property.first == "ExitDwellTime")
        {
            // Convert Dbus time from milliseconds to seconds
            const uint64_t* timeMilliseconds =
                std::get_if<uint64_t>(&property.second);
            if (!timeMilliseconds)
            {
                return false;
            }
            const std::chrono::duration<uint64_t, std::milli> ms(
                *timeMilliseconds);
            aResp->res.jsonValue["IdlePowerSaver"]["ExitDwellTimeSeconds"] =
                std::chrono::duration_cast<std::chrono::duration<uint64_t>>(ms)
                    .count();
        }
        else
        {
            BMCWEB_LOG_WARNING << "Unexpected IdlePowerSaver property: "
                               << property.first;
        }
    }

    return true;
}

/**
 * @brief Retrieves host watchdog timer properties over DBUS
 *
 * @param[in] aResp     Shared pointer for completing asynchronous calls.
 *
 * @return None.
 */
inline void getIdlePowerSaver(const std::shared_ptr<bmcweb::AsyncResp>& aResp)
{
    BMCWEB_LOG_DEBUG << "Get idle power saver parameters";

    // Get IdlePowerSaver object path:
    crow::connections::systemBus->async_method_call(
        [aResp](
            const boost::system::error_code ec,
            const std::vector<std::pair<
                std::string,
                std::vector<std::pair<std::string, std::vector<std::string>>>>>&
                subtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG
                    << "DBUS response error on Power.IdlePowerSaver GetSubTree "
                    << ec;
                messages::internalError(aResp->res);
                return;
            }
            if (subtree.empty())
            {
                // This is an optional interface so just return
                // if there is no instance found
                BMCWEB_LOG_DEBUG << "No instances found";
                return;
            }
            if (subtree.size() > 1)
            {
                // More then one PowerIdlePowerSaver object is not supported and
                // is an error
                BMCWEB_LOG_DEBUG << "Found more than 1 system D-Bus "
                                    "Power.IdlePowerSaver objects: "
                                 << subtree.size();
                messages::internalError(aResp->res);
                return;
            }
            if ((subtree[0].first.empty()) || (subtree[0].second.size() != 1))
            {
                BMCWEB_LOG_DEBUG << "Power.IdlePowerSaver mapper error!";
                messages::internalError(aResp->res);
                return;
            }
            const std::string& path = subtree[0].first;
            const std::string& service = subtree[0].second.begin()->first;
            if (service.empty())
            {
                BMCWEB_LOG_DEBUG
                    << "Power.IdlePowerSaver service mapper error!";
                messages::internalError(aResp->res);
                return;
            }

            // Valid IdlePowerSaver object found, now read the current values
            crow::connections::systemBus->async_method_call(
                [aResp](const boost::system::error_code ec,
                        ipsPropertiesType& properties) {
                    if (ec)
                    {
                        BMCWEB_LOG_ERROR
                            << "DBUS response error on IdlePowerSaver GetAll: "
                            << ec;
                        messages::internalError(aResp->res);
                        return;
                    }

                    if (parseIpsProperties(aResp, properties) == false)
                    {
                        messages::internalError(aResp->res);
                        return;
                    }
                },
                service, path, "org.freedesktop.DBus.Properties", "GetAll",
                "xyz.openbmc_project.Control.Power.IdlePowerSaver");
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree", "/", int32_t(0),
        std::array<const char*, 1>{
            "xyz.openbmc_project.Control.Power.IdlePowerSaver"});

    BMCWEB_LOG_DEBUG << "EXIT: Get idle power saver parameters";
}

/**
 * @brief Sets Idle Power Saver properties.
 *
 * @param[in] aResp      Shared pointer for generating response message.
 * @param[in] ipsEnable  The IPS Enable value (true/false) from incoming
 *                       RF request.
 * @param[in] ipsEnterUtil The utilization limit to enter idle state.
 * @param[in] ipsEnterTime The time the utilization must be below ipsEnterUtil
 * before entering idle state.
 * @param[in] ipsExitUtil The utilization limit when exiting idle state.
 * @param[in] ipsExitTime The time the utilization must be above ipsExutUtil
 * before exiting idle state
 *
 * @return None.
 */
inline void setIdlePowerSaver(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                              const std::optional<bool> ipsEnable,
                              const std::optional<uint8_t> ipsEnterUtil,
                              const std::optional<uint64_t> ipsEnterTime,
                              const std::optional<uint8_t> ipsExitUtil,
                              const std::optional<uint64_t> ipsExitTime)
{
    BMCWEB_LOG_DEBUG << "Set idle power saver properties";

    // Get IdlePowerSaver object path:
    crow::connections::systemBus->async_method_call(
        [aResp, ipsEnable, ipsEnterUtil, ipsEnterTime, ipsExitUtil,
         ipsExitTime](
            const boost::system::error_code ec,
            const std::vector<std::pair<
                std::string,
                std::vector<std::pair<std::string, std::vector<std::string>>>>>&
                subtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG
                    << "DBUS response error on Power.IdlePowerSaver GetSubTree "
                    << ec;
                messages::internalError(aResp->res);
                return;
            }
            if (subtree.empty())
            {
                // This is an optional D-Bus object, but user attempted to patch
                messages::resourceNotFound(aResp->res, "ComputerSystem",
                                           "IdlePowerSaver");
                return;
            }
            if (subtree.size() > 1)
            {
                // More then one PowerIdlePowerSaver object is not supported and
                // is an error
                BMCWEB_LOG_DEBUG
                    << "Found more than 1 system D-Bus Power.IdlePowerSaver objects: "
                    << subtree.size();
                messages::internalError(aResp->res);
                return;
            }
            if ((subtree[0].first.empty()) || (subtree[0].second.size() != 1))
            {
                BMCWEB_LOG_DEBUG << "Power.IdlePowerSaver mapper error!";
                messages::internalError(aResp->res);
                return;
            }
            const std::string& path = subtree[0].first;
            const std::string& service = subtree[0].second.begin()->first;
            if (service.empty())
            {
                BMCWEB_LOG_DEBUG
                    << "Power.IdlePowerSaver service mapper error!";
                messages::internalError(aResp->res);
                return;
            }

            // Valid Power IdlePowerSaver object found, now set any values that
            // need to be updated

            if (ipsEnable)
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
                    service, path, "org.freedesktop.DBus.Properties", "Set",
                    "xyz.openbmc_project.Control.Power.IdlePowerSaver",
                    "Enabled", std::variant<bool>(*ipsEnable));
            }
            if (ipsEnterUtil)
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
                    service, path, "org.freedesktop.DBus.Properties", "Set",
                    "xyz.openbmc_project.Control.Power.IdlePowerSaver",
                    "EnterUtilizationPercent",
                    std::variant<uint8_t>(*ipsEnterUtil));
            }
            if (ipsEnterTime)
            {
                // Convert from seconds into milliseconds for DBus
                const uint64_t timeMilliseconds = *ipsEnterTime * 1000;
                crow::connections::systemBus->async_method_call(
                    [aResp](const boost::system::error_code ec) {
                        if (ec)
                        {
                            BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                            messages::internalError(aResp->res);
                            return;
                        }
                    },
                    service, path, "org.freedesktop.DBus.Properties", "Set",
                    "xyz.openbmc_project.Control.Power.IdlePowerSaver",
                    "EnterDwellTime", std::variant<uint64_t>(timeMilliseconds));
            }
            if (ipsExitUtil)
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
                    service, path, "org.freedesktop.DBus.Properties", "Set",
                    "xyz.openbmc_project.Control.Power.IdlePowerSaver",
                    "ExitUtilizationPercent",
                    std::variant<uint8_t>(*ipsExitUtil));
            }
            if (ipsExitTime)
            {
                // Convert from seconds into milliseconds for DBus
                const uint64_t timeMilliseconds = *ipsExitTime * 1000;
                crow::connections::systemBus->async_method_call(
                    [aResp](const boost::system::error_code ec) {
                        if (ec)
                        {
                            BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                            messages::internalError(aResp->res);
                            return;
                        }
                    },
                    service, path, "org.freedesktop.DBus.Properties", "Set",
                    "xyz.openbmc_project.Control.Power.IdlePowerSaver",
                    "ExitDwellTime", std::variant<uint64_t>(timeMilliseconds));
            }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree", "/", int32_t(0),
        std::array<const char*, 1>{
            "xyz.openbmc_project.Control.Power.IdlePowerSaver"});

    BMCWEB_LOG_DEBUG << "EXIT: Set idle power saver parameters";
}

/**
 * SystemsCollection derived class for delivering ComputerSystems Collection
 * Schema
 */
inline void requestRoutesSystemsCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/")
        .privileges(redfish::privileges::getComputerSystemCollection)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request& /*req*/,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                asyncResp->res.jsonValue["@odata.type"] =
                    "#ComputerSystemCollection.ComputerSystemCollection";
                asyncResp->res.jsonValue["@odata.id"] = "/redfish/v1/Systems";
                asyncResp->res.jsonValue["Name"] = "Computer System Collection";

                crow::connections::systemBus->async_method_call(
                    [asyncResp](const boost::system::error_code ec,
                                const std::variant<std::string>& /*hostName*/) {
                        nlohmann::json& ifaceArray =
                            asyncResp->res.jsonValue["Members"];
                        ifaceArray = nlohmann::json::array();
                        auto& count =
                            asyncResp->res.jsonValue["Members@odata.count"];
                        ifaceArray.push_back(
                            {{"@odata.id", "/redfish/v1/Systems/system"}});
                        count = ifaceArray.size();
                        if (!ec)
                        {
                            BMCWEB_LOG_DEBUG << "Hypervisor is available";
                            ifaceArray.push_back(
                                {{"@odata.id",
                                  "/redfish/v1/Systems/hypervisor"}});
                            count = ifaceArray.size();
                        }
                    },
                    "xyz.openbmc_project.Settings",
                    "/xyz/openbmc_project/network/hypervisor",
                    "org.freedesktop.DBus.Properties", "Get",
                    "xyz.openbmc_project.Network.SystemConfiguration",
                    "HostName");
            });
}

/**
 * Function transceives data with dbus directly.
 */
inline void doNMI(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    constexpr char const* serviceName = "xyz.openbmc_project.Control.Host.NMI";
    constexpr char const* objectPath = "/xyz/openbmc_project/control/host0/nmi";
    constexpr char const* interfaceName =
        "xyz.openbmc_project.Control.Host.NMI";
    constexpr char const* method = "NMI";

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

/**
 * SystemActionsReset class supports handle POST method for Reset action.
 * The class retrieves and sends data directly to D-Bus.
 */
inline void requestRoutesSystemActionsReset(App& app)
{
    /**
     * Function handles POST method request.
     * Analyzes POST body message before sends Reset request data to D-Bus.
     */
    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/system/Actions/ComputerSystem.Reset/")
        .privileges(redfish::privileges::postComputerSystem)
        .methods(
            boost::beast::http::verb::
                post)([](const crow::Request& req,
                         const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
            std::string resetType;
            if (!json_util::readJson(req, asyncResp->res, "ResetType",
                                     resetType))
            {
                return;
            }

            // Get the command and host vs. chassis
            std::string command;
            bool hostCommand;
            if ((resetType == "On") || (resetType == "ForceOn"))
            {
                command = "xyz.openbmc_project.State.Host.Transition.On";
                hostCommand = true;
            }
            else if (resetType == "ForceOff")
            {
                command = "xyz.openbmc_project.State.Chassis.Transition.Off";
                hostCommand = false;
            }
            else if (resetType == "ForceRestart")
            {
                command =
                    "xyz.openbmc_project.State.Host.Transition.ForceWarmReboot";
                hostCommand = true;
            }
            else if (resetType == "GracefulShutdown")
            {
                command = "xyz.openbmc_project.State.Host.Transition.Off";
                hostCommand = true;
            }
            else if (resetType == "GracefulRestart")
            {
                command =
                    "xyz.openbmc_project.State.Host.Transition.GracefulWarmReboot";
                hostCommand = true;
            }
            else if (resetType == "PowerCycle")
            {
                command = "xyz.openbmc_project.State.Host.Transition.Reboot";
                hostCommand = true;
            }
            else if (resetType == "Nmi")
            {
                doNMI(asyncResp);
                return;
            }
            else
            {
                messages::actionParameterUnknown(asyncResp->res, "Reset",
                                                 resetType);
                return;
            }

            if (hostCommand)
            {
                crow::connections::systemBus->async_method_call(
                    [asyncResp, resetType](const boost::system::error_code ec) {
                        if (ec)
                        {
                            BMCWEB_LOG_ERROR << "D-Bus responses error: " << ec;
                            if (ec.value() ==
                                boost::asio::error::invalid_argument)
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
                            if (ec.value() ==
                                boost::asio::error::invalid_argument)
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
                    "xyz.openbmc_project.State.Chassis",
                    "RequestedPowerTransition",
                    std::variant<std::string>{command});
            }
        });
}

/**
 * Systems derived class for delivering Computer Systems Schema.
 */
inline void requestRoutesSystems(App& app)
{

    /**
     * Functions triggers appropriate requests on DBus
     */
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/")
        .privileges(redfish::privileges::getComputerSystem)
        .methods(
            boost::beast::http::verb::
                get)([](const crow::Request&,
                        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
            asyncResp->res.jsonValue["@odata.type"] =
                "#ComputerSystem.v1_16_0.ComputerSystem";
            asyncResp->res.jsonValue["Name"] = "system";
            asyncResp->res.jsonValue["Id"] = "system";
            asyncResp->res.jsonValue["SystemType"] = "Physical";
            asyncResp->res.jsonValue["Description"] = "Computer System";
            asyncResp->res.jsonValue["ProcessorSummary"]["Count"] = 0;
            asyncResp->res.jsonValue["ProcessorSummary"]["Status"]["State"] =
                "Disabled";
            asyncResp->res.jsonValue["MemorySummary"]["TotalSystemMemoryGiB"] =
                uint64_t(0);
            asyncResp->res.jsonValue["MemorySummary"]["Status"]["State"] =
                "Disabled";
            asyncResp->res.jsonValue["@odata.id"] =
                "/redfish/v1/Systems/system";

            asyncResp->res.jsonValue["Processors"] = {
                {"@odata.id", "/redfish/v1/Systems/system/Processors"}};
            asyncResp->res.jsonValue["Memory"] = {
                {"@odata.id", "/redfish/v1/Systems/system/Memory"}};
            asyncResp->res.jsonValue["Storage"] = {
                {"@odata.id", "/redfish/v1/Systems/system/Storage"}};

            asyncResp->res.jsonValue["Actions"]["#ComputerSystem.Reset"] = {
                {"target",
                 "/redfish/v1/Systems/system/Actions/ComputerSystem.Reset"},
                {"@Redfish.ActionInfo",
                 "/redfish/v1/Systems/system/ResetActionInfo"}};

            asyncResp->res.jsonValue["LogServices"] = {
                {"@odata.id", "/redfish/v1/Systems/system/LogServices"}};

            asyncResp->res.jsonValue["Bios"] = {
                {"@odata.id", "/redfish/v1/Systems/system/Bios"}};

            asyncResp->res.jsonValue["Links"]["ManagedBy"] = {
                {{"@odata.id", "/redfish/v1/Managers/bmc"}}};

            asyncResp->res.jsonValue["Status"] = {
                {"Health", "OK"},
                {"State", "Enabled"},
            };

            // Fill in SerialConsole info
            asyncResp->res.jsonValue["SerialConsole"]["MaxConcurrentSessions"] =
                15;
            asyncResp->res.jsonValue["SerialConsole"]["IPMI"] = {
                {"ServiceEnabled", true},
            };
            // TODO (Gunnar): Should look for obmc-console-ssh@2200.service
            asyncResp->res.jsonValue["SerialConsole"]["SSH"] = {
                {"ServiceEnabled", true},
                {"Port", 2200},
                // https://github.com/openbmc/docs/blob/master/console.md
                {"HotKeySequenceDisplay", "Press ~. to exit console"},
            };

#ifdef BMCWEB_ENABLE_KVM
            // Fill in GraphicalConsole info
            asyncResp->res.jsonValue["GraphicalConsole"] = {
                {"ServiceEnabled", true},
                {"MaxConcurrentSessions", 4},
                {"ConnectTypesSupported", {"KVMIP"}},
            };
#endif // BMCWEB_ENABLE_KVM
            constexpr const std::array<const char*, 4> inventoryForSystems = {
                "xyz.openbmc_project.Inventory.Item.Dimm",
                "xyz.openbmc_project.Inventory.Item.Cpu",
                "xyz.openbmc_project.Inventory.Item.Drive",
                "xyz.openbmc_project.Inventory.Item.StorageController"};

            auto health = std::make_shared<HealthPopulate>(asyncResp);
            crow::connections::systemBus->async_method_call(
                [health](const boost::system::error_code ec,
                         std::vector<std::string>& resp) {
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

            getMainChassisId(
                asyncResp, [](const std::string& chassisId,
                              const std::shared_ptr<bmcweb::AsyncResp>& aRsp) {
                    aRsp->res.jsonValue["Links"]["Chassis"] = {
                        {{"@odata.id", "/redfish/v1/Chassis/" + chassisId}}};
                });

            getLocationIndicatorActive(asyncResp);
            // TODO (Gunnar): Remove IndicatorLED after enough time has passed
            getIndicatorLedState(asyncResp);
            getComputerSystem(asyncResp, health);
            getHostState(asyncResp);
            getBootProperties(asyncResp);
            getBootProgress(asyncResp);
            getPCIeDeviceList(asyncResp, "PCIeDevices");
            getHostWatchdogTimer(asyncResp);
            getPowerRestorePolicy(asyncResp);
            getAutomaticRetry(asyncResp);
            getLastResetTime(asyncResp);
#ifdef BMCWEB_ENABLE_REDFISH_PROVISIONING_FEATURE
            getProvisioningStatus(asyncResp);
#endif
            getPlatformState(asyncResp);
            getTrustedModuleRequiredToBoot(asyncResp);
            getPowerMode(asyncResp);
            getIdlePowerSaver(asyncResp);
        });
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/")
        .privileges(redfish::privileges::patchComputerSystem)
        .methods(boost::beast::http::verb::patch)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                std::optional<bool> locationIndicatorActive;
                std::optional<std::string> indicatorLed;
                std::optional<nlohmann::json> bootProps;
                std::optional<nlohmann::json> wdtTimerProps;
                std::optional<std::string> assetTag;
                std::optional<std::string> powerRestorePolicy;
                std::optional<std::string> powerMode;
                std::optional<nlohmann::json> ipsProps;
                if (!json_util::readJson(
                        req, asyncResp->res, "IndicatorLED", indicatorLed,
                        "LocationIndicatorActive", locationIndicatorActive,
                        "Boot", bootProps, "WatchdogTimer", wdtTimerProps,
                        "PowerRestorePolicy", powerRestorePolicy, "AssetTag",
                        assetTag, "PowerMode", powerMode, "IdlePowerSaver",
                        ipsProps))
                {
                    return;
                }

                asyncResp->res.result(boost::beast::http::status::no_content);

                if (assetTag)
                {
                    setAssetTag(asyncResp, *assetTag);
                }

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
                    setWDTProperties(asyncResp, wdtEnable, wdtTimeOutAction);
                }

                if (bootProps)
                {
                    std::optional<std::string> bootSource;
                    std::optional<std::string> bootType;
                    std::optional<std::string> bootEnable;
                    std::optional<std::string> automaticRetryConfig;
                    std::optional<bool> trustedModuleRequiredToBoot;

                    if (!json_util::readJson(
                            *bootProps, asyncResp->res,
                            "BootSourceOverrideTarget", bootSource,
                            "BootSourceOverrideMode", bootType,
                            "BootSourceOverrideEnabled", bootEnable,
                            "AutomaticRetryConfig", automaticRetryConfig,
                            "TrustedModuleRequiredToBoot",
                            trustedModuleRequiredToBoot))
                    {
                        return;
                    }

                    if (bootSource || bootType || bootEnable)
                    {
                        setBootProperties(asyncResp, bootSource, bootType,
                                          bootEnable);
                    }
                    if (automaticRetryConfig)
                    {
                        setAutomaticRetry(asyncResp, *automaticRetryConfig);
                    }

                    if (trustedModuleRequiredToBoot)
                    {
                        setTrustedModuleRequiredToBoot(
                            asyncResp, *trustedModuleRequiredToBoot);
                    }
                }

                if (locationIndicatorActive)
                {
                    setLocationIndicatorActive(asyncResp,
                                               *locationIndicatorActive);
                }

                // TODO (Gunnar): Remove IndicatorLED after enough time has
                // passed
                if (indicatorLed)
                {
                    setIndicatorLedState(asyncResp, *indicatorLed);
                    asyncResp->res.addHeader(
                        boost::beast::http::field::warning,
                        "299 - \"IndicatorLED is deprecated. Use "
                        "LocationIndicatorActive instead.\"");
                }

                if (powerRestorePolicy)
                {
                    setPowerRestorePolicy(asyncResp, *powerRestorePolicy);
                }

                if (powerMode)
                {
                    setPowerMode(asyncResp, *powerMode);
                }

                if (ipsProps)
                {
                    std::optional<bool> ipsEnable;
                    std::optional<uint8_t> ipsEnterUtil;
                    std::optional<uint64_t> ipsEnterTime;
                    std::optional<uint8_t> ipsExitUtil;
                    std::optional<uint64_t> ipsExitTime;

                    if (!json_util::readJson(
                            *ipsProps, asyncResp->res, "Enabled", ipsEnable,
                            "EnterUtilizationPercent", ipsEnterUtil,
                            "EnterDwellTimeSeconds", ipsEnterTime,
                            "ExitUtilizationPercent", ipsExitUtil,
                            "ExitDwellTimeSeconds", ipsExitTime))
                    {
                        return;
                    }
                    setIdlePowerSaver(asyncResp, ipsEnable, ipsEnterUtil,
                                      ipsEnterTime, ipsExitUtil, ipsExitTime);
                }
            });
}

/**
 * SystemResetActionInfo derived class for delivering Computer Systems
 * ResetType AllowableValues using ResetInfo schema.
 */
inline void requestRoutesSystemResetActionInfo(App& app)
{

    /**
     * Functions triggers appropriate requests on DBus
     */
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/ResetActionInfo/")
        .privileges(redfish::privileges::getActionInfo)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                asyncResp->res.jsonValue = {
                    {"@odata.type", "#ActionInfo.v1_1_2.ActionInfo"},
                    {"@odata.id", "/redfish/v1/Systems/system/ResetActionInfo"},
                    {"Name", "Reset Action Info"},
                    {"Id", "ResetActionInfo"},
                    {"Parameters",
                     {{{"Name", "ResetType"},
                       {"Required", true},
                       {"DataType", "String"},
                       {"AllowableValues",
                        {"On", "ForceOff", "ForceOn", "ForceRestart",
                         "GracefulRestart", "GracefulShutdown", "PowerCycle",
                         "Nmi"}}}}}};
            });
}
} // namespace redfish
