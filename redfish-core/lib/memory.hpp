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
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/unpack_properties.hpp>
#include <utils/collection.hpp>
#include <utils/dbus_utils.hpp>
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
                          const char* key, const uint16_t* value)
{
    if (value)
    {
        aResp->res.jsonValue[key] = "0x" + intToHexString(*value);
    }
}

inline void getPersistentMemoryProperties(
    const std::shared_ptr<bmcweb::AsyncResp>& aResp,
    const DimmProperties& properties)
{
    const uint16_t* moduleManufacturerID = nullptr;
    const uint16_t* moduleProductID = nullptr;
    const uint16_t* subsystemVendorID = nullptr;
    const uint16_t* subsystemDeviceID = nullptr;
    const uint64_t* volatileRegionSizeLimitInKiB = nullptr;
    const uint64_t* pmRegionSizeLimitInKiB = nullptr;
    const uint64_t* volatileSizeInKiB = nullptr;
    const uint64_t* pmSizeInKiB = nullptr;
    const uint64_t* cacheSizeInKB = nullptr;
    const uint64_t* voltaileRegionMaxSizeInKib = nullptr;
    const uint64_t* pmRegionMaxSizeInKiB = nullptr;
    const uint64_t* allocationIncrementInKiB = nullptr;
    const uint64_t* allocationAlignmentInKiB = nullptr;
    const uint64_t* volatileRegionNumberLimit = nullptr;
    const uint64_t* pmRegionNumberLimit = nullptr;
    const uint64_t* spareDeviceCount = nullptr;
    const bool* isSpareDeviceInUse = nullptr;
    const bool* isRankSpareEnabled = nullptr;
    const uint32_t* maxAveragePowerLimitmW = nullptr;
    const bool* configurationLocked = nullptr;
    const std::string* allowedMemoryModes = nullptr;
    const std::string* memoryMedia = nullptr;
    const bool* configurationLockCapable = nullptr;
    const bool* dataLockCapable = nullptr;
    const bool* passphraseCapable = nullptr;
    const uint64_t* maxPassphraseCount = nullptr;
    const uint64_t* passphraseLockLimit = nullptr;

    const bool error = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), properties, "ModuleManufacturerID",
        moduleManufacturerID, "ModuleProductID", moduleProductID,
        "SubsystemVendorID", subsystemVendorID, "SubsystemDeviceID",
        subsystemDeviceID, "VolatileRegionSizeLimitInKiB",
        volatileRegionSizeLimitInKiB, "PmRegionSizeLimitInKiB",
        pmRegionSizeLimitInKiB, "VolatileSizeInKiB", volatileSizeInKiB,
        "PmSizeInKiB", pmSizeInKiB, "CacheSizeInKB", cacheSizeInKB,
        "VoltaileRegionMaxSizeInKib", voltaileRegionMaxSizeInKib,
        "PmRegionMaxSizeInKiB", pmRegionMaxSizeInKiB,
        "AllocationIncrementInKiB", allocationIncrementInKiB,
        "AllocationAlignmentInKiB", allocationAlignmentInKiB,
        "VolatileRegionNumberLimit", volatileRegionNumberLimit,
        "PmRegionNumberLimit", pmRegionNumberLimit, "SpareDeviceCount",
        spareDeviceCount, "IsSpareDeviceInUse", isSpareDeviceInUse,
        "IsRankSpareEnabled", isRankSpareEnabled, "MaxAveragePowerLimitmW",
        maxAveragePowerLimitmW, "ConfigurationLocked", configurationLocked,
        "AllowedMemoryModes", allowedMemoryModes, "MemoryMedia", memoryMedia,
        "ConfigurationLockCapable", configurationLockCapable, "DataLockCapable",
        dataLockCapable, "PassphraseCapable", passphraseCapable,
        "MaxPassphraseCount", maxPassphraseCount, "PassphraseLockLimit",
        passphraseLockLimit);

    if (error)
    {
        messages::internalError(aResp->res);
        return;
    }

    dimmPropToHex(aResp, "ModuleManufacturerID", moduleManufacturerID);
    dimmPropToHex(aResp, "ModuleProductID", moduleProductID);
    dimmPropToHex(aResp, "MemorySubsystemControllerManufacturerID",
                  subsystemVendorID);
    dimmPropToHex(aResp, "MemorySubsystemControllerProductID",
                  subsystemDeviceID);

    if (volatileRegionSizeLimitInKiB)
    {
        aResp->res.jsonValue["VolatileRegionSizeLimitMiB"] =
            (*volatileRegionSizeLimitInKiB) >> 10;
    }

    if (pmRegionSizeLimitInKiB)
    {
        aResp->res.jsonValue["PersistentRegionSizeLimitMiB"] =
            (*pmRegionSizeLimitInKiB) >> 10;
    }

    if (volatileSizeInKiB)
    {
        aResp->res.jsonValue["VolatileSizeMiB"] = (*volatileSizeInKiB) >> 10;
    }

    if (pmSizeInKiB)
    {
        aResp->res.jsonValue["NonVolatileSizeMiB"] = (*pmSizeInKiB) >> 10;
    }

    if (cacheSizeInKB)
    {
        aResp->res.jsonValue["CacheSizeMiB"] = (*cacheSizeInKB >> 10);
    }

    if (voltaileRegionMaxSizeInKib)
    {
        aResp->res.jsonValue["VolatileRegionSizeMaxMiB"] =
            (*voltaileRegionMaxSizeInKib) >> 10;
    }

    if (pmRegionMaxSizeInKiB)
    {
        aResp->res.jsonValue["PersistentRegionSizeMaxMiB"] =
            (*pmRegionMaxSizeInKiB) >> 10;
    }

    if (allocationIncrementInKiB)
    {
        aResp->res.jsonValue["AllocationIncrementMiB"] =
            (*allocationIncrementInKiB) >> 10;
    }

    if (allocationAlignmentInKiB)
    {
        aResp->res.jsonValue["AllocationAlignmentMiB"] =
            (*allocationAlignmentInKiB) >> 10;
    }

    if (volatileRegionNumberLimit)
    {
        aResp->res.jsonValue["VolatileRegionNumberLimit"] =
            *volatileRegionNumberLimit;
    }

    if (pmRegionNumberLimit)
    {
        aResp->res.jsonValue["PersistentRegionNumberLimit"] =
            *pmRegionNumberLimit;
    }

    if (spareDeviceCount)
    {
        aResp->res.jsonValue["SpareDeviceCount"] = *spareDeviceCount;
    }

    if (isSpareDeviceInUse)
    {
        aResp->res.jsonValue["IsSpareDeviceEnabled"] = *isSpareDeviceInUse;
    }

    if (isRankSpareEnabled)
    {
        aResp->res.jsonValue["IsRankSpareEnabled"] = *isRankSpareEnabled;
    }

    if (maxAveragePowerLimitmW)
    {
        aResp->res.jsonValue["MaxTDPMilliWatts"] = *maxAveragePowerLimitmW;
    }

    if (configurationLocked)
    {
        aResp->res.jsonValue["ConfigurationLocked"] = *configurationLocked;
    }

    if (allowedMemoryModes)
    {
        constexpr const std::array<const char*, 3> values{"Volatile", "PMEM",
                                                          "Block"};

        for (const char* v : values)
        {
            if (boost::ends_with(*allowedMemoryModes, v))
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
            if (boost::ends_with(*memoryMedia, v))
            {
                aResp->res.jsonValue["MemoryMedia"] = v;
                break;
            }
        }
    }

    if (configurationLockCapable)
    {
        aResp->res
            .jsonValue["SecurityCapabilities"]["ConfigurationLockCapable"] =
            *configurationLockCapable;
    }

    if (dataLockCapable)
    {
        aResp->res.jsonValue["SecurityCapabilities"]["DataLockCapable"] =
            *dataLockCapable;
    }

    if (passphraseCapable)
    {
        aResp->res.jsonValue["SecurityCapabilities"]["PassphraseCapable"] =
            *passphraseCapable;
    }

    if (maxPassphraseCount)
    {
        aResp->res.jsonValue["SecurityCapabilities"]["MaxPassphraseCount"] =
            *maxPassphraseCount;
    }

    if (passphraseLockLimit)
    {
        aResp->res.jsonValue["SecurityCapabilities"]["PassphraseLockLimit"] =
            *passphraseLockLimit;
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
                BMCWEB_LOG_DEBUG << "DBUS response error: " << ec.value();
                messages::internalError(aResp->res);
                return;
            }

            const uint32_t* memorySizeInKB = nullptr;
            const uint16_t* memoryDataWidth = nullptr;
            const std::string* partNumber = nullptr;
            const std::string* serialNumber = nullptr;
            const std::string* manufacturer = nullptr;
            const uint16_t* revisionCode = nullptr;
            const bool* present = nullptr;
            const uint16_t* memoryTotalWidth = nullptr;
            const std::string* eCC = nullptr;
            const std::string* formFactor = nullptr;
            const std::vector<uint16_t>* allowedSpeedsMT = nullptr;
            const uint8_t* memoryAttributes = nullptr;
            const uint16_t* memoryConfiguredSpeedInMhz = nullptr;
            const std::string* memoryType = nullptr;
            const std::string* channel = nullptr;
            const std::string* memoryController = nullptr;
            const std::string* slot = nullptr;
            const std::string* socket = nullptr;
            const std::string* sparePartNumber = nullptr;
            const std::string* model = nullptr;
            const std::string* locationCode = nullptr;

            const bool error = sdbusplus::unpackPropertiesNoThrow(
                dbus_utils::UnpackErrorPrinter(), properties, "MemorySizeInKB",
                memorySizeInKB, "MemoryDataWidth", memoryDataWidth,
                "PartNumber", partNumber, "SerialNumber", serialNumber,
                "Manufacturer", manufacturer, "RevisionCode", revisionCode,
                "Present", present, "MemoryTotalWidth", memoryTotalWidth, "ECC",
                eCC, "FormFactor", formFactor, "AllowedSpeedsMT",
                allowedSpeedsMT, "MemoryAttributes", memoryAttributes,
                "MemoryConfiguredSpeedInMhz", memoryConfiguredSpeedInMhz,
                "MemoryType", memoryType, "Channel", channel,
                "MemoryController", memoryController, "Slot", slot, "Socket",
                socket, "SparePartNumber", sparePartNumber, "Model", model,
                "LocationCode", locationCode);

            if (error)
            {
                messages::internalError(aResp->res);
                return;
            }

            if (memorySizeInKB)
            {
                aResp->res.jsonValue["CapacityMiB"] = (*memorySizeInKB >> 10);
            }

            if (memoryDataWidth)
            {
                aResp->res.jsonValue["DataWidthBits"] = *memoryDataWidth;
            }

            if (partNumber)
            {
                aResp->res.jsonValue["PartNumber"] = *partNumber;
            }

            if (serialNumber)
            {
                aResp->res.jsonValue["SerialNumber"] = *serialNumber;
            }

            if (manufacturer)
            {
                aResp->res.jsonValue["Manufacturer"] = *manufacturer;
            }

            if (revisionCode)
            {
                aResp->res.jsonValue["FirmwareRevision"] =
                    std::to_string(*revisionCode);
            }

            if (present && *present == false)
            {
                aResp->res.jsonValue["Status"]["State"] = "Absent";
            }

            if (memoryTotalWidth)
            {
                aResp->res.jsonValue["BusWidthBits"] = *memoryTotalWidth;
            }

            if (eCC)
            {
                constexpr const std::array<const char*, 4> values{
                    "NoECC", "SingleBitECC", "MultiBitECC", "AddressParity"};

                for (const char* v : values)
                {
                    if (boost::ends_with(*eCC, v))
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
                    if (boost::ends_with(*formFactor, v))
                    {
                        aResp->res.jsonValue["BaseModuleType"] = v;
                        break;
                    }
                }
            }

            if (allowedSpeedsMT)
            {
                nlohmann::json& jValue =
                    aResp->res.jsonValue["AllowedSpeedsMHz"];
                jValue = nlohmann::json::array();
                for (uint16_t subVal : *allowedSpeedsMT)
                {
                    jValue.push_back(subVal);
                }
            }

            if (memoryAttributes)
            {
                aResp->res.jsonValue["RankCount"] =
                    static_cast<uint64_t>(*memoryAttributes);
            }

            if (memoryConfiguredSpeedInMhz)
            {
                aResp->res.jsonValue["OperatingSpeedMhz"] =
                    *memoryConfiguredSpeedInMhz;
            }

            if (memoryType)
            {
                std::string memoryDeviceType =
                    translateMemoryTypeToRedfish(*memoryType);
                // Values like "Unknown" or "Other" will return empty
                // so just leave off
                if (!memoryDeviceType.empty())
                {
                    aResp->res.jsonValue["MemoryDeviceType"] = memoryDeviceType;
                }
                if (memoryType->find("DDR") != std::string::npos)
                {
                    aResp->res.jsonValue["MemoryType"] = "DRAM";
                }
                else if (boost::ends_with(*memoryType, "Logical"))
                {
                    aResp->res.jsonValue["MemoryType"] = "IntelOptane";
                }
            }

            if (channel)
            {
                aResp->res.jsonValue["MemoryLocation"]["Channel"] = *channel;
            }

            if (memoryController)
            {
                aResp->res.jsonValue["MemoryLocation"]["MemoryController"] =
                    *memoryController;
            }

            if (slot)
            {
                aResp->res.jsonValue["MemoryLocation"]["Slot"] = *slot;
            }

            if (socket)
            {
                aResp->res.jsonValue["MemoryLocation"]["Socket"] = *socket;
            }

            if (sparePartNumber)
            {
                aResp->res.jsonValue["SparePartNumber"] = *sparePartNumber;
            }

            if (model)
            {
                aResp->res.jsonValue["Model"] = *model;
            }

            if (locationCode)
            {
                aResp->res
                    .jsonValue["Location"]["PartLocation"]["ServiceLabel"] =
                    *locationCode;
            }

            aResp->res.jsonValue["Id"] = dimmId;
            aResp->res.jsonValue["Name"] = "DIMM Slot";
            aResp->res.jsonValue["Status"]["State"] = "Enabled";
            aResp->res.jsonValue["Status"]["Health"] = "OK";

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
                BMCWEB_LOG_DEBUG << "DBUS response error: " << ec.value();
                messages::internalError(aResp->res);
                return;
            }

            const std::string* memoryClassification = nullptr;
            const uint64_t* offsetInKiB = nullptr;
            const std::string* partitionId = nullptr;
            const bool* passphraseState = nullptr;
            const uint64_t* sizeInKiB = nullptr;

            const bool error = sdbusplus::unpackPropertiesNoThrow(
                dbus_utils::UnpackErrorPrinter(), properties,
                "MemoryClassification", memoryClassification, "OffsetInKiB",
                offsetInKiB, "PartitionId", partitionId, "PassphraseState",
                passphraseState, "SizeInKiB", sizeInKiB);

            if (error)
            {
                messages::internalError(aResp->res);
                return;
            }

            nlohmann::json& partition =
                aResp->res.jsonValue["Regions"].emplace_back(
                    nlohmann::json::object());

            if (memoryClassification)
            {
                partition["MemoryClassification"] = *memoryClassification;
            }

            if (offsetInKiB)
            {
                partition["OffsetMiB"] = (*offsetInKiB >> 10);
            }

            if (partitionId)
            {
                partition["RegionId"] = *partitionId;
            }

            if (passphraseState)
            {
                partition["PassphraseEnabled"] = *passphraseState;
            }

            if (sizeInKiB)
            {
                partition["SizeMiB"] = (*sizeInKiB >> 10);
            }
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
                BMCWEB_LOG_DEBUG << "DBUS response error: " << ec.value();
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
