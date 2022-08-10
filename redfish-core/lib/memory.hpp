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
#include <dbus_utility.hpp>
#include <http_utility.hpp>
#include <nlohmann/json.hpp>
#include <query.hpp>
#include <registries/privilege_registry.hpp>
#include <utils/collection.hpp>
#include <utils/hex_utils.hpp>
#include <utils/json_utils.hpp>

#include <algorithm>
#include <unordered_map>
#include <unordered_set>

namespace redfish
{

inline std::string translateMemoryTypeToRedfish(const std::string& memoryType)
{
    if (memoryType == "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType.DDR")
    {
        return "DDR";
    }
    if (memoryType == "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType.DDR2")
    {
        return "DDR2";
    }
    if (memoryType == "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType.DDR3")
    {
        return "DDR3";
    }
    if (memoryType == "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType.DDR4")
    {
        return "DDR4";
    }
    if (memoryType ==
        "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType.DDR4E_SDRAM")
    {
        return "DDR4E_SDRAM";
    }
    if (memoryType == "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType.DDR5")
    {
        return "DDR5";
    }
    if (memoryType ==
        "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType.LPDDR4_SDRAM")
    {
        return "LPDDR4_SDRAM";
    }
    if (memoryType ==
        "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType.LPDDR3_SDRAM")
    {
        return "LPDDR3_SDRAM";
    }
    if (memoryType ==
        "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType.DDR2_SDRAM_FB_DIMM")
    {
        return "DDR2_SDRAM_FB_DIMM";
    }
    if (memoryType ==
        "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType.DDR2_SDRAM_FB_DIMM_PROB")
    {
        return "DDR2_SDRAM_FB_DIMM_PROBE";
    }
    if (memoryType ==
        "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType.DDR_SGRAM")
    {
        return "DDR_SGRAM";
    }
    if (memoryType == "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType.ROM")
    {
        return "ROM";
    }
    if (memoryType ==
        "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType.SDRAM")
    {
        return "SDRAM";
    }
    if (memoryType == "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType.EDO")
    {
        return "EDO";
    }
    if (memoryType ==
        "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType.FastPageMode")
    {
        return "FastPageMode";
    }
    if (memoryType ==
        "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType.PipelinedNibble")
    {
        return "PipelinedNibble";
    }
    if (memoryType ==
        "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType.Logical")
    {
        return "Logical";
    }
    if (memoryType == "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType.HBM")
    {
        return "HBM";
    }
    if (memoryType == "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType.HBM2")
    {
        return "HBM2";
    }
    // This is values like Other or Unknown
    // Also D-Bus values:
    // DRAM
    // EDRAM
    // VRAM
    // SRAM
    // RAM
    // FLASH
    // EEPROM
    // FEPROM
    // EPROM
    // CDRAM
    // ThreeDRAM
    // RDRAM
    // FBD2
    // LPDDR_SDRAM
    // LPDDR2_SDRAM
    // LPDDR5_SDRAM
    return "";
}

inline void
    dimmPropToHex(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                  const char* key,
                  const dbus::utility::DBusPropertiesMap::value_type& property,
                  const nlohmann::json::json_pointer& jsonPtr)
{
    const uint16_t* value = std::get_if<uint16_t>(&property.second);
    if (value == nullptr)
    {
        messages::internalError(aResp->res);
        BMCWEB_LOG_DEBUG << "Invalid property type for " << property.first;
        return;
    }
    aResp->res.jsonValue[jsonPtr][key] = "0x" + intToHexString(*value, 4);
}

inline void getPersistentMemoryProperties(
    const std::shared_ptr<bmcweb::AsyncResp>& aResp,
    const dbus::utility::DBusPropertiesMap::value_type& property,
    const nlohmann::json::json_pointer& jsonPtr)
{
    if (property.first == "ModuleManufacturerID")
    {
        dimmPropToHex(aResp, "ModuleManufacturerID", property, jsonPtr);
    }
    else if (property.first == "ModuleProductID")
    {
        dimmPropToHex(aResp, "ModuleProductID", property, jsonPtr);
    }
    else if (property.first == "SubsystemVendorID")
    {
        dimmPropToHex(aResp, "MemorySubsystemControllerManufacturerID",
                      property, jsonPtr);
    }
    else if (property.first == "SubsystemDeviceID")
    {
        dimmPropToHex(aResp, "MemorySubsystemControllerProductID", property,
                      jsonPtr);
    }
    else if (property.first == "VolatileRegionSizeLimitInKiB")
    {
        const uint64_t* value = std::get_if<uint64_t>(&property.second);

        if (value == nullptr)
        {
            messages::internalError(aResp->res);
            BMCWEB_LOG_DEBUG
                << "Invalid property type for VolatileRegionSizeLimitKiB";
            return;
        }
        aResp->res.jsonValue[jsonPtr]["VolatileRegionSizeLimitMiB"] =
            (*value) >> 10;
    }
    else if (property.first == "PmRegionSizeLimitInKiB")
    {
        const uint64_t* value = std::get_if<uint64_t>(&property.second);

        if (value == nullptr)
        {
            messages::internalError(aResp->res);
            BMCWEB_LOG_DEBUG << "Invalid property type for PmRegioSizeLimitKiB";
            return;
        }
        aResp->res.jsonValue[jsonPtr]["PersistentRegionSizeLimitMiB"] =
            (*value) >> 10;
    }
    else if (property.first == "VolatileSizeInKiB")
    {
        const uint64_t* value = std::get_if<uint64_t>(&property.second);

        if (value == nullptr)
        {
            messages::internalError(aResp->res);
            BMCWEB_LOG_DEBUG << "Invalid property type for VolatileSizeInKiB";
            return;
        }
        aResp->res.jsonValue[jsonPtr]["VolatileSizeMiB"] = (*value) >> 10;
    }
    else if (property.first == "PmSizeInKiB")
    {
        const uint64_t* value = std::get_if<uint64_t>(&property.second);
        if (value == nullptr)
        {
            messages::internalError(aResp->res);
            BMCWEB_LOG_DEBUG << "Invalid property type for PmSizeInKiB";
            return;
        }
        aResp->res.jsonValue[jsonPtr]["NonVolatileSizeMiB"] = (*value) >> 10;
    }
    else if (property.first == "CacheSizeInKB")
    {
        const uint64_t* value = std::get_if<uint64_t>(&property.second);
        if (value == nullptr)
        {
            messages::internalError(aResp->res);
            BMCWEB_LOG_DEBUG << "Invalid property type for CacheSizeInKB";
            return;
        }
        aResp->res.jsonValue[jsonPtr]["CacheSizeMiB"] = (*value >> 10);
    }

    else if (property.first == "VoltaileRegionMaxSizeInKib")
    {
        const uint64_t* value = std::get_if<uint64_t>(&property.second);

        if (value == nullptr)
        {
            messages::internalError(aResp->res);
            BMCWEB_LOG_DEBUG
                << "Invalid property type for VolatileRegionMaxSizeInKib";
            return;
        }
        aResp->res.jsonValue[jsonPtr]["VolatileRegionSizeMaxMiB"] =
            (*value) >> 10;
    }
    else if (property.first == "PmRegionMaxSizeInKiB")
    {
        const uint64_t* value = std::get_if<uint64_t>(&property.second);

        if (value == nullptr)
        {
            messages::internalError(aResp->res);
            BMCWEB_LOG_DEBUG
                << "Invalid property type for PmRegionMaxSizeInKiB";
            return;
        }
        aResp->res.jsonValue[jsonPtr]["PersistentRegionSizeMaxMiB"] =
            (*value) >> 10;
    }
    else if (property.first == "AllocationIncrementInKiB")
    {
        const uint64_t* value = std::get_if<uint64_t>(&property.second);

        if (value == nullptr)
        {
            messages::internalError(aResp->res);
            BMCWEB_LOG_DEBUG
                << "Invalid property type for AllocationIncrementInKiB";
            return;
        }
        aResp->res.jsonValue[jsonPtr]["AllocationIncrementMiB"] =
            (*value) >> 10;
    }
    else if (property.first == "AllocationAlignmentInKiB")
    {
        const uint64_t* value = std::get_if<uint64_t>(&property.second);

        if (value == nullptr)
        {
            messages::internalError(aResp->res);
            BMCWEB_LOG_DEBUG
                << "Invalid property type for AllocationAlignmentInKiB";
            return;
        }
        aResp->res.jsonValue[jsonPtr]["AllocationAlignmentMiB"] =
            (*value) >> 10;
    }
    else if (property.first == "VolatileRegionNumberLimit")
    {
        const uint64_t* value = std::get_if<uint64_t>(&property.second);
        if (value == nullptr)
        {
            messages::internalError(aResp->res);
            return;
        }
        aResp->res.jsonValue[jsonPtr]["VolatileRegionNumberLimit"] = *value;
    }
    else if (property.first == "PmRegionNumberLimit")
    {
        const uint64_t* value = std::get_if<uint64_t>(&property.second);
        if (value == nullptr)
        {
            messages::internalError(aResp->res);
            return;
        }
        aResp->res.jsonValue[jsonPtr]["PersistentRegionNumberLimit"] = *value;
    }
    else if (property.first == "SpareDeviceCount")
    {
        const uint64_t* value = std::get_if<uint64_t>(&property.second);
        if (value == nullptr)
        {
            messages::internalError(aResp->res);
            return;
        }
        aResp->res.jsonValue[jsonPtr]["SpareDeviceCount"] = *value;
    }
    else if (property.first == "IsSpareDeviceInUse")
    {
        const bool* value = std::get_if<bool>(&property.second);
        if (value == nullptr)
        {
            messages::internalError(aResp->res);
            return;
        }
        aResp->res.jsonValue[jsonPtr]["IsSpareDeviceEnabled"] = *value;
    }
    else if (property.first == "IsRankSpareEnabled")
    {
        const bool* value = std::get_if<bool>(&property.second);
        if (value == nullptr)
        {
            messages::internalError(aResp->res);
            return;
        }
        aResp->res.jsonValue[jsonPtr]["IsRankSpareEnabled"] = *value;
    }
    else if (property.first == "MaxAveragePowerLimitmW")
    {
        const auto* value =
            std::get_if<std::vector<uint32_t>>(&property.second);
        if (value == nullptr)
        {
            messages::internalError(aResp->res);
            BMCWEB_LOG_DEBUG
                << "Invalid property type for MaxAveragePowerLimitmW";
            return;
        }
        aResp->res.jsonValue[jsonPtr]["MaxTDPMilliWatts"] = *value;
    }
    else if (property.first == "ConfigurationLocked")
    {
        const bool* value = std::get_if<bool>(&property.second);
        if (value == nullptr)
        {
            messages::internalError(aResp->res);
            return;
        }
        aResp->res.jsonValue[jsonPtr]["ConfigurationLocked"] = *value;
    }
    else if (property.first == "AllowedMemoryModes")
    {
        const std::string* value = std::get_if<std::string>(&property.second);
        if (value == nullptr)
        {
            messages::internalError(aResp->res);
            BMCWEB_LOG_DEBUG << "Invalid property type for FormFactor";
            return;
        }
        constexpr const std::array<const char*, 3> values{"Volatile", "PMEM",
                                                          "Block"};

        for (const char* v : values)
        {
            if (value->ends_with(v))
            {
                aResp->res.jsonValue[jsonPtr]["OperatingMemoryModes"].push_back(
                    v);
                break;
            }
        }
    }
    else if (property.first == "MemoryMedia")
    {
        const std::string* value = std::get_if<std::string>(&property.second);
        if (value == nullptr)
        {
            messages::internalError(aResp->res);
            BMCWEB_LOG_DEBUG << "Invalid property type for MemoryMedia";
            return;
        }
        constexpr const std::array<const char*, 3> values{"DRAM", "NAND",
                                                          "Intel3DXPoint"};

        for (const char* v : values)
        {
            if (value->ends_with(v))
            {
                aResp->res.jsonValue[jsonPtr]["MemoryMedia"].push_back(v);
                break;
            }
        }
    }
    // PersistantMemory.SecurityCapabilites interface
    else if (property.first == "ConfigurationLockCapable" ||
             property.first == "DataLockCapable" ||
             property.first == "PassphraseCapable")
    {
        const bool* value = std::get_if<bool>(&property.second);
        if (value == nullptr)
        {
            messages::internalError(aResp->res);
            return;
        }
        aResp->res.jsonValue[jsonPtr]["SecurityCapabilities"][property.first] =
            *value;
    }
    else if (property.first == "MaxPassphraseCount" ||
             property.first == "PassphraseLockLimit")
    {
        const uint64_t* value = std::get_if<uint64_t>(&property.second);
        if (value == nullptr)
        {
            messages::internalError(aResp->res);
            return;
        }
        aResp->res.jsonValue[jsonPtr]["SecurityCapabilities"][property.first] =
            *value;
    }
}

inline void
    assembleDimmProperties(std::string_view dimmId,
                           const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                           const dbus::utility::DBusPropertiesMap& properties,
                           const nlohmann::json::json_pointer& jsonPtr)
{
    for (const auto& property : properties)
    {
        if (property.first == "MemoryDataWidth")
        {
            const uint16_t* value = std::get_if<uint16_t>(&property.second);
            if (value == nullptr)
            {
                continue;
            }
            aResp->res.jsonValue[jsonPtr]["DataWidthBits"] = *value;
        }
        else if (property.first == "MemorySizeInKB")
        {
            const size_t* memorySize = std::get_if<size_t>(&property.second);
            if (memorySize == nullptr)
            {
                // Important property not in desired type
                messages::internalError(aResp->res);
                return;
            }
            aResp->res.jsonValue[jsonPtr]["CapacityMiB"] = (*memorySize >> 10);
        }
        else if (property.first == "PartNumber")
        {
            const std::string* value =
                std::get_if<std::string>(&property.second);
            if (value == nullptr)
            {
                continue;
            }
            aResp->res.jsonValue[jsonPtr]["PartNumber"] = *value;
        }
        else if (property.first == "SerialNumber")
        {
            const std::string* value =
                std::get_if<std::string>(&property.second);
            if (value == nullptr)
            {
                continue;
            }
            aResp->res.jsonValue[jsonPtr]["SerialNumber"] = *value;
        }
        else if (property.first == "Manufacturer")
        {
            const std::string* value =
                std::get_if<std::string>(&property.second);
            if (value == nullptr)
            {
                continue;
            }
            aResp->res.jsonValue[jsonPtr]["Manufacturer"] = *value;
        }
        else if (property.first == "RevisionCode")
        {
            const uint16_t* value = std::get_if<uint16_t>(&property.second);

            if (value == nullptr)
            {
                messages::internalError(aResp->res);
                BMCWEB_LOG_DEBUG << "Invalid property type for RevisionCode";
                return;
            }
            aResp->res.jsonValue[jsonPtr]["FirmwareRevision"] =
                std::to_string(*value);
        }
        else if (property.first == "Present")
        {
            const bool* value = std::get_if<bool>(&property.second);
            if (value == nullptr)
            {
                messages::internalError(aResp->res);
                BMCWEB_LOG_DEBUG << "Invalid property type for Dimm Presence";
                return;
            }
            if (!*value)
            {
                aResp->res.jsonValue[jsonPtr]["Status"]["State"] = "Absent";
            }
        }
        else if (property.first == "MemoryTotalWidth")
        {
            const uint16_t* value = std::get_if<uint16_t>(&property.second);
            if (value == nullptr)
            {
                continue;
            }
            aResp->res.jsonValue[jsonPtr]["BusWidthBits"] = *value;
        }
        else if (property.first == "ECC")
        {
            const std::string* value =
                std::get_if<std::string>(&property.second);
            if (value == nullptr)
            {
                messages::internalError(aResp->res);
                BMCWEB_LOG_DEBUG << "Invalid property type for ECC";
                return;
            }
            constexpr const std::array<const char*, 4> values{
                "NoECC", "SingleBitECC", "MultiBitECC", "AddressParity"};

            for (const char* v : values)
            {
                if (value->ends_with(v))
                {
                    aResp->res.jsonValue[jsonPtr]["ErrorCorrection"] = v;
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
                BMCWEB_LOG_DEBUG << "Invalid property type for FormFactor";
                return;
            }
            constexpr const std::array<const char*, 11> values{
                "RDIMM",       "UDIMM",       "SO_DIMM",      "LRDIMM",
                "Mini_RDIMM",  "Mini_UDIMM",  "SO_RDIMM_72b", "SO_UDIMM_72b",
                "SO_DIMM_16b", "SO_DIMM_32b", "Die"};

            for (const char* v : values)
            {
                if (value->ends_with(v))
                {
                    aResp->res.jsonValue[jsonPtr]["BaseModuleType"] = v;
                    break;
                }
            }
        }
        else if (property.first == "AllowedSpeedsMT")
        {
            const std::vector<uint16_t>* value =
                std::get_if<std::vector<uint16_t>>(&property.second);
            if (value == nullptr)
            {
                continue;
            }
            nlohmann::json& jValue =
                aResp->res.jsonValue[jsonPtr]["AllowedSpeedsMHz"];
            jValue = nlohmann::json::array();
            for (uint16_t subVal : *value)
            {
                jValue.push_back(subVal);
            }
        }
        else if (property.first == "MemoryAttributes")
        {
            const uint8_t* value = std::get_if<uint8_t>(&property.second);

            if (value == nullptr)
            {
                messages::internalError(aResp->res);
                BMCWEB_LOG_DEBUG
                    << "Invalid property type for MemoryAttributes";
                return;
            }
            aResp->res.jsonValue[jsonPtr]["RankCount"] =
                static_cast<uint64_t>(*value);
        }
        else if (property.first == "MemoryConfiguredSpeedInMhz")
        {
            const uint16_t* value = std::get_if<uint16_t>(&property.second);
            if (value == nullptr)
            {
                continue;
            }
            aResp->res.jsonValue[jsonPtr]["OperatingSpeedMhz"] = *value;
        }
        else if (property.first == "MemoryType")
        {
            const auto* value = std::get_if<std::string>(&property.second);
            if (value != nullptr)
            {
                std::string memoryDeviceType =
                    translateMemoryTypeToRedfish(*value);
                // Values like "Unknown" or "Other" will return empty
                // so just leave off
                if (!memoryDeviceType.empty())
                {
                    aResp->res.jsonValue[jsonPtr]["MemoryDeviceType"] =
                        memoryDeviceType;
                }
                if (value->find("DDR") != std::string::npos)
                {
                    aResp->res.jsonValue[jsonPtr]["MemoryType"] = "DRAM";
                }
                else if (value->ends_with("Logical"))
                {
                    aResp->res.jsonValue[jsonPtr]["MemoryType"] = "IntelOptane";
                }
            }
        }
        // memory location interface
        else if (property.first == "Channel" ||
                 property.first == "MemoryController" ||
                 property.first == "Slot" || property.first == "Socket")
        {
            const std::string* value =
                std::get_if<std::string>(&property.second);
            if (value == nullptr)
            {
                messages::internalError(aResp->res);
                return;
            }
            aResp->res.jsonValue[jsonPtr]["MemoryLocation"][property.first] =
                *value;
        }
        else if (property.first == "SparePartNumber")
        {
            const std::string* value =
                std::get_if<std::string>(&property.second);
            if (value == nullptr)
            {
                messages::internalError(aResp->res);
                return;
            }
            aResp->res.jsonValue[jsonPtr]["SparePartNumber"] = *value;
        }
        else if (property.first == "Model")
        {
            const std::string* value =
                std::get_if<std::string>(&property.second);
            if (value == nullptr)
            {
                messages::internalError(aResp->res);
                return;
            }
            aResp->res.jsonValue[jsonPtr]["Model"] = *value;
        }
        else if (property.first == "LocationCode")
        {
            const std::string* value =
                std::get_if<std::string>(&property.second);
            if (value == nullptr)
            {
                messages::internalError(aResp->res);
                return;
            }
            aResp->res.jsonValue[jsonPtr]["Location"]["PartLocation"]
                                ["ServiceLabel"] = *value;
        }
        else
        {
            getPersistentMemoryProperties(aResp, property, jsonPtr);
        }
    }
    aResp->res.jsonValue[jsonPtr]["@odata.id"] = crow::utility::urlFromPieces(
        "redfish", "v1", "Systems", "system", "Memory", dimmId);
    aResp->res.jsonValue[jsonPtr]["@odata.type"] = "#Memory.v1_11_0.Memory";
}

inline void assembleDimmPartitionData(
    const std::shared_ptr<bmcweb::AsyncResp>& aResp,
    const dbus::utility::DBusPropertiesMap& properties,
    const nlohmann::json::json_pointer& regionPtr)
{
    nlohmann::json::object_t partition;
    for (const auto& [key, val] : properties)
    {
        if (key == "MemoryClassification")
        {
            const std::string* value = std::get_if<std::string>(&val);
            if (value == nullptr)
            {
                messages::internalError(aResp->res);
                return;
            }
            partition[key] = *value;
        }
        else if (key == "OffsetInKiB")
        {
            const uint64_t* value = std::get_if<uint64_t>(&val);
            if (value == nullptr)
            {
                messages::internalError(aResp->res);
                return;
            }

            partition["OffsetMiB"] = (*value >> 10);
        }
        else if (key == "PartitionId")
        {
            const std::string* value = std::get_if<std::string>(&val);
            if (value == nullptr)
            {
                messages::internalError(aResp->res);
                return;
            }
            partition["RegionId"] = *value;
        }

        else if (key == "PassphraseState")
        {
            const bool* value = std::get_if<bool>(&val);
            if (value == nullptr)
            {
                messages::internalError(aResp->res);
                return;
            }
            partition["PassphraseEnabled"] = *value;
        }
        else if (key == "SizeInKiB")
        {
            const uint64_t* value = std::get_if<uint64_t>(&val);
            if (value == nullptr)
            {
                messages::internalError(aResp->res);
                BMCWEB_LOG_DEBUG << "Invalid property type for SizeInKiB";
                return;
            }
            partition["SizeMiB"] = (*value >> 10);
        }
    }
    aResp->res.jsonValue[regionPtr].emplace_back(std::move(partition));
}

namespace memory
{

// An RAII wrapper such that we can populate partitions and health data
// efficiently in the specialized expand handler.
class HealthAndPartition :
    public std::enable_shared_from_this<HealthAndPartition>
{
  public:
    HealthAndPartition(const std::shared_ptr<bmcweb::AsyncResp>& responseIn,
                       std::unordered_map<std::string, std::string>&& mapIn) :
        asyncResponse(responseIn),
        partitionServiceToObjectManagerPath(mapIn)
    {}

    // HealthAndPartition is move-only because we don't want to fetch
    // health or partition data for the same AsyncReponse twice.
    HealthAndPartition(HealthAndPartition&& other) = default;
    HealthAndPartition& operator=(HealthAndPartition&& other) = default;

    HealthAndPartition(const HealthAndPartition&) = delete;
    HealthAndPartition& operator=(const HealthAndPartition&) = delete;

    ~HealthAndPartition()
    {
        BMCWEB_LOG_DEBUG << "HealthAndPartition destructs";
        populatePartitions();
    }

    void getAllPartitions()
    {
        BMCWEB_LOG_DEBUG << "getAllPartitions entered";
        for (const auto& [service, objectManagerPath] :
             partitionServiceToObjectManagerPath)
        {
            crow::connections::systemBus->async_method_call(
                [self{shared_from_this()}](
                    const boost::system::error_code ec,
                    dbus::utility::ManagedObjectType& objects) {
                if (ec)
                {
                    BMCWEB_LOG_DEBUG << "DBUS response error";
                    messages::internalError(self->asyncResponse->res);
                    return;
                }
                BMCWEB_LOG_DEBUG << "Partition objects stored";
                self->objectsForPartition.insert(
                    self->objectsForPartition.end(),
                    std::make_move_iterator(objects.begin()),
                    std::make_move_iterator(objects.end()));
                },
                service, objectManagerPath,
                "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
        }
    }

    std::shared_ptr<bmcweb::AsyncResp> asyncResponse;
    std::shared_ptr<HealthPopulate> health;
    std::unordered_map<std::string, nlohmann::json::json_pointer> dimmToPtr;

  private:
    void populatePartitions()
    {
        BMCWEB_LOG_DEBUG << "populatePartitions entered";
        for (const auto& [objectPath, interfaces] : objectsForPartition)
        {
            for (const auto& [interface, properties] : interfaces)
            {
                if (interface ==
                    "xyz.openbmc_project.Inventory.Item.PersistentMemory.Partition")
                {
                    BMCWEB_LOG_DEBUG << "Found a partition; objectPath="
                                     << objectPath.str;
                    // Example objectPath:
                    // /xyz/openbmc_project/Inventory/Item/Dimm1/Partition1
                    const std::string& dimm =
                        objectPath.parent_path().filename();
                    if (dimm.empty() || !dimmToPtr.contains(dimm))
                    {
                        continue;
                    }
                    assembleDimmPartitionData(asyncResponse, properties,
                                              dimmToPtr[dimm] / "Regions");
                }
            }
        }
        auto it = asyncResponse->res.jsonValue.find("Members");
        if (it == asyncResponse->res.jsonValue.end())
        {
            return;
        }
        auto* members = it->get_ptr<nlohmann::json::array_t*>();
        if (members == nullptr)
        {
            BMCWEB_LOG_ERROR << "Members is not array?!";
            messages::internalError(asyncResponse->res);
        }
        if (!json_util::sortJsonArrayByKey("@odata.id", *members))
        {
            BMCWEB_LOG_ERROR << "Unable to sort the DIMM collection";
            messages::internalError(asyncResponse->res);
        }
    }
    std::unordered_map<std::string, std::string>
        partitionServiceToObjectManagerPath;
    dbus::utility::ManagedObjectType objectsForPartition;
};
} // namespace memory

inline void getAllDimmsCallback(
    const std::shared_ptr<memory::HealthAndPartition>& healthAndPartition,
    const std::optional<std::string>& dimmId,
    const boost::system::error_code ec,
    const dbus::utility::ManagedObjectType& objects)
{
    BMCWEB_LOG_DEBUG << "getAllDimmsCallback entered";
    if (ec)
    {
        BMCWEB_LOG_DEBUG << "DBUS response error";
        messages::internalError(healthAndPartition->asyncResponse->res);
        return;
    }
    for (const auto& [objectPath, interfaces] : objects)
    {
        std::string thisDimmID = objectPath.filename();
        if (thisDimmID.empty())
        {
            continue;
        }
        if (dimmId && *dimmId != thisDimmID)
        {
            continue;
        }
        bool hasDimmInterface = false;
        for (const auto& [interface, _] : interfaces)
        {
            if (interface == "xyz.openbmc_project.Inventory.Item.Dimm")
            {
                hasDimmInterface = true;
                break;
            }
        }
        if (!hasDimmInterface)
        {
            continue;
        }
        BMCWEB_LOG_DEBUG << "Found a dimm; objectPath=" << objectPath.str;
        nlohmann::json::json_pointer healthPtr;
        if (!dimmId)
        {
            size_t index =
                healthAndPartition->asyncResponse->res.jsonValue["Members"]
                    .size();
            healthPtr = "/Members"_json_pointer / index / "Status";
            healthAndPartition->dimmToPtr[thisDimmID] =
                "/Members"_json_pointer / index;
        }
        else
        {
            healthPtr = "/Status"_json_pointer;
            healthAndPartition->dimmToPtr[thisDimmID] = ""_json_pointer;
        }
        auto dimmHealth = std::make_shared<HealthPopulate>(
            healthAndPartition->asyncResponse, healthPtr);
        dimmHealth->selfPath = objectPath;
        if (healthAndPartition->health == nullptr)
        {
            dimmHealth->populate();
            healthAndPartition->health = std::move(dimmHealth);
        }
        else
        {
            // if there is already a health object, append other
            // objects to its children to avoid duplicate DBus
            // queries
            healthAndPartition->health->children.push_back(
                std::move(dimmHealth));
        }

        healthAndPartition->asyncResponse->res.jsonValue[healthAndPartition->dimmToPtr[thisDimmID]]["Id"] = dimmId;
        healthAndPartition->asyncResponse->res.jsonValue[healthAndPartition->dimmToPtr[thisDimmID]]["Name"] = "DIMM Slot";
        healthAndPartition->asyncResponse->res.jsonValue[healthAndPartition->dimmToPtr[thisDimmID]]["Status"]["State"] = "Enabled";
        healthAndPartition->asyncResponse->res.jsonValue[healthAndPartition->dimmToPtr[thisDimmID]]["Status"]["Health"] = "OK";
        for (const auto& [interface, properties] : interfaces)
        {
            assembleDimmProperties(
                thisDimmID, healthAndPartition->asyncResponse, properties,
                healthAndPartition->dimmToPtr[thisDimmID]);
        }
    }
    if (!dimmId)
    {
        healthAndPartition->asyncResponse->res
            .jsonValue["Members@odata.count"] =
            healthAndPartition->asyncResponse->res.jsonValue["Members"].size();
        return;
    }
}

inline void getAllDimms(
    const std::shared_ptr<memory::HealthAndPartition>& healthAndPartition,
    const std::optional<std::string>& dimmId,
    const std::unordered_map<std::string, std::string>&
        serviceToObjectManagerPath)
{
    if (!dimmId)
    {
        healthAndPartition->asyncResponse->res.jsonValue["Members"] =
            nlohmann::json::array();
        healthAndPartition->asyncResponse->res
            .jsonValue["Members@odata.count"] = 0;
    }
    for (const auto& [service, objectManagerPath] : serviceToObjectManagerPath)
    {
        crow::connections::systemBus->async_method_call(
            [healthAndPartition,
             dimmId{dimmId}](const boost::system::error_code ec,
                             dbus::utility::ManagedObjectType& objects) {
            getAllDimmsCallback(healthAndPartition, dimmId, ec, objects);
            },
            service, objectManagerPath, "org.freedesktop.DBus.ObjectManager",
            "GetManagedObjects");
    }
}

inline void getObjectManagerPathsGivenServicesCallBack(
    const std::shared_ptr<bmcweb::AsyncResp>& aResp,
    const std::optional<std::string>& dimmId,
    const std::unordered_set<std::string>& dimmServices,
    const std::unordered_set<std::string>& partitionServices,
    const boost::system::error_code ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec)
    {
        BMCWEB_LOG_DEBUG << "DBUS response error";
        messages::internalError(aResp->res);
        return;
    }

    BMCWEB_LOG_DEBUG << "There are " << dimmServices.size()
                     << " services which implement the DIMM interface";
    BMCWEB_LOG_DEBUG << "There are " << partitionServices.size()
                     << " services which implement the Partition interface";

    std::unordered_map<std::string, std::string> dimmServiceToObjectManagerPath;
    std::unordered_map<std::string, std::string>
        partitionServiceToObjectManagerPath;
    for (const auto& [objectManagerPath, mapperServiceMap] : subtree)
    {
        for (const auto& [service, _] : mapperServiceMap)
        {
            if (dimmServices.contains(service))
            {
                dimmServiceToObjectManagerPath[service] = objectManagerPath;
            }
            if (partitionServices.contains(service))
            {
                partitionServiceToObjectManagerPath[service] =
                    objectManagerPath;
            }
        }
    }

    auto healthAndPartition = std::make_shared<memory::HealthAndPartition>(
        aResp, std::move(partitionServiceToObjectManagerPath));
    healthAndPartition->getAllPartitions();
    getAllDimms(healthAndPartition, dimmId, dimmServiceToObjectManagerPath);
}

inline void getObjectManagerPathsGivenServices(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::optional<std::string>& dimmId,
    std::unordered_set<std::string>&& dimmServices,
    std::unordered_set<std::string>&& partitionServices)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp{asyncResp}, dimmId{dimmId},
         dimmServices{std::move(dimmServices)},
         partitionServices{std::move(partitionServices)}](
            const boost::system::error_code ec,
            const dbus::utility::MapperGetSubTreeResponse& subtree) {
        getObjectManagerPathsGivenServicesCallBack(
            asyncResp, dimmId, dimmServices, partitionServices, ec, subtree);
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree", "/", 0,
        std::array<std::string, 1>{"org.freedesktop.DBus.ObjectManager"});
}

// If |dimmId| is set, will only keep the first matched dimmId.
inline void getDimmData(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::optional<std::string>& dimmId)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp{asyncResp}, dimmId{dimmId}](
            const boost::system::error_code ec,
            const dbus::utility::MapperGetSubTreeResponse& subtree) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG << "DBUS response error";
            messages::internalError(asyncResp->res);
            return;
        }
        BMCWEB_LOG_DEBUG
            << "Collect services that implement DIMM/partition interface for "
            << dimmId.value_or("all DIMMs");
        bool foundGivenDimm = false;
        std::unordered_set<std::string> dimmServices;
        std::unordered_set<std::string> partitionServices;
        for (const auto& [path, object] : subtree)
        {
            BMCWEB_LOG_DEBUG << "Object path=" << path;
            sdbusplus::message::object_path objectPath(path);
            for (const auto& [service, interfaces] : object)
            {
                for (const std::string& interface : interfaces)
                {
                    if (interface == "xyz.openbmc_project.Inventory.Item.Dimm")
                    {
                        if (dimmId && objectPath.filename() != *dimmId)
                        {
                            continue;
                        }
                        BMCWEB_LOG_DEBUG << "Added DIMM services " << service;
                        dimmServices.insert(service);
                        foundGivenDimm = true;
                    }

                    // partitions are separate as there can be multiple
                    // per device, i.e.
                    // /xyz/openbmc_project/Inventory/Item/Dimm1/Partition1
                    // /xyz/openbmc_project/Inventory/Item/Dimm1/Partition2
                    if (interface ==
                        "xyz.openbmc_project.Inventory.Item.PersistentMemory.Partition")
                    {
                        if (dimmId &&
                            objectPath.parent_path().filename() != *dimmId)
                        {
                            continue;
                        }
                        BMCWEB_LOG_DEBUG << "Added Partition services "
                                         << service;
                        partitionServices.insert(service);
                    }
                }
            }
            // Fetch the first matched DIMM
            if (dimmId && foundGivenDimm)
            {
                break;
            }
        }
        if (dimmId && !foundGivenDimm)
        {
            messages::resourceNotFound(asyncResp->res, "Memory", *dimmId);
            return;
        }
        getObjectManagerPathsGivenServices(asyncResp, dimmId,
                                           std::move(dimmServices),
                                           std::move(partitionServices));
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", 0,
        std::array<const char*, 2>{
            "xyz.openbmc_project.Inventory.Item.Dimm",
            "xyz.openbmc_project.Inventory.Item.PersistentMemory.Partition"});
}

inline void requestRoutesMemoryCollection(App& app)
{
    /**
     * Functions triggers appropriate requests on DBus
     */
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/Memory/")
        .privileges(redfish::privileges::getMemoryCollection)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
        query_param::Query delegated;
        query_param::QueryCapabilities capabilities = {
            .canDelegateExpandLevel = 1,
        };
        if (!redfish::setUpRedfishRouteWithDelegation(app, req, asyncResp,
                                                      delegated, capabilities))
        {
            return;
        }
        asyncResp->res.jsonValue["@odata.type"] =
            "#MemoryCollection.MemoryCollection";
        asyncResp->res.jsonValue["Name"] = "Memory Module Collection";
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Systems/system/Memory";

        if (delegated.expandLevel > 0 &&
            delegated.expandType != query_param::ExpandType::None)
        {
            BMCWEB_LOG_DEBUG << "Use efficient expand handler";
            getDimmData(asyncResp, std::nullopt);
        }
        else
        {
            BMCWEB_LOG_DEBUG << "Use default expand handler";
            collection_util::getCollectionMembers(
                asyncResp, "/redfish/v1/Systems/system/Memory",
                {"xyz.openbmc_project.Inventory.Item.Dimm"});
        }
        });
}

inline void requestRoutesMemory(App& app)
{
    /**
     * Functions triggers appropriate requests on DBus
     */
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/Memory/<str>/")
        .privileges(redfish::privileges::getMemory)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& dimmId) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        getDimmData(asyncResp, dimmId);
        });
}

} // namespace redfish
