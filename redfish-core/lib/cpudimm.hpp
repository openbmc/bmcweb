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

#include <boost/container/flat_map.hpp>
#include <boost/format.hpp>
#include <node.hpp>
#include <utils/json_utils.hpp>

namespace redfish
{

using InterfacesProperties = boost::container::flat_map<
    std::string,
    boost::container::flat_map<std::string, dbus::utility::DbusVariantType>>;

using MapperGetSubTreeResponse = boost::container::flat_map<
    std::string,
    boost::container::flat_map<std::string, std::vector<std::string>>>;
using MapperGetObjectResponse =
    std::vector<std::pair<std::string, std::vector<std::string>>>;

inline void getResourceList(std::shared_ptr<AsyncResp> aResp,
                            const std::string& subclass,
                            const std::vector<const char*>& collectionName)
{
    BMCWEB_LOG_DEBUG << "Get available system cpu/mem resources.";
    crow::connections::systemBus->async_method_call(
        [subclass, aResp{std::move(aResp)}](
            const boost::system::error_code ec,
            const boost::container::flat_map<
                std::string, boost::container::flat_map<
                                 std::string, std::vector<std::string>>>&
                subtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(aResp->res);
                return;
            }
            nlohmann::json& members = aResp->res.jsonValue["Members"];
            members = nlohmann::json::array();

            for (const auto& object : subtree)
            {
                auto iter = object.first.rfind("/");
                if ((iter != std::string::npos) && (iter < object.first.size()))
                {
                    members.push_back(
                        {{"@odata.id", "/redfish/v1/Systems/system/" +
                                           subclass + "/" +
                                           object.first.substr(iter + 1)}});
                }
            }
            aResp->res.jsonValue["Members@odata.count"] = members.size();
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", 0, collectionName);
}

inline void
    getCpuDataByInterface(std::shared_ptr<AsyncResp> aResp,
                          const InterfacesProperties& cpuInterfacesProperties)
{
    BMCWEB_LOG_DEBUG << "Get CPU resources by interface.";

    // Added for future purpose. Once present and functional attributes added
    // in busctl call, need to add actual logic to fetch original values.
    bool present = false;
    const bool functional = true;
    auto health = std::make_shared<HealthPopulate>(aResp);
    health->populate();

    for (const auto& interface : cpuInterfacesProperties)
    {
        for (const auto& property : interface.second)
        {
            if (property.first == "CoreCount")
            {
                const uint16_t* coresCount =
                    std::get_if<uint16_t>(&property.second);
                if (coresCount == nullptr)
                {
                    // Important property not in desired type
                    messages::internalError(aResp->res);
                    return;
                }
                if (*coresCount == 0)
                {
                    // Slot is not populated, set status end return
                    aResp->res.jsonValue["Status"]["State"] = "Absent";
                    // HTTP Code will be set up automatically, just return
                    return;
                }
                else
                {
                    aResp->res.jsonValue["Status"]["State"] = "Enabled";
                    present = true;
                }
                aResp->res.jsonValue["TotalCores"] = *coresCount;
            }
            else if (property.first == "MaxSpeedInMhz")
            {
                aResp->res.jsonValue["MaxSpeedMHz"] = property.second;
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
                aResp->res.jsonValue["TotalThreads"] = property.second;
            }
            else if (property.first == "Family")
            {
                const std::string* value =
                    std::get_if<std::string>(&property.second);
                if (value != nullptr)
                {
                    aResp->res.jsonValue["ProcessorId"]["EffectiveFamily"] =
                        *value;
                }
            }
            else if (property.first == "Id")
            {
                const uint64_t* value = std::get_if<uint64_t>(&property.second);
                if (value != nullptr && *value != 0)
                {
                    present = true;
                    aResp->res
                        .jsonValue["ProcessorId"]["IdentificationRegisters"] =
                        boost::lexical_cast<std::string>(*value);
                }
            }
        }
    }

    if (present == false)
    {
        aResp->res.jsonValue["Status"]["State"] = "Absent";
        aResp->res.jsonValue["Status"]["Health"] = "OK";
    }
    else
    {
        aResp->res.jsonValue["Status"]["State"] = "Enabled";
        if (functional)
        {
            aResp->res.jsonValue["Status"]["Health"] = "OK";
        }
        else
        {
            aResp->res.jsonValue["Status"]["Health"] = "Critical";
        }
    }

    return;
}

inline void getCpuDataByService(std::shared_ptr<AsyncResp> aResp,
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

inline void getCpuAssetData(std::shared_ptr<AsyncResp> aResp,
                            const std::string& /* cpuId */,
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
            }
        },
        service, objPath, "org.freedesktop.DBus.Properties", "GetAll",
        "xyz.openbmc_project.Inventory.Decorator.Asset");
}

inline void getCpuRevisionData(std::shared_ptr<AsyncResp> aResp,
                               const std::string& /* cpuId */,
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

inline void getAcceleratorDataByService(std::shared_ptr<AsyncResp> aResp,
                                        const std::string& acclrtrId,
                                        const std::string& service,
                                        const std::string& objPath)
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
            std::string state = "";

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

            if (!accPresent || !accFunctional)
            {
                BMCWEB_LOG_DEBUG << "Required properties missing in DBUS "
                                    "response";
                messages::internalError(aResp->res);
                return;
            }

            if (*accPresent && *accFunctional)
            {
                state = "Enabled";
            }
            else if (*accPresent)
            {
                state = "UnavailableOffline";
            }
            else
            {
                state = "Absent";
            }
            aResp->res.jsonValue["Status"]["State"] = state;
            aResp->res.jsonValue["Status"]["Health"] = "OK";
            aResp->res.jsonValue["ProcessorType"] = "Accelerator";
        },
        service, objPath, "org.freedesktop.DBus.Properties", "GetAll", "");
}

// OperatingConfig D-Bus Types
using TurboProfileProperty = std::vector<std::tuple<int32_t, size_t>>;
using BaseSpeedPrioritySettingsProperty =
    std::vector<std::tuple<int32_t, std::vector<int32_t>>>;
using OperatingConfigProperties =
    std::map<std::string, std::variant<int32_t, size_t, TurboProfileProperty,
                                       BaseSpeedPrioritySettingsProperty>>;

/**
 * Fill out the HighSpeedCoreIDs in a Processor resource from the given
 * OperatingConfig D-Bus property.
 *
 * @param[in,out]   aResp       Async HTTP response.
 * @param[in]       property    Full list of base speed priority groups, to use
 *                              to determine the list of high speed cores.
 */
inline void highSpeedCoreIdsHandler(
    std::shared_ptr<AsyncResp> aResp,
    const std::variant<BaseSpeedPrioritySettingsProperty>& property)
{
    // The D-Bus property does not indicate which bucket is the "high
    // priority" group, so let's discern that by looking for the one with
    // highest base frequency.
    const auto& baseSpeedSettings = std::get<0>(property);
    auto highPriorityGroup = baseSpeedSettings.cend();
    int highestBaseSpeed = -1;
    for (auto it = baseSpeedSettings.cbegin(); it != baseSpeedSettings.cend();
         ++it)
    {
        const auto baseFreq = std::get<0>(*it);
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
        jsonCoreIds = std::get<1>(*highPriorityGroup);
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
inline void getCpuConfigData(std::shared_ptr<AsyncResp> aResp,
                             const std::string& cpuId,
                             const std::string& service,
                             const std::string& objPath)
{
    BMCWEB_LOG_INFO << "Getting CPU operating configs for " << cpuId;

    auto curConfigHandler =
        [aResp, cpuId, service](
            const boost::system::error_code ec,
            const std::map<std::string,
                           std::variant<sdbusplus::message::object_path, bool>>&
                properties) {
            if (ec)
            {
                BMCWEB_LOG_INFO << "curConfigHandler dbus error " << ec
                                << ec.message();
                messages::internalError(aResp->res);
                return;
            }

            auto& json = aResp->res.jsonValue;

            const std::string& appliedConfigObjectPath =
                std::get<sdbusplus::message::object_path>(
                    properties.at("AppliedConfig"));
            auto configsPath =
                std::string("/redfish/v1/Systems/system/Processors/") + cpuId +
                "/OperatingConfigs";
            json["OperatingConfigs"] = {{"@odata.id", configsPath}};

            // Reuse the D-Bus config object name for the resource URI
            auto objectBaseName = appliedConfigObjectPath.substr(
                appliedConfigObjectPath.rfind('/') + 1);
            json["AppliedOperatingConfig"] = {
                {"@odata.id", configsPath + '/' + objectBaseName}};

            json["TurboState"] =
                std::get<bool>(properties.at("TurboProfileEnabled"))
                    ? "Enabled"
                    : "Disabled";

            json["BaseSpeedPriorityState"] =
                std::get<bool>(properties.at("BaseSpeedPriorityEnabled"))
                    ? "Enabled"
                    : "Disabled";

            crow::connections::systemBus->async_method_call(
                [aResp](const boost::system::error_code ec,
                        const std::variant<BaseSpeedPrioritySettingsProperty>&
                            property) {
                    if (ec)
                    {
                        BMCWEB_LOG_INFO << "highspeedcoreidshandler dbus error "
                                        << ec;
                        messages::internalError(aResp->res);
                        return;
                    }
                    highSpeedCoreIdsHandler(aResp, property);
                },
                service, appliedConfigObjectPath,
                "org.freedesktop.DBus.Properties", "Get",
                "xyz.openbmc_project.Inventory.Item.Cpu.OperatingConfig",
                "BaseSpeedPrioritySettings");
        };

    crow::connections::systemBus->async_method_call(
        curConfigHandler, service, objPath, "org.freedesktop.DBus.Properties",
        "GetAll",
        "xyz.openbmc_project.Control.Processor.CurrentOperatingConfig");
}

void getCpuData(std::shared_ptr<AsyncResp> aResp, const std::string& cpuId)
{
    BMCWEB_LOG_DEBUG << "Get available system cpu resources.";

    // Define mapping of discovered interface names to a handler function which
    // knows how to process the data in that interface.
    using CpuDataHandler = decltype(&getCpuAssetData);
    static const std::map<std::string, CpuDataHandler> cpuDataHandlers = {
        {"xyz.openbmc_project.Inventory.Decorator.Asset", &getCpuAssetData},
        {"xyz.openbmc_project.Inventory.Decorator.Revision",
         &getCpuRevisionData},
        {"xyz.openbmc_project.Inventory.Item.Cpu", &getCpuDataByService},
        {"xyz.openbmc_project.Inventory.Item.Accelerator",
         &getAcceleratorDataByService},
        {"xyz.openbmc_project.Control.Processor.CurrentOperatingConfig",
         &getCpuConfigData},
    };
    // Extract just the interface keys for passing to ObjectMapper
    std::vector<decltype(cpuDataHandlers)::key_type> cpuDataInterfaces;
    for (const auto& handler : cpuDataHandlers)
    {
        cpuDataInterfaces.push_back(handler.first);
    }

    crow::connections::systemBus->async_method_call(
        [cpuId,
         aResp{std::move(aResp)}](const boost::system::error_code ec,
                                  const MapperGetSubTreeResponse& subtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(aResp->res);
                return;
            }
            for (const auto& object : subtree)
            {
                // Ignore any objects which don't end with our desired cpu name
                if (!boost::ends_with(object.first, cpuId))
                {
                    continue;
                }

                // Process the first object which does match our cpu name
                // suffix, and potentially ignore any other matching objects.
                // Assume all interfaces we want to process must be on the same
                // object.

                for (const auto& service : object.second)
                {
                    for (const auto& interface : service.second)
                    {
                        auto handlerIter = cpuDataHandlers.find(interface);
                        if (handlerIter != cpuDataHandlers.end())
                        {
                            auto handler = handlerIter->second;
                            (*handler)(aResp, cpuId, service.first,
                                       object.first);
                        }
                    }
                }
                return;
            }
            // Object not found
            messages::resourceNotFound(aResp->res, "Processor", cpuId);
            return;
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", 0, cpuDataInterfaces);
}
using DimmProperty =
    std::variant<std::string, std::vector<uint32_t>, std::vector<uint16_t>,
                 uint64_t, uint32_t, uint16_t, uint8_t, bool>;

using DimmProperties = boost::container::flat_map<std::string, DimmProperty>;

inline void dimmPropToHex(std::shared_ptr<AsyncResp> aResp, const char* key,
                          const std::pair<std::string, DimmProperty>& property)
{
    const uint16_t* value = std::get_if<uint16_t>(&property.second);
    if (value == nullptr)
    {
        messages::internalError(aResp->res);
        BMCWEB_LOG_DEBUG << "Invalid property type for " << property.first;
        return;
    }

    aResp->res.jsonValue[key] = (boost::format("0x%04x") % *value).str();
}

inline void getPersistentMemoryProperties(std::shared_ptr<AsyncResp> aResp,
                                          const DimmProperties& properties)
{
    for (const auto& property : properties)
    {
        if (property.first == "ModuleManufacturerID")
        {
            dimmPropToHex(aResp, "ModuleManufacturerID", property);
        }
        else if (property.first == "ModuleProductID")
        {
            dimmPropToHex(aResp, "ModuleProductID", property);
        }
        else if (property.first == "SubsystemVendorID")
        {
            dimmPropToHex(aResp, "MemorySubsystemControllerManufacturerID",
                          property);
        }
        else if (property.first == "SubsystemDeviceID")
        {
            dimmPropToHex(aResp, "MemorySubsystemControllerProductID",
                          property);
        }
        else if (property.first == "VolatileRegionSizeLimitInKiB")
        {
            const uint64_t* value = std::get_if<uint64_t>(&property.second);

            if (value == nullptr)
            {
                messages::internalError(aResp->res);
                BMCWEB_LOG_DEBUG << "Invalid property type for "
                                    "VolatileRegionSizeLimitKiB";
                continue;
            }
            aResp->res.jsonValue["VolatileRegionSizeLimitMiB"] = (*value) >> 10;
        }
        else if (property.first == "PmRegionSizeLimitInKiB")
        {
            const uint64_t* value = std::get_if<uint64_t>(&property.second);

            if (value == nullptr)
            {
                messages::internalError(aResp->res);
                BMCWEB_LOG_DEBUG
                    << "Invalid property type for PmRegioSizeLimitKiB";
                continue;
            }
            aResp->res.jsonValue["PersistentRegionSizeLimitMiB"] =
                (*value) >> 10;
        }
        else if (property.first == "VolatileSizeInKiB")
        {
            const uint64_t* value = std::get_if<uint64_t>(&property.second);

            if (value == nullptr)
            {
                messages::internalError(aResp->res);
                BMCWEB_LOG_DEBUG
                    << "Invalid property type for VolatileSizeInKiB";
                continue;
            }
            aResp->res.jsonValue["VolatileSizeMiB"] = (*value) >> 10;
        }
        else if (property.first == "PmSizeInKiB")
        {
            const uint64_t* value = std::get_if<uint64_t>(&property.second);
            if (value == nullptr)
            {
                messages::internalError(aResp->res);
                BMCWEB_LOG_DEBUG << "Invalid property type for PmSizeInKiB";
                continue;
            }
            aResp->res.jsonValue["NonVolatileSizeMiB"] = (*value) >> 10;
        }
        else if (property.first == "CacheSizeInKB")
        {
            const uint64_t* value = std::get_if<uint64_t>(&property.second);
            if (value == nullptr)
            {
                messages::internalError(aResp->res);
                BMCWEB_LOG_DEBUG << "Invalid property type for CacheSizeInKB";
                continue;
            }
            aResp->res.jsonValue["CacheSizeMiB"] = (*value >> 10);
        }

        else if (property.first == "VoltaileRegionMaxSizeInKib")
        {
            const uint64_t* value = std::get_if<uint64_t>(&property.second);

            if (value == nullptr)
            {
                messages::internalError(aResp->res);
                BMCWEB_LOG_DEBUG << "Invalid property type for "
                                    "VolatileRegionMaxSizeInKib";
                continue;
            }
            aResp->res.jsonValue["VolatileRegionSizeMaxMiB"] = (*value) >> 10;
        }
        else if (property.first == "PmRegionMaxSizeInKiB")
        {
            const uint64_t* value = std::get_if<uint64_t>(&property.second);

            if (value == nullptr)
            {
                messages::internalError(aResp->res);
                BMCWEB_LOG_DEBUG
                    << "Invalid property type for PmRegionMaxSizeInKiB";
                continue;
            }
            aResp->res.jsonValue["PersistentRegionSizeMaxMiB"] = (*value) >> 10;
        }
        else if (property.first == "AllocationIncrementInKiB")
        {
            const uint64_t* value = std::get_if<uint64_t>(&property.second);

            if (value == nullptr)
            {
                messages::internalError(aResp->res);
                BMCWEB_LOG_DEBUG << "Invalid property type for "
                                    "AllocationIncrementInKiB";
                continue;
            }
            aResp->res.jsonValue["AllocationIncrementMiB"] = (*value) >> 10;
        }
        else if (property.first == "AllocationAlignmentInKiB")
        {
            const uint64_t* value = std::get_if<uint64_t>(&property.second);

            if (value == nullptr)
            {
                messages::internalError(aResp->res);
                BMCWEB_LOG_DEBUG << "Invalid property type for "
                                    "AllocationAlignmentInKiB";
                continue;
            }
            aResp->res.jsonValue["AllocationAlignmentMiB"] = (*value) >> 10;
        }
        else if (property.first == "VolatileRegionNumberLimit")
        {
            aResp->res.jsonValue["VolatileRegionNumberLimit"] = property.second;
        }
        else if (property.first == "PmRegionNumberLimit")
        {
            aResp->res.jsonValue["PersistentRegionNumberLimit"] =
                property.second;
        }
        else if (property.first == "SpareDeviceCount")
        {
            aResp->res.jsonValue["SpareDeviceCount"] = property.second;
        }
        else if (property.first == "IsSpareDeviceInUse")
        {
            aResp->res.jsonValue["IsSpareDeviceEnabled"] = property.second;
        }
        else if (property.first == "IsRankSpareEnabled")
        {
            aResp->res.jsonValue["IsRankSpareEnabled"] = property.second;
        }
        else if (property.first == "MaxAveragePowerLimitmW")
        {
            const auto* value =
                std::get_if<std::vector<uint32_t>>(&property.second);
            if (value == nullptr)
            {
                messages::internalError(aResp->res);
                BMCWEB_LOG_DEBUG << "Invalid property type for "
                                    "MaxAveragePowerLimitmW";
                continue;
            }
            aResp->res.jsonValue["MaxTDPMilliWatts"] = *value;
        }
        else if (property.first == "CurrentSecurityState")
        {
            aResp->res.jsonValue["SecurityState"] = property.second;
        }
        else if (property.first == "ConfigurationLocked")
        {
            aResp->res.jsonValue["ConfigurationLocked"] = property.second;
        }
        else if (property.first == "AllowedMemoryModes")
        {
            const std::string* value =
                std::get_if<std::string>(&property.second);
            if (value == nullptr)
            {
                messages::internalError(aResp->res);
                BMCWEB_LOG_DEBUG << "Invalid property type for FormFactor";
                continue;
            }
            constexpr const std::array<const char*, 3> values{"Volatile",
                                                              "PMEM", "Block"};

            for (const char* v : values)
            {
                if (boost::ends_with(*value, v))
                {
                    aResp->res.jsonValue["OperatingMemoryModes "] = v;
                    break;
                }
            }
        }
        else if (property.first == "MemoryMedia")
        {
            const std::string* value =
                std::get_if<std::string>(&property.second);
            if (value == nullptr)
            {
                messages::internalError(aResp->res);
                BMCWEB_LOG_DEBUG << "Invalid property type for MemoryMedia";
                continue;
            }
            constexpr const std::array<const char*, 3> values{"DRAM", "NAND",
                                                              "Intel3DXPoint"};

            for (const char* v : values)
            {
                if (boost::ends_with(*value, v))
                {
                    aResp->res.jsonValue["MemoryMedia"] = v;
                    break;
                }
            }
        }
        // PersistantMemory.PowerManagmentPolicy interface
        else if (property.first == "AveragePowerBudgetmW" ||
                 property.first == "MaxTDPmW" ||
                 property.first == "PeakPowerBudgetmW" ||
                 property.first == "PolicyEnabled")
        {
            std::string name =
                boost::replace_all_copy(property.first, "mW", "MilliWatts");
            aResp->res.jsonValue["PowerManagementPolicy"][name] =
                property.second;
        }
        // PersistantMemory.SecurityCapabilites interface
        else if (property.first == "ConfigurationLockCapable" ||
                 property.first == "DataLockCapable" ||
                 property.first == "MaxPassphraseCount" ||
                 property.first == "PassphraseCapable" ||
                 property.first == "PassphraseLockLimit")
        {
            aResp->res.jsonValue["SecurityCapabilities"][property.first] =
                property.second;
        }
    }
}

inline void getDimmDataByService(std::shared_ptr<AsyncResp> aResp,
                                 const std::string& dimmId,
                                 const std::string& service,
                                 const std::string& objPath)
{
    auto health = std::make_shared<HealthPopulate>(aResp);
    health->selfPath = objPath;
    health->populate();

    BMCWEB_LOG_DEBUG << "Get available system components.";
    crow::connections::systemBus->async_method_call(
        [dimmId, aResp{std::move(aResp)}](const boost::system::error_code ec,
                                          const DimmProperties& properties) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(aResp->res);

                return;
            }
            aResp->res.jsonValue["Id"] = dimmId;
            aResp->res.jsonValue["Name"] = "DIMM Slot";

            const auto memorySizeProperty = properties.find("MemorySizeInKB");
            if (memorySizeProperty != properties.end())
            {
                const uint32_t* memorySize =
                    std::get_if<uint32_t>(&memorySizeProperty->second);
                if (memorySize == nullptr)
                {
                    // Important property not in desired type
                    messages::internalError(aResp->res);

                    return;
                }
                if (*memorySize == 0)
                {
                    // Slot is not populated, set status end return
                    aResp->res.jsonValue["Status"]["State"] = "Absent";
                    aResp->res.jsonValue["Status"]["Health"] = "OK";
                    // HTTP Code will be set up automatically, just return
                    return;
                }
                aResp->res.jsonValue["CapacityMiB"] = (*memorySize >> 10);
            }
            aResp->res.jsonValue["Status"]["State"] = "Enabled";
            aResp->res.jsonValue["Status"]["Health"] = "OK";

            for (const auto& property : properties)
            {
                if (property.first == "MemoryDataWidth")
                {
                    aResp->res.jsonValue["DataWidthBits"] = property.second;
                }
                else if (property.first == "PartNumber")
                {
                    aResp->res.jsonValue["PartNumber"] = property.second;
                }
                else if (property.first == "SerialNumber")
                {
                    aResp->res.jsonValue["SerialNumber"] = property.second;
                }
                else if (property.first == "Manufacturer")
                {
                    aResp->res.jsonValue["Manufacturer"] = property.second;
                }
                else if (property.first == "RevisionCode")
                {
                    const uint16_t* value =
                        std::get_if<uint16_t>(&property.second);

                    if (value == nullptr)
                    {
                        messages::internalError(aResp->res);
                        BMCWEB_LOG_DEBUG
                            << "Invalid property type for RevisionCode";
                        continue;
                    }
                    aResp->res.jsonValue["FirmwareRevision"] =
                        std::to_string(*value);
                }
                else if (property.first == "MemoryTotalWidth")
                {
                    aResp->res.jsonValue["BusWidthBits"] = property.second;
                }
                else if (property.first == "ECC")
                {
                    const std::string* value =
                        std::get_if<std::string>(&property.second);
                    if (value == nullptr)
                    {
                        messages::internalError(aResp->res);
                        BMCWEB_LOG_DEBUG << "Invalid property type for ECC";
                        continue;
                    }
                    constexpr const std::array<const char*, 4> values{
                        "NoECC", "SingleBitECC", "MultiBitECC",
                        "AddressParity"};

                    for (const char* v : values)
                    {
                        if (boost::ends_with(*value, v))
                        {
                            aResp->res.jsonValue["ErrorCorrection"] = v;
                            break;
                        }
                    }
                }
                else if (property.first == "FormFactor")
                {
                    const std::string* value =
                        std::get_if<std::string>(&property.second);
                    if (value == nullptr)
                    {
                        messages::internalError(aResp->res);
                        BMCWEB_LOG_DEBUG
                            << "Invalid property type for FormFactor";
                        continue;
                    }
                    constexpr const std::array<const char*, 11> values{
                        "RDIMM",        "UDIMM",        "SO_DIMM",
                        "LRDIMM",       "Mini_RDIMM",   "Mini_UDIMM",
                        "SO_RDIMM_72b", "SO_UDIMM_72b", "SO_DIMM_16b",
                        "SO_DIMM_32b",  "Die"};

                    for (const char* v : values)
                    {
                        if (boost::ends_with(*value, v))
                        {
                            aResp->res.jsonValue["BaseModuleType"] = v;
                            break;
                        }
                    }
                }
                else if (property.first == "AllowedSpeedsMT")
                {
                    aResp->res.jsonValue["AllowedSpeedsMHz"] = property.second;
                }
                else if (property.first == "MemoryAttributes")
                {
                    const uint8_t* value =
                        std::get_if<uint8_t>(&property.second);

                    if (value == nullptr)
                    {
                        messages::internalError(aResp->res);
                        BMCWEB_LOG_DEBUG
                            << "Invalid property type for MemoryAttributes";
                        continue;
                    }
                    aResp->res.jsonValue["RankCount"] =
                        static_cast<uint64_t>(*value);
                }
                else if (property.first == "MemoryConfiguredSpeedInMhz")
                {
                    aResp->res.jsonValue["OperatingSpeedMhz"] = property.second;
                }
                else if (property.first == "MemoryType")
                {
                    const auto* value =
                        std::get_if<std::string>(&property.second);
                    if (value != nullptr)
                    {
                        size_t idx = value->rfind(".");
                        if (idx == std::string::npos ||
                            idx + 1 >= value->size())
                        {
                            messages::internalError(aResp->res);
                            BMCWEB_LOG_DEBUG << "Invalid property type for "
                                                "MemoryType";
                        }
                        std::string result = value->substr(idx + 1);
                        aResp->res.jsonValue["MemoryDeviceType"] = result;
                        if (value->find("DDR") != std::string::npos)
                        {
                            aResp->res.jsonValue["MemoryType"] = "DRAM";
                        }
                        else if (boost::ends_with(*value, "Logical"))
                        {
                            aResp->res.jsonValue["MemoryType"] = "IntelOptane";
                        }
                    }
                }
                // memory location interface
                else if (property.first == "Channel" ||
                         property.first == "MemoryController" ||
                         property.first == "Slot" || property.first == "Socket")
                {
                    aResp->res.jsonValue["MemoryLocation"][property.first] =
                        property.second;
                }
                else
                {
                    getPersistentMemoryProperties(aResp, properties);
                }
            }
        },
        service, objPath, "org.freedesktop.DBus.Properties", "GetAll", "");
}

inline void getDimmPartitionData(std::shared_ptr<AsyncResp> aResp,
                                 const std::string& service,
                                 const std::string& path)
{
    crow::connections::systemBus->async_method_call(
        [aResp{std::move(aResp)}](
            const boost::system::error_code ec,
            const boost::container::flat_map<
                std::string, std::variant<std::string, uint64_t, uint32_t,
                                          bool>>& properties) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(aResp->res);

                return;
            }

            nlohmann::json& partition =
                aResp->res.jsonValue["Regions"].emplace_back(
                    nlohmann::json::object());
            for (const auto& [key, val] : properties)
            {
                if (key == "MemoryClassification")
                {
                    partition[key] = val;
                }
                else if (key == "OffsetInKiB")
                {
                    const uint64_t* value = std::get_if<uint64_t>(&val);
                    if (value == nullptr)
                    {
                        messages::internalError(aResp->res);
                        BMCWEB_LOG_DEBUG
                            << "Invalid property type for OffsetInKiB";
                        continue;
                    }

                    partition["OffsetMiB"] = (*value >> 10);
                }
                else if (key == "PartitionId")
                {
                    partition["RegionId"] = val;
                }

                else if (key == "PassphraseState")
                {
                    partition["PassphraseEnabled"] = val;
                }
                else if (key == "SizeInKiB")
                {
                    const uint64_t* value = std::get_if<uint64_t>(&val);
                    if (value == nullptr)
                    {
                        messages::internalError(aResp->res);
                        BMCWEB_LOG_DEBUG
                            << "Invalid property type for SizeInKiB";
                        continue;
                    }
                    partition["SizeMiB"] = (*value >> 10);
                }
            }
        },

        service, path, "org.freedesktop.DBus.Properties", "GetAll",
        "xyz.openbmc_project.Inventory.Item.PersistentMemory.Partition");
}

inline void getDimmData(std::shared_ptr<AsyncResp> aResp,
                        const std::string& dimmId)
{
    BMCWEB_LOG_DEBUG << "Get available system dimm resources.";
    crow::connections::systemBus->async_method_call(
        [dimmId, aResp{std::move(aResp)}](
            const boost::system::error_code ec,
            const boost::container::flat_map<
                std::string, boost::container::flat_map<
                                 std::string, std::vector<std::string>>>&
                subtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(aResp->res);

                return;
            }
            bool found = false;
            for (const auto& [path, object] : subtree)
            {
                if (path.find(dimmId) != std::string::npos)
                {
                    for (const auto& [service, interfaces] : object)
                    {
                        if (!found &&
                            (std::find(
                                 interfaces.begin(), interfaces.end(),
                                 "xyz.openbmc_project.Inventory.Item.Dimm") !=
                             interfaces.end()))
                        {
                            getDimmDataByService(aResp, dimmId, service, path);
                            found = true;
                        }

                        // partitions are separate as there can be multiple per
                        // device, i.e.
                        // /xyz/openbmc_project/Inventory/Item/Dimm1/Partition1
                        // /xyz/openbmc_project/Inventory/Item/Dimm1/Partition2
                        if (std::find(interfaces.begin(), interfaces.end(),
                                      "xyz.openbmc_project.Inventory.Item."
                                      "PersistentMemory.Partition") !=
                            interfaces.end())
                        {
                            getDimmPartitionData(aResp, service, path);
                        }
                    }
                }
            }
            // Object not found
            if (!found)
            {
                messages::resourceNotFound(aResp->res, "Memory", dimmId);
            }
            return;
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", 0,
        std::array<const char*, 2>{
            "xyz.openbmc_project.Inventory.Item.Dimm",
            "xyz.openbmc_project.Inventory.Item.PersistentMemory.Partition"});
}

class OperatingConfigCollection : public Node
{
  public:
    OperatingConfigCollection(App& app) :
        Node(app,
             "/redfish/v1/Systems/system/Processors/<str>/OperatingConfigs/",
             std::string())
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
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params)
    {
        if (params.size() != 1)
        {
            messages::internalError(res);
            res.end();
            return;
        }

        const std::string& cpuName = params[0];
        res.jsonValue["@odata.type"] =
            "#OperatingConfigCollection.OperatingConfigCollection";
        res.jsonValue["@odata.id"] = req.url;
        res.jsonValue["Name"] = "Operating Config Collection";

        auto asyncResp = std::make_shared<AsyncResp>(res);

        getResourceList(
            asyncResp, "Processors/" + cpuName + "/OperatingConfigs",
            {"xyz.openbmc_project.Inventory.Item.Cpu.OperatingConfig"});
    }
};

/**
 * Fill the response contents of an OperatingConfig resource using the given
 * properties retrieved from D-Bus.
 *
 * @param[in,out]   aResp       Async HTTP response.
 * @param[in]       properties  D-Bus OperatingConfig properties.
 */
inline void
    getOperatingConfigData(std::shared_ptr<AsyncResp> aResp,
                           // const OperatingConfigProperties& properties)
                           const std::string& service,
                           const std::string& objPath)
{
    // Mapping of D-Bus name to Redfish name for the basic scalar properties, to
    // avoid handling them individually.
    static const std::map<std::string, std::string> dbusRedfishMap = {
        {"AvailableCoreCount", "TotalAvailableCoreCount"},
        {"BaseSpeed", "BaseSpeedMHz"},
        {"MaxJunctionTemperature", "MaxJunctionTemperatureCelsius"},
        {"MaxSpeed", "MaxSpeedMHz"},
        {"PowerLimit", "TDPWatts"},
    };

    crow::connections::systemBus->async_method_call(
        [aResp](boost::system::error_code ec,
                const OperatingConfigProperties& properties) {
            if (ec)
            {
                BMCWEB_LOG_WARNING << "D-Bus error: " << ec << ": "
                                   << ec.message();
                messages::internalError(aResp->res);
                return;
            }

            auto& json = aResp->res.jsonValue;
            nlohmann::json& baseSpeedArray = json["BaseSpeedPrioritySettings"];
            nlohmann::json& turboArray = json["TurboProfile"];
            // Want empty arrays on Redfish in case there is actually no data in
            // the D-Bus properties
            baseSpeedArray = nlohmann::json::array();
            turboArray = nlohmann::json::array();
            for (const auto& [key, val] : properties)
            {
                if (dbusRedfishMap.count(key) == 1)
                {
                    json[dbusRedfishMap.at(key)] = val;
                }
                else if (key == "TurboProfile")
                {
                    for (const auto& turboBucket :
                         std::get<TurboProfileProperty>(val))
                    {
                        turboArray.push_back(
                            {{"ActiveCoreCount", std::get<1>(turboBucket)},
                             {"MaxSpeedMHz", std::get<0>(turboBucket)}});
                    }
                }
                else if (key == "BaseSpeedPrioritySettings")
                {
                    for (const auto& baseSpeedSetting :
                         std::get<BaseSpeedPrioritySettingsProperty>(val))
                    {
                        baseSpeedArray.push_back(
                            {{"CoreCount",
                              std::get<1>(baseSpeedSetting).size()},
                             {"CoreIDs", std::get<1>(baseSpeedSetting)},
                             {"BaseSpeedMHz", std::get<0>(baseSpeedSetting)}});
                    }
                }
            }
        },
        service, objPath, "org.freedesktop.DBus.Properties", "GetAll",
        "xyz.openbmc_project.Inventory.Item.Cpu.OperatingConfig");
}

class OperatingConfig : public Node
{
  public:
    OperatingConfig(App& app) :
        Node(app,
             "/redfish/v1/Systems/system/Processors/<str>/OperatingConfigs/"
             "<str>",
             std::string(), std::string())
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
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params)
    {
        if (params.size() != 2)
        {
            messages::internalError(res);
            res.end();
            return;
        }

        const std::string& cpuName = params[0];
        const std::string& configName = params[1];
        res.jsonValue["@odata.type"] =
            "#OperatingConfig.v1_0_0.OperatingConfig";
        res.jsonValue["@odata.id"] = req.url;
        res.jsonValue["Name"] = "Processor Profile";
        res.jsonValue["Id"] = configName;

        auto asyncResp = std::make_shared<AsyncResp>(res);

        const std::string objectPath =
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/" +
            cpuName + "/" + configName;

        crow::connections::systemBus->async_method_call(
            [asyncResp, objectPath,
             configName](boost::system::error_code ec,
                         const MapperGetObjectResponse& serviceMap) {
                // Service responds with org.freedesktop.DBus.Error.FileNotFound
                // if object does not exist.
                if (ec == boost::system::errc::no_such_file_or_directory ||
                    serviceMap.size() == 0)
                {
                    messages::resourceNotFound(asyncResp->res,
                                               "OperatingConfig", configName);
                    return;
                }
                if (ec)
                {
                    BMCWEB_LOG_WARNING
                        << "D-Bus error getting Operating Config: " << ec
                        << ": " << ec.message();
                    messages::internalError(asyncResp->res);
                    return;
                }

                getOperatingConfigData(asyncResp, serviceMap.begin()->first,
                                       objectPath);
            },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetObject", objectPath,
            std::array<const char*, 1>{
                "xyz.openbmc_project.Inventory.Item.Cpu.OperatingConfig"});
    }
};

class ProcessorCollection : public Node
{
  public:
    /*
     * Default Constructor
     */
    ProcessorCollection(App& app) :
        Node(app, "/redfish/v1/Systems/system/Processors/")
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
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>&) override
    {
        res.jsonValue["@odata.type"] =
            "#ProcessorCollection.ProcessorCollection";
        res.jsonValue["Name"] = "Processor Collection";

        res.jsonValue["@odata.id"] = "/redfish/v1/Systems/system/Processors/";
        auto asyncResp = std::make_shared<AsyncResp>(res);

        getResourceList(asyncResp, "Processors",
                        {"xyz.openbmc_project.Inventory.Item.Cpu",
                         "xyz.openbmc_project.Inventory.Item.Accelerator"});
    }
};

class Processor : public Node
{
  public:
    /*
     * Default Constructor
     */
    Processor(App& app) :
        Node(app, "/redfish/v1/Systems/system/Processors/<str>/", std::string())
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
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>& params) override
    {
        // Check if there is required param, truly entering this shall be
        // impossible
        if (params.size() != 1)
        {
            messages::internalError(res);

            res.end();
            return;
        }
        const std::string& processorId = params[0];
        res.jsonValue["@odata.type"] = "#Processor.v1_9_0.Processor";
        res.jsonValue["@odata.id"] =
            "/redfish/v1/Systems/system/Processors/" + processorId;

        auto asyncResp = std::make_shared<AsyncResp>(res);

        getCpuData(asyncResp, processorId);
    }
};

class MemoryCollection : public Node
{
  public:
    /*
     * Default Constructor
     */
    MemoryCollection(App& app) : Node(app, "/redfish/v1/Systems/system/Memory/")
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
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>&) override
    {
        res.jsonValue["@odata.type"] = "#MemoryCollection.MemoryCollection";
        res.jsonValue["Name"] = "Memory Module Collection";
        res.jsonValue["@odata.id"] = "/redfish/v1/Systems/system/Memory";
        auto asyncResp = std::make_shared<AsyncResp>(res);

        getResourceList(asyncResp, "Memory",
                        {"xyz.openbmc_project.Inventory.Item.Dimm"});
    }
};

class Memory : public Node
{
  public:
    /*
     * Default Constructor
     */
    Memory(App& app) :
        Node(app, "/redfish/v1/Systems/system/Memory/<str>/", std::string())
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
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>& params) override
    {
        // Check if there is required param, truly entering this shall be
        // impossible
        if (params.size() != 1)
        {
            messages::internalError(res);
            res.end();
            return;
        }
        const std::string& dimmId = params[0];

        res.jsonValue["@odata.type"] = "#Memory.v1_7_0.Memory";
        res.jsonValue["@odata.id"] =
            "/redfish/v1/Systems/system/Memory/" + dimmId;
        auto asyncResp = std::make_shared<AsyncResp>(res);

        getDimmData(asyncResp, dimmId);
    }
};

} // namespace redfish
