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
#include "dbus_singleton.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/processor.hpp"
#include "led.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/collection.hpp"
#include "utils/dbus_utils.hpp"
#include "utils/hw_isolation.hpp"
#include "utils/json_utils.hpp"
#include "utils/name_utils.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/message/native_types.hpp>
#include <sdbusplus/unpack_properties.hpp>
#include <sdbusplus/utility/dedup_variant.hpp>

#include <array>
#include <functional>
#include <limits>
#include <memory>
#include <ranges>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace redfish
{

// Interfaces which imply a D-Bus object represents a Processor
constexpr std::array<std::string_view, 2> processorInterfaces = {
    "xyz.openbmc_project.Inventory.Item.Cpu",
    "xyz.openbmc_project.Inventory.Item.Accelerator"};

// Interfaces which imply a D-Bus object represents a Processor Core
constexpr std::array<std::string_view, 1> procCoreInterfaces = {
    "xyz.openbmc_project.Inventory.Item.CpuCore"};

/**
 * @brief Workaround to handle DCM (Dual-Chip Module) package for Redfish
 *
 * Make sure processor modeled as dual chip module ("dcmN-cpuN"),
 * If yes then, replace Redfish processor id as "dcmN/cpuN" and check with
 * given object path because Redfish does not support chip module concept.
 *
 * @param[in] processorId - The Redfish processor Id
 * @param[in] objectPath  - The D-Bus object path that contain the processor
 *                          segment
 *
 * @return true if matched with the given object path else false.
 *
 * @note Inventory modeled as "dcmN/cpuN" to support DCM so wherever using
 *       Redfish processor id as "dcmN-cpuN" then this function (it support
 *       both SCM and DCM) can be used for the inventory processor object
 *       path validation.
 */
inline bool
    isProcObjectMatched(const std::string& processorId,
                        const sdbusplus::message::object_path& objectPath)
{
    bool isMatched = false;
    if (processorId.find("dcm") != std::string::npos)
    {
        std::size_t delimiterPos = processorId.find('-');
        if (delimiterPos != std::string::npos)
        {
            std::string procParent = processorId.substr(0, delimiterPos);
            std::string procId = processorId.substr(delimiterPos + 1,
                                                    processorId.length());

            if ((objectPath.parent_path().filename() == procParent) &&
                (objectPath.filename() == procId))
            {
                isMatched = true;
            }
        }
    }
    else
    {
        if (objectPath.filename() == processorId)
        {
            isMatched = true;
        }
    }
    return isMatched;
}

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
    sdbusplus::asio::getProperty<std::string>(
        *crow::connections::systemBus, service, objPath,
        "xyz.openbmc_project.Common.UUID", "UUID",
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
    asyncResp->res.jsonValue["Status"]["State"] = "Enabled";
    asyncResp->res.jsonValue["Status"]["Health"] = "OK";

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
                    asyncResp->res.jsonValue["Status"]["State"] = "Absent";
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
                    asyncResp->res.jsonValue["Status"]["Health"] = "Critical";
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
                        "0x" + intToHexString(*value, 4);
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
                        "0x" + intToHexString(*value, 4);
                }
            }
            else if (property.first == "Id")
            {
                const uint64_t* value = std::get_if<uint64_t>(&property.second);
                if (value != nullptr && *value != 0)
                {
                    asyncResp->res
                        .jsonValue["ProcessorId"]["IdentificationRegisters"] =
                        "0x" + intToHexString(*value, 16);
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
                        "0x" + intToHexString(*value, 8);
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
                        "0x" + intToHexString(*value, 4);
                }
            }
        }
    }
}

inline void getCpuDataByService(std::shared_ptr<bmcweb::AsyncResp> asyncResp,
                                const std::string& cpuId,
                                const std::string& service,
                                const std::string& objPath)
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
        asyncResp->res.jsonValue["ProcessorType"] = "CPU";

        bool slotPresent = false;
        std::string corePath = objPath + "/core";
        size_t totalCores = 0;
        for (const auto& object : dbusData)
        {
            if (object.first.str == objPath)
            {
                getCpuDataByInterface(asyncResp, object.second);
            }
            else if (object.first.str.starts_with(corePath))
            {
                for (const auto& interface : object.second)
                {
                    if (interface.first == "xyz.openbmc_project.Inventory.Item")
                    {
                        for (const auto& property : interface.second)
                        {
                            if (property.first == "Present")
                            {
                                const bool* present =
                                    std::get_if<bool>(&property.second);
                                if (present != nullptr)
                                {
                                    if (*present)
                                    {
                                        slotPresent = true;
                                        totalCores++;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        // In getCpuDataByInterface(), state and health are set
        // based on the present and functional status. If core
        // count is zero, then it has a higher precedence.
        if (slotPresent)
        {
            if (totalCores == 0)
            {
                // Slot is not populated, set status end return
                asyncResp->res.jsonValue["Status"]["State"] = "Absent";
                asyncResp->res.jsonValue["Status"]["Health"] = "OK";
            }
            asyncResp->res.jsonValue["TotalCores"] = totalCores;
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
inline processor::ThrottleCause
    dbusToRfThrottleCause(const std::string& dbusSource)
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

inline void
    readThrottleProperties(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
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

inline void
    getThrottleProperties(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& service,
                          const std::string& objectPath)
{
    BMCWEB_LOG_DEBUG("Get processor throttle resources");

    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, service, objectPath,
        "xyz.openbmc_project.Control.Power.Throttle",
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
    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, service, objPath,
        "xyz.openbmc_project.Inventory.Decorator.Asset",
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
    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, service, objPath,
        "xyz.openbmc_project.Inventory.Decorator.Revision",
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
            dbus_utils::UnpackErrorPrinter(), properties, "Version", version);

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
    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, service, objPath, "",
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
        asyncResp->res.jsonValue["ProcessorType"] = "Accelerator";
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
inline void
    getCpuConfigData(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     const std::string& cpuId, const std::string& service,
                     const std::string& objPath)
{
    BMCWEB_LOG_INFO("Getting CPU operating configs for {}", cpuId);

    // First, GetAll CurrentOperatingConfig properties on the object
    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, service, objPath,
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
                "/redfish/v1/Systems/system/Processors/{}/OperatingConfigs",
                cpuId);
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
                "/redfish/v1/Systems/system/Processors/{}/OperatingConfigs/{}",
                cpuId, dbusPath.substr(baseNamePos + 1));
            json["AppliedOperatingConfig"] = std::move(appliedOperatingConfig);

            // Once we found the current applied config, queue another
            // request to read the base freq core ids out of that
            // config.
            sdbusplus::asio::getProperty<BaseSpeedPrioritySettingsProperty>(
                *crow::connections::systemBus, service, dbusPath,
                "xyz.openbmc_project.Inventory.Item.Cpu.OperatingConfig",
                "BaseSpeedPrioritySettings",
                [asyncResp](
                    const boost::system::error_code& ec2,
                    const BaseSpeedPrioritySettingsProperty& baseSpeedList) {
                if (ec2)
                {
                    BMCWEB_LOG_WARNING("D-Bus Property Get error: {}", ec2);
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
    sdbusplus::asio::getProperty<std::string>(
        *crow::connections::systemBus, service, objPath,
        "xyz.openbmc_project.Inventory.Decorator.LocationCode", "LocationCode",
        [objPath, asyncResp{std::move(asyncResp)}](
            const boost::system::error_code& ec, const std::string& property) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG("DBUS response error");
            messages::internalError(asyncResp->res);
            return;
        }

        asyncResp->res.jsonValue["Location"]["PartLocation"]["ServiceLabel"] =
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
    sdbusplus::asio::getProperty<std::string>(
        *crow::connections::systemBus, service, objectPath,
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

/**
 * Find the D-Bus object representing the requested Processor, and call the
 * handler with the results. If matching object is not found, add 404 error to
 * response and don't call the handler.
 *
 * @param[in,out]   resp            Async HTTP response.
 * @param[in]       processorId     Redfish Processor Id.
 * @param[in]       handler         Callback to continue processing request upon
 *                                  successfully finding object.
 */
template <typename Handler>
inline void getProcessorObject(const std::shared_ptr<bmcweb::AsyncResp>& resp,
                               const std::string& processorId,
                               Handler&& handler)
{
    BMCWEB_LOG_DEBUG("Get available system processor resources.");

    // GetSubTree on all interfaces which provide info about a Processor
    constexpr std::array<std::string_view, 10> interfaces = {
        "xyz.openbmc_project.Common.UUID",
        "xyz.openbmc_project.Inventory.Decorator.Asset",
        "xyz.openbmc_project.Inventory.Decorator.Revision",
        "xyz.openbmc_project.Inventory.Item.Cpu",
        "xyz.openbmc_project.Inventory.Decorator.LocationCode",
        "xyz.openbmc_project.Inventory.Item.Accelerator",
        "xyz.openbmc_project.Control.Processor.CurrentOperatingConfig",
        "xyz.openbmc_project.Inventory.Decorator.UniqueIdentifier",
        "xyz.openbmc_project.Control.Power.Throttle",
        "xyz.openbmc_project.Association.Definitions"};
    dbus::utility::getSubTree(
        "/xyz/openbmc_project/inventory", 0, interfaces,
        [resp, processorId, handler = std::forward<Handler>(handler)](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreeResponse& subtree) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG("DBUS response error: {}", ec);
            messages::internalError(resp->res);
            return;
        }
        for (const auto& [objectPath, serviceMap] : subtree)
        {
            // Ignore any objects which don't end with our desired cpu name
            if (!isProcObjectMatched(
                    processorId, sdbusplus::message::object_path(objectPath)))
            {
                continue;
            }

            // Filter out objects that don't have the CPU-specific
            // interfaces to make sure we can return 404 on non-CPUs
            // (e.g. /redfish/../Processors/dimm0)
            for (const auto& serviceEntry : serviceMap)
            {
                const auto& interfaceList = serviceEntry.second;
                if (std::ranges::find_first_of(interfaceList,
                                               processorInterfaces) !=
                    std::end(interfaceList))
                {
                    // Process the first object which does match our cpu name
                    // and required interfaces, and potentially ignore any other
                    // matching objects. Assume all interfaces we want to
                    // process must be on the same object path.

                    handler(objectPath, serviceMap);
                    const dbus::utility::MapperServiceMap& serviceMatch = {
                        serviceEntry};
                    name_util::getPrettyName(resp, objectPath, serviceMatch,
                                             "/Name"_json_pointer);
                    return;
                }
            }
        }
        messages::resourceNotFound(resp->res, "Processor", processorId);
    });
}

inline void
    getProcessorData(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     const std::string& processorId,
                     const std::string& objectPath,
                     const dbus::utility::MapperServiceMap& serviceMap)
{
    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/Processor/Processor.json>; rel=describedby");
    asyncResp->res.jsonValue["@odata.type"] = "#Processor.v1_18_0.Processor";
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Systems/system/Processors/{}", processorId);
    asyncResp->res.jsonValue["SubProcessors"]["@odata.id"] =
        boost::urls::format(
            "/redfish/v1/Systems/system/Processors/{}/SubProcessors",
            processorId);

    for (const auto& [serviceName, interfaceList] : serviceMap)
    {
        bool assetInterface = false;
        bool cpuInterface = false;
        bool associationInterface = false;
        bool revisionInterface = false;
        bool locationCodeInterface = false;
        for (const auto& interface : interfaceList)
        {
            if (interface == "xyz.openbmc_project.Inventory.Decorator.Asset")
            {
                assetInterface = true;
            }
            else if (interface ==
                     "xyz.openbmc_project.Inventory.Decorator.Revision")
            {
                revisionInterface = true;
            }
            else if (interface == "xyz.openbmc_project.Inventory.Item.Cpu")
            {
                cpuInterface = true;
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
                locationCodeInterface = true;
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
                associationInterface = true;
            }
        }

        if (cpuInterface && assetInterface)
        {
            getCpuAssetData(asyncResp, serviceName, objectPath);
        }

        if (cpuInterface && revisionInterface)
        {
            getCpuRevisionData(asyncResp, serviceName, objectPath);
        }

        if (cpuInterface && locationCodeInterface)
        {
            getCpuLocationCode(asyncResp, serviceName, objectPath);
        }

        if (cpuInterface && associationInterface)
        {
            getLocationIndicatorActive(asyncResp, objectPath);
        }
    }
}

inline void handleProcessorPaths(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& processorId,
    const std::function<void(const std::string& cpuPath)>& handler,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreePathsResponse& subTreePaths)
{
    if (ec)
    {
        // No processor objects found by mapper
        if (ec.value() == boost::system::errc::io_error)
        {
            messages::resourceNotFound(
                asyncResp->res, "#Processor.v1_18_0.Processor", processorId);
            return;
        }

        BMCWEB_LOG_ERROR("DBUS response error: {}", ec.value());
        messages::internalError(asyncResp->res);
        return;
    }

    for (const std::string& cpuPath : subTreePaths)
    {
        if (!isProcObjectMatched(processorId,
                                 sdbusplus::message::object_path(cpuPath)))
        {
            continue;
        }

        handler(cpuPath);
        return;
    }

    // Object not found
    messages::resourceNotFound(asyncResp->res, "#Processor.v1_18_0.Processor",
                               processorId);
}

inline void
    getProcessorPaths(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const std::string& processorId,
                      std::function<void(const std::string& cpuPath)>&& handler)
{
    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Inventory.Item.Cpu"};
    dbus::utility::getSubTreePaths(
        "/xyz/openbmc_project/inventory", 0, interfaces,
        [asyncResp, processorId, handler{std::move(handler)}](
            const boost::system::error_code& ec,
            const std::vector<std::string>& subTreePaths) {
        handleProcessorPaths(asyncResp, processorId, handler, ec, subTreePaths);
    });
}

inline void afterGetCpuCoreDataByService(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& objPath, const boost::system::error_code& ec,
    const dbus::utility::ManagedObjectType& dbusData)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("DBUS response error, ec: {}", ec.value());
        messages::internalError(asyncResp->res);
        return;
    }

    for (const auto& [path, interfaces] : dbusData)
    {
        if (path != objPath)
        {
            continue;
        }

        bool present = true;
        bool functional = true;
        bool available = true;

        for (const auto& [interface, properties] : interfaces)
        {
            if (interface ==
                "xyz.openbmc_project.State.Decorator.OperationalStatus")
            {
                for (const auto& [propName, propValue] : properties)
                {
                    if (propName == "Functional")
                    {
                        const bool* value = std::get_if<bool>(&propValue);
                        if (value == nullptr)
                        {
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        functional = *value;
                    }
                }
            }
            else if (interface == "xyz.openbmc_project.Inventory.Item")
            {
                for (const auto& [propName, propValue] : properties)
                {
                    if (propName == "Present")
                    {
                        const bool* value = std::get_if<bool>(&propValue);
                        if (value == nullptr)
                        {
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        present = *value;
                    }
                }
            }
            else if (interface ==
                     "xyz.openbmc_project.State.Decorator.Availability")
            {
                for (const auto& [propName, propValue] : properties)
                {
                    if (propName == "Available")
                    {
                        const bool* value = std::get_if<bool>(&propValue);
                        if (value == nullptr)
                        {
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        available = *value;
                    }
                }
            }
        }

        if (!available)
        {
            asyncResp->res.jsonValue["Status"]["State"] = "UnavailableOffline";
        }
        else if (!present)
        {
            asyncResp->res.jsonValue["Status"]["State"] = "Absent";
        }

        if (!functional)
        {
            asyncResp->res.jsonValue["Status"]["Health"] = "Critical";
        }
#ifdef BMCWEB_ENABLE_HW_ISOLATION
        // Check for the hardware status event
        hw_isolation_utils::getHwIsolationStatus(asyncResp, objPath);
#endif // end of BMCWEB_ENABLE_HW_ISOLATION
    }
}

inline void
    getCpuCoreDataByService(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& service,
                            const std::string& objPath)
{
    BMCWEB_LOG_DEBUG("Get available system cpu core resources by service.");

    asyncResp->res.jsonValue["Status"]["State"] = "Enabled";
    asyncResp->res.jsonValue["Status"]["Health"] = "OK";

    dbus::utility::getManagedObjects(
        service,
        sdbusplus::message::object_path("/xyz/openbmc_project/inventory"),
        std::bind_front(afterGetCpuCoreDataByService, asyncResp, objPath));
}

inline void
    getSubProcessorData(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& processorId,
                        const std::string& coreId,
                        const boost::system::error_code& ec,
                        const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec)
    {
        // No processor objects found by mapper
        if (ec.value() == boost::system::errc::io_error)
        {
            messages::resourceNotFound(
                asyncResp->res, "#Processor.v1_18_0.Processor", processorId);
            return;
        }

        BMCWEB_LOG_ERROR("DBUS response error, ec: {}", ec.value());
        messages::internalError(asyncResp->res);
        return;
    }

    for (const auto& object : subtree)
    {
        if (sdbusplus::message::object_path(object.first).filename() != coreId)
        {
            continue;
        }

        asyncResp->res.jsonValue["@odata.type"] =
            "#Processor.v1_18_0.Processor";
        asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
            "/redfish/v1/Systems/system/Processors/{}/SubProcessors/{}",
            processorId, coreId);
        asyncResp->res.jsonValue["Name"] = "SubProcessor";
        asyncResp->res.jsonValue["Id"] = coreId;

        for (const auto& service : object.second)
        {
            getCpuCoreDataByService(asyncResp, service.first, object.first);
            name_util::getPrettyName(asyncResp, object.first,
                                     std::ref(object.second),
                                     "/Name"_json_pointer);
            break;
        }
        return;
    }

    // Object not found
    messages::resourceNotFound(asyncResp->res, "#Processor.v1_18_0.Processor",
                               coreId);
}

inline void getSubProcessorMembers(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& processorId, const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreePathsResponse& subTreePaths)
{
    if (ec)
    {
        // No processor objects found by mapper
        if (ec.value() == boost::system::errc::io_error)
        {
            messages::resourceNotFound(
                asyncResp->res, "#Processor.v1_18_0.Processor", processorId);
            return;
        }

        BMCWEB_LOG_ERROR("DBUS response error: {}", ec.value());
        messages::internalError(asyncResp->res);
        return;
    }

    asyncResp->res.jsonValue["@odata.type"] =
        "#ProcessorCollection.ProcessorCollection";
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Systems/system/Processors/{}/SubProcessors", processorId);
    asyncResp->res.jsonValue["Name"] = "SubProcessor Collection";
    nlohmann::json& members = asyncResp->res.jsonValue["Members"];
    members = nlohmann::json::array();
    std::string subProcessorsPath =
        (boost::urls::format(
             "/redfish/v1/Systems/system/Processors/{}/SubProcessors",
             processorId)
             .buffer());
    for (const std::string& corePath : subTreePaths)
    {
        nlohmann::json::object_t item;
        item["@odata.id"] = boost::urls::format(
            "{}/{}", subProcessorsPath,
            sdbusplus::message::object_path(corePath).filename());
        members.emplace_back(std::move(item));
    }
    asyncResp->res.jsonValue["Members@odata.count"] = members.size();
}

/**
 * Request all the properties for the given D-Bus object and fill out the
 * related entries in the Redfish OperatingConfig response.
 *
 * @param[in,out]   asyncResp       Async HTTP response.
 * @param[in]       service     D-Bus service name to query.
 * @param[in]       objPath     D-Bus object to query.
 */
inline void
    getOperatingConfigData(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const std::string& service,
                           const std::string& objPath)
{
    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, service, objPath,
        "xyz.openbmc_project.Inventory.Item.Cpu.OperatingConfig",
        [asyncResp](const boost::system::error_code& ec,
                    const dbus::utility::DBusPropertiesMap& properties) {
        if (ec)
        {
            BMCWEB_LOG_WARNING("D-Bus error: {}, {}", ec, ec.message());
            messages::internalError(asyncResp->res);
            return;
        }

        const size_t* availableCoreCount = nullptr;
        const uint32_t* baseSpeed = nullptr;
        const uint32_t* maxJunctionTemperature = nullptr;
        const uint32_t* maxSpeed = nullptr;
        const uint32_t* powerLimit = nullptr;
        const TurboProfileProperty* turboProfile = nullptr;
        const BaseSpeedPrioritySettingsProperty* baseSpeedPrioritySettings =
            nullptr;

        const bool success = sdbusplus::unpackPropertiesNoThrow(
            dbus_utils::UnpackErrorPrinter(), properties, "AvailableCoreCount",
            availableCoreCount, "BaseSpeed", baseSpeed,
            "MaxJunctionTemperature", maxJunctionTemperature, "MaxSpeed",
            maxSpeed, "PowerLimit", powerLimit, "TurboProfile", turboProfile,
            "BaseSpeedPrioritySettings", baseSpeedPrioritySettings);

        if (!success)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        nlohmann::json& json = asyncResp->res.jsonValue;

        if (availableCoreCount != nullptr)
        {
            json["TotalAvailableCoreCount"] = *availableCoreCount;
        }

        if (baseSpeed != nullptr)
        {
            json["BaseSpeedMHz"] = *baseSpeed;
        }

        if (maxJunctionTemperature != nullptr)
        {
            json["MaxJunctionTemperatureCelsius"] = *maxJunctionTemperature;
        }

        if (maxSpeed != nullptr)
        {
            json["MaxSpeedMHz"] = *maxSpeed;
        }

        if (powerLimit != nullptr)
        {
            json["TDPWatts"] = *powerLimit;
        }

        if (turboProfile != nullptr)
        {
            nlohmann::json& turboArray = json["TurboProfile"];
            turboArray = nlohmann::json::array();
            for (const auto& [turboSpeed, coreCount] : *turboProfile)
            {
                nlohmann::json::object_t turbo;
                turbo["ActiveCoreCount"] = coreCount;
                turbo["MaxSpeedMHz"] = turboSpeed;
                turboArray.emplace_back(std::move(turbo));
            }
        }

        if (baseSpeedPrioritySettings != nullptr)
        {
            nlohmann::json& baseSpeedArray = json["BaseSpeedPrioritySettings"];
            baseSpeedArray = nlohmann::json::array();
            for (const auto& [baseSpeedMhz, coreList] :
                 *baseSpeedPrioritySettings)
            {
                nlohmann::json::object_t speed;
                speed["CoreCount"] = coreList.size();
                speed["CoreIDs"] = coreList;
                speed["BaseSpeedMHz"] = baseSpeedMhz;
                baseSpeedArray.emplace_back(std::move(speed));
            }
        }
    });
}

/**
 * Handle the D-Bus response from attempting to set the CPU's AppliedConfig
 * property. Main task is to translate error messages into Redfish errors.
 *
 * @param[in,out]   resp    HTTP response.
 * @param[in]       setPropVal  Value which we attempted to set.
 * @param[in]       ec      D-Bus response error code.
 * @param[in]       msg     D-Bus response message.
 */
inline void
    handleAppliedConfigResponse(const std::shared_ptr<bmcweb::AsyncResp>& resp,
                                const std::string& setPropVal,
                                const boost::system::error_code& ec,
                                const sdbusplus::message_t& msg)
{
    if (!ec)
    {
        BMCWEB_LOG_DEBUG("Set Property succeeded");
        return;
    }

    BMCWEB_LOG_DEBUG("Set Property failed: {}", ec);

    const sd_bus_error* dbusError = msg.get_error();
    if (dbusError == nullptr)
    {
        messages::internalError(resp->res);
        return;
    }

    // The asio error code doesn't know about our custom errors, so we have to
    // parse the error string. Some of these D-Bus -> Redfish translations are a
    // stretch, but it's good to try to communicate something vaguely useful.
    if (strcmp(dbusError->name,
               "xyz.openbmc_project.Common.Error.InvalidArgument") == 0)
    {
        // Service did not like the object_path we tried to set.
        messages::propertyValueIncorrect(
            resp->res, "AppliedOperatingConfig/@odata.id", setPropVal);
    }
    else if (strcmp(dbusError->name,
                    "xyz.openbmc_project.Common.Error.NotAllowed") == 0)
    {
        // Service indicates we can never change the config for this processor.
        messages::propertyNotWritable(resp->res, "AppliedOperatingConfig");
    }
    else if (strcmp(dbusError->name,
                    "xyz.openbmc_project.Common.Error.Unavailable") == 0)
    {
        // Service indicates the config cannot be changed right now, but maybe
        // in a different system state.
        messages::resourceInStandby(resp->res);
    }
    else
    {
        messages::internalError(resp->res);
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
    const std::string& processorId, const std::string& appliedConfigUri,
    const std::string& cpuObjectPath,
    const dbus::utility::MapperServiceMap& serviceMap)
{
    // Check that the property even exists by checking for the interface
    const std::string* controlService = nullptr;
    for (const auto& [serviceName, interfaceList] : serviceMap)
    {
        if (std::ranges::find(
                interfaceList,
                "xyz.openbmc_project.Control.Processor.CurrentOperatingConfig") !=
            interfaceList.end())
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
    std::string expectedPrefix("/redfish/v1/Systems/system/Processors/");
    expectedPrefix += processorId;
    expectedPrefix += "/OperatingConfigs/";
    if (!appliedConfigUri.starts_with(expectedPrefix) ||
        expectedPrefix.size() == appliedConfigUri.size())
    {
        messages::propertyValueIncorrect(
            resp->res, "AppliedOperatingConfig/@odata.id", appliedConfigUri);
        return;
    }

    // Generate the D-Bus path of the OperatingConfig object, by assuming it's a
    // direct child of the CPU object.
    // Strip the expectedPrefix from the config URI to get the "filename", and
    // append to the CPU's path.
    std::string configBaseName = appliedConfigUri.substr(expectedPrefix.size());
    sdbusplus::message::object_path configPath(cpuObjectPath);
    configPath /= configBaseName;

    BMCWEB_LOG_INFO("Setting config to {}", configPath.str);

    // Set the property, with handler to check error responses
    sdbusplus::asio::setProperty(
        *crow::connections::systemBus, *controlService, cpuObjectPath,
        "xyz.openbmc_project.Control.Processor.CurrentOperatingConfig",
        "AppliedConfig", configPath,
        [resp, appliedConfigUri](const boost::system::error_code& ec,
                                 const sdbusplus::message_t& msg) {
        handleAppliedConfigResponse(resp, appliedConfigUri, ec, msg);
    });
}

inline void
    handleProcessorHead(crow::App& app, const crow::Request& req,
                        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& /* systemName */,
                        const std::string& /* processorId */)
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

inline void
    setProcessorData(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     const dbus::utility::MapperServiceMap& serviceMap,
                     const std::string& objectPath,
                     bool locationIndicatorActive)
{
    for (const auto& [serviceName, interfaceList] : serviceMap)
    {
        bool cpuInterface = false;
        bool associationInterface = false;
        for (const auto& interface : interfaceList)
        {
            if (interface == "xyz.openbmc_project.Inventory.Item.Cpu")
            {
                cpuInterface = true;
            }
            else if (interface == "xyz.openbmc_project.Association.Definitions")
            {
                associationInterface = true;
            }
        }

        if (cpuInterface && associationInterface)
        {
            setLocationIndicatorActive(asyncResp, objectPath,
                                       locationIndicatorActive);
        }
    }
}

inline void afterSetProcessorObject(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& processorId, bool locationIndicatorActive,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec)
    {
        BMCWEB_LOG_DEBUG("DBUS response error {}", ec.value());
        messages::internalError(asyncResp->res);
        return;
    }
    for (const auto& [objectPath, serviceMap] : subtree)
    {
        // Ignore any objects which don't end with our desired cpu name
        if (!isProcObjectMatched(processorId,
                                 sdbusplus::message::object_path(objectPath)))
        {
            continue;
        }

        // Filter out objects that don't have the CPU-specific
        // interfaces to make sure we can return 404 on non-CPUs (e.g.
        // /redfish/../Processors/dimm0)
        for (const auto& [serviceName, interfaceList] : serviceMap)
        {
            if (std::ranges::find_first_of(interfaceList,
                                           processorInterfaces) !=
                std::end(interfaceList))
            {
                // Process the first object which does match our cpu name and
                // required interfaces, and potentially ignore any other
                // matching objects. Assume all interfaces we want to process
                // must be on the same object path.
                setProcessorData(asyncResp, serviceMap, objectPath,
                                 locationIndicatorActive);
                return;
            }
        }
    }
    messages::resourceNotFound(asyncResp->res, "#Processor.v1_18_0.Processor",
                               processorId);
}

/**
 * Find the D-Bus object representing the requested Processor, and call the
 * setProcessorData with the results. If matching object is not found, add 404
 * error to response and don't call the setProcessorData.
 *
 * @param[in,out]   asyncResp                       Async HTTP response.
 * @param[in]       processorId                     Redfish Processor Id.
 * @param[in]       locationIndicatorActive         Value of the property
 */
inline void
    setProcessorObject(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& processorId,
                       bool locationIndicatorActive)
{
    // GetSubTree on all interfaces which provide info about a Processor
    constexpr std::array<std::string_view, 3> interfaces{
        "xyz.openbmc_project.Inventory.Item.Cpu",
        "xyz.openbmc_project.Inventory.Item.Accelerator",
        "xyz.openbmc_project.Association.Definitions"};
    dbus::utility::getSubTree("/xyz/openbmc_project/inventory", 0, interfaces,
                              std::bind_front(afterSetProcessorObject,
                                              asyncResp, processorId,
                                              locationIndicatorActive));
}

inline void requestRoutesOperatingConfigCollection(App& app)
{
    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/<str>/Processors/<str>/OperatingConfigs/")
        .privileges(redfish::privileges::getOperatingConfigCollection)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemName, const std::string& cpuName) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }

        if constexpr (bmcwebEnableMultiHost)
        {
            // Option currently returns no systems.  TBD
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }

        if (systemName != "system")
        {
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }
        asyncResp->res.jsonValue["@odata.type"] =
            "#OperatingConfigCollection.OperatingConfigCollection";
        asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
            "/redfish/v1/Systems/system/Processors/{}/OperatingConfigs",
            cpuName);
        asyncResp->res.jsonValue["Name"] = "Operating Config Collection";

        // First find the matching CPU object so we know how to
        // constrain our search for related Config objects.
        const std::array<std::string_view, 1> interfaces = {
            "xyz.openbmc_project.Control.Processor.CurrentOperatingConfig"};
        dbus::utility::getSubTreePaths(
            "/xyz/openbmc_project/inventory", 0, interfaces,
            [asyncResp, cpuName](
                const boost::system::error_code& ec,
                const dbus::utility::MapperGetSubTreePathsResponse& objects) {
            if (ec)
            {
                BMCWEB_LOG_WARNING("D-Bus error: {}, {}", ec, ec.message());
                messages::internalError(asyncResp->res);
                return;
            }

            for (const std::string& object : objects)
            {
                if (!isProcObjectMatched(
                        cpuName, sdbusplus::message::object_path(object)))
                {
                    continue;
                }

                // Not expected that there will be multiple matching
                // CPU objects, but if there are just use the first
                // one.

                // Use the common search routine to construct the
                // Collection of all Config objects under this CPU.
                constexpr std::array<std::string_view, 1> interface{
                    "xyz.openbmc_project.Inventory.Item.Cpu.OperatingConfig"};
                collection_util::getCollectionMembers(
                    asyncResp,
                    boost::urls::format(
                        "/redfish/v1/Systems/system/Processors/{}/OperatingConfigs",
                        cpuName),
                    interface, object);
                return;
            }
        });
    });
}

inline void requestRoutesOperatingConfig(App& app)
{
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Systems/<str>/Processors/<str>/OperatingConfigs/<str>/")
        .privileges(redfish::privileges::getOperatingConfig)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemName, const std::string& cpuName,
                   const std::string& configName) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        if constexpr (bmcwebEnableMultiHost)
        {
            // Option currently returns no systems.  TBD
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }

        if (systemName != "system")
        {
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }
        // Ask for all objects implementing OperatingConfig so we can search
        // for one with a matching name
        constexpr std::array<std::string_view, 1> interfaces = {
            "xyz.openbmc_project.Inventory.Item.Cpu.OperatingConfig"};
        dbus::utility::getSubTree(
            "/xyz/openbmc_project/inventory", 0, interfaces,
            [asyncResp, cpuName, configName](
                const boost::system::error_code& ec,
                const dbus::utility::MapperGetSubTreeResponse& subtree) {
            if (ec)
            {
                BMCWEB_LOG_WARNING("D-Bus error: {}, {}", ec, ec.message());
                messages::internalError(asyncResp->res);
                return;
            }
            const std::string expectedEnding = cpuName + '/' + configName;
            for (const auto& [objectPath, serviceMap] : subtree)
            {
                if (!isProcObjectMatched(
                        cpuName, sdbusplus::message::object_path(objectPath)
                                     .parent_path()))
                {
                    continue;
                }
                // Ignore any configs without matching cpuX/configY
                if (!objectPath.ends_with(expectedEnding) || serviceMap.empty())
                {
                    continue;
                }

                nlohmann::json& json = asyncResp->res.jsonValue;
                json["@odata.type"] = "#OperatingConfig.v1_0_0.OperatingConfig";
                json["@odata.id"] = boost::urls::format(
                    "/redfish/v1/Systems/system/Processors/{}/OperatingConfigs/{}",
                    cpuName, configName);
                json["Name"] = "Processor Profile";
                json["Id"] = configName;

                // Just use the first implementation of the object - not
                // expected that there would be multiple matching
                // services
                getOperatingConfigData(asyncResp, serviceMap.begin()->first,
                                       objectPath);
                return;
            }
            messages::resourceNotFound(asyncResp->res, "OperatingConfig",
                                       configName);
        });
    });
}

inline void handleProcessors(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreePathsResponse& objects)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("DBUS response error {}", ec.value());
        messages::internalError(asyncResp->res);
        return;
    }
    nlohmann::json& members = asyncResp->res.jsonValue["Members"];
    members = nlohmann::json::array();

    for (const auto& object : objects)
    {
        sdbusplus::message::object_path path(object);
        std::string leaf;

        /**
         * @brief Workaround to handle DCM (Dual-Chip Module)
         *        package for Redfish
         *
         * Make sure processor modeled as dual chip module,
         * If yes then, replace redfish processor id as
         * "dcmN-cpuN" because redfish does not support chip
         * module concept.
         *
         * @note Inventory modeled as "dcmN/cpuN" so wherever
         *       using redfish processor id as "dcmN-cpuN" then
         *       that need to convert as "dcmN/cpuN" before
         *       validating the inventory processor object path
         */
        if (path.parent_path().filename().find("dcm") != std::string::npos)
        {
            leaf = std::format("{}-{}", path.parent_path().filename(),
                               path.filename());
        }
        else
        {
            leaf = path.filename();
        }

        if (leaf.empty())
        {
            continue;
        }

        nlohmann::json::object_t member;
        member["@odata.id"] = boost::urls::format(
            "/redfish/v1/Systems/system/Processors/{}", leaf);
        members.emplace_back(std::move(member));
    }
    asyncResp->res.jsonValue["Members@odata.count"] = members.size();
}

inline void requestRoutesProcessorCollection(App& app)
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
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemName) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        if constexpr (bmcwebEnableMultiHost)
        {
            // Option currently returns no systems.  TBD
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }

        if (systemName != "system")
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

        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Systems/system/Processors";

        dbus::utility::getSubTreePaths(
            "/xyz/openbmc_project/inventory", 0, processorInterfaces,
            std::bind_front(handleProcessors, asyncResp));
    });
}

inline void
    handleProcessorPatch(App& app, const crow::Request& req,
                         const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::string& systemName,
                         const std::string& processorId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    if constexpr (bmcwebEnableMultiHost)
    {
        // Option currently returns no systems.  TBD
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    if (systemName != "system")
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    std::optional<nlohmann::json> appliedConfigJson;
    std::optional<bool> locationIndicatorActive;
    if (!json_util::readJsonPatch(req, asyncResp->res, "AppliedOperatingConfig",
                                  appliedConfigJson, "LocationIndicatorActive",
                                  locationIndicatorActive))

    {
        return;
    }

    if (appliedConfigJson)
    {
        std::string appliedConfigUri;
        if (!json_util::readJson(*appliedConfigJson, asyncResp->res,
                                 "@odata.id", appliedConfigUri))
        {
            return;
        }
        // Check for 404 and find matching D-Bus object, then run
        // property patch handlers if that all succeeds.
        getProcessorObject(asyncResp, processorId,
                           std::bind_front(patchAppliedOperatingConfig,
                                           asyncResp, processorId,
                                           appliedConfigUri));
    }
    if (locationIndicatorActive)
    {
        setProcessorObject(asyncResp, processorId, *locationIndicatorActive);
    }
}

inline void requestRoutesProcessor(App& app)
{
    /**
     * Functions triggers appropriate requests on DBus
     */

    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Processors/<str>/")
        .privileges(redfish::privileges::headProcessor)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handleProcessorHead, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Processors/<str>/")
        .privileges(redfish::privileges::getProcessor)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemName,
                   const std::string& processorId) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        if constexpr (bmcwebEnableMultiHost)
        {
            // Option currently returns no systems.  TBD
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }
        if (systemName != "system")
        {
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }

        getProcessorObject(
            asyncResp, processorId,
            std::bind_front(getProcessorData, asyncResp, processorId));
    });

    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Processors/<str>/")
        .privileges(redfish::privileges::patchProcessor)
        .methods(boost::beast::http::verb::patch)(
            std::bind_front(handleProcessorPatch, std::ref(app)));
}

/**
 * @brief API used to process the Processor Core "Enabled" member which is
 *        patched to do appropriate action.
 *
 * @param[in] asyncResp - The redfish response to return.
 * @param[in] procObjPath - The parent processor object path.
 * @param[in] coreId - The patched Processor Core resource id.
 * @param[in] enabled - The patched "Enabled" member value.
 *
 * @return The redfish response in the given buffer.
 *
 * @note - The "Enabled" member of the Processor Core is used to enable
 *         (aka isolate) or disable (aka deisolate) the resource from the
 *         system boot so this function will call
 * "processHardwareIsolationReq" function which is used to handle the
 * resource isolation request.
 *       - The "Enabled" member of the Processor Core is mapped with
 *         "xyz.openbmc_project.Object.Enable::Enabled" dbus property.
 */
inline void
    patchCpuCoreMemberEnabled(const std::shared_ptr<bmcweb::AsyncResp>& resp,
                              const std::string& procObjPath,
                              const std::string& coreId, const bool enabled)
{
    redfish::hw_isolation_utils::processHardwareIsolationReq(
        resp, "Core", coreId, enabled,
        std::vector<std::string_view>(procCoreInterfaces.begin(),
                                      procCoreInterfaces.end()),
        procObjPath);
}

/**
 * @brief API used to process the Processor Core members which are tried to
 *        patch.
 *
 * @param[in] req - The redfish patched request to identify the patched
 * members
 * @param[in] asyncResp - The redfish response to return.
 * @param[in] processorId - The patched Core Processor resource id (unused
 * now)
 * @param[in] coreId - The patched Processor Core resource id.
 *
 * @return The redfish response in the given buffer.
 *
 * @note This function will call the appropriate function to handle the
 * patched members of the Processor Core.
 */
inline void
    patchCpuCoreMembers(const crow::Request& req,
                        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& processorId,
                        const std::string& coreId)
{
    std::optional<bool> enabled;

    if (!json_util::readJsonPatch(req, asyncResp->res, "Enabled", enabled))
    {
        return;
    }

    auto callback = [asyncResp, coreId, enabled](const std::string& cpuPath) {
        // Handle patched Enabled Redfish property
        if (enabled.has_value())
        {
            patchCpuCoreMemberEnabled(asyncResp, cpuPath, coreId, *enabled);
        }
    };

    getProcessorPaths(asyncResp, processorId, std::move(callback));
}

inline void
    handleSubProcessorGet(App& app, const crow::Request& req,
                          const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& systemName,
                          const std::string& processorId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    if constexpr (bmcwebEnableMultiHost)
    {
        // Option currently returns no systems.  TBD
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    if (systemName != "system")
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    getProcessorPaths(asyncResp, processorId,
                      [asyncResp, processorId](const std::string& cpuPath) {
        dbus::utility::getSubTreePaths(
            cpuPath, 0, procCoreInterfaces,
            std::bind_front(getSubProcessorMembers, asyncResp, processorId));
    });
}

inline void requestRoutesSubProcessors(App& app)
{
    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/<str>/Processors/<str>/SubProcessors")
        .privileges(redfish::privileges::getProcessorCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleSubProcessorGet, std::ref(app)));
}

inline void handleSubProcessorCoreGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& processorId,
    const std::string& coreId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    if constexpr (bmcwebEnableMultiHost)
    {
        // Option currently returns no systems.  TBD
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    if (systemName != "system")
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    getProcessorPaths(
        asyncResp, processorId,
        [asyncResp, processorId, coreId](const std::string& cpuPath) {
        dbus::utility::getSubTree(cpuPath, 0, procCoreInterfaces,
                                  std::bind_front(getSubProcessorData,
                                                  asyncResp, processorId,
                                                  coreId));
    });
}

inline void requestRoutesSubProcessorsCore(App& app)
{
    BMCWEB_ROUTE(
        app, "/redfish/v1/Systems/<str>/Processors/<str>/SubProcessors/<str>")
        .privileges(redfish::privileges::getProcessor)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleSubProcessorCoreGet, std::ref(app)));

    BMCWEB_ROUTE(
        app, "/redfish/v1/Systems/system/Processors/<str>/SubProcessors/<str>")
        .privileges(redfish::privileges::patchProcessor)
        .methods(boost::beast::http::verb::patch)(patchCpuCoreMembers);
}

} // namespace redfish
