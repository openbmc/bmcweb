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

void getResourceList(std::shared_ptr<AsyncResp> aResp,
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

void getCpuDataByInterface(std::shared_ptr<AsyncResp> aResp,
                           const InterfacesProperties& cpuInterfacesProperties)
{
    BMCWEB_LOG_DEBUG << "Get CPU resources by interface.";

    const bool* present = nullptr;
    const bool* functional = nullptr;
    for (const auto& interface : cpuInterfacesProperties)
    {
        for (const auto& property : interface.second)
        {
            if (property.first == "ProcessorCoreCount")
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
                    aResp->res.jsonValue["Status"]["Health"] = "OK";
                    // HTTP Code will be set up automatically, just return
                    return;
                }

                aResp->res.jsonValue["TotalCores"] = *coresCount;
            }
            else if (property.first == "ProcessorType")
            {
                aResp->res.jsonValue["Name"] = property.second;
            }
            else if (property.first == "Manufacturer")
            {
                const std::string* value =
                    std::get_if<std::string>(&property.second);
                if (value != nullptr)
                {
                    aResp->res.jsonValue["Manufacturer"] = property.second;
                    // Otherwise would be unexpected.
                    if (value->find("Intel") != std::string::npos)
                    {
                        aResp->res.jsonValue["ProcessorArchitecture"] = "x86";
                        aResp->res.jsonValue["InstructionSet"] = "x86-64";
                    }
                    else if (value->find("IBM") != std::string::npos)
                    {
                        aResp->res.jsonValue["ProcessorArchitecture"] = "Power";
                        aResp->res.jsonValue["InstructionSet"] = "PowerISA";
                    }
                }
            }
            else if (property.first == "ProcessorMaxSpeed")
            {
                aResp->res.jsonValue["MaxSpeedMHz"] = property.second;
            }
            else if (property.first == "ProcessorThreadCount")
            {
                aResp->res.jsonValue["TotalThreads"] = property.second;
            }
            else if (property.first == "Model")
            {
                const std::string* value =
                    std::get_if<std::string>(&property.second);
                if (value != nullptr)
                {
                    aResp->res.jsonValue["Model"] = *value;
                }
            }
            else if (property.first == "PartNumber")
            {
                aResp->res.jsonValue["PartNumber"] = property.second;
            }
            else if (property.first == "SerialNumber")
            {
                aResp->res.jsonValue["SerialNumber"] = property.second;
            }
            else if (property.first == "Version")
            {
                aResp->res.jsonValue["Version"] = property.second;
            }
            else if (property.first == "Present")
            {
                present = std::get_if<bool>(&property.second);
            }
            else if (property.first == "Functional")
            {
                functional = std::get_if<bool>(&property.second);
            }
        }
    }

    if ((present == nullptr) || (functional == nullptr))
    {
        // Important property not in desired type
        messages::internalError(aResp->res);
        return;
    }

    if (*present == false)
    {
        aResp->res.jsonValue["Status"]["State"] = "Absent";
        aResp->res.jsonValue["Status"]["Health"] = "OK";
    }
    else
    {
        aResp->res.jsonValue["Status"]["State"] = "Enabled";
        if (*functional == true)
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

void getCpuDataByService(std::shared_ptr<AsyncResp> aResp,
                         const std::string& cpuId, const std::string& service,
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
            if (totalCores == 0)
            {
                // Slot is not populated, set status end return
                aResp->res.jsonValue["Status"]["State"] = "Absent";
                aResp->res.jsonValue["Status"]["Health"] = "OK";
            }
            aResp->res.jsonValue["TotalCores"] = totalCores;
            return;
        },
        service, "/xyz/openbmc_project/inventory",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
}

void getCpuAssetData(std::shared_ptr<AsyncResp> aResp,
                     const std::string& service, const std::string& objPath)
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
                    if (sn != nullptr)
                    {
                        aResp->res.jsonValue["SerialNumber"] = *sn;
                    }
                }
                else if (property.first == "Model")
                {
                    const std::string* model =
                        std::get_if<std::string>(&property.second);
                    if (model != nullptr)
                    {
                        aResp->res.jsonValue["Model"] = *model;
                    }
                }
            }
        },
        service, objPath, "org.freedesktop.DBus.Properties", "GetAll",
        "xyz.openbmc_project.Inventory.Decorator.Asset");
}

void getAcceleratorDataByService(std::shared_ptr<AsyncResp> aResp,
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

void getCpuData(std::shared_ptr<AsyncResp> aResp, const std::string& cpuId,
                const std::vector<const char*> inventoryItems)
{
    BMCWEB_LOG_DEBUG << "Get available system cpu resources.";

    crow::connections::systemBus->async_method_call(
        [cpuId, aResp{std::move(aResp)}](
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
            for (const auto& object : subtree)
            {
                if (boost::ends_with(object.first, cpuId))
                {
                    for (const auto& service : object.second)
                    {
                        for (const auto& inventory : service.second)
                        {
                            if (inventory == "xyz.openbmc_project."
                                             "Inventory.Decorator.Asset")
                            {
                                getCpuAssetData(aResp, service.first,
                                                object.first);
                            }
                            else if (inventory ==
                                     "xyz.openbmc_project.Inventory.Item.Cpu")
                            {
                                getCpuDataByService(aResp, cpuId, service.first,
                                                    object.first);
                            }
                            else if (inventory == "xyz.openbmc_project."
                                                  "Inventory.Item.Accelerator")
                            {
                                getAcceleratorDataByService(
                                    aResp, cpuId, service.first, object.first);
                            }
                        }
                    }
                    return;
                }
            }
            // Object not found
            messages::resourceNotFound(aResp->res, "Processor", cpuId);
            return;
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", 0, inventoryItems);
}

using DimmProperty =
    std::variant<std::string, std::vector<uint32_t>, std::vector<uint16_t>,
                 uint64_t, uint32_t, uint16_t, uint8_t, bool>;

using DimmProperties = boost::container::flat_map<std::string, DimmProperty>;

void dimmPropToHex(std::shared_ptr<AsyncResp> aResp, const char* key,
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

void getPersistentMemoryProperties(std::shared_ptr<AsyncResp> aResp,
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

void getDimmDataByService(std::shared_ptr<AsyncResp> aResp,
                          const std::string& dimmId, const std::string& service,
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

void getDimmData(std::shared_ptr<AsyncResp> aResp, const std::string& dimmId)
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
            for (const auto& object : subtree)
            {
                if (boost::ends_with(object.first, dimmId))
                {
                    for (const auto& service : object.second)
                    {
                        getDimmDataByService(aResp, dimmId, service.first,
                                             object.first);
                        return;
                    }
                }
            }
            // Object not found
            messages::resourceNotFound(aResp->res, "Memory", dimmId);
            return;
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", 0,
        std::array<const char*, 1>{"xyz.openbmc_project.Inventory.Item.Dimm"});
}

class ProcessorCollection : public Node
{
  public:
    /*
     * Default Constructor
     */
    ProcessorCollection(CrowApp& app) :
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
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
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
    Processor(CrowApp& app) :
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
    void doGet(crow::Response& res, const crow::Request& req,
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
        res.jsonValue["@odata.type"] = "#Processor.v1_7_0.Processor";
        res.jsonValue["@odata.id"] =
            "/redfish/v1/Systems/system/Processors/" + processorId;

        auto asyncResp = std::make_shared<AsyncResp>(res);

        getCpuData(asyncResp, processorId,
                   {"xyz.openbmc_project.Inventory.Item.Cpu",
                    "xyz.openbmc_project.Inventory.Decorator.Asset",
                    "xyz.openbmc_project.Inventory.Item.Accelerator"});
    }
};

class MemoryCollection : public Node
{
  public:
    /*
     * Default Constructor
     */
    MemoryCollection(CrowApp& app) :
        Node(app, "/redfish/v1/Systems/system/Memory/")
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
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
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
    Memory(CrowApp& app) :
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
    void doGet(crow::Response& res, const crow::Request& req,
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
