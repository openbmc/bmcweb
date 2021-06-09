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

#include <app.hpp>
#include <boost/container/flat_map.hpp>
#include <registries/privilege_registry.hpp>
#include <sdbusplus/message/native_types.hpp>
#include <sdbusplus/utility/dedup_variant.hpp>
#include <utils/collection.hpp>
#include <utils/json_utils.hpp>
#include <utils/name_utils.hpp>

namespace redfish
{

using InterfacesProperties = boost::container::flat_map<
    std::string,
    boost::container::flat_map<std::string, dbus::utility::DbusVariantType>>;

// Map of service name to list of interfaces
using MapperServiceMap =
    std::vector<std::pair<std::string, std::vector<std::string>>>;

// Map of object paths to MapperServiceMaps
using MapperGetSubTreeResponse =
    std::vector<std::pair<std::string, MapperServiceMap>>;

// Interfaces which imply a D-Bus object represents a Processor
constexpr std::array<const char*, 2> processorInterfaces = {
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
    crow::connections::systemBus->async_method_call(
        [objPath,
         aResp{std::move(aResp)}](const boost::system::error_code ec,
                                  const std::variant<std::string>& property) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(aResp->res);
                return;
            }
            const std::string* value = std::get_if<std::string>(&property);
            if (value == nullptr)
            {
                BMCWEB_LOG_DEBUG << "Null value returned "
                                    "for UUID";
                messages::internalError(aResp->res);
                return;
            }
            aResp->res.jsonValue["UUID"] = *value;
        },
        service, objPath, "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Common.UUID", "UUID");
}

inline void
    getCpuDataByInterface(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                          const InterfacesProperties& cpuInterfacesProperties)
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
                if (*cpuPresent == false)
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
                if (*cpuFunctional == false)
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
                if (value != nullptr)
                {
                    aResp->res.jsonValue["ProcessorId"]["EffectiveFamily"] =
                        "0x" + intToHexString(*value);
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
                aResp->res.jsonValue["ProcessorId"]["EffectiveModel"] =
                    "0x" + intToHexString(*value);
            }
            else if (property.first == "Id")
            {
                const uint64_t* value = std::get_if<uint64_t>(&property.second);
                if (value != nullptr && *value != 0)
                {
                    aResp->res
                        .jsonValue["ProcessorId"]["IdentificationRegisters"] =
                        "0x" + intToHexString(*value);
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
                aResp->res.jsonValue["ProcessorId"]["MicrocodeInfo"] =
                    "0x" + intToHexString(*value);
            }
            else if (property.first == "Step")
            {
                const uint16_t* value = std::get_if<uint16_t>(&property.second);
                if (value == nullptr)
                {
                    messages::internalError(aResp->res);
                    return;
                }
                aResp->res.jsonValue["ProcessorId"]["Step"] =
                    "0x" + intToHexString(*value);
            }
        }
    }

    return;
}

inline void getCpuDataByService(std::shared_ptr<bmcweb::AsyncResp> aResp,
                                const std::string& cpuId,
                                const std::string& service,
                                const std::string& objPath)
{
    BMCWEB_LOG_DEBUG << "Get available system cpu resources by service.";

    crow::connections::systemBus->async_method_call(
        [cpuId, service, objPath, aResp{std::move(aResp)}](
            const boost::system::error_code ec,
            const dbus::utility::ManagedObjectType& dbusData) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(aResp->res);
                return;
            }
            aResp->res.jsonValue["Id"] = cpuId;
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
                else if (boost::starts_with(object.first.str, corePath))
                {
                    for (const auto& interface : object.second)
                    {
                        if (interface.first ==
                            "xyz.openbmc_project.Inventory.Item")
                        {
                            for (const auto& property : interface.second)
                            {
                                if (property.first == "Present")
                                {
                                    const bool* present =
                                        std::get_if<bool>(&property.second);
                                    if (present != nullptr)
                                    {
                                        if (*present == true)
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
    crow::connections::systemBus->async_method_call(
        [objPath, aResp{std::move(aResp)}](
            const boost::system::error_code ec,
            const boost::container::flat_map<
                std::string, std::variant<std::string, uint32_t, uint16_t,
                                          bool>>& properties) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(aResp->res);
                return;
            }

            for (const auto& property : properties)
            {
                if (property.first == "SerialNumber")
                {
                    const std::string* sn =
                        std::get_if<std::string>(&property.second);
                    if (sn != nullptr && !sn->empty())
                    {
                        aResp->res.jsonValue["SerialNumber"] = *sn;
                    }
                }
                else if (property.first == "Model")
                {
                    const std::string* model =
                        std::get_if<std::string>(&property.second);
                    if (model != nullptr && !model->empty())
                    {
                        aResp->res.jsonValue["Model"] = *model;
                    }
                }
                else if (property.first == "Manufacturer")
                {

                    const std::string* mfg =
                        std::get_if<std::string>(&property.second);
                    if (mfg != nullptr)
                    {
                        aResp->res.jsonValue["Manufacturer"] = *mfg;

                        // Otherwise would be unexpected.
                        if (mfg->find("Intel") != std::string::npos)
                        {
                            aResp->res.jsonValue["ProcessorArchitecture"] =
                                "x86";
                            aResp->res.jsonValue["InstructionSet"] = "x86-64";
                        }
                        else if (mfg->find("IBM") != std::string::npos)
                        {
                            aResp->res.jsonValue["ProcessorArchitecture"] =
                                "Power";
                            aResp->res.jsonValue["InstructionSet"] = "PowerISA";
                        }
                    }
                }
                else if (property.first == "PartNumber")
                {
                    const std::string* partNumber =
                        std::get_if<std::string>(&property.second);

                    if (partNumber == nullptr)
                    {
                        messages::internalError(aResp->res);
                        return;
                    }
                    aResp->res.jsonValue["PartNumber"] = *partNumber;
                }
                else if (property.first == "SparePartNumber")
                {
                    const std::string* sparePartNumber =
                        std::get_if<std::string>(&property.second);

                    if (sparePartNumber == nullptr)
                    {
                        messages::internalError(aResp->res);
                        return;
                    }
                    aResp->res.jsonValue["SparePartNumber"] = *sparePartNumber;
                }
            }
        },
        service, objPath, "org.freedesktop.DBus.Properties", "GetAll",
        "xyz.openbmc_project.Inventory.Decorator.Asset");
}

inline void getCpuRevisionData(std::shared_ptr<bmcweb::AsyncResp> aResp,
                               const std::string& service,
                               const std::string& objPath)
{
    BMCWEB_LOG_DEBUG << "Get Cpu Revision Data";
    crow::connections::systemBus->async_method_call(
        [objPath, aResp{std::move(aResp)}](
            const boost::system::error_code ec,
            const boost::container::flat_map<
                std::string, std::variant<std::string, uint32_t, uint16_t,
                                          bool>>& properties) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(aResp->res);
                return;
            }

            for (const auto& property : properties)
            {
                if (property.first == "Version")
                {
                    const std::string* ver =
                        std::get_if<std::string>(&property.second);
                    if (ver != nullptr)
                    {
                        aResp->res.jsonValue["Version"] = *ver;
                    }
                    break;
                }
            }
        },
        service, objPath, "org.freedesktop.DBus.Properties", "GetAll",
        "xyz.openbmc_project.Inventory.Decorator.Revision");
}

inline void getAcceleratorDataByService(
    std::shared_ptr<bmcweb::AsyncResp> aResp, const std::string& acclrtrId,
    const std::string& service, const std::string& objPath)
{
    BMCWEB_LOG_DEBUG
        << "Get available system Accelerator resources by service.";
    crow::connections::systemBus->async_method_call(
        [acclrtrId, aResp{std::move(aResp)}](
            const boost::system::error_code ec,
            const boost::container::flat_map<
                std::string, std::variant<std::string, uint32_t, uint16_t,
                                          bool>>& properties) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(aResp->res);
                return;
            }
            aResp->res.jsonValue["Id"] = acclrtrId;
            aResp->res.jsonValue["Name"] = "Processor";
            const bool* accPresent = nullptr;
            const bool* accFunctional = nullptr;

            for (const auto& property : properties)
            {
                if (property.first == "Functional")
                {
                    accFunctional = std::get_if<bool>(&property.second);
                }
                else if (property.first == "Present")
                {
                    accPresent = std::get_if<bool>(&property.second);
                }
            }

            std::string state = "Enabled";
            std::string health = "OK";

            if (accPresent != nullptr && *accPresent == false)
            {
                state = "Absent";
            }

            if ((accFunctional != nullptr) && (*accFunctional == false))
            {
                if (state == "Enabled")
                {
                    health = "Critical";
                }
            }

            aResp->res.jsonValue["Status"]["State"] = state;
            aResp->res.jsonValue["Status"]["Health"] = health;
            aResp->res.jsonValue["ProcessorType"] = "Accelerator";
        },
        service, objPath, "org.freedesktop.DBus.Properties", "GetAll", "");
}

// OperatingConfig D-Bus Types
using TurboProfileProperty = std::vector<std::tuple<uint32_t, size_t>>;
using BaseSpeedPrioritySettingsProperty =
    std::vector<std::tuple<uint32_t, std::vector<uint32_t>>>;
// uint32_t and size_t may or may not be the same type, requiring a dedup'd
// variant
using OperatingConfigProperties = std::vector<std::pair<
    std::string,
    sdbusplus::utility::dedup_variant_t<uint32_t, size_t, TurboProfileProperty,
                                        BaseSpeedPrioritySettingsProperty>>>;

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
    crow::connections::systemBus->async_method_call(
        [aResp, cpuId, service](
            const boost::system::error_code ec,
            const std::vector<
                std::pair<std::string,
                          std::variant<sdbusplus::message::object_path, bool>>>&
                properties) {
            if (ec)
            {
                BMCWEB_LOG_WARNING << "D-Bus error: " << ec << ", "
                                   << ec.message();
                messages::internalError(aResp->res);
                return;
            }

            nlohmann::json& json = aResp->res.jsonValue;

            for (const auto& [dbusPropName, variantVal] : properties)
            {
                if (dbusPropName == "AppliedConfig")
                {
                    const sdbusplus::message::object_path* dbusPathWrapper =
                        std::get_if<sdbusplus::message::object_path>(
                            &variantVal);
                    if (dbusPathWrapper == nullptr)
                    {
                        continue;
                    }

                    const std::string& dbusPath = dbusPathWrapper->str;
                    std::string uri = "/redfish/v1/Systems/system/Processors/" +
                                      cpuId + "/OperatingConfigs";
                    json["OperatingConfigs"] = {{"@odata.id", uri}};

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
                        break;
                    }
                    uri += '/';
                    uri += dbusPath.substr(baseNamePos + 1);
                    json["AppliedOperatingConfig"] = {{"@odata.id", uri}};

                    // Once we found the current applied config, queue another
                    // request to read the base freq core ids out of that
                    // config.
                    crow::connections::systemBus->async_method_call(
                        [aResp](
                            const boost::system::error_code ec,
                            const std::variant<
                                BaseSpeedPrioritySettingsProperty>& property) {
                            if (ec)
                            {
                                BMCWEB_LOG_WARNING
                                    << "D-Bus Property Get error: " << ec;
                                messages::internalError(aResp->res);
                                return;
                            }
                            auto baseSpeedList =
                                std::get_if<BaseSpeedPrioritySettingsProperty>(
                                    &property);
                            if (baseSpeedList != nullptr)
                            {
                                highSpeedCoreIdsHandler(aResp, *baseSpeedList);
                            }
                        },
                        service, dbusPath, "org.freedesktop.DBus.Properties",
                        "Get",
                        "xyz.openbmc_project.Inventory.Item.Cpu.OperatingConfig",
                        "BaseSpeedPrioritySettings");
                }
                else if (dbusPropName == "BaseSpeedPriorityEnabled")
                {
                    const bool* state = std::get_if<bool>(&variantVal);
                    if (state != nullptr)
                    {
                        json["BaseSpeedPriorityState"] =
                            *state ? "Enabled" : "Disabled";
                    }
                }
            }
        },
        service, objPath, "org.freedesktop.DBus.Properties", "GetAll",
        "xyz.openbmc_project.Control.Processor.CurrentOperatingConfig");
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
    crow::connections::systemBus->async_method_call(
        [objPath,
         aResp{std::move(aResp)}](const boost::system::error_code ec,
                                  const std::variant<std::string>& property) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(aResp->res);
                return;
            }

            const std::string* value = std::get_if<std::string>(&property);

            if (value == nullptr)
            {
                // illegal value
                BMCWEB_LOG_DEBUG << "Location code value error";
                messages::internalError(aResp->res);
                return;
            }

            aResp->res.jsonValue["Location"]["PartLocation"]["ServiceLabel"] =
                *value;
        },
        service, objPath, "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Inventory.Decorator.LocationCode", "LocationCode");
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
    crow::connections::systemBus->async_method_call(
        [aResp](boost::system::error_code ec,
                const std::variant<std::string>& property) {
            const std::string* id = std::get_if<std::string>(&property);
            if (ec || id == nullptr)
            {
                BMCWEB_LOG_ERROR << "Failed to read cpu unique id: " << ec;
                messages::internalError(aResp->res);
                return;
            }
            aResp->res
                .jsonValue["ProcessorId"]["ProtectedIdentificationNumber"] =
                *id;
        },
        service, objectPath, "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Inventory.Decorator.UniqueIdentifier",
        "UniqueIdentifier");
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
    crow::connections::systemBus->async_method_call(
        [resp, processorId, handler = std::forward<Handler>(handler)](
            boost::system::error_code ec,
            const MapperGetSubTreeResponse& subtree) mutable {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error: " << ec;
                messages::internalError(resp->res);
                return;
            }
            for (const auto& [objectPath, serviceMap] : subtree)
            {
                // Ignore any objects which don't end with our desired cpu name
                if (!boost::ends_with(objectPath, processorId))
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

                handler(resp, processorId, objectPath, serviceMap);

                return;
            }
            messages::resourceNotFound(resp->res, "Processor", processorId);
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", 0,
        std::array<const char*, 8>{
            "xyz.openbmc_project.Common.UUID",
            "xyz.openbmc_project.Inventory.Decorator.Asset",
            "xyz.openbmc_project.Inventory.Decorator.Revision",
            "xyz.openbmc_project.Inventory.Item.Cpu",
            "xyz.openbmc_project.Inventory.Decorator.LocationCode",
            "xyz.openbmc_project.Inventory.Item.Accelerator",
            "xyz.openbmc_project.Control.Processor.CurrentOperatingConfig",
            "xyz.openbmc_project.Inventory.Decorator.UniqueIdentifier"});
}

inline void getProcessorData(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                             const std::string& processorId,
                             const std::string& objectPath,
                             const MapperServiceMap& serviceMap)
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
            else if (interface == "xyz.openbmc_project.Inventory.Item")
            {
                name_util::getPrettyName(aResp, serviceName, objectPath,
                                         "/Name"_json_pointer);
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
    crow::connections::systemBus->async_method_call(
        [aResp](boost::system::error_code ec,
                const OperatingConfigProperties& properties) {
            if (ec)
            {
                BMCWEB_LOG_WARNING << "D-Bus error: " << ec << ", "
                                   << ec.message();
                messages::internalError(aResp->res);
                return;
            }

            nlohmann::json& json = aResp->res.jsonValue;
            for (const auto& [key, variant] : properties)
            {
                if (key == "AvailableCoreCount")
                {
                    const size_t* cores = std::get_if<size_t>(&variant);
                    if (cores != nullptr)
                    {
                        json["TotalAvailableCoreCount"] = *cores;
                    }
                }
                else if (key == "BaseSpeed")
                {
                    const uint32_t* speed = std::get_if<uint32_t>(&variant);
                    if (speed != nullptr)
                    {
                        json["BaseSpeedMHz"] = *speed;
                    }
                }
                else if (key == "MaxJunctionTemperature")
                {
                    const uint32_t* temp = std::get_if<uint32_t>(&variant);
                    if (temp != nullptr)
                    {
                        json["MaxJunctionTemperatureCelsius"] = *temp;
                    }
                }
                else if (key == "MaxSpeed")
                {
                    const uint32_t* speed = std::get_if<uint32_t>(&variant);
                    if (speed != nullptr)
                    {
                        json["MaxSpeedMHz"] = *speed;
                    }
                }
                else if (key == "PowerLimit")
                {
                    const uint32_t* tdp = std::get_if<uint32_t>(&variant);
                    if (tdp != nullptr)
                    {
                        json["TDPWatts"] = *tdp;
                    }
                }
                else if (key == "TurboProfile")
                {
                    const auto* turboList =
                        std::get_if<TurboProfileProperty>(&variant);
                    if (turboList == nullptr)
                    {
                        continue;
                    }

                    nlohmann::json& turboArray = json["TurboProfile"];
                    turboArray = nlohmann::json::array();
                    for (const auto& [turboSpeed, coreCount] : *turboList)
                    {
                        turboArray.push_back({{"ActiveCoreCount", coreCount},
                                              {"MaxSpeedMHz", turboSpeed}});
                    }
                }
                else if (key == "BaseSpeedPrioritySettings")
                {
                    const auto* baseSpeedList =
                        std::get_if<BaseSpeedPrioritySettingsProperty>(
                            &variant);
                    if (baseSpeedList == nullptr)
                    {
                        continue;
                    }

                    nlohmann::json& baseSpeedArray =
                        json["BaseSpeedPrioritySettings"];
                    baseSpeedArray = nlohmann::json::array();
                    for (const auto& [baseSpeed, coreList] : *baseSpeedList)
                    {
                        baseSpeedArray.push_back(
                            {{"CoreCount", coreList.size()},
                             {"CoreIDs", coreList},
                             {"BaseSpeedMHz", baseSpeed}});
                    }
                }
            }
        },
        service, objPath, "org.freedesktop.DBus.Properties", "GetAll",
        "xyz.openbmc_project.Inventory.Item.Cpu.OperatingConfig");
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
                                boost::system::error_code ec,
                                const sdbusplus::message::message& msg)
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
    else if (strcmp(dbusError->name,
                    "xyz.openbmc_project.Common.Device.Error.WriteFailure") ==
             0)
    {
        // Service tried to change the config, but it failed.
        messages::operationFailed(resp->res);
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
    const std::string& cpuObjectPath, const MapperServiceMap& serviceMap)
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
    if (!boost::starts_with(appliedConfigUri, expectedPrefix) ||
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
        [resp, appliedConfigUri](boost::system::error_code ec,
                                 sdbusplus::message::message& msg) {
            handleAppliedConfigResponse(resp, appliedConfigUri, ec, msg);
        },
        *controlService, cpuObjectPath, "org.freedesktop.DBus.Properties",
        "Set", "xyz.openbmc_project.Control.Processor.CurrentOperatingConfig",
        "AppliedConfig",
        std::variant<sdbusplus::message::object_path>(std::move(configPath)));
}

inline void requestRoutesOperatingConfigCollection(App& app)
{

    BMCWEB_ROUTE(
        app, "/redfish/v1/Systems/system/Processors/<str>/OperatingConfigs/")
        .privileges(redfish::privileges::getOperatingConfigCollection)
        .methods(
            boost::beast::http::verb::get)([](const crow::Request& req,
                                              const std::shared_ptr<
                                                  bmcweb::AsyncResp>& asyncResp,
                                              const std::string& cpuName) {
            asyncResp->res.jsonValue["@odata.type"] =
                "#OperatingConfigCollection.OperatingConfigCollection";
            asyncResp->res.jsonValue["@odata.id"] = req.url;
            asyncResp->res.jsonValue["Name"] = "Operating Config Collection";

            // First find the matching CPU object so we know how to
            // constrain our search for related Config objects.
            crow::connections::systemBus->async_method_call(
                [asyncResp, cpuName](const boost::system::error_code ec,
                                     const std::vector<std::string>& objects) {
                    if (ec)
                    {
                        BMCWEB_LOG_WARNING << "D-Bus error: " << ec << ", "
                                           << ec.message();
                        messages::internalError(asyncResp->res);
                        return;
                    }

                    for (const std::string& object : objects)
                    {
                        if (!boost::ends_with(object, cpuName))
                        {
                            continue;
                        }

                        // Not expected that there will be multiple matching
                        // CPU objects, but if there are just use the first
                        // one.

                        // Use the common search routine to construct the
                        // Collection of all Config objects under this CPU.
                        collection_util::getCollectionMembers(
                            asyncResp,
                            "/redfish/v1/Systems/system/Processors/" + cpuName +
                                "/OperatingConfigs",
                            {"xyz.openbmc_project.Inventory.Item.Cpu.OperatingConfig"},
                            object.c_str());
                        return;
                    }
                },
                "xyz.openbmc_project.ObjectMapper",
                "/xyz/openbmc_project/object_mapper",
                "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
                "/xyz/openbmc_project/inventory", 0,
                std::array<const char*, 1>{
                    "xyz.openbmc_project.Control.Processor.CurrentOperatingConfig"});
        });
}

inline void requestRoutesOperatingConfig(App& app)
{
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Systems/system/Processors/<str>/OperatingConfigs/<str>/")
        .privileges(redfish::privileges::getOperatingConfig)
        .methods(
            boost::beast::http::verb::get)([](const crow::Request& req,
                                              const std::shared_ptr<
                                                  bmcweb::AsyncResp>& asyncResp,
                                              const std::string& cpuName,
                                              const std::string& configName) {
            // Ask for all objects implementing OperatingConfig so we can search
            // for one with a matching name
            crow::connections::systemBus->async_method_call(
                [asyncResp, cpuName, configName,
                 reqUrl{req.url}](boost::system::error_code ec,
                                  const MapperGetSubTreeResponse& subtree) {
                    if (ec)
                    {
                        BMCWEB_LOG_WARNING << "D-Bus error: " << ec << ", "
                                           << ec.message();
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    const std::string expectedEnding =
                        cpuName + '/' + configName;
                    for (const auto& [objectPath, serviceMap] : subtree)
                    {
                        // Ignore any configs without matching cpuX/configY
                        if (!boost::ends_with(objectPath, expectedEnding) ||
                            serviceMap.empty())
                        {
                            continue;
                        }

                        nlohmann::json& json = asyncResp->res.jsonValue;
                        json["@odata.type"] =
                            "#OperatingConfig.v1_0_0.OperatingConfig";
                        json["@odata.id"] = reqUrl;
                        json["Name"] = "Processor Profile";
                        json["Id"] = configName;

                        // Just use the first implementation of the object - not
                        // expected that there would be multiple matching
                        // services
                        getOperatingConfigData(
                            asyncResp, serviceMap.begin()->first, objectPath);
                        return;
                    }
                    messages::resourceNotFound(asyncResp->res,
                                               "OperatingConfig", configName);
                },
                "xyz.openbmc_project.ObjectMapper",
                "/xyz/openbmc_project/object_mapper",
                "xyz.openbmc_project.ObjectMapper", "GetSubTree",
                "/xyz/openbmc_project/inventory", 0,
                std::array<const char*, 1>{
                    "xyz.openbmc_project.Inventory.Item.Cpu.OperatingConfig"});
        });
}

inline void requestRoutesProcessorCollection(App& app)
{
    /**
     * Functions triggers appropriate requests on DBus
     */
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/Processors/")
        .privileges(redfish::privileges::getProcessorCollection)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                asyncResp->res.jsonValue["@odata.type"] =
                    "#ProcessorCollection.ProcessorCollection";
                asyncResp->res.jsonValue["Name"] = "Processor Collection";

                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/Systems/system/Processors";

                collection_util::getCollectionMembers(
                    asyncResp, "/redfish/v1/Systems/system/Processors",
                    std::vector<const char*>(processorInterfaces.begin(),
                                             processorInterfaces.end()));
            });
}

inline void requestRoutesProcessor(App& app)
{
    /**
     * Functions triggers appropriate requests on DBus
     */

    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/Processors/<str>/")
        .privileges(redfish::privileges::getProcessor)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& processorId) {
                asyncResp->res.jsonValue["@odata.type"] =
                    "#Processor.v1_11_0.Processor";
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/Systems/system/Processors/" + processorId;
                asyncResp->res.jsonValue["Name"] = "Processor";

                getProcessorObject(asyncResp, processorId, getProcessorData);
            });

    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/Processors/<str>/")
        .privileges(redfish::privileges::patchProcessor)
        .methods(boost::beast::http::verb::patch)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& processorId) {
                std::optional<nlohmann::json> appliedConfigJson;
                if (!json_util::readJson(req, asyncResp->res,
                                         "AppliedOperatingConfig",
                                         appliedConfigJson))
                {
                    return;
                }

                std::string appliedConfigUri;
                if (appliedConfigJson)
                {
                    if (!json_util::readJson(*appliedConfigJson, asyncResp->res,
                                             "@odata.id", appliedConfigUri))
                    {
                        return;
                    }
                    // Check for 404 and find matching D-Bus object, then run
                    // property patch handlers if that all succeeds.
                    getProcessorObject(
                        asyncResp, processorId,
                        [appliedConfigUri = std::move(appliedConfigUri)](
                            const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& processorId,
                            const std::string& objectPath,
                            const MapperServiceMap& serviceMap) {
                            patchAppliedOperatingConfig(asyncResp, processorId,
                                                        appliedConfigUri,
                                                        objectPath, serviceMap);
                        });
                }
            });
}

} // namespace redfish
