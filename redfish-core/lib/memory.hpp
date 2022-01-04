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
#include <dbus_utility.hpp>
#include <registries/privilege_registry.hpp>
#include <utils/collection.hpp>
#include <utils/hex_utils.hpp>
#include <utils/json_utils.hpp>

namespace redfish
{

using DimmProperties =
    boost::container::flat_map<std::string, dbus::utility::DbusVariantType>;

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

inline void dimmPropToHex(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                          const char* key,
                          const std::optional<const uint16_t*> value)
{
    if (value)
    {
        aResp->res.jsonValue[key] = "0x" + intToHexString(**value);
    }
}

inline void kibPropToMib(nlohmann::json& jsonData, const char* key,
                         const std::optional<const uint64_t*> value)
{
    if (value)
    {
        jsonData[key] = (**value) >> 10;
    }
}

template <typename T>
inline void assignSecurityCapabilitiesProp(
    const std::shared_ptr<bmcweb::AsyncResp>& aResp, const char* key,
    const std::optional<const T*> value)
{
    if (value)
    {
        aResp->res.jsonValue["SecurityCapabilities"][key] = **value;
    }
}

inline void getPersistentMemoryProperties(
    const std::shared_ptr<bmcweb::AsyncResp>& aResp,
    const DimmProperties& properties)
{
    std::optional<const uint16_t*> moduleManufacturerID, moduleProductID,
        subsystemVendorID, subsystemDeviceID;
    std::optional<const uint64_t*> volatileRegionSizeLimitInKiB,
        pmRegionSizeLimitInKiB, volatileSizeInKiB, pmSizeInKiB, cacheSizeInKB,
        voltaileRegionMaxSizeInKib, pmRegionMaxSizeInKiB,
        allocationIncrementInKiB, allocationAlignmentInKiB, maxPassphraseCount,
        passphraseLockLimit, volatileRegionNumberLimit, pmRegionNumberLimit,
        spareDeviceCount;
    std::optional<const bool*> configurationLockCapable, dataLockCapable,
        passphraseCapable, isRankSpareEnabled, configurationLocked;
    std::optional<const uint32_t*> maxAveragePowerLimitmW;
    std::optional<const std::string*> allowedMemoryModes, memoryMedia;

    std::optional<sdbusplus::UnpackError> error =
        sdbusplus::unpackPropertiesNoThrow(
            properties, "ModuleManufacturerID", moduleManufacturerID,
            "ModuleProductID", moduleProductID, "SubsystemVendorID",
            subsystemVendorID, "SubsystemDeviceID", subsystemDeviceID,
            "VolatileRegionSizeLimitInKiB", volatileRegionSizeLimitInKiB,
            "PmRegionSizeLimitInKiB", pmRegionSizeLimitInKiB,
            "VolatileSizeInKiB", volatileSizeInKiB, "PmSizeInKiB", pmSizeInKiB,
            "CacheSizeInKB", cacheSizeInKB, "VoltaileRegionMaxSizeInKib",
            voltaileRegionMaxSizeInKib, "PmRegionMaxSizeInKiB",
            pmRegionMaxSizeInKiB, "AllocationIncrementInKiB",
            allocationIncrementInKiB, "AllocationAlignmentInKiB",
            allocationAlignmentInKiB, "VolatileRegionNumberLimit",
            volatileRegionNumberLimit, "PmRegionNumberLimit",
            pmRegionNumberLimit, "SpareDeviceCount", spareDeviceCount,
            "IsRankSpareEnabled", isRankSpareEnabled, "MaxAveragePowerLimitmW",
            maxAveragePowerLimitmW, "ConfigurationLocked", configurationLocked,
            "AllowedMemoryModes", allowedMemoryModes, "MemoryMedia",
            memoryMedia, "ConfigurationLockCapable", configurationLockCapable,
            "DataLockCapable", dataLockCapable, "PassphraseCapable",
            passphraseCapable, "MaxPassphraseCount", maxPassphraseCount,
            "PassphraseLockLimit", passphraseLockLimit);

    if (error)
    {
        BMCWEB_LOG_DEBUG << "Invalid property type at index " << error->index;
        messages::internalError(aResp->res);
        return;
    }

    dimmPropToHex(aResp, "ModuleManufacturerID", moduleManufacturerID);
    dimmPropToHex(aResp, "ModuleProductID", moduleManufacturerID);
    dimmPropToHex(aResp, "SubsystemVendorID", moduleManufacturerID);
    dimmPropToHex(aResp, "SubsystemDeviceID", moduleManufacturerID);

    kibPropToMib(aResp->res.jsonValue, "VolatileRegionSizeLimitMiB",
                 volatileRegionSizeLimitInKiB);
    kibPropToMib(aResp->res.jsonValue, "PmRegionSizeLimitMiB",
                 pmRegionSizeLimitInKiB);
    kibPropToMib(aResp->res.jsonValue, "VolatileSizeMiB", volatileSizeInKiB);
    kibPropToMib(aResp->res.jsonValue, "PmSizeMiB", pmSizeInKiB);
    kibPropToMib(aResp->res.jsonValue, "CacheSizeInKB", cacheSizeInKB);
    kibPropToMib(aResp->res.jsonValue, "VoltaileRegionMaxSizeInKib",
                 voltaileRegionMaxSizeInKib);
    kibPropToMib(aResp->res.jsonValue, "PmRegionMaxSizeMiB",
                 pmRegionMaxSizeInKiB);
    kibPropToMib(aResp->res.jsonValue, "AllocationIncrementMiB",
                 allocationIncrementInKiB);
    kibPropToMib(aResp->res.jsonValue, "AllocationAlignmentMiB",
                 allocationAlignmentInKiB);

    json_util::assignIf(aResp->res.jsonValue, "IsRankSpareEnabled",
                        isRankSpareEnabled);
    json_util::assignIf(aResp->res.jsonValue, "MaxTDPMilliWatts",
                        maxAveragePowerLimitmW);
    json_util::assignIf(aResp->res.jsonValue, "VolatileRegionNumberLimit",
                        volatileRegionNumberLimit);
    json_util::assignIf(aResp->res.jsonValue, "PersistentRegionNumberLimit",
                        pmRegionNumberLimit);
    json_util::assignIf(aResp->res.jsonValue, "SpareDeviceCount",
                        spareDeviceCount);
    json_util::assignIf(aResp->res.jsonValue, "ConfigurationLocked",
                        configurationLocked);

    assignSecurityCapabilitiesProp(aResp, "ConfigurationLockCapable",
                                   configurationLockCapable);
    assignSecurityCapabilitiesProp(aResp, "DataLockCapable", dataLockCapable);
    assignSecurityCapabilitiesProp(aResp, "PassphraseCapable",
                                   passphraseCapable);
    assignSecurityCapabilitiesProp(aResp, "MaxPassphraseCount",
                                   maxPassphraseCount);
    assignSecurityCapabilitiesProp(aResp, "PassphraseLockLimit",
                                   passphraseLockLimit);

    if (allowedMemoryModes)
    {
        constexpr const std::array<const char*, 3> values{"Volatile", "PMEM",
                                                          "Block"};

        for (const char* v : values)
        {
            if (boost::ends_with(**allowedMemoryModes, v))
            {
                aResp->res.jsonValue["OperatingMemoryModes "] = v;
                break;
            }
        }
    }

    if (memoryMedia)
    {
        constexpr const std::array<const char*, 3> values{"DRAM", "NAND",
                                                          "Intel3DXPoint"};

        for (const char* v : values)
        {
            if (boost::ends_with(**memoryMedia, v))
            {
                aResp->res.jsonValue["MemoryMedia"] = v;
                break;
            }
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
    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, service, objPath, "",
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

            uint32_t memorySize = 0;
            std::optional<const uint16_t*> dataWidth;
            std::optional<const uint16_t*> revisionCode;
            std::optional<const bool*> present;
            std::optional<const uint16_t*> totalWidth;
            std::optional<const std::string*> ecc;
            std::optional<const std::string*> formFactor;
            std::optional<const std::vector<uint16_t>*> allowedSpeedMt;
            std::optional<const uint8_t*> memoryAttributes;
            std::optional<const uint16_t*> memoryConfiguredSpeedInMhz;
            std::optional<const std::string*> memoryType;
            std::optional<const std::string*> locationCode;
            std::optional<const std::string*> channel, memoryController, slot,
                socket, partNumber, serialNumber, manufacturer, sparePartNumber,
                model;

            auto assignMemoryLocationIf =
                [&aResp](const std::string& key,
                         const std::optional<const std::string*>& value) {
                    if (value)
                    {
                        aResp->res.jsonValue["MemoryLocation"][key] = **value;
                    }
                };

            std::optional<sdbusplus::UnpackError> error =
                sdbusplus::unpackPropertiesNoThrow(
                    properties, "MemorySizeInKB", memorySize, "MemoryDataWidth",
                    dataWidth, "PartNumber", partNumber, "SerialNumber",
                    serialNumber, "Manufacturer", manufacturer, "RevisionCode",
                    revisionCode, "Present", present, "MemoryTotalWidth",
                    totalWidth, "ECC", ecc, "FormFactor", formFactor,
                    "AllowedSpeedsMT", allowedSpeedMt, "MemoryAttributes",
                    memoryAttributes, "MemoryConfiguredSpeedInMhz",
                    memoryConfiguredSpeedInMhz, "MemoryType", memoryType,
                    "Channel", channel, "MemoryController", memoryController,
                    "Slot", slot, "Socket", socket, "SparePartNumber",
                    sparePartNumber, "Model", model, "LocationCode",
                    locationCode);

            if (error)
            {
                BMCWEB_LOG_DEBUG << "Invalid property type at index "
                                 << error->index;
                messages::internalError(aResp->res);
                return;
            }

            aResp->res.jsonValue["CapacityMiB"] = memorySize >> 10;
            aResp->res.jsonValue["Status"]["State"] = "Enabled";
            aResp->res.jsonValue["Status"]["Health"] = "OK";

            assignMemoryLocationIf("Channel", channel);
            assignMemoryLocationIf("MemoryController", memoryController);
            assignMemoryLocationIf("Slot", slot);
            assignMemoryLocationIf("Socket", socket);

            json_util::assignIf(aResp->res.jsonValue, "PartNumber", partNumber);
            json_util::assignIf(aResp->res.jsonValue, "SerialNumber",
                                serialNumber);
            json_util::assignIf(aResp->res.jsonValue, "Manufacturer",
                                manufacturer);
            json_util::assignIf(aResp->res.jsonValue, "SparePartNumber",
                                sparePartNumber);
            json_util::assignIf(aResp->res.jsonValue, "Model", model);
            json_util::assignIf(aResp->res.jsonValue, "DataWidthBits",
                                dataWidth);
            json_util::assignIf(aResp->res.jsonValue, "BusWidthBits",
                                totalWidth);
            json_util::assignIf(aResp->res.jsonValue, "OperatingSpeedMhz",
                                memoryConfiguredSpeedInMhz);

            if (revisionCode)
            {
                aResp->res.jsonValue["FirmwareRevision"] =
                    std::to_string(**revisionCode);
            }

            if (present && **present == false)
            {
                aResp->res.jsonValue["Status"]["State"] = "Absent";
            }

            if (ecc)
            {
                constexpr const std::array<const char*, 4> values{
                    "NoECC", "SingleBitECC", "MultiBitECC", "AddressParity"};

                for (const char* v : values)
                {
                    if (boost::ends_with(**ecc, v))
                    {
                        aResp->res.jsonValue["ErrorCorrection"] = v;
                        break;
                    }
                }
            }

            if (formFactor)
            {
                constexpr const std::array<const char*, 11> values{
                    "RDIMM",        "UDIMM",        "SO_DIMM",
                    "LRDIMM",       "Mini_RDIMM",   "Mini_UDIMM",
                    "SO_RDIMM_72b", "SO_UDIMM_72b", "SO_DIMM_16b",
                    "SO_DIMM_32b",  "Die"};

                for (const char* v : values)
                {
                    if (boost::ends_with(**formFactor, v))
                    {
                        aResp->res.jsonValue["BaseModuleType"] = v;
                        break;
                    }
                }
            }

            if (allowedSpeedMt)
            {
                nlohmann::json& jValue =
                    aResp->res.jsonValue["AllowedSpeedsMHz"];
                jValue = nlohmann::json::array();
                for (uint16_t subVal : **allowedSpeedMt)
                {
                    jValue.push_back(subVal);
                }
            }

            if (memoryAttributes)
            {
                aResp->res.jsonValue["RankCount"] =
                    static_cast<uint64_t>(**memoryAttributes);
            }

            if (memoryType)
            {
                std::string memoryDeviceType =
                    translateMemoryTypeToRedfish(**memoryType);
                // Values like "Unknown" or "Other" will return empty
                // so just leave off
                if (!memoryDeviceType.empty())
                {
                    aResp->res.jsonValue["MemoryDeviceType"] = memoryDeviceType;
                }
                if ((*memoryType)->find("DDR") != std::string::npos)
                {
                    aResp->res.jsonValue["MemoryType"] = "DRAM";
                }
                else if (boost::ends_with(**memoryType, "Logical"))
                {
                    aResp->res.jsonValue["MemoryType"] = "IntelOptane";
                }
            }

            if (locationCode)
            {
                aResp->res
                    .jsonValue["Location"]["PartLocation"]["ServiceLabel"] =
                    **locationCode;
            }

            getPersistentMemoryProperties(aResp, properties);
        });
}

inline void getDimmPartitionData(std::shared_ptr<bmcweb::AsyncResp> aResp,
                                 const std::string& service,
                                 const std::string& path)
{
    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, service, path,
        "xyz.openbmc_project.Inventory.Item.PersistentMemory.Partition",
        [aResp{std::move(aResp)}](
            const boost::system::error_code ec,
            const boost::container::flat_map<
                std::string, dbus::utility::DbusVariantType>& properties) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(aResp->res);

                return;
            }

            std::optional<const std::string*> memoryClassification, partitionId;
            std::optional<const uint64_t*> offsetInKiB, sizeInKiB;
            std::optional<const bool*> passphraseState;

            std::optional<sdbusplus::UnpackError> error =
                sdbusplus::unpackPropertiesNoThrow(
                    properties, "MemoryClassification", memoryClassification,
                    "OffsetInKiB", offsetInKiB, "PartitionId", partitionId,
                    "PassphraseState", passphraseState, "SizeInKiB", sizeInKiB);

            if (error)
            {
                messages::internalError(aResp->res);
                return;
            }

            nlohmann::json& partition =
                aResp->res.jsonValue["Regions"].emplace_back(
                    nlohmann::json::object());

            json_util::assignIf(partition, "MemoryClassification",
                                memoryClassification);
            json_util::assignIf(partition, "RegionId", partitionId);
            json_util::assignIf(partition, "PassphraseEnabled",
                                passphraseState);

            kibPropToMib(partition, "OffsetMiB", offsetInKiB);
            kibPropToMib(partition, "SizeInKiB", sizeInKiB);
        });
}

inline void getDimmData(std::shared_ptr<bmcweb::AsyncResp> aResp,
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
                        if (std::find(
                                interfaces.begin(), interfaces.end(),
                                "xyz.openbmc_project.Inventory.Item.PersistentMemory.Partition") !=
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

inline void requestRoutesMemoryCollection(App& app)
{
    /**
     * Functions triggers appropriate requests on DBus
     */
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/Memory/")
        .privileges(redfish::privileges::getMemoryCollection)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                asyncResp->res.jsonValue["@odata.type"] =
                    "#MemoryCollection.MemoryCollection";
                asyncResp->res.jsonValue["Name"] = "Memory Module Collection";
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/Systems/system/Memory";

                collection_util::getCollectionMembers(
                    asyncResp, "/redfish/v1/Systems/system/Memory",
                    {"xyz.openbmc_project.Inventory.Item.Dimm"});
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
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& dimmId) {
                asyncResp->res.jsonValue["@odata.type"] =
                    "#Memory.v1_11_0.Memory";
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/Systems/system/Memory/" + dimmId;

                getDimmData(asyncResp, dimmId);
            });
}

} // namespace redfish
