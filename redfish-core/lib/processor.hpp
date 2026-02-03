// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
// SPDX-FileCopyrightText: Copyright 2018 Intel Corporation
#pragma once

#include "bmcweb_config.h"

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/processor.hpp"
#include "generated/enums/resource.hpp"
#include "http_request.hpp"
#include "led.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/collection.hpp"
#include "utils/dbus_utils.hpp"
#include "utils/hex_utils.hpp"
#include "utils/json_utils.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>
#include <sdbusplus/message/native_types.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

namespace redfish
{

// Interfaces which imply a D-Bus object represents a Processor
constexpr std::array<std::string_view, 2> processorInterfaces = {
    "xyz.openbmc_project.Inventory.Item.Cpu",
    "xyz.openbmc_project.Inventory.Item.Accelerator"};

/**
 * @brief Fill out uuid info of a processor by
 * requesting data from the given D-Bus object.
 *
 * @param[in,out]   asyncResp       Async HTTP response.
 * @param[in]       service     D-Bus service to query.
 * @param[in]       objPath     D-Bus object to query.
 */
inline void getProcessorUUID(std::shared_ptr<bmcweb::AsyncResp> asyncResp,
                             const std::string& service,
                             const std::string& objPath)
{
    BMCWEB_LOG_DEBUG("Get Processor UUID");
    dbus::utility::getProperty<std::string>(
        service, objPath, "xyz.openbmc_project.Common.UUID", "UUID",
        [objPath, asyncResp{std::move(asyncResp)}](
            const boost::system::error_code& ec, const std::string& property) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG("DBUS response error");
                messages::internalError(asyncResp->res);
                return;
            }
            asyncResp->res.jsonValue["UUID"] = property;
        });
}

inline void getCpuDataByInterface(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const dbus::utility::DBusInterfacesMap& cpuInterfacesProperties)
{
    BMCWEB_LOG_DEBUG("Get CPU resources by interface.");

    // Set the default value of state
    asyncResp->res.jsonValue["Status"]["State"] = resource::State::Enabled;
    asyncResp->res.jsonValue["Status"]["Health"] = resource::Health::OK;

    for (const auto& interface : cpuInterfacesProperties)
    {
        for (const auto& property : interface.second)
        {
            if (property.first == "Present")
            {
                const bool* cpuPresent = std::get_if<bool>(&property.second);
                if (cpuPresent == nullptr)
                {
                    // Important property not in desired type
                    messages::internalError(asyncResp->res);
                    return;
                }
                if (!*cpuPresent)
                {
                    // Slot is not populated
                    asyncResp->res.jsonValue["Status"]["State"] =
                        resource::State::Absent;
                }
            }
            else if (property.first == "Functional")
            {
                const bool* cpuFunctional = std::get_if<bool>(&property.second);
                if (cpuFunctional == nullptr)
                {
                    messages::internalError(asyncResp->res);
                    return;
                }
                if (!*cpuFunctional)
                {
                    asyncResp->res.jsonValue["Status"]["Health"] =
                        resource::Health::Critical;
                }
            }
            else if (property.first == "CoreCount")
            {
                const uint16_t* coresCount =
                    std::get_if<uint16_t>(&property.second);
                if (coresCount == nullptr)
                {
                    messages::internalError(asyncResp->res);
                    return;
                }
                asyncResp->res.jsonValue["TotalCores"] = *coresCount;
            }
            else if (property.first == "MaxSpeedInMhz")
            {
                const uint32_t* value = std::get_if<uint32_t>(&property.second);
                if (value != nullptr)
                {
                    asyncResp->res.jsonValue["MaxSpeedMHz"] = *value;
                }
            }
            else if (property.first == "Socket")
            {
                const std::string* value =
                    std::get_if<std::string>(&property.second);
                if (value != nullptr)
                {
                    asyncResp->res.jsonValue["Socket"] = *value;
                }
            }
            else if (property.first == "ThreadCount")
            {
                const uint16_t* value = std::get_if<uint16_t>(&property.second);
                if (value != nullptr)
                {
                    asyncResp->res.jsonValue["TotalThreads"] = *value;
                }
            }
            else if (property.first == "EffectiveFamily")
            {
                const uint16_t* value = std::get_if<uint16_t>(&property.second);
                if (value != nullptr && *value != 2)
                {
                    asyncResp->res.jsonValue["ProcessorId"]["EffectiveFamily"] =

                        std::format("{:#04X}", *value);
                }
            }
            else if (property.first == "EffectiveModel")
            {
                const uint16_t* value = std::get_if<uint16_t>(&property.second);
                if (value == nullptr)
                {
                    messages::internalError(asyncResp->res);
                    return;
                }
                if (*value != 0)
                {
                    asyncResp->res.jsonValue["ProcessorId"]["EffectiveModel"] =
                        std::format("{:#04X}", *value);
                }
            }
            else if (property.first == "Id")
            {
                const uint64_t* value = std::get_if<uint64_t>(&property.second);
                if (value != nullptr && *value != 0)
                {
                    asyncResp->res
                        .jsonValue["ProcessorId"]["IdentificationRegisters"] =
                        std::format("{:#016X}", *value);
                }
            }
            else if (property.first == "Microcode")
            {
                const uint32_t* value = std::get_if<uint32_t>(&property.second);
                if (value == nullptr)
                {
                    messages::internalError(asyncResp->res);
                    return;
                }
                if (*value != 0)
                {
                    asyncResp->res.jsonValue["ProcessorId"]["MicrocodeInfo"] =
                        std::format("{:#08X}", *value);
                }
            }
            else if (property.first == "Step")
            {
                const uint16_t* value = std::get_if<uint16_t>(&property.second);
                if (value == nullptr)
                {
                    messages::internalError(asyncResp->res);
                    return;
                }
                if (*value != std::numeric_limits<uint16_t>::max())
                {
                    asyncResp->res.jsonValue["ProcessorId"]["Step"] =
                        std::format("{:#04X}", *value);
                }
            }
        }
    }
}

inline void getCpuDataByService(
    std::shared_ptr<bmcweb::AsyncResp> asyncResp, const std::string& cpuId,
    const std::string& service, const std::string& objPath)
{
    BMCWEB_LOG_DEBUG("Get available system cpu resources by service.");

    sdbusplus::message::object_path path("/xyz/openbmc_project/inventory");
    dbus::utility::getManagedObjects(
        service, path,
        [cpuId, service, objPath, asyncResp{std::move(asyncResp)}](
            const boost::system::error_code& ec,
            const dbus::utility::ManagedObjectType& dbusData) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG("DBUS response error");
                messages::internalError(asyncResp->res);
                return;
            }
            asyncResp->res.jsonValue["Id"] = cpuId;
            asyncResp->res.jsonValue["Name"] = "Processor";
            asyncResp->res.jsonValue["ProcessorType"] =
                processor::ProcessorType::CPU;

            for (const auto& object : dbusData)
            {
                if (object.first.str == objPath)
                {
                    getCpuDataByInterface(asyncResp, object.second);
                }
            }
            return;
        });
}

/**
 * @brief Translates throttle cause DBUS property to redfish.
 *
 * @param[in] dbusSource    The throttle cause from DBUS
 *
 * @return Returns as a string, the throttle cause in Redfish terms. If
 * translation cannot be done, returns "Unknown" throttle reason.
 */
inline processor::ThrottleCause dbusToRfThrottleCause(
    const std::string& dbusSource)
{
    if (dbusSource ==
        "xyz.openbmc_project.Control.Power.Throttle.ThrottleReasons.ClockLimit")
    {
        return processor::ThrottleCause::ClockLimit;
    }
    if (dbusSource ==
        "xyz.openbmc_project.Control.Power.Throttle.ThrottleReasons.ManagementDetectedFault")
    {
        return processor::ThrottleCause::ManagementDetectedFault;
    }
    if (dbusSource ==
        "xyz.openbmc_project.Control.Power.Throttle.ThrottleReasons.PowerLimit")
    {
        return processor::ThrottleCause::PowerLimit;
    }
    if (dbusSource ==
        "xyz.openbmc_project.Control.Power.Throttle.ThrottleReasons.ThermalLimit")
    {
        return processor::ThrottleCause::ThermalLimit;
    }
    if (dbusSource ==
        "xyz.openbmc_project.Control.Power.Throttle.ThrottleReasons.Unknown")
    {
        return processor::ThrottleCause::Unknown;
    }
    return processor::ThrottleCause::Invalid;
}

inline void readThrottleProperties(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const dbus::utility::DBusPropertiesMap& properties)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("Processor Throttle getAllProperties error {}", ec);
        messages::internalError(asyncResp->res);
        return;
    }

    const bool* status = nullptr;
    const std::vector<std::string>* causes = nullptr;

    if (!sdbusplus::unpackPropertiesNoThrow(dbus_utils::UnpackErrorPrinter(),
                                            properties, "Throttled", status,
                                            "ThrottleCauses", causes))
    {
        messages::internalError(asyncResp->res);
        return;
    }

    asyncResp->res.jsonValue["Throttled"] = *status;
    nlohmann::json::array_t rCauses;
    for (const std::string& cause : *causes)
    {
        processor::ThrottleCause rfCause = dbusToRfThrottleCause(cause);
        if (rfCause == processor::ThrottleCause::Invalid)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        rCauses.emplace_back(rfCause);
    }
    asyncResp->res.jsonValue["ThrottleCauses"] = std::move(rCauses);
}

inline void getThrottleProperties(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& service, const std::string& objectPath)
{
    BMCWEB_LOG_DEBUG("Get processor throttle resources");

    dbus::utility::getAllProperties(
        service, objectPath, "xyz.openbmc_project.Control.Power.Throttle",
        [asyncResp](const boost::system::error_code& ec,
                    const dbus::utility::DBusPropertiesMap& properties) {
            readThrottleProperties(asyncResp, ec, properties);
        });
}

inline void getCpuAssetData(std::shared_ptr<bmcweb::AsyncResp> asyncResp,
                            const std::string& service,
                            const std::string& objPath)
{
    BMCWEB_LOG_DEBUG("Get Cpu Asset Data");
    dbus::utility::getAllProperties(
        service, objPath, "xyz.openbmc_project.Inventory.Decorator.Asset",
        [objPath, asyncResp{std::move(asyncResp)}](
            const boost::system::error_code& ec,
            const dbus::utility::DBusPropertiesMap& properties) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG("DBUS response error");
                messages::internalError(asyncResp->res);
                return;
            }

            const std::string* serialNumber = nullptr;
            const std::string* model = nullptr;
            const std::string* manufacturer = nullptr;
            const std::string* partNumber = nullptr;
            const std::string* sparePartNumber = nullptr;

            const bool success = sdbusplus::unpackPropertiesNoThrow(
                dbus_utils::UnpackErrorPrinter(), properties, "SerialNumber",
                serialNumber, "Model", model, "Manufacturer", manufacturer,
                "PartNumber", partNumber, "SparePartNumber", sparePartNumber);

            if (!success)
            {
                messages::internalError(asyncResp->res);
                return;
            }

            if (serialNumber != nullptr && !serialNumber->empty())
            {
                asyncResp->res.jsonValue["SerialNumber"] = *serialNumber;
            }

            if ((model != nullptr) && !model->empty())
            {
                asyncResp->res.jsonValue["Model"] = *model;
            }

            if (manufacturer != nullptr)
            {
                asyncResp->res.jsonValue["Manufacturer"] = *manufacturer;

                // Otherwise would be unexpected.
                if (manufacturer->find("Intel") != std::string::npos)
                {
                    asyncResp->res.jsonValue["ProcessorArchitecture"] = "x86";
                    asyncResp->res.jsonValue["InstructionSet"] = "x86-64";
                }
                else if (manufacturer->find("IBM") != std::string::npos)
                {
                    asyncResp->res.jsonValue["ProcessorArchitecture"] = "Power";
                    asyncResp->res.jsonValue["InstructionSet"] = "PowerISA";
                }
                else if (manufacturer->find("Ampere") != std::string::npos)
                {
                    asyncResp->res.jsonValue["ProcessorArchitecture"] = "ARM";
                    asyncResp->res.jsonValue["InstructionSet"] = "ARM-A64";
                }
            }

            if (partNumber != nullptr)
            {
                asyncResp->res.jsonValue["PartNumber"] = *partNumber;
            }

            if (sparePartNumber != nullptr && !sparePartNumber->empty())
            {
                asyncResp->res.jsonValue["SparePartNumber"] = *sparePartNumber;
            }
        });
}

inline void getCpuRevisionData(std::shared_ptr<bmcweb::AsyncResp> asyncResp,
                               const std::string& service,
                               const std::string& objPath)
{
    BMCWEB_LOG_DEBUG("Get Cpu Revision Data");
    dbus::utility::getAllProperties(
        service, objPath, "xyz.openbmc_project.Inventory.Decorator.Revision",
        [objPath, asyncResp{std::move(asyncResp)}](
            const boost::system::error_code& ec,
            const dbus::utility::DBusPropertiesMap& properties) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG("DBUS response error");
                messages::internalError(asyncResp->res);
                return;
            }

            const std::string* version = nullptr;

            const bool success = sdbusplus::unpackPropertiesNoThrow(
                dbus_utils::UnpackErrorPrinter(), properties, "Version",
                version);

            if (!success)
            {
                messages::internalError(asyncResp->res);
                return;
            }

            if (version != nullptr)
            {
                asyncResp->res.jsonValue["Version"] = *version;
            }
        });
}

inline void getAcceleratorDataByService(
    std::shared_ptr<bmcweb::AsyncResp> asyncResp, const std::string& acclrtrId,
    const std::string& service, const std::string& objPath)
{
    BMCWEB_LOG_DEBUG("Get available system Accelerator resources by service.");
    dbus::utility::getAllProperties(
        service, objPath, "",
        [acclrtrId, asyncResp{std::move(asyncResp)}](
            const boost::system::error_code& ec,
            const dbus::utility::DBusPropertiesMap& properties) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG("DBUS response error");
                messages::internalError(asyncResp->res);
                return;
            }

            const bool* functional = nullptr;
            const bool* present = nullptr;

            const bool success = sdbusplus::unpackPropertiesNoThrow(
                dbus_utils::UnpackErrorPrinter(), properties, "Functional",
                functional, "Present", present);

            if (!success)
            {
                messages::internalError(asyncResp->res);
                return;
            }

            std::string state = "Enabled";
            std::string health = "OK";

            if (present != nullptr && !*present)
            {
                state = "Absent";
            }

            if (functional != nullptr && !*functional)
            {
                if (state == "Enabled")
                {
                    health = "Critical";
                }
            }

            asyncResp->res.jsonValue["Id"] = acclrtrId;
            asyncResp->res.jsonValue["Name"] = "Processor";
            asyncResp->res.jsonValue["Status"]["State"] = state;
            asyncResp->res.jsonValue["Status"]["Health"] = health;
            asyncResp->res.jsonValue["ProcessorType"] =
                processor::ProcessorType::Accelerator;
        });
}

// OperatingConfig D-Bus Types
using TurboProfileProperty = std::vector<std::tuple<uint32_t, size_t>>;
using BaseSpeedPrioritySettingsProperty =
    std::vector<std::tuple<uint32_t, std::vector<uint32_t>>>;
// uint32_t and size_t may or may not be the same type, requiring a dedup'd
// variant

/**
 * Fill out the HighSpeedCoreIDs in a Processor resource from the given
 * OperatingConfig D-Bus property.
 *
 * @param[in,out]   asyncResp           Async HTTP response.
 * @param[in]       baseSpeedSettings   Full list of base speed priority groups,
 *                                      to use to determine the list of high
 *                                      speed cores.
 */
inline void highSpeedCoreIdsHandler(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const BaseSpeedPrioritySettingsProperty& baseSpeedSettings)
{
    // The D-Bus property does not indicate which bucket is the "high
    // priority" group, so let's discern that by looking for the one with
    // highest base frequency.
    auto highPriorityGroup = baseSpeedSettings.cend();
    uint32_t highestBaseSpeed = 0;
    for (auto it = baseSpeedSettings.cbegin(); it != baseSpeedSettings.cend();
         ++it)
    {
        const uint32_t baseFreq = std::get<uint32_t>(*it);
        if (baseFreq > highestBaseSpeed)
        {
            highestBaseSpeed = baseFreq;
            highPriorityGroup = it;
        }
    }

    nlohmann::json& jsonCoreIds = asyncResp->res.jsonValue["HighSpeedCoreIDs"];
    jsonCoreIds = nlohmann::json::array();

    // There may not be any entries in the D-Bus property, so only populate
    // if there was actually something there.
    if (highPriorityGroup != baseSpeedSettings.cend())
    {
        jsonCoreIds = std::get<std::vector<uint32_t>>(*highPriorityGroup);
    }
}

/**
 * Fill out OperatingConfig related items in a Processor resource by requesting
 * data from the given D-Bus object.
 *
 * @param[in,out]   asyncResp       Async HTTP response.
 * @param[in]       cpuId       CPU D-Bus name.
 * @param[in]       service     D-Bus service to query.
 * @param[in]       objPath     D-Bus object to query.
 */
inline void getCpuConfigData(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& cpuId, const std::string& service,
    const std::string& objPath)
{
    BMCWEB_LOG_INFO("Getting CPU operating configs for {}", cpuId);

    // First, GetAll CurrentOperatingConfig properties on the object
    dbus::utility::getAllProperties(
        service, objPath,
        "xyz.openbmc_project.Control.Processor.CurrentOperatingConfig",
        [asyncResp, cpuId,
         service](const boost::system::error_code& ec,
                  const dbus::utility::DBusPropertiesMap& properties) {
            if (ec)
            {
                BMCWEB_LOG_WARNING("D-Bus error: {}, {}", ec, ec.message());
                messages::internalError(asyncResp->res);
                return;
            }

            nlohmann::json& json = asyncResp->res.jsonValue;

            const sdbusplus::message::object_path* appliedConfig = nullptr;
            const bool* baseSpeedPriorityEnabled = nullptr;

            const bool success = sdbusplus::unpackPropertiesNoThrow(
                dbus_utils::UnpackErrorPrinter(), properties, "AppliedConfig",
                appliedConfig, "BaseSpeedPriorityEnabled",
                baseSpeedPriorityEnabled);

            if (!success)
            {
                messages::internalError(asyncResp->res);
                return;
            }

            if (appliedConfig != nullptr)
            {
                const std::string& dbusPath = appliedConfig->str;
                nlohmann::json::object_t operatingConfig;
                operatingConfig["@odata.id"] = boost::urls::format(
                    "/redfish/v1/Systems/{}/Processors/{}/OperatingConfigs",
                    BMCWEB_REDFISH_SYSTEM_URI_NAME, cpuId);
                json["OperatingConfigs"] = std::move(operatingConfig);

                // Reuse the D-Bus config object name for the Redfish
                // URI
                size_t baseNamePos = dbusPath.rfind('/');
                if (baseNamePos == std::string::npos ||
                    baseNamePos == (dbusPath.size() - 1))
                {
                    // If the AppliedConfig was somehow not a valid path,
                    // skip adding any more properties, since everything
                    // else is tied to this applied config.
                    messages::internalError(asyncResp->res);
                    return;
                }
                nlohmann::json::object_t appliedOperatingConfig;
                appliedOperatingConfig["@odata.id"] = boost::urls::format(
                    "/redfish/v1/Systems/{}/Processors/{}/OperatingConfigs/{}",
                    BMCWEB_REDFISH_SYSTEM_URI_NAME, cpuId,
                    dbusPath.substr(baseNamePos + 1));
                json["AppliedOperatingConfig"] =
                    std::move(appliedOperatingConfig);

                // Once we found the current applied config, queue another
                // request to read the base freq core ids out of that
                // config.
                dbus::utility::getProperty<BaseSpeedPrioritySettingsProperty>(
                    service, dbusPath,
                    "xyz.openbmc_project.Inventory.Item.Cpu."
                    "OperatingConfig",
                    "BaseSpeedPrioritySettings",
                    [asyncResp](const boost::system::error_code& ec2,
                                const BaseSpeedPrioritySettingsProperty&
                                    baseSpeedList) {
                        if (ec2)
                        {
                            BMCWEB_LOG_WARNING("D-Bus Property Get error: {}",
                                               ec2);
                            messages::internalError(asyncResp->res);
                            return;
                        }

                        highSpeedCoreIdsHandler(asyncResp, baseSpeedList);
                    });
            }

            if (baseSpeedPriorityEnabled != nullptr)
            {
                json["BaseSpeedPriorityState"] =
                    *baseSpeedPriorityEnabled ? "Enabled" : "Disabled";
            }
        });
}

/**
 * @brief Fill out location info of a processor by
 * requesting data from the given D-Bus object.
 *
 * @param[in,out]   asyncResp       Async HTTP response.
 * @param[in]       service     D-Bus service to query.
 * @param[in]       objPath     D-Bus object to query.
 */
inline void getCpuLocationCode(std::shared_ptr<bmcweb::AsyncResp> asyncResp,
                               const std::string& service,
                               const std::string& objPath)
{
    BMCWEB_LOG_DEBUG("Get Cpu Location Data");
    dbus::utility::getProperty<std::string>(
        service, objPath,
        "xyz.openbmc_project.Inventory.Decorator.LocationCode", "LocationCode",
        [objPath, asyncResp{std::move(asyncResp)}](
            const boost::system::error_code& ec, const std::string& property) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG("DBUS response error");
                messages::internalError(asyncResp->res);
                return;
            }

            asyncResp->res
                .jsonValue["Location"]["PartLocation"]["ServiceLabel"] =
                property;
        });
}

/**
 * Populate the unique identifier in a Processor resource by requesting data
 * from the given D-Bus object.
 *
 * @param[in,out]   asyncResp   Async HTTP response.
 * @param[in]       service     D-Bus service to query.
 * @param[in]       objPath     D-Bus object to query.
 */
inline void getCpuUniqueId(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const std::string& service,
                           const std::string& objectPath)
{
    BMCWEB_LOG_DEBUG("Get CPU UniqueIdentifier");
    dbus::utility::getProperty<std::string>(
        service, objectPath,
        "xyz.openbmc_project.Inventory.Decorator.UniqueIdentifier",
        "UniqueIdentifier",
        [asyncResp](const boost::system::error_code& ec,
                    const std::string& id) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("Failed to read cpu unique id: {}", ec);
                messages::internalError(asyncResp->res);
                return;
            }
            asyncResp->res
                .jsonValue["ProcessorId"]["ProtectedIdentificationNumber"] = id;
        });
}

inline void handleProcessorSubtree(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& processorId,
    const std::function<
        void(const std::string& objectPath,
             const dbus::utility::MapperServiceMap& serviceMap)>& callback,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("DBUS response error: {}", ec);
        messages::internalError(asyncResp->res);
        return;
    }
    for (const auto& [objectPath, serviceMap] : subtree)
    {
        // Ignore any objects which don't end with our desired cpu name
        sdbusplus::message::object_path path(objectPath);
        if (path.filename() == processorId)
        {
            // Filter out objects that don't have the CPU-specific
            // interfaces to make sure we can return 404 on non-CPUs
            // (e.g. /redfish/../Processors/dimm0)
            for (const auto& [serviceName, interfaceList] : serviceMap)
            {
                if (std::ranges::find_first_of(interfaceList,
                                               processorInterfaces) !=
                    interfaceList.end())
                {
                    // Process the first object which matches cpu name and
                    // required interfaces, and potentially ignore any other
                    // matching objects. Assume all interfaces we want to
                    // process must be on the same object path.

                    callback(objectPath, serviceMap);
                    return;
                }
            }
        }
    }
    messages::resourceNotFound(asyncResp->res, "Processor", processorId);
}

/**
 * Find the D-Bus object representing the requested Processor, and call the
 * handler with the results. If matching object is not found, add 404 error to
 * response and don't call the handler.
 *
 * @param[in,out]   asyncResp       Async HTTP response.
 * @param[in]       processorId     Redfish Processor Id.
 * @param[in]       callback        Callback to continue processing request upon
 *                                  successfully finding object.
 */
inline void getProcessorObject(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& processorId,
    std::function<void(const std::string& objectPath,
                       const dbus::utility::MapperServiceMap& serviceMap)>&&
        callback)
{
    BMCWEB_LOG_DEBUG("Get available system processor resources.");

    // GetSubTree on all interfaces which provide info about a Processor
    constexpr std::array<std::string_view, 9> interfaces = {
        "xyz.openbmc_project.Common.UUID",
        "xyz.openbmc_project.Inventory.Decorator.Asset",
        "xyz.openbmc_project.Inventory.Decorator.Revision",
        "xyz.openbmc_project.Inventory.Item.Cpu",
        "xyz.openbmc_project.Inventory.Decorator.LocationCode",
        "xyz.openbmc_project.Inventory.Item.Accelerator",
        "xyz.openbmc_project.Control.Processor.CurrentOperatingConfig",
        "xyz.openbmc_project.Inventory.Decorator.UniqueIdentifier",
        "xyz.openbmc_project.Control.Power.Throttle"};
    dbus::utility::getSubTree(
        "/xyz/openbmc_project/inventory", 0, interfaces,
        [asyncResp, processorId, callback{std::move(callback)}](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreeResponse& subtree) {
            handleProcessorSubtree(asyncResp, processorId, callback, ec,
                                   subtree);
        });
}

inline void getProcessorData(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& processorId, const std::string& objectPath,
    const dbus::utility::MapperServiceMap& serviceMap)
{
    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/Processor/Processor.json>; rel=describedby");
    asyncResp->res.jsonValue["@odata.type"] = "#Processor.v1_18_0.Processor";
    asyncResp->res.jsonValue["@odata.id"] =
        boost::urls::format("/redfish/v1/Systems/{}/Processors/{}",
                            BMCWEB_REDFISH_SYSTEM_URI_NAME, processorId);

    // Add MemorySummary with link to MemoryMetrics
    asyncResp->res.jsonValue["MemorySummary"]["Metrics"]["@odata.id"] =
        boost::urls::format(
            "/redfish/v1/Systems/{}/Processors/{}/MemorySummary/MemoryMetrics",
            BMCWEB_REDFISH_SYSTEM_URI_NAME, processorId);

    for (const auto& [serviceName, interfaceList] : serviceMap)
    {
        for (const auto& interface : interfaceList)
        {
            if (interface == "xyz.openbmc_project.Inventory.Decorator.Asset")
            {
                getCpuAssetData(asyncResp, serviceName, objectPath);
            }
            else if (interface ==
                     "xyz.openbmc_project.Inventory.Decorator.Revision")
            {
                getCpuRevisionData(asyncResp, serviceName, objectPath);
            }
            else if (interface == "xyz.openbmc_project.Inventory.Item.Cpu")
            {
                getCpuDataByService(asyncResp, processorId, serviceName,
                                    objectPath);
            }
            else if (interface ==
                     "xyz.openbmc_project.Inventory.Item.Accelerator")
            {
                getAcceleratorDataByService(asyncResp, processorId, serviceName,
                                            objectPath);
            }
            else if (
                interface ==
                "xyz.openbmc_project.Control.Processor.CurrentOperatingConfig")
            {
                getCpuConfigData(asyncResp, processorId, serviceName,
                                 objectPath);
            }
            else if (interface ==
                     "xyz.openbmc_project.Inventory.Decorator.LocationCode")
            {
                getCpuLocationCode(asyncResp, serviceName, objectPath);
            }
            else if (interface == "xyz.openbmc_project.Common.UUID")
            {
                getProcessorUUID(asyncResp, serviceName, objectPath);
            }
            else if (interface ==
                     "xyz.openbmc_project.Inventory.Decorator.UniqueIdentifier")
            {
                getCpuUniqueId(asyncResp, serviceName, objectPath);
            }
            else if (interface == "xyz.openbmc_project.Control.Power.Throttle")
            {
                getThrottleProperties(asyncResp, serviceName, objectPath);
            }
            else if (interface == "xyz.openbmc_project.Association.Definitions")
            {
                getLocationIndicatorActive(asyncResp, objectPath);
            }
        }
    }
}

/**
 * Handle the PATCH operation of the AppliedOperatingConfig property. Do basic
 * validation of the input data, and then set the D-Bus property.
 *
 * @param[in,out]   resp            Async HTTP response.
 * @param[in]       processorId     Processor's Id.
 * @param[in]       appliedConfigUri    New property value to apply.
 * @param[in]       cpuObjectPath   Path of CPU object to modify.
 * @param[in]       serviceMap      Service map for CPU object.
 */
inline void patchAppliedOperatingConfig(
    const std::shared_ptr<bmcweb::AsyncResp>& resp,
    const std::string& processorId, const boost::urls::url& appliedConfigUri,
    const std::string& cpuObjectPath,
    const dbus::utility::MapperServiceMap& serviceMap)
{
    // Check that the property even exists by checking for the interface
    const std::string* controlService = nullptr;
    for (const auto& [serviceName, interfaceList] : serviceMap)
    {
        if (std::ranges::find(interfaceList,
                              "xyz.openbmc_project.Control.Processor."
                              "CurrentOperatingConfig") != interfaceList.end())
        {
            controlService = &serviceName;
            break;
        }
    }

    if (controlService == nullptr)
    {
        messages::internalError(resp->res);
        return;
    }

    // Check that the config URI is a child of the cpu URI being patched.
    boost::urls::url expectedPrefix = boost::urls::format(
        "/redfish/v1/Systems/{}/Processors/{}/OperatingConfigs/",
        BMCWEB_REDFISH_SYSTEM_URI_NAME, processorId);
    if (!appliedConfigUri.buffer().starts_with(expectedPrefix.buffer()) ||
        expectedPrefix.size() == appliedConfigUri.size())
    {
        messages::propertyValueIncorrect(resp->res, "AppliedOperatingConfig",
                                         appliedConfigUri);
        return;
    }

    // Generate the D-Bus path of the OperatingConfig object, by assuming it's a
    // direct child of the CPU object.
    // Strip the expectedPrefix from the config URI to get the "filename", and
    // append to the CPU's path.
    std::string configBaseName =
        appliedConfigUri.buffer().substr(expectedPrefix.buffer().size());
    sdbusplus::message::object_path configPath(cpuObjectPath);
    configPath /= configBaseName;

    BMCWEB_LOG_INFO("Setting config to {}", configPath.str);

    // Set the property, with handler to check error responses
    setDbusProperty(
        resp, "AppliedOperatingConfig", *controlService, cpuObjectPath,
        "xyz.openbmc_project.Control.Processor.CurrentOperatingConfig",
        "AppliedConfig", configPath);
}

inline void handleProcessorHead(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& /* systemName */, const std::string& /* processorId */)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/Processor/Processor.json>; rel=describedby");
}

inline void handleProcessorCollectionHead(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& /* systemName */)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/ProcessorCollection/ProcessorCollection.json>; rel=describedby");
}

inline void handleProcessorGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& processorId)
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

    getProcessorObject(
        asyncResp, processorId,
        std::bind_front(getProcessorData, asyncResp, processorId));
}

inline void doPatchProcessor(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& processorId,
    const std::optional<boost::urls::url>& appliedConfigUri,
    std::optional<bool> locationIndicatorActive, const std::string& objectPath,
    const dbus::utility::MapperServiceMap& serviceMap)
{
    if (appliedConfigUri)
    {
        patchAppliedOperatingConfig(asyncResp, processorId, *appliedConfigUri,
                                    objectPath, serviceMap);
    }

    if (locationIndicatorActive)
    {
        // Utility function handles reporting errors
        setLocationIndicatorActive(asyncResp, objectPath,
                                   *locationIndicatorActive);
    }
}

inline void handleProcessorPatch(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& processorId)
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

    std::optional<std::string> appliedConfigStr;
    std::optional<bool> locationIndicatorActive;
    if (!json_util::readJsonPatch(
            req, asyncResp->res,                                  //
            "AppliedOperatingConfig/@odata.id", appliedConfigStr, //
            "LocationIndicatorActive", locationIndicatorActive    //
            ))
    {
        return;
    }
    std::optional<boost::urls::url> appliedConfigUri;
    if (appliedConfigStr)
    {
        boost::system::result<boost::urls::url> parsed =
            boost::urls::parse_relative_ref(*appliedConfigStr);
        if (!parsed)
        {
            messages::propertyValueFormatError(
                asyncResp->res, "AppliedOperatingConfig", *appliedConfigStr);
            return;
        }
        appliedConfigUri = std::move(*parsed);
    }

    // Check for 404 and find matching D-Bus object, then run
    // property patch handlers if that all succeeds.
    getProcessorObject(
        asyncResp, processorId,
        std::bind_front(doPatchProcessor, asyncResp, processorId,
                        appliedConfigUri, locationIndicatorActive));
}

inline void handleProcessorCollectionGet(
    App& app, const crow::Request& req,
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
        "</redfish/v1/JsonSchemas/ProcessorCollection/ProcessorCollection.json>; rel=describedby");

    asyncResp->res.jsonValue["@odata.type"] =
        "#ProcessorCollection.ProcessorCollection";
    asyncResp->res.jsonValue["Name"] = "Processor Collection";

    asyncResp->res.jsonValue["@odata.id"] = std::format(
        "/redfish/v1/Systems/{}/Processors", BMCWEB_REDFISH_SYSTEM_URI_NAME);

    collection_util::getCollectionMembers(
        asyncResp,
        boost::urls::format("/redfish/v1/Systems/{}/Processors",
                            BMCWEB_REDFISH_SYSTEM_URI_NAME),
        processorInterfaces, "/xyz/openbmc_project/inventory");
}

inline void requestRoutesProcessor(App& app)
{
    /**
     * Functions triggers appropriate requests on DBus
     */
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Processors/")
        .privileges(redfish::privileges::headProcessorCollection)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handleProcessorCollectionHead, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Processors/")
        .privileges(redfish::privileges::getProcessorCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleProcessorCollectionGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Processors/<str>/")
        .privileges(redfish::privileges::headProcessor)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handleProcessorHead, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Processors/<str>/")
        .privileges(redfish::privileges::getProcessor)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleProcessorGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Processors/<str>/")
        .privileges(redfish::privileges::patchProcessor)
        .methods(boost::beast::http::verb::patch)(
            std::bind_front(handleProcessorPatch, std::ref(app)));
}

} // namespace redfish
