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

#include "bmcweb_config.h"

#include "app.hpp"
#include "dbus_singleton.hpp"
#include "dbus_utility.hpp"
#include "generated/enums/computer_system.hpp"
#include "generated/enums/resource.hpp"
#include "health.hpp"
#include "hypervisor_system.hpp"
#include "led.hpp"
#include "oem/ibm/lamp_test.hpp"
#include "oem/ibm/pcie_topology_refresh.hpp"
#include "oem/ibm/system_attention_indicator.hpp"
#include "query.hpp"
#include "redfish_util.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/dbus_utils.hpp"
#include "utils/json_utils.hpp"
#include "utils/pcie_util.hpp"
#include "utils/sw_utils.hpp"
#include "utils/time_utils.hpp"

#include <boost/asio/error.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/linux_error.hpp>
#include <boost/url/format.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace redfish
{

const static std::array<std::pair<std::string_view, std::string_view>, 2>
    protocolToDBusForSystems{
        {{"SSH", "obmc-console-ssh"}, {"IPMI", "phosphor-ipmi-net"}}};

/**
 * @brief Updates the Functional State of DIMMs
 *
 * @param[in] asyncResp Shared pointer for completing asynchronous calls
 * @param[in] dimmState Dimm's Functional state, true/false
 *
 * @return None.
 */
inline void
    updateDimmProperties(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         bool isDimmFunctional)
{
    BMCWEB_LOG_DEBUG("Dimm Functional: {}", isDimmFunctional);

    // Set it as Enabled if at least one DIMM is functional
    // Update STATE only if previous State was DISABLED and current Dimm is
    // ENABLED.
    const nlohmann::json& prevMemSummary =
        asyncResp->res.jsonValue["MemorySummary"]["Status"]["State"];
    if (prevMemSummary == "Disabled")
    {
        if (isDimmFunctional)
        {
            asyncResp->res.jsonValue["MemorySummary"]["Status"]["State"] =
                "Enabled";
        }
    }
}

/*
 * @brief Update "ProcessorSummary" "Status" "State" based on
 *        CPU Functional State
 *
 * @param[in] asyncResp Shared pointer for completing asynchronous calls
 * @param[in] cpuFunctionalState is CPU functional true/false
 *
 * @return None.
 */
inline void modifyCpuFunctionalState(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp, bool isCpuFunctional)
{
    BMCWEB_LOG_DEBUG("Cpu Functional: {}", isCpuFunctional);

    const nlohmann::json& prevProcState =
        asyncResp->res.jsonValue["ProcessorSummary"]["Status"]["State"];

    // Set it as Enabled if at least one CPU is functional
    // Update STATE only if previous State was Non_Functional and current CPU is
    // Functional.
    if (prevProcState == "Disabled")
    {
        if (isCpuFunctional)
        {
            asyncResp->res.jsonValue["ProcessorSummary"]["Status"]["State"] =
                "Enabled";
        }
    }
}

/*
 * @brief Update "ProcessorSummary" "Count" based on Cpu PresenceState
 *
 * @param[in] asyncResp Shared pointer for completing asynchronous calls
 * @param[in] cpuPresenceState CPU present or not
 *
 * @return None.
 */
inline void
    modifyCpuPresenceState(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           bool isCpuPresent)
{
    BMCWEB_LOG_DEBUG("Cpu Present: {}", isCpuPresent);

    if (isCpuPresent)
    {
        nlohmann::json& procCount =
            asyncResp->res.jsonValue["ProcessorSummary"]["Count"];
        auto* procCountPtr =
            procCount.get_ptr<nlohmann::json::number_integer_t*>();
        if (procCountPtr != nullptr)
        {
            // shouldn't be possible to be nullptr
            *procCountPtr += 1;
        }
    }
}

inline void getProcessorProperties(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::vector<std::pair<std::string, dbus::utility::DbusVariantType>>&
        properties)
{
    BMCWEB_LOG_DEBUG("Got {} Cpu properties.", properties.size());

    // TODO: Get Model

    const uint16_t* coreCount = nullptr;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), properties, "CoreCount", coreCount);

    if (!success)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    if (coreCount != nullptr)
    {
        nlohmann::json& coreCountJson =
            asyncResp->res.jsonValue["ProcessorSummary"]["CoreCount"];
        uint64_t* coreCountJsonPtr = coreCountJson.get_ptr<uint64_t*>();

        if (coreCountJsonPtr == nullptr)
        {
            coreCountJson = *coreCount;
        }
        else
        {
            *coreCountJsonPtr += *coreCount;
        }
    }
}

/*
 * @brief Get ProcessorSummary fields
 *
 * @param[in] asyncResp Shared pointer for completing asynchronous calls
 * @param[in] service dbus service for Cpu Information
 * @param[in] path dbus path for Cpu
 *
 * @return None.
 */
inline void
    getProcessorSummary(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& service, const std::string& path)
{
    auto getCpuPresenceState = [asyncResp](const boost::system::error_code& ec3,
                                           const bool cpuPresenceCheck) {
        if (ec3)
        {
            BMCWEB_LOG_ERROR("DBUS response error {}", ec3);
            return;
        }
        modifyCpuPresenceState(asyncResp, cpuPresenceCheck);
    };

    // Get the Presence of CPU
    sdbusplus::asio::getProperty<bool>(
        *crow::connections::systemBus, service, path,
        "xyz.openbmc_project.Inventory.Item", "Present",
        std::move(getCpuPresenceState));

    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, service, path,
        "xyz.openbmc_project.Inventory.Item.Cpu",
        [asyncResp, service,
         path](const boost::system::error_code& ec2,
               const dbus::utility::DBusPropertiesMap& properties) {
        if (ec2)
        {
            BMCWEB_LOG_ERROR("DBUS response error {}", ec2);
            messages::internalError(asyncResp->res);
            return;
        }
        getProcessorProperties(asyncResp, properties);
    });
}

/*
 * @brief processMemoryProperties fields
 *
 * @param[in] asyncResp Shared pointer for completing asynchronous calls
 * @param[in] DBUS properties for memory
 *
 * @return None.
 */
inline void
    processMemoryProperties(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const dbus::utility::DBusPropertiesMap& properties)
{
    BMCWEB_LOG_DEBUG("Got {} Dimm properties.", properties.size());

    if (properties.empty())
    {
        return;
    }

    const size_t* memorySizeInKB = nullptr;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), properties, "MemorySizeInKB",
        memorySizeInKB);

    if (!success)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    if (memorySizeInKB != nullptr)
    {
        nlohmann::json& totalMemory =
            asyncResp->res.jsonValue["MemorySummary"]["TotalSystemMemoryGiB"];
        const double* preValue = totalMemory.get_ptr<const double*>();
        if (preValue == nullptr)
        {
            asyncResp->res.jsonValue["MemorySummary"]["TotalSystemMemoryGiB"] =
                static_cast<double>(*memorySizeInKB) / (1024 * 1024);
        }
        else
        {
            asyncResp->res.jsonValue["MemorySummary"]["TotalSystemMemoryGiB"] =
                static_cast<double>(*memorySizeInKB) / (1024 * 1024) +
                *preValue;
        }
    }
}

/*
 * @brief Get getMemorySummary fields
 *
 * @param[in] asyncResp Shared pointer for completing asynchronous calls
 * @param[in] service dbus service for memory Information
 * @param[in] path dbus path for memory
 *
 * @return None.
 */
inline void
    getMemorySummary(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     const std::string& service, const std::string& path)
{
    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, service, path,
        "xyz.openbmc_project.Inventory.Item.Dimm",
        [asyncResp, service,
         path](const boost::system::error_code& ec2,
               const dbus::utility::DBusPropertiesMap& properties) {
        if (ec2)
        {
            BMCWEB_LOG_ERROR("DBUS response error {}", ec2);
            messages::internalError(asyncResp->res);
            return;
        }
        processMemoryProperties(asyncResp, properties);
    });
}

inline void afterGetUUID(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const boost::system::error_code& ec,
                         const dbus::utility::DBusPropertiesMap& properties)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("DBUS response error {}", ec);
        messages::internalError(asyncResp->res);
        return;
    }
    BMCWEB_LOG_DEBUG("Got {} UUID properties.", properties.size());

    const std::string* uUID = nullptr;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), properties, "UUID", uUID);

    if (!success)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    if (uUID != nullptr)
    {
        std::string valueStr = *uUID;
        if (valueStr.size() == 32)
        {
            valueStr.insert(8, 1, '-');
            valueStr.insert(13, 1, '-');
            valueStr.insert(18, 1, '-');
            valueStr.insert(23, 1, '-');
        }
        BMCWEB_LOG_DEBUG("UUID = {}", valueStr);
        asyncResp->res.jsonValue["UUID"] = valueStr;
    }
}

inline void
    afterGetInventory(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const boost::system::error_code& ec,
                      const dbus::utility::DBusPropertiesMap& propertiesList)
{
    if (ec)
    {
        // doesn't have to include this
        // interface
        return;
    }
    BMCWEB_LOG_DEBUG("Got {} properties for system", propertiesList.size());

    const std::string* partNumber = nullptr;
    const std::string* serialNumber = nullptr;
    const std::string* manufacturer = nullptr;
    const std::string* model = nullptr;
    const std::string* subModel = nullptr;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), propertiesList, "PartNumber",
        partNumber, "SerialNumber", serialNumber, "Manufacturer", manufacturer,
        "Model", model, "SubModel", subModel);

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

    if (subModel != nullptr)
    {
        asyncResp->res.jsonValue["SubModel"] = *subModel;
    }

    // Grab the bios version
    sw_util::populateSoftwareInformation(asyncResp, sw_util::biosPurpose,
                                         "BiosVersion", false);
}

inline void
    afterGetAssetTag(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     const boost::system::error_code& ec,
                     const std::string& value)
{
    if (ec)
    {
        // doesn't have to include this
        // interface
        return;
    }

    asyncResp->res.jsonValue["AssetTag"] = value;
}

inline void afterSystemGetSubTree(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("DBUS response error {}", ec);
        messages::internalError(asyncResp->res);
        return;
    }
    // Iterate over all retrieved ObjectPaths.
    for (const std::pair<
             std::string,
             std::vector<std::pair<std::string, std::vector<std::string>>>>&
             object : subtree)
    {
        const std::string& path = object.first;
        BMCWEB_LOG_DEBUG("Got path: {}", path);
        const std::vector<std::pair<std::string, std::vector<std::string>>>&
            connectionNames = object.second;
        if (connectionNames.empty())
        {
            continue;
        }

        // This is not system, so check if it's cpu, dimm, UUID or
        // BiosVer
        for (const auto& connection : connectionNames)
        {
            for (const auto& interfaceName : connection.second)
            {
                if (interfaceName == "xyz.openbmc_project.Inventory.Item.Dimm")
                {
                    BMCWEB_LOG_DEBUG("Found Dimm, now get its properties.");

                    getMemorySummary(asyncResp, connection.first, path);
                }
                else if (interfaceName ==
                         "xyz.openbmc_project.Inventory.Item.Cpu")
                {
                    BMCWEB_LOG_DEBUG("Found Cpu, now get its properties.");

                    getProcessorSummary(asyncResp, connection.first, path);
                }
                else if (interfaceName == "xyz.openbmc_project.Common.UUID")
                {
                    BMCWEB_LOG_DEBUG("Found UUID, now get its properties.");

                    sdbusplus::asio::getAllProperties(
                        *crow::connections::systemBus, connection.first, path,
                        "xyz.openbmc_project.Common.UUID",
                        [asyncResp](const boost::system::error_code& ec3,
                                    const dbus::utility::DBusPropertiesMap&
                                        properties) {
                        afterGetUUID(asyncResp, ec3, properties);
                    });
                }
                else if (interfaceName ==
                         "xyz.openbmc_project.Inventory.Item.System")
                {
                    sdbusplus::asio::getAllProperties(
                        *crow::connections::systemBus, connection.first, path,
                        "xyz.openbmc_project.Inventory.Decorator.Asset",
                        [asyncResp](const boost::system::error_code& ec3,
                                    const dbus::utility::DBusPropertiesMap&
                                        properties) {
                        afterGetInventory(asyncResp, ec3, properties);
                    });

                    sdbusplus::asio::getProperty<std::string>(
                        *crow::connections::systemBus, connection.first, path,
                        "xyz.openbmc_project.Inventory.Decorator."
                        "AssetTag",
                        "AssetTag",
                        std::bind_front(afterGetAssetTag, asyncResp));
                }
            }
        }
    }
}

/*
 * @brief Retrieves computer system properties over dbus
 *
 * @param[in] asyncResp Shared pointer for completing asynchronous calls
 *
 * @return None.
 */
inline void
    getComputerSystem(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    BMCWEB_LOG_DEBUG("Get available system components.");
    constexpr std::array<std::string_view, 5> interfaces = {
        "xyz.openbmc_project.Inventory.Decorator.Asset",
        "xyz.openbmc_project.Inventory.Item.Cpu",
        "xyz.openbmc_project.Inventory.Item.Dimm",
        "xyz.openbmc_project.Inventory.Item.System",
        "xyz.openbmc_project.Common.UUID",
    };
    dbus::utility::getSubTree(
        "/xyz/openbmc_project/inventory", 0, interfaces,
        std::bind_front(afterSystemGetSubTree, asyncResp));
}

/**
 * @brief Retrieves host state properties over dbus
 *
 * @param[in] asyncResp     Shared pointer for completing asynchronous calls.
 *
 * @return None.
 */
inline void getHostState(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    BMCWEB_LOG_DEBUG("Get host information.");
    sdbusplus::asio::getProperty<std::string>(
        *crow::connections::systemBus, "xyz.openbmc_project.State.Host",
        "/xyz/openbmc_project/state/host0", "xyz.openbmc_project.State.Host",
        "CurrentHostState",
        [asyncResp](const boost::system::error_code& ec,
                    const std::string& hostState) {
        if (ec)
        {
            if (ec == boost::system::errc::host_unreachable)
            {
                // Service not available, no error, just don't return
                // host state info
                BMCWEB_LOG_DEBUG("Service not available {}", ec);
                return;
            }
            BMCWEB_LOG_ERROR("DBUS response error {}", ec);
            messages::internalError(asyncResp->res);
            return;
        }

        BMCWEB_LOG_DEBUG("Host state: {}", hostState);
        // Verify Host State
        if (hostState == "xyz.openbmc_project.State.Host.HostState.Running")
        {
            asyncResp->res.jsonValue["PowerState"] = "On";
            asyncResp->res.jsonValue["Status"]["State"] = "Enabled";
        }
        else if (hostState ==
                 "xyz.openbmc_project.State.Host.HostState.Quiesced")
        {
            asyncResp->res.jsonValue["PowerState"] = "On";
            asyncResp->res.jsonValue["Status"]["State"] = "Quiesced";
        }
        else if (hostState ==
                 "xyz.openbmc_project.State.Host.HostState.DiagnosticMode")
        {
            asyncResp->res.jsonValue["PowerState"] = "On";
            asyncResp->res.jsonValue["Status"]["State"] = "InTest";
        }
        else if (
            hostState ==
            "xyz.openbmc_project.State.Host.HostState.TransitioningToRunning")
        {
            asyncResp->res.jsonValue["PowerState"] = "PoweringOn";
            asyncResp->res.jsonValue["Status"]["State"] = "Starting";
        }
        else if (hostState ==
                 "xyz.openbmc_project.State.Host.HostState.TransitioningToOff")
        {
            asyncResp->res.jsonValue["PowerState"] = "PoweringOff";
            asyncResp->res.jsonValue["Status"]["State"] = "Disabled";
        }
        else
        {
            asyncResp->res.jsonValue["PowerState"] = "Off";
            asyncResp->res.jsonValue["Status"]["State"] = "Disabled";
        }
    });
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
 * @brief Translates boot progress DBUS property value to redfish.
 *
 * @param[in] dbusBootProgress    The boot progress in DBUS speak.
 *
 * @return Returns as a string, the boot progress in Redfish terms. If
 *         translation cannot be done, returns "None".
 */
inline std::string dbusToRfBootProgress(const std::string& dbusBootProgress)
{
    // Now convert the D-Bus BootProgress to the appropriate Redfish
    // enum
    std::string rfBpLastState = "None";
    if (dbusBootProgress == "xyz.openbmc_project.State.Boot.Progress."
                            "ProgressStages.Unspecified")
    {
        rfBpLastState = "None";
    }
    else if (dbusBootProgress ==
             "xyz.openbmc_project.State.Boot.Progress.ProgressStages."
             "PrimaryProcInit")
    {
        rfBpLastState = "PrimaryProcessorInitializationStarted";
    }
    else if (dbusBootProgress ==
             "xyz.openbmc_project.State.Boot.Progress.ProgressStages."
             "BusInit")
    {
        rfBpLastState = "BusInitializationStarted";
    }
    else if (dbusBootProgress ==
             "xyz.openbmc_project.State.Boot.Progress.ProgressStages."
             "MemoryInit")
    {
        rfBpLastState = "MemoryInitializationStarted";
    }
    else if (dbusBootProgress ==
             "xyz.openbmc_project.State.Boot.Progress.ProgressStages."
             "SecondaryProcInit")
    {
        rfBpLastState = "SecondaryProcessorInitializationStarted";
    }
    else if (dbusBootProgress ==
             "xyz.openbmc_project.State.Boot.Progress.ProgressStages."
             "PCIInit")
    {
        rfBpLastState = "PCIResourceConfigStarted";
    }
    else if (dbusBootProgress ==
             "xyz.openbmc_project.State.Boot.Progress.ProgressStages."
             "SystemSetup")
    {
        rfBpLastState = "SetupEntered";
    }
    else if (dbusBootProgress ==
             "xyz.openbmc_project.State.Boot.Progress.ProgressStages."
             "SystemInitComplete")
    {
        rfBpLastState = "SystemHardwareInitializationComplete";
    }
    else if (dbusBootProgress ==
             "xyz.openbmc_project.State.Boot.Progress.ProgressStages."
             "OSStart")
    {
        rfBpLastState = "OSBootStarted";
    }
    else if (dbusBootProgress ==
             "xyz.openbmc_project.State.Boot.Progress.ProgressStages."
             "OSRunning")
    {
        rfBpLastState = "OSRunning";
    }
    else
    {
        BMCWEB_LOG_DEBUG("Unsupported D-Bus BootProgress {}", dbusBootProgress);
        // Just return the default
    }
    return rfBpLastState;
}

/**
 * @brief Retrieves boot progress of the system
 *
 * @param[in] asyncResp  Shared pointer for generating response message.
 *
 * @return None.
 */
inline void getBootProgress(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    sdbusplus::asio::getProperty<std::string>(
        *crow::connections::systemBus, "xyz.openbmc_project.State.Host",
        "/xyz/openbmc_project/state/host0",
        "xyz.openbmc_project.State.Boot.Progress", "BootProgress",
        [asyncResp](const boost::system::error_code& ec,
                    const std::string& bootProgressStr) {
        if (ec)
        {
            // BootProgress is an optional object so just do nothing if
            // not found
            return;
        }

        BMCWEB_LOG_DEBUG("Boot Progress: {}", bootProgressStr);

        asyncResp->res.jsonValue["BootProgress"]["LastState"] =
            dbusToRfBootProgress(bootProgressStr);
    });
}

/**
 * @brief Retrieves boot progress Last Update of the system
 *
 * @param[in] asyncResp  Shared pointer for generating response message.
 *
 * @return None.
 */
inline void getBootProgressLastStateTime(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    sdbusplus::asio::getProperty<uint64_t>(
        *crow::connections::systemBus, "xyz.openbmc_project.State.Host",
        "/xyz/openbmc_project/state/host0",
        "xyz.openbmc_project.State.Boot.Progress", "BootProgressLastUpdate",
        [asyncResp](const boost::system::error_code& ec,
                    const uint64_t lastStateTime) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG("D-BUS response error {}", ec);
            return;
        }

        // BootProgressLastUpdate is the last time the BootProgress property
        // was updated. The time is the Epoch time, number of microseconds
        // since 1 Jan 1970 00::00::00 UTC."
        // https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/
        // yaml/xyz/openbmc_project/State/Boot/Progress.interface.yaml#L11

        // Convert to ISO 8601 standard
        asyncResp->res.jsonValue["BootProgress"]["LastStateTime"] =
            redfish::time_utils::getDateTimeUintUs(lastStateTime);
    });
}

/**
 * @brief Retrieves the Last Reset Time
 *
 * "Reset" is an overloaded term in Redfish, "Reset" includes power on
 * and power off. Even though this is the "system" Redfish object look at the
 * chassis D-Bus interface for the LastStateChangeTime since this has the
 * last power operation time.
 *
 * @param[in] asyncResp     Shared pointer for generating response message.
 *
 * @return None.
 */
inline void
    getLastResetTime(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    BMCWEB_LOG_DEBUG("Getting System Last Reset Time");

    sdbusplus::asio::getProperty<uint64_t>(
        *crow::connections::systemBus, "xyz.openbmc_project.State.Chassis",
        "/xyz/openbmc_project/state/chassis0",
        "xyz.openbmc_project.State.Chassis", "LastStateChangeTime",
        [asyncResp](const boost::system::error_code& ec,
                    uint64_t lastResetTime) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG("D-BUS response error {}", ec);
            return;
        }

        // LastStateChangeTime is epoch time, in milliseconds
        // https://github.com/openbmc/phosphor-dbus-interfaces/blob/33e8e1dd64da53a66e888d33dc82001305cd0bf9/xyz/openbmc_project/State/Chassis.interface.yaml#L19
        uint64_t lastResetTimeStamp = lastResetTime / 1000;

        // Convert to ISO 8601 standard
        asyncResp->res.jsonValue["LastResetTime"] =
            redfish::time_utils::getDateTimeUint(lastResetTimeStamp);
    });
}

/**
 * @brief Retrieves the number of automatic boot Retry attempts allowed/left.
 *
 * The total number of automatic reboot retries allowed "RetryAttempts" and its
 * corresponding property "AttemptsLeft" that keeps track of the amount of
 * automatic retry attempts left are hosted in phosphor-state-manager through
 * dbus.
 *
 * @param[in] asyncResp     Shared pointer for generating response message.
 *
 * @return None.
 */
inline void getAutomaticRebootAttempts(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    BMCWEB_LOG_DEBUG("Get Automatic Retry policy");

    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, "xyz.openbmc_project.State.Host",
        "/xyz/openbmc_project/state/host0",
        "xyz.openbmc_project.Control.Boot.RebootAttempts",
        [asyncResp{asyncResp}](
            const boost::system::error_code& ec,
            const dbus::utility::DBusPropertiesMap& propertiesList) {
        if (ec)
        {
            if (ec.value() != EBADR)
            {
                BMCWEB_LOG_ERROR("D-Bus responses error: {}", ec);
                messages::internalError(asyncResp->res);
            }
            return;
        }

        const uint32_t* attemptsLeft = nullptr;
        const uint32_t* retryAttempts = nullptr;

        const bool success = sdbusplus::unpackPropertiesNoThrow(
            dbus_utils::UnpackErrorPrinter(), propertiesList, "AttemptsLeft",
            attemptsLeft, "RetryAttempts", retryAttempts);

        if (!success)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        if (attemptsLeft != nullptr)
        {
            asyncResp->res
                .jsonValue["Boot"]["RemainingAutomaticRetryAttempts"] =
                *attemptsLeft;
        }

        if (retryAttempts != nullptr)
        {
            asyncResp->res.jsonValue["Boot"]["AutomaticRetryAttempts"] =
                *retryAttempts;
        }
    });
}

/**
 * @brief Retrieves Automatic Retry properties. Known on D-Bus as AutoReboot.
 *
 * @param[in] asyncResp     Shared pointer for generating response message.
 *
 * @return None.
 */
inline void
    getAutomaticRetryPolicy(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    BMCWEB_LOG_DEBUG("Get Automatic Retry policy");

    sdbusplus::asio::getProperty<bool>(
        *crow::connections::systemBus, "xyz.openbmc_project.Settings",
        "/xyz/openbmc_project/control/host0/auto_reboot",
        "xyz.openbmc_project.Control.Boot.RebootPolicy", "AutoReboot",
        [asyncResp](const boost::system::error_code& ec,
                    bool autoRebootEnabled) {
        if (ec)
        {
            if (ec.value() != EBADR)
            {
                BMCWEB_LOG_ERROR("D-Bus responses error: {}", ec);
                messages::internalError(asyncResp->res);
            }
            return;
        }

        BMCWEB_LOG_DEBUG("Auto Reboot: {}", autoRebootEnabled);
        if (autoRebootEnabled)
        {
            asyncResp->res.jsonValue["Boot"]["AutomaticRetryConfig"] =
                "RetryAttempts";
        }
        else
        {
            asyncResp->res.jsonValue["Boot"]["AutomaticRetryConfig"] =
                "Disabled";
        }
        getAutomaticRebootAttempts(asyncResp);

        // "AutomaticRetryConfig" can be 3 values, Disabled, RetryAlways,
        // and RetryAttempts. OpenBMC only supports Disabled and
        // RetryAttempts.
        asyncResp->res
            .jsonValue["Boot"]["AutomaticRetryConfig@Redfish.AllowableValues"] =
            {"Disabled", "RetryAttempts"};
    });
}

/**
 * @brief Sets RetryAttempts
 *
 * @param[in] asyncResp   Shared pointer for generating response message.
 * @param[in] retryAttempts  "AutomaticRetryAttempts" from request.
 *
 *@return None.
 */

inline void setAutomaticRetryAttempts(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const uint32_t retryAttempts)
{
    BMCWEB_LOG_DEBUG("Set Automatic Retry Attempts.");
    setDbusProperty(
        asyncResp, "xyz.openbmc_project.State.Host",
        sdbusplus::message::object_path("/xyz/openbmc_project/state/host0"),
        "xyz.openbmc_project.Control.Boot.RebootAttempts", "RetryAttempts",
        "Boot/AutomaticRetryAttempts", retryAttempts);
}

inline computer_system::PowerRestorePolicyTypes
    redfishPowerRestorePolicyFromDbus(std::string_view value)
{
    if (value ==
        "xyz.openbmc_project.Control.Power.RestorePolicy.Policy.AlwaysOn")
    {
        return computer_system::PowerRestorePolicyTypes::AlwaysOn;
    }
    if (value ==
        "xyz.openbmc_project.Control.Power.RestorePolicy.Policy.AlwaysOff")
    {
        return computer_system::PowerRestorePolicyTypes::AlwaysOff;
    }
    if (value ==
        "xyz.openbmc_project.Control.Power.RestorePolicy.Policy.Restore")
    {
        return computer_system::PowerRestorePolicyTypes::LastState;
    }
    if (value == "xyz.openbmc_project.Control.Power.RestorePolicy.Policy.None")
    {
        return computer_system::PowerRestorePolicyTypes::AlwaysOff;
    }
    return computer_system::PowerRestorePolicyTypes::Invalid;
}
/**
 * @brief Retrieves power restore policy over DBUS.
 *
 * @param[in] asyncResp     Shared pointer for generating response message.
 *
 * @return None.
 */
inline void
    getPowerRestorePolicy(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    BMCWEB_LOG_DEBUG("Get power restore policy");

    sdbusplus::asio::getProperty<std::string>(
        *crow::connections::systemBus, "xyz.openbmc_project.Settings",
        "/xyz/openbmc_project/control/host0/power_restore_policy",
        "xyz.openbmc_project.Control.Power.RestorePolicy", "PowerRestorePolicy",
        [asyncResp](const boost::system::error_code& ec,
                    const std::string& policy) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG("DBUS response error {}", ec);
            return;
        }
        computer_system::PowerRestorePolicyTypes restore =
            redfishPowerRestorePolicyFromDbus(policy);
        if (restore == computer_system::PowerRestorePolicyTypes::Invalid)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        asyncResp->res.jsonValue["PowerRestorePolicy"] = restore;
    });
}

/**
 * @brief Stop Boot On Fault over DBUS.
 *
 * @param[in] asyncResp     Shared pointer for generating response message.
 *
 * @return None.
 */
inline void
    getStopBootOnFault(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    BMCWEB_LOG_DEBUG("Get Stop Boot On Fault");

    sdbusplus::asio::getProperty<bool>(
        *crow::connections::systemBus, "xyz.openbmc_project.Settings",
        "/xyz/openbmc_project/logging/settings",
        "xyz.openbmc_project.Logging.Settings", "QuiesceOnHwError",
        [asyncResp](const boost::system::error_code& ec, bool value) {
        if (ec)
        {
            if (ec.value() != EBADR)
            {
                BMCWEB_LOG_ERROR("DBUS response error {}", ec);
                messages::internalError(asyncResp->res);
            }
            return;
        }

        if (value)
        {
            asyncResp->res.jsonValue["Boot"]["StopBootOnFault"] = "AnyFault";
        }
        else
        {
            asyncResp->res.jsonValue["Boot"]["StopBootOnFault"] = "Never";
        }
    });
}

/**
 * @brief Get TrustedModuleRequiredToBoot property. Determines whether or not
 * TPM is required for booting the host.
 *
 * @param[in] asyncResp     Shared pointer for generating response message.
 *
 * @return None.
 */
inline void getTrustedModuleRequiredToBoot(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    BMCWEB_LOG_DEBUG("Get TPM required to boot.");
    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Control.TPM.Policy"};
    dbus::utility::getSubTree(
        "/", 0, interfaces,
        [asyncResp](const boost::system::error_code& ec,
                    const dbus::utility::MapperGetSubTreeResponse& subtree) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG("DBUS response error on TPM.Policy GetSubTree{}",
                             ec);
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

        /* When there is more than one TPMEnable object... */
        if (subtree.size() > 1)
        {
            BMCWEB_LOG_DEBUG(
                "DBUS response has more than 1 TPM Enable object:{}",
                subtree.size());
            // Throw an internal Error and return
            messages::internalError(asyncResp->res);
            return;
        }

        // Make sure the Dbus response map has a service and objectPath
        // field
        if (subtree[0].first.empty() || subtree[0].second.size() != 1)
        {
            BMCWEB_LOG_DEBUG("TPM.Policy mapper error!");
            messages::internalError(asyncResp->res);
            return;
        }

        const std::string& path = subtree[0].first;
        const std::string& serv = subtree[0].second.begin()->first;

        // Valid TPM Enable object found, now reading the current value
        sdbusplus::asio::getProperty<bool>(
            *crow::connections::systemBus, serv, path,
            "xyz.openbmc_project.Control.TPM.Policy", "TPMEnable",
            [asyncResp](const boost::system::error_code& ec2,
                        bool tpmRequired) {
            if (ec2)
            {
                BMCWEB_LOG_ERROR("D-BUS response error on TPM.Policy Get{}",
                                 ec2);
                messages::internalError(asyncResp->res);
                return;
            }

            if (tpmRequired)
            {
                asyncResp->res
                    .jsonValue["Boot"]["TrustedModuleRequiredToBoot"] =
                    "Required";
            }
            else
            {
                asyncResp->res
                    .jsonValue["Boot"]["TrustedModuleRequiredToBoot"] =
                    "Disabled";
            }
        });
    });
}

/**
 * @brief Set TrustedModuleRequiredToBoot property. Determines whether or not
 * TPM is required for booting the host.
 *
 * @param[in] asyncResp     Shared pointer for generating response message.
 * @param[in] tpmRequired   Value to set TPM Required To Boot property to.
 *
 * @return None.
 */
inline void setTrustedModuleRequiredToBoot(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp, const bool tpmRequired)
{
    BMCWEB_LOG_DEBUG("Set TrustedModuleRequiredToBoot.");
    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Control.TPM.Policy"};
    dbus::utility::getSubTree(
        "/", 0, interfaces,
        [asyncResp,
         tpmRequired](const boost::system::error_code& ec,
                      const dbus::utility::MapperGetSubTreeResponse& subtree) {
        if (ec)
        {
            BMCWEB_LOG_ERROR("DBUS response error on TPM.Policy GetSubTree{}",
                             ec);
            messages::internalError(asyncResp->res);
            return;
        }
        if (subtree.empty())
        {
            messages::propertyValueNotInList(asyncResp->res, "ComputerSystem",
                                             "TrustedModuleRequiredToBoot");
            return;
        }

        /* When there is more than one TPMEnable object... */
        if (subtree.size() > 1)
        {
            BMCWEB_LOG_DEBUG(
                "DBUS response has more than 1 TPM Enable object:{}",
                subtree.size());
            // Throw an internal Error and return
            messages::internalError(asyncResp->res);
            return;
        }

        // Make sure the Dbus response map has a service and objectPath
        // field
        if (subtree[0].first.empty() || subtree[0].second.size() != 1)
        {
            BMCWEB_LOG_DEBUG("TPM.Policy mapper error!");
            messages::internalError(asyncResp->res);
            return;
        }

        const std::string& path = subtree[0].first;
        const std::string& serv = subtree[0].second.begin()->first;

        if (serv.empty())
        {
            BMCWEB_LOG_DEBUG("TPM.Policy service mapper error!");
            messages::internalError(asyncResp->res);
            return;
        }

        // Valid TPM Enable object found, now setting the value
        setDbusProperty(asyncResp, serv, path,
                        "xyz.openbmc_project.Control.TPM.Policy", "TPMEnable",
                        "Boot/TrustedModuleRequiredToBoot", tpmRequired);
    });
}

/**
 * @brief Sets AssetTag
 *
 * @param[in] asyncResp Shared pointer for generating response message.
 * @param[in] assetTag  "AssetTag" from request.
 *
 * @return None.
 */
inline void setAssetTag(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& assetTag)
{
    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Inventory.Item.System"};
    dbus::utility::getSubTree(
        "/xyz/openbmc_project/inventory", 0, interfaces,
        [asyncResp,
         assetTag](const boost::system::error_code& ec,
                   const dbus::utility::MapperGetSubTreeResponse& subtree) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG("D-Bus response error on GetSubTree {}", ec);
            messages::internalError(asyncResp->res);
            return;
        }
        if (subtree.empty())
        {
            BMCWEB_LOG_DEBUG("Can't find system D-Bus object!");
            messages::internalError(asyncResp->res);
            return;
        }
        // Assume only 1 system D-Bus object
        // Throw an error if there is more than 1
        if (subtree.size() > 1)
        {
            BMCWEB_LOG_DEBUG("Found more than 1 system D-Bus object!");
            messages::internalError(asyncResp->res);
            return;
        }
        if (subtree[0].first.empty() || subtree[0].second.size() != 1)
        {
            BMCWEB_LOG_DEBUG("Asset Tag Set mapper error!");
            messages::internalError(asyncResp->res);
            return;
        }

        const std::string& path = subtree[0].first;
        const std::string& service = subtree[0].second.begin()->first;

        if (service.empty())
        {
            BMCWEB_LOG_DEBUG("Asset Tag Set service mapper error!");
            messages::internalError(asyncResp->res);
            return;
        }

        setDbusProperty(asyncResp, service, path,
                        "xyz.openbmc_project.Inventory.Decorator.AssetTag",
                        "AssetTag", "AssetTag", assetTag);
    });
}

/**
 * @brief Validate the specified stopBootOnFault is valid and return the
 * stopBootOnFault name associated with that string
 *
 * @param[in] stopBootOnFaultString  String representing the desired
 * stopBootOnFault
 *
 * @return stopBootOnFault value or empty  if incoming value is not valid
 */
inline std::optional<bool>
    validstopBootOnFault(const std::string& stopBootOnFaultString)
{
    if (stopBootOnFaultString == "AnyFault")
    {
        return true;
    }

    if (stopBootOnFaultString == "Never")
    {
        return false;
    }

    return std::nullopt;
}

/**
 * @brief Sets stopBootOnFault
 *
 * @param[in] asyncResp   Shared pointer for generating response message.
 * @param[in] stopBootOnFault  "StopBootOnFault" from request.
 *
 * @return None.
 */
inline void
    setStopBootOnFault(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& stopBootOnFault)
{
    BMCWEB_LOG_DEBUG("Set Stop Boot On Fault.");

    std::optional<bool> stopBootEnabled = validstopBootOnFault(stopBootOnFault);
    if (!stopBootEnabled)
    {
        BMCWEB_LOG_DEBUG("Invalid property value for StopBootOnFault: {}",
                         stopBootOnFault);
        messages::propertyValueNotInList(asyncResp->res, stopBootOnFault,
                                         "StopBootOnFault");
        return;
    }

    setDbusProperty(asyncResp, "xyz.openbmc_project.Settings",
                    sdbusplus::message::object_path(
                        "/xyz/openbmc_project/logging/settings"),
                    "xyz.openbmc_project.Logging.Settings", "QuiesceOnHwError",
                    "Boot/StopBootOnFault", *stopBootEnabled);
}

/**
 * @brief Sets automaticRetry (Auto Reboot)
 *
 * @param[in] asyncResp   Shared pointer for generating response message.
 * @param[in] automaticRetryConfig  "AutomaticRetryConfig" from request.
 *
 * @return None.
 */
inline void
    setAutomaticRetry(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const std::string& automaticRetryConfig)
{
    BMCWEB_LOG_DEBUG("Set Automatic Retry.");

    // OpenBMC only supports "Disabled" and "RetryAttempts".
    bool autoRebootEnabled = false;

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
        BMCWEB_LOG_DEBUG("Invalid property value for AutomaticRetryConfig: {}",
                         automaticRetryConfig);
        messages::propertyValueNotInList(asyncResp->res, automaticRetryConfig,
                                         "AutomaticRetryConfig");
        return;
    }

    setDbusProperty(asyncResp, "xyz.openbmc_project.Settings",
                    sdbusplus::message::object_path(
                        "/xyz/openbmc_project/control/host0/auto_reboot"),
                    "xyz.openbmc_project.Control.Boot.RebootPolicy",
                    "AutoReboot", "Boot/AutomaticRetryConfig",
                    autoRebootEnabled);
}

inline std::string dbusPowerRestorePolicyFromRedfish(std::string_view policy)
{
    if (policy == "AlwaysOn")
    {
        return "xyz.openbmc_project.Control.Power.RestorePolicy.Policy.AlwaysOn";
    }
    if (policy == "AlwaysOff")
    {
        return "xyz.openbmc_project.Control.Power.RestorePolicy.Policy.AlwaysOff";
    }
    if (policy == "LastState")
    {
        return "xyz.openbmc_project.Control.Power.RestorePolicy.Policy.Restore";
    }
    return "";
}

/**
 * @brief Sets power restore policy properties.
 *
 * @param[in] asyncResp   Shared pointer for generating response message.
 * @param[in] policy  power restore policy properties from request.
 *
 * @return None.
 */
inline void
    setPowerRestorePolicy(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          std::string_view policy)
{
    BMCWEB_LOG_DEBUG("Set power restore policy.");

    std::string powerRestorePolicy = dbusPowerRestorePolicyFromRedfish(policy);

    if (powerRestorePolicy.empty())
    {
        messages::propertyValueNotInList(asyncResp->res, policy,
                                         "PowerRestorePolicy");
        return;
    }

    setDbusProperty(
        asyncResp, "xyz.openbmc_project.Settings",
        sdbusplus::message::object_path(
            "/xyz/openbmc_project/control/host0/power_restore_policy"),
        "xyz.openbmc_project.Control.Power.RestorePolicy", "PowerRestorePolicy",
        "PowerRestorePolicy", powerRestorePolicy);
}

/**
 * @brief Retrieves provisioning status
 *
 * @param[in] asyncResp     Shared pointer for completing asynchronous
 * calls.
 *
 * @return None.
 */
inline void
    getProvisioningStatus(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    BMCWEB_LOG_DEBUG("Get OEM information.");
    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, "xyz.openbmc_project.PFR.Manager",
        "/xyz/openbmc_project/pfr", "xyz.openbmc_project.PFR.Attributes",
        [asyncResp](const boost::system::error_code& ec,
                    const dbus::utility::DBusPropertiesMap& propertiesList) {
        nlohmann::json& oemPFR =
            asyncResp->res.jsonValue["Oem"]["OpenBmc"]["FirmwareProvisioning"];
        asyncResp->res.jsonValue["Oem"]["OpenBmc"]["@odata.type"] =
            "#OemComputerSystem.OpenBmc";
        oemPFR["@odata.type"] = "#OemComputerSystem.FirmwareProvisioning";

        if (ec)
        {
            BMCWEB_LOG_DEBUG("DBUS response error {}", ec);
            // not an error, don't have to have the interface
            oemPFR["ProvisioningStatus"] = "NotProvisioned";
            return;
        }

        const bool* provState = nullptr;
        const bool* lockState = nullptr;

        const bool success = sdbusplus::unpackPropertiesNoThrow(
            dbus_utils::UnpackErrorPrinter(), propertiesList, "UfmProvisioned",
            provState, "UfmLocked", lockState);

        if (!success)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        if ((provState == nullptr) || (lockState == nullptr))
        {
            BMCWEB_LOG_DEBUG("Unable to get PFR attributes.");
            messages::internalError(asyncResp->res);
            return;
        }

        if (*provState)
        {
            if (*lockState)
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
    });
}

/**
 * @brief Translate the PowerMode string to enum value
 *
 * @param[in]  modeString PowerMode string to be translated
 *
 * @return PowerMode enum
 */
inline computer_system::PowerMode
    translatePowerModeString(const std::string& modeString)
{
    using PowerMode = computer_system::PowerMode;

    if (modeString == "xyz.openbmc_project.Control.Power.Mode.PowerMode.Static")
    {
        return PowerMode::Static;
    }
    if (modeString ==
        "xyz.openbmc_project.Control.Power.Mode.PowerMode.MaximumPerformance")
    {
        return PowerMode::MaximumPerformance;
    }
    if (modeString ==
        "xyz.openbmc_project.Control.Power.Mode.PowerMode.PowerSaving")
    {
        return PowerMode::PowerSaving;
    }
    if (modeString ==
        "xyz.openbmc_project.Control.Power.Mode.PowerMode.BalancedPerformance")
    {
        return PowerMode::BalancedPerformance;
    }
    if (modeString ==
        "xyz.openbmc_project.Control.Power.Mode.PowerMode.EfficiencyFavorPerformance")
    {
        return PowerMode::EfficiencyFavorPerformance;
    }
    if (modeString ==
        "xyz.openbmc_project.Control.Power.Mode.PowerMode.EfficiencyFavorPower")
    {
        return PowerMode::EfficiencyFavorPower;
    }
    if (modeString == "xyz.openbmc_project.Control.Power.Mode.PowerMode.OEM")
    {
        return PowerMode::OEM;
    }
    // Any other values would be invalid
    BMCWEB_LOG_ERROR("PowerMode value was not valid: {}", modeString);
    return PowerMode::Invalid;
}

inline void
    afterGetPowerMode(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const boost::system::error_code& ec,
                      const dbus::utility::DBusPropertiesMap& properties)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("DBUS response error on PowerMode GetAll: {}", ec);
        messages::internalError(asyncResp->res);
        return;
    }

    std::string powerMode;
    const std::vector<std::string>* allowedModes = nullptr;
    std::optional<bool> safeMode;
    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), properties, "PowerMode", powerMode,
        "AllowedPowerModes", allowedModes, "SafeMode", safeMode);

    if (!success)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    nlohmann::json::array_t modeList;
    if (allowedModes == nullptr)
    {
        modeList.emplace_back("Static");
        modeList.emplace_back("MaximumPerformance");
        modeList.emplace_back("PowerSaving");
    }
    else
    {
        for (const auto& aMode : *allowedModes)
        {
            computer_system::PowerMode modeValue =
                translatePowerModeString(aMode);
            if (modeValue == computer_system::PowerMode::Invalid)
            {
                messages::internalError(asyncResp->res);
                continue;
            }
            modeList.emplace_back(modeValue);
        }
    }
    asyncResp->res.jsonValue["PowerMode@Redfish.AllowableValues"] = modeList;

    BMCWEB_LOG_DEBUG("Current power mode: {}", powerMode);
    const computer_system::PowerMode modeValue =
        translatePowerModeString(powerMode);
    if (modeValue == computer_system::PowerMode::Invalid)
    {
        messages::internalError(asyncResp->res);
        return;
    }
    asyncResp->res.jsonValue["PowerMode"] = modeValue;

    if (safeMode)
    {
        BMCWEB_LOG_DEBUG("Safe mode: {}", *safeMode);
        nlohmann::json& oemSafeMode = asyncResp->res.jsonValue["Oem"];
        oemSafeMode["@odata.type"] = "#OemComputerSystem.Oem";
        oemSafeMode["IBM"]["@odata.type"] = "#OemComputerSystem.v1_0_0.IBM";
        oemSafeMode["IBM"]["SafeMode"] = *safeMode;
    }
}

/**
 * @brief Retrieves system power mode
 *
 * @param[in] asyncResp  Shared pointer for generating response message.
 *
 * @return None.
 */
inline void getPowerMode(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    BMCWEB_LOG_DEBUG("Get power mode.");

    // Get Power Mode object path:
    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Control.Power.Mode"};
    dbus::utility::getSubTree(
        "/", 0, interfaces,
        [asyncResp](const boost::system::error_code& ec,
                    const dbus::utility::MapperGetSubTreeResponse& subtree) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG("DBUS response error on Power.Mode GetSubTree {}",
                             ec);
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
            BMCWEB_LOG_DEBUG(
                "Found more than 1 system D-Bus Power.Mode objects: {}",
                subtree.size());
            messages::internalError(asyncResp->res);
            return;
        }
        if ((subtree[0].first.empty()) || (subtree[0].second.size() != 1))
        {
            BMCWEB_LOG_DEBUG("Power.Mode mapper error!");
            messages::internalError(asyncResp->res);
            return;
        }
        const std::string& path = subtree[0].first;
        const std::string& service = subtree[0].second.begin()->first;
        if (service.empty())
        {
            BMCWEB_LOG_DEBUG("Power.Mode service mapper error!");
            messages::internalError(asyncResp->res);
            return;
        }

        // Valid Power Mode object found, now read the mode properties
        sdbusplus::asio::getAllProperties(
            *crow::connections::systemBus, service, path,
            "xyz.openbmc_project.Control.Power.Mode",
            [asyncResp](const boost::system::error_code& ec2,
                        const dbus::utility::DBusPropertiesMap& properties) {
            afterGetPowerMode(asyncResp, ec2, properties);
        });
    });
}

/**
 * @brief Validate the specified mode is valid and return the PowerMode
 * name associated with that string
 *
 * @param[in] asyncResp   Shared pointer for generating response message.
 * @param[in] modeValue   String representing the desired PowerMode
 *
 * @return PowerMode value or empty string if mode is not valid
 */
inline std::string
    validatePowerMode(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const nlohmann::json& modeValue)
{
    using PowerMode = computer_system::PowerMode;
    std::string mode;

    if (modeValue == PowerMode::Static)
    {
        mode = "xyz.openbmc_project.Control.Power.Mode.PowerMode.Static";
    }
    else if (modeValue == PowerMode::MaximumPerformance)
    {
        mode =
            "xyz.openbmc_project.Control.Power.Mode.PowerMode.MaximumPerformance";
    }
    else if (modeValue == PowerMode::PowerSaving)
    {
        mode = "xyz.openbmc_project.Control.Power.Mode.PowerMode.PowerSaving";
    }
    else if (modeValue == PowerMode::BalancedPerformance)
    {
        mode =
            "xyz.openbmc_project.Control.Power.Mode.PowerMode.BalancedPerformance";
    }
    else if (modeValue == PowerMode::EfficiencyFavorPerformance)
    {
        mode =
            "xyz.openbmc_project.Control.Power.Mode.PowerMode.EfficiencyFavorPerformance";
    }
    else if (modeValue == PowerMode::EfficiencyFavorPower)
    {
        mode =
            "xyz.openbmc_project.Control.Power.Mode.PowerMode.EfficiencyFavorPower";
    }
    else
    {
        messages::propertyValueNotInList(asyncResp->res, modeValue.dump(),
                                         "PowerMode");
    }
    return mode;
}

/**
 * @brief Sets system power mode.
 *
 * @param[in] asyncResp   Shared pointer for generating response message.
 * @param[in] pmode   System power mode from request.
 *
 * @return None.
 */
inline void setPowerMode(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::string& pmode)
{
    BMCWEB_LOG_DEBUG("Set power mode.");

    std::string powerMode = validatePowerMode(asyncResp, pmode);
    if (powerMode.empty())
    {
        return;
    }

    // Get Power Mode object path:
    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Control.Power.Mode"};
    dbus::utility::getSubTree(
        "/", 0, interfaces,
        [asyncResp,
         powerMode](const boost::system::error_code& ec,
                    const dbus::utility::MapperGetSubTreeResponse& subtree) {
        if (ec)
        {
            BMCWEB_LOG_ERROR("DBUS response error on Power.Mode GetSubTree {}",
                             ec);
            // This is an optional D-Bus object, but user attempted to patch
            messages::internalError(asyncResp->res);
            return;
        }
        if (subtree.empty())
        {
            // This is an optional D-Bus object, but user attempted to patch
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       "PowerMode");
            return;
        }
        if (subtree.size() > 1)
        {
            // More then one PowerMode object is not supported and is an
            // error
            BMCWEB_LOG_DEBUG(
                "Found more than 1 system D-Bus Power.Mode objects: {}",
                subtree.size());
            messages::internalError(asyncResp->res);
            return;
        }
        if ((subtree[0].first.empty()) || (subtree[0].second.size() != 1))
        {
            BMCWEB_LOG_DEBUG("Power.Mode mapper error!");
            messages::internalError(asyncResp->res);
            return;
        }
        const std::string& path = subtree[0].first;
        const std::string& service = subtree[0].second.begin()->first;
        if (service.empty())
        {
            BMCWEB_LOG_DEBUG("Power.Mode service mapper error!");
            messages::internalError(asyncResp->res);
            return;
        }

        BMCWEB_LOG_DEBUG("Setting power mode({}) -> {}", powerMode, path);

        // Set the Power Mode property
        setDbusProperty(asyncResp, service, path,
                        "xyz.openbmc_project.Control.Power.Mode", "PowerMode",
                        "PowerMode", powerMode);
    });
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
 * @param[in] asyncResp     Shared pointer for completing asynchronous calls.
 *
 * @return None.
 */
inline void
    getHostWatchdogTimer(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    BMCWEB_LOG_DEBUG("Get host watchodg");
    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, "xyz.openbmc_project.Watchdog",
        "/xyz/openbmc_project/watchdog/host0",
        "xyz.openbmc_project.State.Watchdog",
        [asyncResp](const boost::system::error_code& ec,
                    const dbus::utility::DBusPropertiesMap& properties) {
        if (ec)
        {
            // watchdog service is stopped
            BMCWEB_LOG_DEBUG("DBUS response error {}", ec);
            return;
        }

        BMCWEB_LOG_DEBUG("Got {} wdt prop.", properties.size());

        nlohmann::json& hostWatchdogTimer =
            asyncResp->res.jsonValue["HostWatchdogTimer"];

        // watchdog service is running/enabled
        hostWatchdogTimer["Status"]["State"] = "Enabled";

        const bool* enabled = nullptr;
        const std::string* expireAction = nullptr;

        const bool success = sdbusplus::unpackPropertiesNoThrow(
            dbus_utils::UnpackErrorPrinter(), properties, "Enabled", enabled,
            "ExpireAction", expireAction);

        if (!success)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        if (enabled != nullptr)
        {
            hostWatchdogTimer["FunctionEnabled"] = *enabled;
        }

        if (expireAction != nullptr)
        {
            std::string action = dbusToRfWatchdogAction(*expireAction);
            if (action.empty())
            {
                messages::internalError(asyncResp->res);
                return;
            }
            hostWatchdogTimer["TimeoutAction"] = action;
        }
    });
}

/**
 * @brief Sets Host WatchDog Timer properties.
 *
 * @param[in] asyncResp  Shared pointer for generating response message.
 * @param[in] wdtEnable  The WDTimer Enable value (true/false) from incoming
 *                       RF request.
 * @param[in] wdtTimeOutAction The WDT Timeout action, from incoming RF request.
 *
 * @return None.
 */
inline void
    setWDTProperties(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     const std::optional<bool> wdtEnable,
                     const std::optional<std::string>& wdtTimeOutAction)
{
    BMCWEB_LOG_DEBUG("Set host watchdog");

    if (wdtTimeOutAction)
    {
        std::string wdtTimeOutActStr = rfToDbusWDTTimeOutAct(*wdtTimeOutAction);
        // check if TimeOut Action is Valid
        if (wdtTimeOutActStr.empty())
        {
            BMCWEB_LOG_DEBUG("Unsupported value for TimeoutAction: {}",
                             *wdtTimeOutAction);
            messages::propertyValueNotInList(asyncResp->res, *wdtTimeOutAction,
                                             "TimeoutAction");
            return;
        }

        setDbusProperty(asyncResp, "xyz.openbmc_project.Watchdog",
                        sdbusplus::message::object_path(
                            "/xyz/openbmc_project/watchdog/host0"),
                        "xyz.openbmc_project.State.Watchdog", "ExpireAction",
                        "HostWatchdogTimer/TimeoutAction", wdtTimeOutActStr);
    }

    if (wdtEnable)
    {
        setDbusProperty(asyncResp, "xyz.openbmc_project.Watchdog",
                        sdbusplus::message::object_path(
                            "/xyz/openbmc_project/watchdog/host0"),
                        "xyz.openbmc_project.State.Watchdog", "Enabled",
                        "HostWatchdogTimer/FunctionEnabled", *wdtEnable);
    }
}

/**
 * @brief Parse the Idle Power Saver properties into json
 *
 * @param[in] asyncResp   Shared pointer for completing asynchronous calls.
 * @param[in] properties  IPS property data from DBus.
 *
 * @return true if successful
 */
inline bool
    parseIpsProperties(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const dbus::utility::DBusPropertiesMap& properties)
{
    const bool* enabled = nullptr;
    const uint8_t* enterUtilizationPercent = nullptr;
    const uint64_t* enterDwellTime = nullptr;
    const uint8_t* exitUtilizationPercent = nullptr;
    const uint64_t* exitDwellTime = nullptr;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), properties, "Enabled", enabled,
        "EnterUtilizationPercent", enterUtilizationPercent, "EnterDwellTime",
        enterDwellTime, "ExitUtilizationPercent", exitUtilizationPercent,
        "ExitDwellTime", exitDwellTime);

    if (!success)
    {
        return false;
    }

    if (enabled != nullptr)
    {
        asyncResp->res.jsonValue["IdlePowerSaver"]["Enabled"] = *enabled;
    }

    if (enterUtilizationPercent != nullptr)
    {
        asyncResp->res.jsonValue["IdlePowerSaver"]["EnterUtilizationPercent"] =
            *enterUtilizationPercent;
    }

    if (enterDwellTime != nullptr)
    {
        const std::chrono::duration<uint64_t, std::milli> ms(*enterDwellTime);
        asyncResp->res.jsonValue["IdlePowerSaver"]["EnterDwellTimeSeconds"] =
            std::chrono::duration_cast<std::chrono::duration<uint64_t>>(ms)
                .count();
    }

    if (exitUtilizationPercent != nullptr)
    {
        asyncResp->res.jsonValue["IdlePowerSaver"]["ExitUtilizationPercent"] =
            *exitUtilizationPercent;
    }

    if (exitDwellTime != nullptr)
    {
        const std::chrono::duration<uint64_t, std::milli> ms(*exitDwellTime);
        asyncResp->res.jsonValue["IdlePowerSaver"]["ExitDwellTimeSeconds"] =
            std::chrono::duration_cast<std::chrono::duration<uint64_t>>(ms)
                .count();
    }

    return true;
}

/**
 * @brief Retrieves host watchdog timer properties over DBUS
 *
 * @param[in] asyncResp     Shared pointer for completing asynchronous calls.
 *
 * @return None.
 */
inline void
    getIdlePowerSaver(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    BMCWEB_LOG_DEBUG("Get idle power saver parameters");

    // Get IdlePowerSaver object path:
    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Control.Power.IdlePowerSaver"};
    dbus::utility::getSubTree(
        "/", 0, interfaces,
        [asyncResp](const boost::system::error_code& ec,
                    const dbus::utility::MapperGetSubTreeResponse& subtree) {
        if (ec)
        {
            BMCWEB_LOG_ERROR(
                "DBUS response error on Power.IdlePowerSaver GetSubTree {}",
                ec);
            messages::internalError(asyncResp->res);
            return;
        }
        if (subtree.empty())
        {
            // This is an optional interface so just return
            // if there is no instance found
            BMCWEB_LOG_DEBUG("No instances found");
            return;
        }
        if (subtree.size() > 1)
        {
            // More then one PowerIdlePowerSaver object is not supported and
            // is an error
            BMCWEB_LOG_DEBUG("Found more than 1 system D-Bus "
                             "Power.IdlePowerSaver objects: {}",
                             subtree.size());
            messages::internalError(asyncResp->res);
            return;
        }
        if ((subtree[0].first.empty()) || (subtree[0].second.size() != 1))
        {
            BMCWEB_LOG_DEBUG("Power.IdlePowerSaver mapper error!");
            messages::internalError(asyncResp->res);
            return;
        }
        const std::string& path = subtree[0].first;
        const std::string& service = subtree[0].second.begin()->first;
        if (service.empty())
        {
            BMCWEB_LOG_DEBUG("Power.IdlePowerSaver service mapper error!");
            messages::internalError(asyncResp->res);
            return;
        }

        // Valid IdlePowerSaver object found, now read the current values
        sdbusplus::asio::getAllProperties(
            *crow::connections::systemBus, service, path,
            "xyz.openbmc_project.Control.Power.IdlePowerSaver",
            [asyncResp](const boost::system::error_code& ec2,
                        const dbus::utility::DBusPropertiesMap& properties) {
            if (ec2)
            {
                BMCWEB_LOG_ERROR(
                    "DBUS response error on IdlePowerSaver GetAll: {}", ec2);
                messages::internalError(asyncResp->res);
                return;
            }

            if (!parseIpsProperties(asyncResp, properties))
            {
                messages::internalError(asyncResp->res);
                return;
            }
        });
    });

    BMCWEB_LOG_DEBUG("EXIT: Get idle power saver parameters");
}

/*
 * Handle Enabled Panel Functions
 */
inline void doGetEnabledPanelFunctions(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    std::function<void(const boost::system::error_code& ec,
                       const std::vector<uint8_t>&)>&& callback)
{
    BMCWEB_LOG_DEBUG("Get Enabled Panel functions");

    crow::connections::systemBus->async_method_call(
        [asyncResp, callback{std::move(callback)}](
            const boost::system::error_code& ec,
            const std::vector<uint8_t>& enabledFuncs) {
        callback(ec, enabledFuncs);
    },
        "com.ibm.PanelApp", "/com/ibm/panel_app", "com.ibm.panel",
        "getEnabledFunctions");
}

/*
 * Get Enabled Panel Functions
 */
inline void getEnabledPanelFunctions(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    doGetEnabledPanelFunctions(
        asyncResp, [asyncResp](const boost::system::error_code& ec,
                               const std::vector<uint8_t>& enabledFuncs) {
        if (ec)
        {
            if (ec.value() != EBADR &&
                ec.value() != boost::asio::error::host_unreachable)
            {
                BMCWEB_LOG_ERROR("Get Enabled Panel Functions D-bus error: {}",
                                 ec.value());
                messages::internalError(asyncResp->res);
            }
            return;
        }
        nlohmann::json& oem = asyncResp->res.jsonValue["Oem"];
        oem["@odata.type"] = "#OemComputerSystem.Oem";
        oem["IBM"]["@odata.type"] = "#OemComputerSystem.v1_0_0.IBM";
        oem["IBM"]["EnabledPanelFunctions"] = enabledFuncs;
    });
}

/**
 * Execute a Panel Enabled Function
 */
inline void
    executePanelFunction(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const uint8_t funcNo)
{
    BMCWEB_LOG_DEBUG("Execute Panel function {}", std::to_string(funcNo));

    crow::connections::systemBus->async_method_call(
        [asyncResp,
         funcNo](const boost::system::error_code& ec,
                 const sdbusplus::message_t& msg,
                 const std::tuple<bool, std::string, std::string>& result) {
        if (ec)
        {
            const sd_bus_error* dbusError = msg.get_error();
            if (dbusError == nullptr)
            {
                BMCWEB_LOG_ERROR("Execute a panel function D-bus error:  {}",
                                 ec.value());
                messages::internalError(asyncResp->res);
                return;
            }
            if (dbusError->name ==
                std::string_view("xyz.openbmc_project.Common.Error.NotAllowed"))
            {
                BMCWEB_LOG_WARNING("PanelFunction {} is not enabled",
                                   std::to_string(funcNo));
                messages::operationNotAllowed(asyncResp->res);
                return;
            }
            if (dbusError->name ==
                std::string_view(
                    "xyz.openbmc_project.Common.Error.InternalFailure"))
            {
                BMCWEB_LOG_ERROR("ExecutePanelFunction {} is failed",
                                 std::to_string(funcNo));
                messages::operationFailed(asyncResp->res);
                return;
            }
            BMCWEB_LOG_ERROR("Execute a panel function D-bus error:  {}",
                             ec.value());
            messages::internalError(asyncResp->res);
            return;
        }

        if (!std::get<0>(result))
        {
            BMCWEB_LOG_ERROR("ExecutePanelFunction {} is failed",
                             std::to_string(funcNo));
            messages::operationFailed(asyncResp->res);
            return;
        }
        asyncResp->res.jsonValue["Result"] = {std::get<1>(result),
                                              std::get<2>(result)};
        messages::success(asyncResp->res);
    },
        "com.ibm.PanelApp", "/com/ibm/panel_app", "com.ibm.panel",
        "ExecuteFunction", funcNo);
}

inline void handleSystemActionsOemExecutePanelFunctionPost(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    BMCWEB_LOG_DEBUG("handleSystemActionsOemExecutePanelFunctionPost...");
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    uint8_t funcNo = 0;
    if (!json_util::readJsonAction(req, asyncResp->res, "FuncNo", funcNo))
    {
        BMCWEB_LOG_WARNING("Missing funcNo");
        messages::actionParameterMissing(asyncResp->res, "ExecutePanelFunction",
                                         "FuncNo");
        return;
    }

    doGetEnabledPanelFunctions(
        asyncResp,
        [funcNo, asyncResp](const boost::system::error_code& ec,
                            const std::vector<uint8_t>& enabledFuncs) {
        if (ec)
        {
            BMCWEB_LOG_ERROR("Get Enabled Panel Functions D-bus error: {}",
                             ec.value());
            messages::internalError(asyncResp->res);
            return;
        }

        auto it = std::ranges::find(enabledFuncs, funcNo);
        if (it == enabledFuncs.end())
        {
            BMCWEB_LOG_WARNING("PanelFunction {} is not enabled",
                               std::to_string(funcNo));
            messages::operationNotAllowed(asyncResp->res);
            return;
        }
        executePanelFunction(asyncResp, funcNo);
    });
}

/**
 * SystemActionsOemExecutePanelFunction class supports handle POST method for
 * ExecutePanelFunction  action. The class retrieves and sends data directly to
 * D-Bus.
 */
inline void requestRoutesSystemActionsOemExecutePanelFunction(App& app)
{
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Systems/system/Actions/Oem/OemComputerSystem.ExecutePanelFunction/")
        .privileges(redfish::privileges::postComputerSystem)
        .methods(boost::beast::http::verb::post)(std::bind_front(
            handleSystemActionsOemExecutePanelFunctionPost, std::ref(app)));
}

/*
 * Get ChapData
 */
inline void getChapData(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    BMCWEB_LOG_DEBUG("Get ChapData");
    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, "xyz.openbmc_project.PLDM",
        "/xyz/openbmc_project/pldm", "com.ibm.PLDM.ChapData",
        [asyncResp](const boost::system::error_code ec,
                    const dbus::utility::DBusPropertiesMap& propertiesList) {
        if (ec)
        {
            // ChapData isn't that important. Ignore all errors.
            BMCWEB_LOG_DEBUG("Get ChapData D-bus error: {}", ec.value());
            return;
        }

        const std::string* chapName = nullptr;
        const std::string* chapSecret = nullptr;
        const bool success = sdbusplus::unpackPropertiesNoThrow(
            dbus_utils::UnpackErrorPrinter(), propertiesList, "ChapName",
            chapName, "ChapSecret", chapSecret);

        if (!success)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        nlohmann::json& oemIBM = asyncResp->res.jsonValue["Oem"]["IBM"];
        oemIBM["@odata.type"] = "#OemComputerSystem.v1_0_0.IBM";

        nlohmann::json& chapData = oemIBM["ChapData"];
        chapData["ChapName"] = *chapName;
        chapData["ChapSecret"] = *chapSecret;
    });
}

/*
 * Set ChapData
 */
inline void setChapData(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        std::optional<std::string> chapName,
                        std::optional<std::string> chapSecret)
{
    BMCWEB_LOG_DEBUG("Set ChapData");
    if (chapName)
    {
        sdbusplus::asio::setProperty(
            *crow::connections::systemBus, "xyz.openbmc_project.PLDM",
            "/xyz/openbmc_project/pldm", "com.ibm.PLDM.ChapData", "ChapName",
            *chapName, [asyncResp](const boost::system::error_code& ec) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("DBUS response error: {}", ec.value());
                messages::internalError(asyncResp->res);
                return;
            }
        });
    }

    if (chapSecret)
    {
        sdbusplus::asio::setProperty(
            *crow::connections::systemBus, "xyz.openbmc_project.PLDM",
            "/xyz/openbmc_project/pldm", "com.ibm.PLDM.ChapData", "ChapSecret",
            *chapSecret, [asyncResp](const boost::system::error_code& ec) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("DBUS response error: {}", ec.value());
                messages::internalError(asyncResp->res);
                return;
            }
        });
    }
}

/**
 * @brief Sets Idle Power Saver properties.
 *
 * @param[in] asyncResp  Shared pointer for generating response message.
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
inline void
    setIdlePowerSaver(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const std::optional<bool> ipsEnable,
                      const std::optional<uint8_t> ipsEnterUtil,
                      const std::optional<uint64_t> ipsEnterTime,
                      const std::optional<uint8_t> ipsExitUtil,
                      const std::optional<uint64_t> ipsExitTime)
{
    BMCWEB_LOG_DEBUG("Set idle power saver properties");

    // Get IdlePowerSaver object path:
    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Control.Power.IdlePowerSaver"};
    dbus::utility::getSubTree(
        "/", 0, interfaces,
        [asyncResp, ipsEnable, ipsEnterUtil, ipsEnterTime, ipsExitUtil,
         ipsExitTime](const boost::system::error_code& ec,
                      const dbus::utility::MapperGetSubTreeResponse& subtree) {
        if (ec)
        {
            BMCWEB_LOG_ERROR(
                "DBUS response error on Power.IdlePowerSaver GetSubTree {}",
                ec);
            messages::internalError(asyncResp->res);
            return;
        }
        if (subtree.empty())
        {
            // This is an optional D-Bus object, but user attempted to patch
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       "IdlePowerSaver");
            return;
        }
        if (subtree.size() > 1)
        {
            // More then one PowerIdlePowerSaver object is not supported and
            // is an error
            BMCWEB_LOG_DEBUG(
                "Found more than 1 system D-Bus Power.IdlePowerSaver objects: {}",
                subtree.size());
            messages::internalError(asyncResp->res);
            return;
        }
        if ((subtree[0].first.empty()) || (subtree[0].second.size() != 1))
        {
            BMCWEB_LOG_DEBUG("Power.IdlePowerSaver mapper error!");
            messages::internalError(asyncResp->res);
            return;
        }
        const std::string& path = subtree[0].first;
        const std::string& service = subtree[0].second.begin()->first;
        if (service.empty())
        {
            BMCWEB_LOG_DEBUG("Power.IdlePowerSaver service mapper error!");
            messages::internalError(asyncResp->res);
            return;
        }

        // Valid Power IdlePowerSaver object found, now set any values that
        // need to be updated

        if (ipsEnable)
        {
            setDbusProperty(asyncResp, service, path,
                            "xyz.openbmc_project.Control.Power.IdlePowerSaver",
                            "Enabled", "IdlePowerSaver/Enabled", *ipsEnable);
        }
        if (ipsEnterUtil)
        {
            setDbusProperty(asyncResp, service, path,
                            "xyz.openbmc_project.Control.Power.IdlePowerSaver",
                            "EnterUtilizationPercent",
                            "IdlePowerSaver/EnterUtilizationPercent",
                            *ipsEnterUtil);
        }
        if (ipsEnterTime)
        {
            // Convert from seconds into milliseconds for DBus
            const uint64_t timeMilliseconds = *ipsEnterTime * 1000;
            setDbusProperty(asyncResp, service, path,
                            "xyz.openbmc_project.Control.Power.IdlePowerSaver",
                            "EnterDwellTime",
                            "IdlePowerSaver/EnterDwellTimeSeconds",
                            timeMilliseconds);
        }
        if (ipsExitUtil)
        {
            setDbusProperty(asyncResp, service, path,
                            "xyz.openbmc_project.Control.Power.IdlePowerSaver",
                            "ExitUtilizationPercent",
                            "IdlePowerSaver/ExitUtilizationPercent",
                            *ipsExitUtil);
        }
        if (ipsExitTime)
        {
            // Convert from seconds into milliseconds for DBus
            const uint64_t timeMilliseconds = *ipsExitTime * 1000;
            setDbusProperty(asyncResp, service, path,
                            "xyz.openbmc_project.Control.Power.IdlePowerSaver",
                            "ExitDwellTime",
                            "IdlePowerSaver/ExitDwellTimeSeconds",
                            timeMilliseconds);
        }
    });

    BMCWEB_LOG_DEBUG("EXIT: Set idle power saver parameters");
}

inline void handleComputerSystemCollectionHead(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/ComputerSystemCollection/ComputerSystemCollection.json>; rel=describedby");
}

inline void handleComputerSystemCollectionGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/ComputerSystemCollection.json>; rel=describedby");
    asyncResp->res.jsonValue["@odata.type"] =
        "#ComputerSystemCollection.ComputerSystemCollection";
    asyncResp->res.jsonValue["@odata.id"] = "/redfish/v1/Systems";
    asyncResp->res.jsonValue["Name"] = "Computer System Collection";

    nlohmann::json& ifaceArray = asyncResp->res.jsonValue["Members"];
    ifaceArray = nlohmann::json::array();
    if constexpr (BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
        asyncResp->res.jsonValue["Members@odata.count"] = 0;
        // Option currently returns no systems.  TBD
        return;
    }
    asyncResp->res.jsonValue["Members@odata.count"] = 1;
    nlohmann::json::object_t system;
    system["@odata.id"] = boost::urls::format("/redfish/v1/Systems/{}",
                                              BMCWEB_REDFISH_SYSTEM_URI_NAME);
    ifaceArray.emplace_back(std::move(system));
    sdbusplus::asio::getProperty<std::string>(
        *crow::connections::systemBus, "xyz.openbmc_project.Network.Hypervisor",
        "/xyz/openbmc_project/network/hypervisor/config",
        "xyz.openbmc_project.Network.SystemConfiguration", "HostName",
        [asyncResp](const boost::system::error_code& ec2,
                    const std::string& /*hostName*/) {
        if (ec2)
        {
            return;
        }
        auto val = asyncResp->res.jsonValue.find("Members@odata.count");
        if (val == asyncResp->res.jsonValue.end())
        {
            BMCWEB_LOG_CRITICAL("Count wasn't found??");
            return;
        }
        int64_t* count = val->get_ptr<int64_t*>();
        if (count == nullptr)
        {
            BMCWEB_LOG_CRITICAL("Count wasn't found??");
            return;
        }
        *count = *count + 1;
        BMCWEB_LOG_DEBUG("Hypervisor is available");
        nlohmann::json& ifaceArray2 = asyncResp->res.jsonValue["Members"];
        nlohmann::json::object_t hypervisor;
        hypervisor["@odata.id"] = "/redfish/v1/Systems/hypervisor";
        ifaceArray2.emplace_back(std::move(hypervisor));
    });
}

/**
 * Function transceives data with dbus directly.
 */
inline void doNMI(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    constexpr const char* serviceName = "xyz.openbmc_project.Control.Host.NMI";
    constexpr const char* objectPath = "/xyz/openbmc_project/control/host0/nmi";
    constexpr const char* interfaceName =
        "xyz.openbmc_project.Control.Host.NMI";
    constexpr const char* method = "NMI";

    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code& ec) {
        if (ec)
        {
            BMCWEB_LOG_ERROR(" Bad D-Bus request error: {}", ec);
            messages::internalError(asyncResp->res);
            return;
        }
        messages::success(asyncResp->res);
    }, serviceName, objectPath, interfaceName, method);
}

inline void handleComputerSystemResetActionPost(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    if (systemName == "hypervisor")
    {
        handleHypervisorSystemResetPost(req, asyncResp);
        return;
    }

    if (systemName != BMCWEB_REDFISH_SYSTEM_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    if constexpr (BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
        // Option currently returns no systems.  TBD
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    std::string resetType;
    if (!json_util::readJsonAction(req, asyncResp->res, "ResetType", resetType))
    {
        return;
    }

    // Get the command and host vs. chassis
    std::string command;
    bool hostCommand = true;
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
        command = "xyz.openbmc_project.State.Host.Transition.ForceWarmReboot";
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
        messages::actionParameterUnknown(asyncResp->res, "Reset", resetType);
        return;
    }
    sdbusplus::message::object_path statePath("/xyz/openbmc_project/state");

    if (hostCommand)
    {
        setDbusProperty(asyncResp, "xyz.openbmc_project.State.Host",
                        statePath / "host0", "xyz.openbmc_project.State.Host",
                        "RequestedHostTransition", "Reset", command);
    }
    else
    {
        setDbusProperty(asyncResp, "xyz.openbmc_project.State.Chassis",
                        statePath / "chassis0",
                        "xyz.openbmc_project.State.Chassis",
                        "RequestedPowerTransition", "Reset", command);
    }
}

inline void handleComputerSystemHead(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& /*systemName*/)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/ComputerSystem/ComputerSystem.json>; rel=describedby");
}

inline void afterPortRequest(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const std::vector<std::tuple<std::string, std::string, bool>>& socketData)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("DBUS response error {}", ec);
        messages::internalError(asyncResp->res);
        return;
    }
    for (const auto& data : socketData)
    {
        const std::string& socketPath = get<0>(data);
        const std::string& protocolName = get<1>(data);
        bool isProtocolEnabled = get<2>(data);
        nlohmann::json& dataJson = asyncResp->res.jsonValue["SerialConsole"];
        dataJson[protocolName]["ServiceEnabled"] = isProtocolEnabled;
        // need to retrieve port number for
        // obmc-console-ssh service
        if (protocolName == "SSH")
        {
            getPortNumber(socketPath, [asyncResp, protocolName](
                                          const boost::system::error_code& ec1,
                                          int portNumber) {
                if (ec1)
                {
                    BMCWEB_LOG_ERROR("DBUS response error {}", ec1);
                    messages::internalError(asyncResp->res);
                    return;
                }
                nlohmann::json& dataJson1 =
                    asyncResp->res.jsonValue["SerialConsole"];
                dataJson1[protocolName]["Port"] = portNumber;
            });
        }
    }
}

inline void
    handleComputerSystemGet(crow::App& app, const crow::Request& req,
                            const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& systemName)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    if constexpr (BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
        // Option currently returns no systems.  TBD
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    if (systemName == "hypervisor")
    {
        handleHypervisorSystemGet(asyncResp);
        return;
    }

    if (systemName != BMCWEB_REDFISH_SYSTEM_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/ComputerSystem/ComputerSystem.json>; rel=describedby");
    asyncResp->res.jsonValue["@odata.type"] =
        "#ComputerSystem.v1_22_0.ComputerSystem";
    asyncResp->res.jsonValue["Name"] = BMCWEB_REDFISH_SYSTEM_URI_NAME;
    asyncResp->res.jsonValue["Id"] = BMCWEB_REDFISH_SYSTEM_URI_NAME;
    asyncResp->res.jsonValue["SystemType"] = "Physical";
    asyncResp->res.jsonValue["Description"] = "Computer System";
    asyncResp->res.jsonValue["ProcessorSummary"]["Count"] = 0;
    asyncResp->res.jsonValue["MemorySummary"]["TotalSystemMemoryGiB"] =
        double(0);
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Systems/{}", BMCWEB_REDFISH_SYSTEM_URI_NAME);

    asyncResp->res.jsonValue["Processors"]["@odata.id"] = boost::urls::format(
        "/redfish/v1/Systems/{}/Processors", BMCWEB_REDFISH_SYSTEM_URI_NAME);
    asyncResp->res.jsonValue["Memory"]["@odata.id"] = boost::urls::format(
        "/redfish/v1/Systems/{}/Memory", BMCWEB_REDFISH_SYSTEM_URI_NAME);
    asyncResp->res.jsonValue["Storage"]["@odata.id"] = boost::urls::format(
        "/redfish/v1/Systems/{}/Storage", BMCWEB_REDFISH_SYSTEM_URI_NAME);
    asyncResp->res.jsonValue["FabricAdapters"]["@odata.id"] =
        boost::urls::format("/redfish/v1/Systems/{}/FabricAdapters",
                            BMCWEB_REDFISH_SYSTEM_URI_NAME);

    asyncResp->res.jsonValue["Actions"]["#ComputerSystem.Reset"]["target"] =
        boost::urls::format(
            "/redfish/v1/Systems/{}/Actions/ComputerSystem.Reset",
            BMCWEB_REDFISH_SYSTEM_URI_NAME);
    asyncResp->res
        .jsonValue["Actions"]["#ComputerSystem.Reset"]["@Redfish.ActionInfo"] =
        boost::urls::format("/redfish/v1/Systems/{}/ResetActionInfo",
                            BMCWEB_REDFISH_SYSTEM_URI_NAME);

    asyncResp->res.jsonValue["LogServices"]["@odata.id"] = boost::urls::format(
        "/redfish/v1/Systems/{}/LogServices", BMCWEB_REDFISH_SYSTEM_URI_NAME);
    asyncResp->res.jsonValue["Bios"]["@odata.id"] = boost::urls::format(
        "/redfish/v1/Systems/{}/Bios", BMCWEB_REDFISH_SYSTEM_URI_NAME);

    nlohmann::json::array_t managedBy;
    nlohmann::json& manager = managedBy.emplace_back();
    manager["@odata.id"] = boost::urls::format("/redfish/v1/Managers/{}",
                                               BMCWEB_REDFISH_MANAGER_URI_NAME);
    asyncResp->res.jsonValue["Links"]["ManagedBy"] = std::move(managedBy);
    asyncResp->res.jsonValue["Status"]["Health"] = "OK";
    asyncResp->res.jsonValue["Status"]["State"] = "Enabled";

    // Fill in SerialConsole info
    asyncResp->res.jsonValue["SerialConsole"]["MaxConcurrentSessions"] = 15;
    asyncResp->res.jsonValue["SerialConsole"]["IPMI"]["ServiceEnabled"] = true;

    asyncResp->res.jsonValue["SerialConsole"]["SSH"]["ServiceEnabled"] = true;
    asyncResp->res.jsonValue["SerialConsole"]["SSH"]["Port"] = 2200;
    asyncResp->res.jsonValue["SerialConsole"]["SSH"]["HotKeySequenceDisplay"] =
        "Press ~. to exit console";
    getPortStatusAndPath(std::span{protocolToDBusForSystems},
                         std::bind_front(afterPortRequest, asyncResp));

    if constexpr (BMCWEB_KVM)
    {
        // Fill in GraphicalConsole info
        asyncResp->res.jsonValue["GraphicalConsole"]["ServiceEnabled"] = true;
        asyncResp->res.jsonValue["GraphicalConsole"]["MaxConcurrentSessions"] =
            4;
        asyncResp->res.jsonValue["GraphicalConsole"]["ConnectTypesSupported"] =
            nlohmann::json::array_t({"KVMIP"});
    }

    getMainChassisId(asyncResp,
                     [](const std::string& chassisId,
                        const std::shared_ptr<bmcweb::AsyncResp>& aRsp) {
        nlohmann::json::array_t chassisArray;
        nlohmann::json& chassis = chassisArray.emplace_back();
        chassis["@odata.id"] = boost::urls::format("/redfish/v1/Chassis/{}",
                                                   chassisId);
        aRsp->res.jsonValue["Links"]["Chassis"] = std::move(chassisArray);
    });

    getSystemLocationIndicatorActive(asyncResp);
    // TODO (Gunnar): Remove IndicatorLED after enough time has passed
    getIndicatorLedState(asyncResp);
    getComputerSystem(asyncResp);
    getHostState(asyncResp);
    getBootProgress(asyncResp);
    getBootProgressLastStateTime(asyncResp);
    pcie_util::getPCIeDeviceList(asyncResp,
                                 nlohmann::json::json_pointer("/PCIeDevices"));
    getHostWatchdogTimer(asyncResp);
    getPowerRestorePolicy(asyncResp);
    getStopBootOnFault(asyncResp);
    getAutomaticRetryPolicy(asyncResp);
    getLastResetTime(asyncResp);
    if constexpr (BMCWEB_IBM_LED_EXTENSIONS)
    {
        getLampTestState(asyncResp);
        getSAI(asyncResp, "PartitionSystemAttentionIndicator");
        getSAI(asyncResp, "PlatformSystemAttentionIndicator");
    }
    if constexpr (BMCWEB_REDFISH_PROVISIONING_FEATURE)
    {
        getProvisioningStatus(asyncResp);
    }
    getTrustedModuleRequiredToBoot(asyncResp);
    getPowerMode(asyncResp);
    getIdlePowerSaver(asyncResp);

    // Panel Function
    getEnabledPanelFunctions(asyncResp);

    nlohmann::json& actionOem = asyncResp->res.jsonValue["Actions"]["Oem"];
    actionOem["#OemComputerSystem.v1_0_0.ExecutePanelFunction"]["target"] =
        "/redfish/v1/Systems/system/Actions/Oem/OemComputerSystem.ExecutePanelFunction";

    // ChapData
    getChapData(asyncResp);
}

inline void handleComputerSystemPatch(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    if constexpr (BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
        // Option currently returns no systems.  TBD
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    if (systemName != BMCWEB_REDFISH_SYSTEM_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/ComputerSystem/ComputerSystem.json>; rel=describedby");

    std::optional<bool> locationIndicatorActive;
    std::optional<std::string> indicatorLed;
    std::optional<std::string> assetTag;
    std::optional<std::string> powerRestorePolicy;
    std::optional<std::string> powerMode;
    std::optional<bool> wdtEnable;
    std::optional<std::string> wdtTimeOutAction;
    std::optional<std::string> bootAutomaticRetry;
    std::optional<uint32_t> bootAutomaticRetryAttempts;
    std::optional<bool> bootTrustedModuleRequired;
    std::optional<std::string> stopBootOnFault;
    std::optional<bool> ipsEnable;
    std::optional<uint8_t> ipsEnterUtil;
    std::optional<uint64_t> ipsEnterTime;
    std::optional<uint8_t> ipsExitUtil;
    std::optional<uint64_t> ipsExitTime;
    std::optional<nlohmann::json> oem;

    // clang-format off
                if (!json_util::readJsonPatch(
                        req, asyncResp->res,
                        "IndicatorLED", indicatorLed,
                        "LocationIndicatorActive", locationIndicatorActive,
                        "AssetTag", assetTag,
                        "PowerRestorePolicy", powerRestorePolicy,
                        "PowerMode", powerMode,
                        "HostWatchdogTimer/FunctionEnabled", wdtEnable,
                        "HostWatchdogTimer/TimeoutAction", wdtTimeOutAction,
                        "Boot/AutomaticRetryConfig", bootAutomaticRetry,
                        "Boot/AutomaticRetryAttempts", bootAutomaticRetryAttempts,
                        "Boot/TrustedModuleRequiredToBoot", bootTrustedModuleRequired,
                        "Boot/StopBootOnFault", stopBootOnFault,
                        "IdlePowerSaver/Enabled", ipsEnable,
                        "IdlePowerSaver/EnterUtilizationPercent", ipsEnterUtil,
                        "IdlePowerSaver/EnterDwellTimeSeconds", ipsEnterTime,
                        "IdlePowerSaver/ExitUtilizationPercent", ipsExitUtil,
                        "IdlePowerSaver/ExitDwellTimeSeconds", ipsExitTime,
                        "Oem", oem))
                {
                    return;
                }
    // clang-format on

    asyncResp->res.result(boost::beast::http::status::no_content);

    if (assetTag)
    {
        setAssetTag(asyncResp, *assetTag);
    }

    if (wdtEnable || wdtTimeOutAction)
    {
        setWDTProperties(asyncResp, wdtEnable, wdtTimeOutAction);
    }

    if (bootAutomaticRetry)
    {
        setAutomaticRetry(asyncResp, *bootAutomaticRetry);
    }

    if (bootAutomaticRetryAttempts)
    {
        setAutomaticRetryAttempts(asyncResp,
                                  bootAutomaticRetryAttempts.value());
    }

    if (bootTrustedModuleRequired)
    {
        setTrustedModuleRequiredToBoot(asyncResp, *bootTrustedModuleRequired);
    }

    if (stopBootOnFault)
    {
        setStopBootOnFault(asyncResp, *stopBootOnFault);
    }

    if (locationIndicatorActive)
    {
        setSystemLocationIndicatorActive(asyncResp, *locationIndicatorActive);
    }

    // TODO (Gunnar): Remove IndicatorLED after enough time has
    // passed
    if (indicatorLed)
    {
        setIndicatorLedState(asyncResp, *indicatorLed);
        asyncResp->res.addHeader(boost::beast::http::field::warning,
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

    if (oem)
    {
        std::optional<nlohmann::json> ibmOem;
        if (!redfish::json_util::readJson(*oem, asyncResp->res, "IBM", ibmOem))
        {
            return;
        }

        if (ibmOem)
        {
            std::optional<bool> pcieTopologyRefresh;
            std::optional<bool> savePCIeTopologyInfo;
            std::optional<std::string> chapName;
            std::optional<std::string> chapSecret;

            if constexpr (BMCWEB_IBM_LED_EXTENSIONS)
            {
                std::optional<bool> lampTest;
                std::optional<bool> partitionSAI;
                std::optional<bool> platformSAI;
                if (!json_util::readJson(
                        *ibmOem, asyncResp->res, "LampTest", lampTest,
                        "PartitionSystemAttentionIndicator", partitionSAI,
                        "PlatformSystemAttentionIndicator", platformSAI,
                        "PCIeTopologyRefresh", pcieTopologyRefresh,
                        "SavePCIeTopologyInfo", savePCIeTopologyInfo,
                        "ChapData/ChapName", chapName, "ChapData/ChapSecret",
                        chapSecret))
                {
                    return;
                }
                if (lampTest)
                {
                    setLampTestState(asyncResp, *lampTest);
                }
                if (partitionSAI)
                {
                    setSAI(asyncResp, "PartitionSystemAttentionIndicator",
                           *partitionSAI);
                }
                if (platformSAI)
                {
                    setSAI(asyncResp, "PlatformSystemAttentionIndicator",
                           *platformSAI);
                }
            }
            else
            {
                if (!json_util::readJson(
                        *ibmOem, asyncResp->res, "PCIeTopologyRefresh",
                        pcieTopologyRefresh, "SavePCIeTopologyInfo",
                        savePCIeTopologyInfo, "ChapData/ChapName", chapName,
                        "ChapData/ChapSecret", chapSecret))
                {
                    return;
                }
            }
            if (pcieTopologyRefresh)
            {
                setPCIeTopologyRefresh(req, asyncResp, *pcieTopologyRefresh);
            }
            if (savePCIeTopologyInfo)
            {
                setSavePCIeTopologyInfo(asyncResp, *savePCIeTopologyInfo);
            }
            if (chapName || chapSecret)
            {
                setChapData(asyncResp, chapName, chapSecret);
            }
        }
    }

    if (ipsEnable || ipsEnterUtil || ipsEnterTime || ipsExitUtil || ipsExitTime)
    {
        setIdlePowerSaver(asyncResp, ipsEnable, ipsEnterUtil, ipsEnterTime,
                          ipsExitUtil, ipsExitTime);
    }
}

inline void handleSystemCollectionResetActionHead(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& /*systemName*/)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/ActionInfo/ActionInfo.json>; rel=describedby");
}

/**
 * @brief Translates allowed host transitions to redfish string
 *
 * @param[in]  dbusAllowedHostTran The allowed host transition on dbus
 * @param[out] allowableValues     The translated host transition(s)
 *
 * @return Emplaces corresponding Redfish translated value(s) in
 * allowableValues. If translation not possible, does nothing to
 * allowableValues.
 */
inline void
    dbusToRfAllowedHostTransitions(const std::string& dbusAllowedHostTran,
                                   nlohmann::json::array_t& allowableValues)
{
    if (dbusAllowedHostTran == "xyz.openbmc_project.State.Host.Transition.On")
    {
        allowableValues.emplace_back(resource::ResetType::On);
        allowableValues.emplace_back(resource::ResetType::ForceOn);
    }
    else if (dbusAllowedHostTran ==
             "xyz.openbmc_project.State.Host.Transition.Off")
    {
        allowableValues.emplace_back(resource::ResetType::GracefulShutdown);
    }
    else if (dbusAllowedHostTran ==
             "xyz.openbmc_project.State.Host.Transition.GracefulWarmReboot")
    {
        allowableValues.emplace_back(resource::ResetType::GracefulRestart);
    }
    else if (dbusAllowedHostTran ==
             "xyz.openbmc_project.State.Host.Transition.ForceWarmReboot")
    {
        allowableValues.emplace_back(resource::ResetType::ForceRestart);
    }
    else
    {
        BMCWEB_LOG_WARNING("Unsupported host tran {}", dbusAllowedHostTran);
    }
}

inline void afterGetAllowedHostTransitions(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const std::vector<std::string>& allowedHostTransitions)
{
    nlohmann::json::array_t allowableValues;

    // Supported on all systems currently
    allowableValues.emplace_back(resource::ResetType::ForceOff);
    allowableValues.emplace_back(resource::ResetType::PowerCycle);
    allowableValues.emplace_back(resource::ResetType::Nmi);

    if (ec)
    {
        if (ec == boost::system::linux_error::bad_request_descriptor ||
            ec == boost::asio::error::basic_errors::host_unreachable)
        {
            // Property not implemented so just return defaults
            BMCWEB_LOG_DEBUG("Property not available {}", ec);
            allowableValues.emplace_back(resource::ResetType::On);
            allowableValues.emplace_back(resource::ResetType::ForceOn);
            allowableValues.emplace_back(resource::ResetType::ForceRestart);
            allowableValues.emplace_back(resource::ResetType::GracefulRestart);
            allowableValues.emplace_back(resource::ResetType::GracefulShutdown);
        }
        else
        {
            BMCWEB_LOG_ERROR("DBUS response error {}", ec);
            messages::internalError(asyncResp->res);
            return;
        }
    }
    else
    {
        for (const std::string& transition : allowedHostTransitions)
        {
            BMCWEB_LOG_DEBUG("Found allowed host tran {}", transition);
            dbusToRfAllowedHostTransitions(transition, allowableValues);
        }
    }

    nlohmann::json::object_t parameter;
    parameter["Name"] = "ResetType";
    parameter["Required"] = true;
    parameter["DataType"] = "String";
    parameter["AllowableValues"] = std::move(allowableValues);
    nlohmann::json::array_t parameters;
    parameters.emplace_back(std::move(parameter));
    asyncResp->res.jsonValue["Parameters"] = std::move(parameters);
}

inline void handleSystemCollectionResetActionGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    if constexpr (BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
        // Option currently returns no systems.  TBD
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    if (systemName == "hypervisor")
    {
        handleHypervisorResetActionGet(asyncResp);
        return;
    }

    if (systemName != BMCWEB_REDFISH_SYSTEM_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/ActionInfo/ActionInfo.json>; rel=describedby");

    asyncResp->res.jsonValue["@odata.id"] =
        boost::urls::format("/redfish/v1/Systems/{}/ResetActionInfo",
                            BMCWEB_REDFISH_SYSTEM_URI_NAME);
    asyncResp->res.jsonValue["@odata.type"] = "#ActionInfo.v1_1_2.ActionInfo";
    asyncResp->res.jsonValue["Name"] = "Reset Action Info";
    asyncResp->res.jsonValue["Id"] = "ResetActionInfo";

    // Look to see if system defines AllowedHostTransitions
    sdbusplus::asio::getProperty<std::vector<std::string>>(
        *crow::connections::systemBus, "xyz.openbmc_project.State.Host",
        "/xyz/openbmc_project/state/host0", "xyz.openbmc_project.State.Host",
        "AllowedHostTransitions",
        [asyncResp](const boost::system::error_code& ec,
                    const std::vector<std::string>& allowedHostTransitions) {
        afterGetAllowedHostTransitions(asyncResp, ec, allowedHostTransitions);
    });
}
/**
 * SystemResetActionInfo derived class for delivering Computer Systems
 * ResetType AllowableValues using ResetInfo schema.
 */
inline void requestRoutesSystems(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/")
        .privileges(redfish::privileges::headComputerSystemCollection)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handleComputerSystemCollectionHead, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Systems/")
        .privileges(redfish::privileges::getComputerSystemCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleComputerSystemCollectionGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/")
        .privileges(redfish::privileges::headComputerSystem)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handleComputerSystemHead, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/")
        .privileges(redfish::privileges::getComputerSystem)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleComputerSystemGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/")
        .privileges(redfish::privileges::patchComputerSystem)
        .methods(boost::beast::http::verb::patch)(
            std::bind_front(handleComputerSystemPatch, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Actions/ComputerSystem.Reset/")
        .privileges(redfish::privileges::postComputerSystem)
        .methods(boost::beast::http::verb::post)(std::bind_front(
            handleComputerSystemResetActionPost, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/ResetActionInfo/")
        .privileges(redfish::privileges::headActionInfo)
        .methods(boost::beast::http::verb::head)(std::bind_front(
            handleSystemCollectionResetActionHead, std::ref(app)));
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/ResetActionInfo/")
        .privileges(redfish::privileges::getActionInfo)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleSystemCollectionResetActionGet, std::ref(app)));
}
} // namespace redfish
