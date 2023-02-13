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
#include "health.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/collection.hpp"
#include "utils/dbus_utils.hpp"
#include "utils/json_utils.hpp"

#include <boost/container/flat_map.hpp>
#include <boost/system/error_code.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/message/native_types.hpp>
#include <sdbusplus/unpack_properties.hpp>
#include <sdbusplus/utility/dedup_variant.hpp>

#include <array>
#include <limits>
#include <string_view>

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
 * @param[in,out]   aResp       Async HTTP response.
 * @param[in]       service     D-Bus service to query.
 * @param[in]       objPath     D-Bus object to query.
 */
inline void getProcessorUUID(std::shared_ptr<bmcweb::AsyncResp> aResp,
                             const std::string& service,
                             const std::string& objPath)
{
    BMCWEB_LOG_DEBUG << "Get Processor UUID";
    sdbusplus::asio::getProperty<std::string>(
        *crow::connections::systemBus, service, objPath,
        "xyz.openbmc_project.Common.UUID", "UUID",
        [objPath, aResp{std::move(aResp)}](const boost::system::error_code& ec,
                                           const std::string& property) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG << "DBUS response error";
            messages::internalError(aResp->res);
            return;
        }
        aResp->res.jsonValue["UUID"] = property;
        });
}

inline void getCpuDataByInterface(
    const std::shared_ptr<bmcweb::AsyncResp>& aResp,
    const dbus::utility::DBusInteracesMap& cpuInterfacesProperties)
{
    BMCWEB_LOG_DEBUG << "Get CPU resources by interface.";

    // Set the default value of state
    aResp->res.jsonValue["Status"]["State"] = "Enabled";
    aResp->res.jsonValue["Status"]["Health"] = "OK";

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
                    messages::internalError(aResp->res);
                    return;
                }
                if (!*cpuPresent)
                {
                    // Slot is not populated
                    aResp->res.jsonValue["Status"]["State"] = "Absent";
                }
            }
            else if (property.first == "Functional")
            {
                const bool* cpuFunctional = std::get_if<bool>(&property.second);
                if (cpuFunctional == nullptr)
                {
                    messages::internalError(aResp->res);
                    return;
                }
                if (!*cpuFunctional)
                {
                    aResp->res.jsonValue["Status"]["Health"] = "Critical";
                }
            }
            else if (property.first == "CoreCount")
            {
                const uint16_t* coresCount =
                    std::get_if<uint16_t>(&property.second);
                if (coresCount == nullptr)
                {
                    messages::internalError(aResp->res);
                    return;
                }
                aResp->res.jsonValue["TotalCores"] = *coresCount;
            }
            else if (property.first == "MaxSpeedInMhz")
            {
                const uint32_t* value = std::get_if<uint32_t>(&property.second);
                if (value != nullptr)
                {
                    aResp->res.jsonValue["MaxSpeedMHz"] = *value;
                }
            }
            else if (property.first == "Socket")
            {
                const std::string* value =
                    std::get_if<std::string>(&property.second);
                if (value != nullptr)
                {
                    aResp->res.jsonValue["Socket"] = *value;
                }
            }
            else if (property.first == "ThreadCount")
            {
                const uint16_t* value = std::get_if<uint16_t>(&property.second);
                if (value != nullptr)
                {
                    aResp->res.jsonValue["TotalThreads"] = *value;
                }
            }
            else if (property.first == "EffectiveFamily")
            {
                const uint16_t* value = std::get_if<uint16_t>(&property.second);
                if (value != nullptr && *value != 2)
                {
                    aResp->res.jsonValue["ProcessorId"]["EffectiveFamily"] =
                        "0x" + intToHexString(*value, 4);
                }
            }
            else if (property.first == "EffectiveModel")
            {
                const uint16_t* value = std::get_if<uint16_t>(&property.second);
                if (value == nullptr)
                {
                    messages::internalError(aResp->res);
                    return;
                }
                if (*value != 0)
                {
                    aResp->res.jsonValue["ProcessorId"]["EffectiveModel"] =
                        "0x" + intToHexString(*value, 4);
                }
            }
            else if (property.first == "Id")
            {
                const uint64_t* value = std::get_if<uint64_t>(&property.second);
                if (value != nullptr && *value != 0)
                {
                    aResp->res
                        .jsonValue["ProcessorId"]["IdentificationRegisters"] =
                        "0x" + intToHexString(*value, 16);
                }
            }
            else if (property.first == "Microcode")
            {
                const uint32_t* value = std::get_if<uint32_t>(&property.second);
                if (value == nullptr)
                {
                    messages::internalError(aResp->res);
                    return;
                }
                if (*value != 0)
                {
                    aResp->res.jsonValue["ProcessorId"]["MicrocodeInfo"] =
                        "0x" + intToHexString(*value, 8);
                }
            }
            else if (property.first == "Step")
            {
                const uint16_t* value = std::get_if<uint16_t>(&property.second);
                if (value == nullptr)
                {
                    messages::internalError(aResp->res);
                    return;
                }
                if (*value != std::numeric_limits<uint16_t>::max())
                {
                    aResp->res.jsonValue["ProcessorId"]["Step"] =
                        "0x" + intToHexString(*value, 4);
                }
            }
        }
    }
}

inline void getCpuDataByService(std::shared_ptr<bmcweb::AsyncResp> aResp,
                                const std::string& cpuId,
                                const std::string& service,
                                const std::string& objPath)
{
    BMCWEB_LOG_DEBUG << "Get available system cpu resources by service.";

    crow::connections::systemBus->async_method_call(
        [cpuId, service, objPath, aResp{std::move(aResp)}](
            const boost::system::error_code& ec,
            const dbus::utility::ManagedObjectType& dbusData) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG << "DBUS response error";
            messages::internalError(aResp->res);
            return;
        }
        aResp->res.jsonValue["Id"] = cpuId;
        aResp->res.jsonValue["Name"] = "Processor";
        aResp->res.jsonValue["ProcessorType"] = "CPU";

        bool slotPresent = false;
        std::string corePath = objPath + "/core";
        size_t totalCores = 0;
        for (const auto& object : dbusData)
        {
            if (object.first.str == objPath)
            {
                getCpuDataByInterface(aResp, object.second);
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
                aResp->res.jsonValue["Status"]["State"] = "Absent";
                aResp->res.jsonValue["Status"]["Health"] = "OK";
            }
            aResp->res.jsonValue["TotalCores"] = totalCores;
        }
        return;
        },
        service, "/xyz/openbmc_project/inventory",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
}

inline void getCpuAssetData(std::shared_ptr<bmcweb::AsyncResp> aResp,
                            const std::string& service,
                            const std::string& objPath)
{
    BMCWEB_LOG_DEBUG << "Get Cpu Asset Data";
    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, service, objPath,
        "xyz.openbmc_project.Inventory.Decorator.Asset",
        [objPath, aResp{std::move(aResp)}](
            const boost::system::error_code& ec,
            const dbus::utility::DBusPropertiesMap& properties) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG << "DBUS response error";
            messages::internalError(aResp->res);
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
            messages::internalError(aResp->res);
            return;
        }

        if (serialNumber != nullptr && !serialNumber->empty())
        {
            aResp->res.jsonValue["SerialNumber"] = *serialNumber;
        }

        if ((model != nullptr) && !model->empty())
        {
            aResp->res.jsonValue["Model"] = *model;
        }

        if (manufacturer != nullptr)
        {
            aResp->res.jsonValue["Manufacturer"] = *manufacturer;

            // Otherwise would be unexpected.
            if (manufacturer->find("Intel") != std::string::npos)
            {
                aResp->res.jsonValue["ProcessorArchitecture"] = "x86";
                aResp->res.jsonValue["InstructionSet"] = "x86-64";
            }
            else if (manufacturer->find("IBM") != std::string::npos)
            {
                aResp->res.jsonValue["ProcessorArchitecture"] = "Power";
                aResp->res.jsonValue["InstructionSet"] = "PowerISA";
            }
        }

        if (partNumber != nullptr)
        {
            aResp->res.jsonValue["PartNumber"] = *partNumber;
        }

        if (sparePartNumber != nullptr && !sparePartNumber->empty())
        {
            aResp->res.jsonValue["SparePartNumber"] = *sparePartNumber;
        }
        });
}

inline void getCpuRevisionData(std::shared_ptr<bmcweb::AsyncResp> aResp,
                               const std::string& service,
                               const std::string& objPath)
{
    BMCWEB_LOG_DEBUG << "Get Cpu Revision Data";
    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, service, objPath,
        "xyz.openbmc_project.Inventory.Decorator.Revision",
        [objPath, aResp{std::move(aResp)}](
            const boost::system::error_code& ec,
            const dbus::utility::DBusPropertiesMap& properties) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG << "DBUS response error";
            messages::internalError(aResp->res);
            return;
        }

        const std::string* version = nullptr;

        const bool success = sdbusplus::unpackPropertiesNoThrow(
            dbus_utils::UnpackErrorPrinter(), properties, "Version", version);

        if (!success)
        {
            messages::internalError(aResp->res);
            return;
        }

        if (version != nullptr)
        {
            aResp->res.jsonValue["Version"] = *version;
        }
        });
}

inline void getAcceleratorDataByService(
    std::shared_ptr<bmcweb::AsyncResp> aResp, const std::string& acclrtrId,
    const std::string& service, const std::string& objPath)
{
    BMCWEB_LOG_DEBUG
        << "Get available system Accelerator resources by service.";
    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, service, objPath, "",
        [acclrtrId, aResp{std::move(aResp)}](
            const boost::system::error_code& ec,
            const dbus::utility::DBusPropertiesMap& properties) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG << "DBUS response error";
            messages::internalError(aResp->res);
            return;
        }

        const bool* functional = nullptr;
        const bool* present = nullptr;

        const bool success = sdbusplus::unpackPropertiesNoThrow(
            dbus_utils::UnpackErrorPrinter(), properties, "Functional",
            functional, "Present", present);

        if (!success)
        {
            messages::internalError(aResp->res);
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

        aResp->res.jsonValue["Id"] = acclrtrId;
        aResp->res.jsonValue["Name"] = "Processor";
        aResp->res.jsonValue["Status"]["State"] = state;
        aResp->res.jsonValue["Status"]["Health"] = health;
        aResp->res.jsonValue["ProcessorType"] = "Accelerator";
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
 * @param[in,out]   aResp               Async HTTP response.
 * @param[in]       baseSpeedSettings   Full list of base speed priority groups,
 *                                      to use to determine the list of high
 *                                      speed cores.
 */
inline void highSpeedCoreIdsHandler(
    const std::shared_ptr<bmcweb::AsyncResp>& aResp,
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

    nlohmann::json& jsonCoreIds = aResp->res.jsonValue["HighSpeedCoreIDs"];
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
 * @param[in,out]   aResp       Async HTTP response.
 * @param[in]       cpuId       CPU D-Bus name.
 * @param[in]       service     D-Bus service to query.
 * @param[in]       objPath     D-Bus object to query.
 */
inline void getCpuConfigData(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                             const std::string& cpuId,
                             const std::string& service,
                             const std::string& objPath)
{
    BMCWEB_LOG_INFO << "Getting CPU operating configs for " << cpuId;

    // First, GetAll CurrentOperatingConfig properties on the object
    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, service, objPath,
        "xyz.openbmc_project.Control.Processor.CurrentOperatingConfig",
        [aResp, cpuId,
         service](const boost::system::error_code& ec,
                  const dbus::utility::DBusPropertiesMap& properties) {
        if (ec)
        {
            BMCWEB_LOG_WARNING << "D-Bus error: " << ec << ", " << ec.message();
            messages::internalError(aResp->res);
            return;
        }

        nlohmann::json& json = aResp->res.jsonValue;

        const sdbusplus::message::object_path* appliedConfig = nullptr;
        const bool* baseSpeedPriorityEnabled = nullptr;

        const bool success = sdbusplus::unpackPropertiesNoThrow(
            dbus_utils::UnpackErrorPrinter(), properties, "AppliedConfig",
            appliedConfig, "BaseSpeedPriorityEnabled",
            baseSpeedPriorityEnabled);

        if (!success)
        {
            messages::internalError(aResp->res);
            return;
        }

        if (appliedConfig != nullptr)
        {
            const std::string& dbusPath = appliedConfig->str;
            nlohmann::json::object_t operatingConfig;
            operatingConfig["@odata.id"] = crow::utility::urlFromPieces(
                "redfish", "v1", "Systems", "system", "Processors", cpuId,
                "OperatingConfigs");
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
                messages::internalError(aResp->res);
                return;
            }
            nlohmann::json::object_t appliedOperatingConfig;
            appliedOperatingConfig["@odata.id"] = crow::utility::urlFromPieces(
                "redfish", "v1", "Systems", "system", "Processors", cpuId,
                "OperatingConfigs", dbusPath.substr(baseNamePos + 1));
            json["AppliedOperatingConfig"] = std::move(appliedOperatingConfig);

            // Once we found the current applied config, queue another
            // request to read the base freq core ids out of that
            // config.
            sdbusplus::asio::getProperty<BaseSpeedPrioritySettingsProperty>(
                *crow::connections::systemBus, service, dbusPath,
                "xyz.openbmc_project.Inventory.Item.Cpu."
                "OperatingConfig",
                "BaseSpeedPrioritySettings",
                [aResp](
                    const boost::system::error_code& ec2,
                    const BaseSpeedPrioritySettingsProperty& baseSpeedList) {
                if (ec2)
                {
                    BMCWEB_LOG_WARNING << "D-Bus Property Get error: " << ec2;
                    messages::internalError(aResp->res);
                    return;
                }

                highSpeedCoreIdsHandler(aResp, baseSpeedList);
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
 * @param[in,out]   aResp       Async HTTP response.
 * @param[in]       service     D-Bus service to query.
 * @param[in]       objPath     D-Bus object to query.
 */
inline void getCpuLocationCode(std::shared_ptr<bmcweb::AsyncResp> aResp,
                               const std::string& service,
                               const std::string& objPath)
{
    BMCWEB_LOG_DEBUG << "Get Cpu Location Data";
    sdbusplus::asio::getProperty<std::string>(
        *crow::connections::systemBus, service, objPath,
        "xyz.openbmc_project.Inventory.Decorator.LocationCode", "LocationCode",
        [objPath, aResp{std::move(aResp)}](const boost::system::error_code& ec,
                                           const std::string& property) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG << "DBUS response error";
            messages::internalError(aResp->res);
            return;
        }

        aResp->res.jsonValue["Location"]["PartLocation"]["ServiceLabel"] =
            property;
        });
}

/**
 * Populate the unique identifier in a Processor resource by requesting data
 * from the given D-Bus object.
 *
 * @param[in,out]   aResp       Async HTTP response.
 * @param[in]       service     D-Bus service to query.
 * @param[in]       objPath     D-Bus object to query.
 */
inline void getCpuUniqueId(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                           const std::string& service,
                           const std::string& objectPath)
{
    BMCWEB_LOG_DEBUG << "Get CPU UniqueIdentifier";
    sdbusplus::asio::getProperty<std::string>(
        *crow::connections::systemBus, service, objectPath,
        "xyz.openbmc_project.Inventory.Decorator.UniqueIdentifier",
        "UniqueIdentifier",
        [aResp](const boost::system::error_code& ec, const std::string& id) {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "Failed to read cpu unique id: " << ec;
            messages::internalError(aResp->res);
            return;
        }
        aResp->res.jsonValue["ProcessorId"]["ProtectedIdentificationNumber"] =
            id;
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
    BMCWEB_LOG_DEBUG << "Get available system processor resources.";

    // GetSubTree on all interfaces which provide info about a Processor
    constexpr std::array<std::string_view, 8> interfaces = {
        "xyz.openbmc_project.Common.UUID",
        "xyz.openbmc_project.Inventory.Decorator.Asset",
        "xyz.openbmc_project.Inventory.Decorator.Revision",
        "xyz.openbmc_project.Inventory.Item.Cpu",
        "xyz.openbmc_project.Inventory.Decorator.LocationCode",
        "xyz.openbmc_project.Inventory.Item.Accelerator",
        "xyz.openbmc_project.Control.Processor.CurrentOperatingConfig",
        "xyz.openbmc_project.Inventory.Decorator.UniqueIdentifier"};
    dbus::utility::getSubTree(
        "/xyz/openbmc_project/inventory", 0, interfaces,
        [resp, processorId, handler = std::forward<Handler>(handler)](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreeResponse& subtree) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG << "DBUS response error: " << ec;
            messages::internalError(resp->res);
            return;
        }
        for (const auto& [objectPath, serviceMap] : subtree)
        {
            // Ignore any objects which don't end with our desired cpu name
            if (!objectPath.ends_with(processorId))
            {
                continue;
            }

            bool found = false;
            // Filter out objects that don't have the CPU-specific
            // interfaces to make sure we can return 404 on non-CPUs
            // (e.g. /redfish/../Processors/dimm0)
            for (const auto& [serviceName, interfaceList] : serviceMap)
            {
                if (std::find_first_of(
                        interfaceList.begin(), interfaceList.end(),
                        processorInterfaces.begin(),
                        processorInterfaces.end()) != interfaceList.end())
                {
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                continue;
            }

            // Process the first object which does match our cpu name and
            // required interfaces, and potentially ignore any other
            // matching objects. Assume all interfaces we want to process
            // must be on the same object path.

            handler(objectPath, serviceMap);
            return;
        }
        messages::resourceNotFound(resp->res, "Processor", processorId);
        });
}

inline void getProcessorData(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                             const std::string& processorId,
                             const std::string& objectPath,
                             const dbus::utility::MapperServiceMap& serviceMap)
{
    for (const auto& [serviceName, interfaceList] : serviceMap)
    {
        for (const auto& interface : interfaceList)
        {
            if (interface == "xyz.openbmc_project.Inventory.Decorator.Asset")
            {
                getCpuAssetData(aResp, serviceName, objectPath);
            }
            else if (interface ==
                     "xyz.openbmc_project.Inventory.Decorator.Revision")
            {
                getCpuRevisionData(aResp, serviceName, objectPath);
            }
            else if (interface == "xyz.openbmc_project.Inventory.Item.Cpu")
            {
                getCpuDataByService(aResp, processorId, serviceName,
                                    objectPath);
            }
            else if (interface ==
                     "xyz.openbmc_project.Inventory.Item.Accelerator")
            {
                getAcceleratorDataByService(aResp, processorId, serviceName,
                                            objectPath);
            }
            else if (
                interface ==
                "xyz.openbmc_project.Control.Processor.CurrentOperatingConfig")
            {
                getCpuConfigData(aResp, processorId, serviceName, objectPath);
            }
            else if (interface ==
                     "xyz.openbmc_project.Inventory.Decorator.LocationCode")
            {
                getCpuLocationCode(aResp, serviceName, objectPath);
            }
            else if (interface == "xyz.openbmc_project.Common.UUID")
            {
                getProcessorUUID(aResp, serviceName, objectPath);
            }
            else if (interface ==
                     "xyz.openbmc_project.Inventory.Decorator.UniqueIdentifier")
            {
                getCpuUniqueId(aResp, serviceName, objectPath);
            }
        }
    }
}

/**
 * Request all the properties for the given D-Bus object and fill out the
 * related entries in the Redfish OperatingConfig response.
 *
 * @param[in,out]   aResp       Async HTTP response.
 * @param[in]       service     D-Bus service name to query.
 * @param[in]       objPath     D-Bus object to query.
 */
inline void
    getOperatingConfigData(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                           const std::string& service,
                           const std::string& objPath)
{
    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, service, objPath,
        "xyz.openbmc_project.Inventory.Item.Cpu.OperatingConfig",
        [aResp](const boost::system::error_code& ec,
                const dbus::utility::DBusPropertiesMap& properties) {
        if (ec)
        {
            BMCWEB_LOG_WARNING << "D-Bus error: " << ec << ", " << ec.message();
            messages::internalError(aResp->res);
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
            messages::internalError(aResp->res);
            return;
        }

        nlohmann::json& json = aResp->res.jsonValue;

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
                turboArray.push_back(std::move(turbo));
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
                baseSpeedArray.push_back(std::move(speed));
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
        BMCWEB_LOG_DEBUG << "Set Property succeeded";
        return;
    }

    BMCWEB_LOG_DEBUG << "Set Property failed: " << ec;

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
        if (std::find(interfaceList.begin(), interfaceList.end(),
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

    BMCWEB_LOG_INFO << "Setting config to " << configPath.str;

    // Set the property, with handler to check error responses
    crow::connections::systemBus->async_method_call(
        [resp, appliedConfigUri](const boost::system::error_code& ec,
                                 const sdbusplus::message_t& msg) {
        handleAppliedConfigResponse(resp, appliedConfigUri, ec, msg);
        },
        *controlService, cpuObjectPath, "org.freedesktop.DBus.Properties",
        "Set", "xyz.openbmc_project.Control.Processor.CurrentOperatingConfig",
        "AppliedConfig", dbus::utility::DbusVariantType(std::move(configPath)));
}

inline void handleProcessorHead(crow::App& app, const crow::Request& req,
                                const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                                const std::string& /* systemName */,
                                const std::string& /* processorId */)
{
    if (!redfish::setUpRedfishRoute(app, req, aResp))
    {
        return;
    }
    aResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/Processor/Processor.json>; rel=describedby");
}

inline void handleProcessorCollectionHead(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& aResp,
    const std::string& /* systemName */)
{
    if (!redfish::setUpRedfishRoute(app, req, aResp))
    {
        return;
    }
    aResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/ProcessorCollection/ProcessorCollection.json>; rel=describedby");
}

inline void requestRoutesOperatingConfigCollection(App& app)
{

    BMCWEB_ROUTE(
        app, "/redfish/v1/Systems/system/Processors/<str>/OperatingConfigs/")
        .privileges(redfish::privileges::getOperatingConfigCollection)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& cpuName) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        asyncResp->res.jsonValue["@odata.type"] =
            "#OperatingConfigCollection.OperatingConfigCollection";
        asyncResp->res.jsonValue["@odata.id"] = req.url;
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
                BMCWEB_LOG_WARNING << "D-Bus error: " << ec << ", "
                                   << ec.message();
                messages::internalError(asyncResp->res);
                return;
            }

            for (const std::string& object : objects)
            {
                if (!object.ends_with(cpuName))
                {
                    continue;
                }

                // Not expected that there will be multiple matching
                // CPU objects, but if there are just use the first
                // one.

                // Use the common search routine to construct the
                // Collection of all Config objects under this CPU.
                constexpr std::array<std::string_view, 1> interface {
                    "xyz.openbmc_project.Inventory.Item.Cpu.OperatingConfig"
                };
                collection_util::getCollectionMembers(
                    asyncResp,
                    crow::utility::urlFromPieces("redfish", "v1", "Systems",
                                                 "system", "Processors",
                                                 cpuName, "OperatingConfigs"),
                    interface, object.c_str());
                return;
            }
            });
        });
}

inline void requestRoutesOperatingConfig(App& app)
{
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Systems/system/Processors/<str>/OperatingConfigs/<str>/")
        .privileges(redfish::privileges::getOperatingConfig)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& cpuName, const std::string& configName) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        // Ask for all objects implementing OperatingConfig so we can search
        // for one with a matching name
        constexpr std::array<std::string_view, 1> interfaces = {
            "xyz.openbmc_project.Inventory.Item.Cpu.OperatingConfig"};
        dbus::utility::getSubTree(
            "/xyz/openbmc_project/inventory", 0, interfaces,
            [asyncResp, cpuName, configName, reqUrl{req.url}](
                const boost::system::error_code& ec,
                const dbus::utility::MapperGetSubTreeResponse& subtree) {
            if (ec)
            {
                BMCWEB_LOG_WARNING << "D-Bus error: " << ec << ", "
                                   << ec.message();
                messages::internalError(asyncResp->res);
                return;
            }
            const std::string expectedEnding = cpuName + '/' + configName;
            for (const auto& [objectPath, serviceMap] : subtree)
            {
                // Ignore any configs without matching cpuX/configY
                if (!objectPath.ends_with(expectedEnding) || serviceMap.empty())
                {
                    continue;
                }

                nlohmann::json& json = asyncResp->res.jsonValue;
                json["@odata.type"] = "#OperatingConfig.v1_0_0.OperatingConfig";
                json["@odata.id"] = reqUrl;
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

        collection_util::getCollectionMembers(
            asyncResp,
            boost::urls::url("/redfish/v1/Systems/system/Processors"),
            processorInterfaces);
        });
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
        if (systemName != "system")
        {
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }

        asyncResp->res.addHeader(
            boost::beast::http::field::link,
            "</redfish/v1/JsonSchemas/Processor/Processor.json>; rel=describedby");
        asyncResp->res.jsonValue["@odata.type"] =
            "#Processor.v1_11_0.Processor";
        asyncResp->res.jsonValue["@odata.id"] = crow::utility::urlFromPieces(
            "redfish", "v1", "Systems", "system", "Processors", processorId);

        getProcessorObject(
            asyncResp, processorId,
            std::bind_front(getProcessorData, asyncResp, processorId));
        });

    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Processors/<str>/")
        .privileges(redfish::privileges::patchProcessor)
        .methods(boost::beast::http::verb::patch)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemName,
                   const std::string& processorId) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        if (systemName != "system")
        {
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }

        std::optional<nlohmann::json> appliedConfigJson;
        if (!json_util::readJsonPatch(req, asyncResp->res,
                                      "AppliedOperatingConfig",
                                      appliedConfigJson))
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
        });
}

} // namespace redfish
