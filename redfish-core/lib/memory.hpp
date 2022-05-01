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
#include <nlohmann/json.hpp>
#include <query.hpp>
#include <registries/privilege_registry.hpp>
#include <utils/collection.hpp>
#include <utils/hex_utils.hpp>
#include <utils/json_utils.hpp>

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

inline void dimmPropToHex(
    const std::shared_ptr<bmcweb::AsyncResp>& aResp, const char* key,
    const std::pair<std::string, dbus::utility::DbusVariantType>& property,
    nlohmann::json& json)
{
    const uint16_t* value = std::get_if<uint16_t>(&property.second);
    if (value == nullptr)
    {
        messages::internalError(aResp->res);
        BMCWEB_LOG_DEBUG << "Invalid property type for " << property.first;
        return;
    }

    json[key] = "0x" + intToHexString(*value, 4);
}

inline void getPersistentMemoryProperties(
    const std::shared_ptr<bmcweb::AsyncResp>& aResp,
    const std::pair<std::string, dbus::utility::DbusVariantType>& property,
    nlohmann::json& json)
{
    if (property.first == "ModuleManufacturerID")
    {
        dimmPropToHex(aResp, "ModuleManufacturerID", property, json);
    }
    else if (property.first == "ModuleProductID")
    {
        dimmPropToHex(aResp, "ModuleProductID", property, json);
    }
    else if (property.first == "SubsystemVendorID")
    {
        dimmPropToHex(aResp, "MemorySubsystemControllerManufacturerID",
                      property, json);
    }
    else if (property.first == "SubsystemDeviceID")
    {
        dimmPropToHex(aResp, "MemorySubsystemControllerProductID", property,
                      json);
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
        json["VolatileRegionSizeLimitMiB"] = (*value) >> 10;
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
        json["PersistentRegionSizeLimitMiB"] = (*value) >> 10;
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
        json["VolatileSizeMiB"] = (*value) >> 10;
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
        json["NonVolatileSizeMiB"] = (*value) >> 10;
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
        json["CacheSizeMiB"] = (*value >> 10);
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
        json["VolatileRegionSizeMaxMiB"] = (*value) >> 10;
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
        json["PersistentRegionSizeMaxMiB"] = (*value) >> 10;
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
        json["AllocationIncrementMiB"] = (*value) >> 10;
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
        json["AllocationAlignmentMiB"] = (*value) >> 10;
    }
    else if (property.first == "VolatileRegionNumberLimit")
    {
        const uint64_t* value = std::get_if<uint64_t>(&property.second);
        if (value == nullptr)
        {
            messages::internalError(aResp->res);
            return;
        }
        json["VolatileRegionNumberLimit"] = *value;
    }
    else if (property.first == "PmRegionNumberLimit")
    {
        const uint64_t* value = std::get_if<uint64_t>(&property.second);
        if (value == nullptr)
        {
            messages::internalError(aResp->res);
            return;
        }
        json["PersistentRegionNumberLimit"] = *value;
    }
    else if (property.first == "SpareDeviceCount")
    {
        const uint64_t* value = std::get_if<uint64_t>(&property.second);
        if (value == nullptr)
        {
            messages::internalError(aResp->res);
            return;
        }
        json["SpareDeviceCount"] = *value;
    }
    else if (property.first == "IsSpareDeviceInUse")
    {
        const bool* value = std::get_if<bool>(&property.second);
        if (value == nullptr)
        {
            messages::internalError(aResp->res);
            return;
        }
        json["IsSpareDeviceEnabled"] = *value;
    }
    else if (property.first == "IsRankSpareEnabled")
    {
        const bool* value = std::get_if<bool>(&property.second);
        if (value == nullptr)
        {
            messages::internalError(aResp->res);
            return;
        }
        json["IsRankSpareEnabled"] = *value;
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
        json["MaxTDPMilliWatts"] = *value;
    }
    else if (property.first == "ConfigurationLocked")
    {
        const bool* value = std::get_if<bool>(&property.second);
        if (value == nullptr)
        {
            messages::internalError(aResp->res);
            return;
        }
        json["ConfigurationLocked"] = *value;
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
            if (boost::ends_with(*value, v))
            {
                json["OperatingMemoryModes"].push_back(v);
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
            if (boost::ends_with(*value, v))
            {
                json["MemoryMedia"].push_back(v);
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
        json["SecurityCapabilities"][property.first] = *value;
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
        json["SecurityCapabilities"][property.first] = *value;
    }
}

inline void assembleDimmProperties(
    std::string_view dimmId, const std::shared_ptr<bmcweb::AsyncResp>& aResp,
    const dbus::utility::DBusPropertiesMap& properties, nlohmann::json& json)
{
    json["Id"] = dimmId;
    json["Name"] = "DIMM Slot";
    json["Status"]["State"] = "Enabled";
    json["Status"]["Health"] = "OK";

    for (const auto& property : properties)
    {
        if (property.first == "MemoryDataWidth")
        {
            const uint16_t* value = std::get_if<uint16_t>(&property.second);
            if (value == nullptr)
            {
                continue;
            }
            json["DataWidthBits"] = *value;
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
            json["CapacityMiB"] = (*memorySize >> 10);
        }
        else if (property.first == "PartNumber")
        {
            const std::string* value =
                std::get_if<std::string>(&property.second);
            if (value == nullptr)
            {
                continue;
            }
            json["PartNumber"] = *value;
        }
        else if (property.first == "SerialNumber")
        {
            const std::string* value =
                std::get_if<std::string>(&property.second);
            if (value == nullptr)
            {
                continue;
            }
            json["SerialNumber"] = *value;
        }
        else if (property.first == "Manufacturer")
        {
            const std::string* value =
                std::get_if<std::string>(&property.second);
            if (value == nullptr)
            {
                continue;
            }
            json["Manufacturer"] = *value;
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
            json["FirmwareRevision"] = std::to_string(*value);
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
                json["Status"]["State"] = "Absent";
            }
        }
        else if (property.first == "MemoryTotalWidth")
        {
            const uint16_t* value = std::get_if<uint16_t>(&property.second);
            if (value == nullptr)
            {
                continue;
            }
            json["BusWidthBits"] = *value;
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
                if (boost::ends_with(*value, v))
                {
                    json["ErrorCorrection"] = v;
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
                if (boost::ends_with(*value, v))
                {
                    json["BaseModuleType"] = v;
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
            nlohmann::json& jValue = json["AllowedSpeedsMHz"];
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
            json["RankCount"] = static_cast<uint64_t>(*value);
        }
        else if (property.first == "MemoryConfiguredSpeedInMhz")
        {
            const uint16_t* value = std::get_if<uint16_t>(&property.second);
            if (value == nullptr)
            {
                continue;
            }
            json["OperatingSpeedMhz"] = *value;
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
                    json["MemoryDeviceType"] = memoryDeviceType;
                }
                if (value->find("DDR") != std::string::npos)
                {
                    json["MemoryType"] = "DRAM";
                }
                else if (boost::ends_with(*value, "Logical"))
                {
                    json["MemoryType"] = "IntelOptane";
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
            json["MemoryLocation"][property.first] = *value;
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
            json["SparePartNumber"] = *value;
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
            json["Model"] = *value;
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
            json["Location"]["PartLocation"]["ServiceLabel"] = *value;
        }
        else
        {
            getPersistentMemoryProperties(aResp, property, json);
        }
    }
}

inline void getDimmDataByService(std::shared_ptr<bmcweb::AsyncResp> aResp,
                                 const std::string& dimmId,
                                 const std::string& service,
                                 const std::string& objPath)
{
    auto health = std::make_shared<HealthPopulate>(aResp);
    health->selfPath = objPath;
    health->populate();

    BMCWEB_LOG_DEBUG << "Get available system components.";
    crow::connections::systemBus->async_method_call(
        [dimmId, aResp{std::move(aResp)}](
            const boost::system::error_code ec,
            const dbus::utility::DBusPropertiesMap& properties) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(aResp->res);
                return;
            }
            assembleDimmProperties(dimmId, aResp, properties,
                                   aResp->res.jsonValue);
        },
        service, objPath, "org.freedesktop.DBus.Properties", "GetAll", "");
}

inline void assembleDimmPartitionData(
    const std::shared_ptr<bmcweb::AsyncResp>& aResp,
    const dbus::utility::DBusPropertiesMap& properties,
    nlohmann::json& partition)
{
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
}

inline void getDimmPartitionData(std::shared_ptr<bmcweb::AsyncResp> aResp,
                                 const std::string& service,
                                 const std::string& path)
{
    crow::connections::systemBus->async_method_call(
        [aResp{std::move(aResp)}](
            const boost::system::error_code ec,
            const dbus::utility::DBusPropertiesMap& properties) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(aResp->res);

                return;
            }
            nlohmann::json& partition =
                aResp->res.jsonValue["Regions"].emplace_back(
                    nlohmann::json::object());
            assembleDimmPartitionData(aResp, properties, partition);
        },

        service, path, "org.freedesktop.DBus.Properties", "GetAll",
        "xyz.openbmc_project.Inventory.Item.PersistentMemory.Partition");
}

namespace memory
{

// An RAII wrapper such that we can populate partitions and health data
// efficiently in the specilized expand handler.
class MemoryAsyncResponse :
    public std::enable_shared_from_this<MemoryAsyncResponse>
{
  public:
    MemoryAsyncResponse(const std::shared_ptr<bmcweb::AsyncResp>& asyncResponse,
                        boost::container::flat_map<std::string, std::string>
                            partitionServiceToObjectManagerPath) :
        asyncResponse(asyncResponse),
        partitionServiceToObjectManagerPath(
            std::move(partitionServiceToObjectManagerPath))
    {}

    // MemoryAsyncResponse is move-only because we don't want to fetch health or
    // partition data for the same AsyncReponse twice.
    MemoryAsyncResponse(MemoryAsyncResponse&& other) = default;
    MemoryAsyncResponse& operator=(MemoryAsyncResponse&& other) = default;

    MemoryAsyncResponse(const MemoryAsyncResponse&) = delete;
    MemoryAsyncResponse& operator=(const MemoryAsyncResponse&) = delete;

    ~MemoryAsyncResponse()
    {
        BMCWEB_LOG_DEBUG << "MemoryAsyncResponse distructs";
        if (asyncResponse->res.resultInt() < 200 ||
            asyncResponse->res.resultInt() >= 400)
        {
            return;
        }
        if (health != nullptr)
        {
            BMCWEB_LOG_DEBUG << "populate health data";
            health->populate();
        }
        populatePartitions();
    }

    void getAllPartitions()
    {
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
    boost::container::flat_map<std::string, nlohmann::json::json_pointer>
        dimmToPtr;

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
                    for (const auto& [dimm, ptr] : dimmToPtr)
                    {
                        // This partition is for the dimm
                        if (objectPath.str.find(dimm) != std::string::npos)
                        {
                            nlohmann::json& partition =
                                asyncResponse->res.jsonValue[ptr]["Regions"]
                                    .emplace_back(nlohmann::json::object());
                            assembleDimmPartitionData(asyncResponse, properties,
                                                      partition);
                        }
                    }
                }
            }
        }
    }
    boost::container::flat_map<std::string, std::string>
        partitionServiceToObjectManagerPath;
    dbus::utility::ManagedObjectType objectsForPartition;
};
} // namespace memory

inline void getAllDimmsCallback(
    const std::shared_ptr<memory::MemoryAsyncResponse>& memoryAsyncResp,
    const boost::system::error_code ec,
    dbus::utility::ManagedObjectType& objects)
{
    BMCWEB_LOG_DEBUG << "getAllDimmsCallback entered";
    if (ec)
    {
        BMCWEB_LOG_DEBUG << "DBUS response error";
        messages::internalError(memoryAsyncResp->asyncResponse->res);
        return;
    }
    for (const auto& [objectPath, interfaces] : objects)
    {
        for (const auto& [interface, properties] : interfaces)
        {
            nlohmann::json::json_pointer ptr = "/Members"_json_pointer;
            if (interface == "xyz.openbmc_project.Inventory.Item.Dimm")
            {
                nlohmann::json& members =
                    memoryAsyncResp->asyncResponse->res.jsonValue["Members"];
                members.push_back(
                    {{"@odata.id", "/redfish/v1/Systems/system/Memory/" +
                                       objectPath.filename()},
                     {"@odata.type", "#Memory.v1_11_0.Memory"}});
                size_t index = members.size() - 1;
                auto dimmHealth = std::make_shared<HealthPopulate>(
                    memoryAsyncResp->asyncResponse, ptr / index / "Status");
                dimmHealth->selfPath = objectPath;
                memoryAsyncResp->dimmToPtr[objectPath.filename()] = ptr / index;
                if (memoryAsyncResp->health == nullptr)
                {
                    memoryAsyncResp->health = std::move(dimmHealth);
                }
                else
                {
                    // if there is already a health object, append other objects
                    // to its children to avoid duplicate DBus queries
                    memoryAsyncResp->health->children.push_back(
                        std::move(dimmHealth));
                }
                assembleDimmProperties(objectPath.filename(),
                                       memoryAsyncResp->asyncResponse,
                                       properties, members.back());
            }
        }
    }
    memoryAsyncResp->asyncResponse->res.jsonValue["Members@odata.count"] =
        memoryAsyncResp->asyncResponse->res.jsonValue["Members"].size();
}

inline void getAllDimms(
    const std::shared_ptr<memory::MemoryAsyncResponse>& memoryAsyncResp,
    const boost::container::flat_map<std::string, std::string>&
        serviceToObjectManagerPath)
{
    memoryAsyncResp->asyncResponse->res.jsonValue["Members"] =
        nlohmann::json::array();
    memoryAsyncResp->asyncResponse->res.jsonValue["Members@odata.count"] = 0;
    for (const auto& [service, objectManagerPath] : serviceToObjectManagerPath)
    {
        crow::connections::systemBus->async_method_call(
            [memoryAsyncResp](const boost::system::error_code ec,
                              dbus::utility::ManagedObjectType& objects) {
                getAllDimmsCallback(memoryAsyncResp, ec, objects);
            },
            service, objectManagerPath, "org.freedesktop.DBus.ObjectManager",
            "GetManagedObjects");
    }
}

inline void getObjectManagerPathsGivenServicesCallBack(
    const std::shared_ptr<bmcweb::AsyncResp>& aResp,
    const boost::container::flat_set<std::string>& dimmServices,
    const boost::container::flat_set<std::string>& partitionServices,
    const boost::system::error_code ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec)
    {
        BMCWEB_LOG_DEBUG << "DBUS response error";
        messages::internalError(aResp->res);
        return;
    }

    boost::container::flat_set<std::string> dimmServicesSet(
        dimmServices.begin(), dimmServices.end());
    BMCWEB_LOG_DEBUG << "There are " << dimmServicesSet.size()
                     << " services which implement the DIMM interface";

    boost::container::flat_set<std::string> paritionServicesSet(
        partitionServices.begin(), partitionServices.end());
    BMCWEB_LOG_DEBUG << "There are " << partitionServices.size()
                     << " services which implement the Partition interface";

    boost::container::flat_map<std::string, std::string>
        dimmServiceToObjectManagerPath;
    boost::container::flat_map<std::string, std::string>
        partitionServiceToObjectManagerPath;
    for (const auto& [objectManagerPath, mapperServiceMap] : subtree)
    {
        for (const auto& [service, _] : mapperServiceMap)
        {
            if (dimmServicesSet.contains(service))
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

    auto memoryAsyncResp = std::make_shared<memory::MemoryAsyncResponse>(
        aResp, std::move(partitionServiceToObjectManagerPath));
    memoryAsyncResp->getAllPartitions();
    getAllDimms(memoryAsyncResp, dimmServiceToObjectManagerPath);
}

inline void getObjectManagerPathsGivenServices(
    std::shared_ptr<bmcweb::AsyncResp> asyncResp,
    boost::container::flat_set<std::string> dimmServices,
    boost::container::flat_set<std::string> partitionServices)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp{std::move(asyncResp)}, dimmServices{std::move(dimmServices)},
         partitionServices{std::move(partitionServices)}](
            const boost::system::error_code ec,
            const dbus::utility::MapperGetSubTreeResponse& subtree) {
            getObjectManagerPathsGivenServicesCallBack(
                asyncResp, dimmServices, partitionServices, ec, subtree);
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree", "/", 0,
        std::array<std::string, 1>{"org.freedesktop.DBus.ObjectManager"});
}

inline void getDimmDataExpanded(std::shared_ptr<bmcweb::AsyncResp> asyncResp)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp{std::move(asyncResp)}](
            const boost::system::error_code ec,
            const dbus::utility::MapperGetSubTreeResponse& subtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(asyncResp->res);

                return;
            }
            BMCWEB_LOG_DEBUG
                << "Collect services that implement DIMM/partition interface";
            boost::container::flat_set<std::string> dimmServices;
            boost::container::flat_set<std::string> partitionServices;
            for (const auto& [path, object] : subtree)
            {
                for (const auto& [service, interfaces] : object)
                {
                    for (const auto& interface : interfaces)
                    {
                        if (interface ==
                            "xyz.openbmc_project.Inventory.Item.Dimm")
                        {
                            BMCWEB_LOG_DEBUG << "Added DIMM services "
                                             << service;
                            dimmServices.insert(service);
                        }

                        // partitions are separate as there can be multiple
                        // per device, i.e.
                        // /xyz/openbmc_project/Inventory/Item/Dimm1/Partition1
                        // /xyz/openbmc_project/Inventory/Item/Dimm1/Partition2
                        if (interface ==
                            "xyz.openbmc_project.Inventory.Item.PersistentMemory.Partition")
                        {
                            BMCWEB_LOG_DEBUG << "Added Partition services "
                                             << service;
                            partitionServices.insert(service);
                        }
                    }
                }
            }
            getObjectManagerPathsGivenServices(asyncResp,
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

inline void getDimmData(std::shared_ptr<bmcweb::AsyncResp> aResp,
                        const std::string& dimmId)
{
    BMCWEB_LOG_DEBUG << "Get available system dimm resources.";
    crow::connections::systemBus->async_method_call(
        [dimmId, aResp{std::move(aResp)}](
            const boost::system::error_code ec,
            const dbus::utility::MapperGetSubTreeResponse& subtree) {
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
                        for (const auto& interface : interfaces)
                        {
                            if (interface ==
                                "xyz.openbmc_project.Inventory.Item.Dimm")
                            {
                                getDimmDataByService(aResp, dimmId, service,
                                                     path);
                                found = true;
                            }

                            // partitions are separate as there can be multiple
                            // per
                            // device, i.e.
                            // /xyz/openbmc_project/Inventory/Item/Dimm1/Partition1
                            // /xyz/openbmc_project/Inventory/Item/Dimm1/Partition2
                            if (interface ==
                                "xyz.openbmc_project.Inventory.Item.PersistentMemory.Partition")
                            {
                                getDimmPartitionData(aResp, service, path);
                            }
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
                if (!redfish::setUpRedfishRouteWithDelegation(
                        app, req, asyncResp->res, delegated, capabilities))
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
                    BMCWEB_LOG_DEBUG << "Use efficient expand hanlder";
                    getDimmDataExpanded(asyncResp);
                }
                else
                {
                    BMCWEB_LOG_DEBUG << "Use default expand hanlder";
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
                if (!redfish::setUpRedfishRoute(app, req, asyncResp->res))
                {
                    return;
                }
                asyncResp->res.jsonValue["@odata.type"] =
                    "#Memory.v1_11_0.Memory";
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/Systems/system/Memory/" + dimmId;

                getDimmData(asyncResp, dimmId);
            });
}

} // namespace redfish
