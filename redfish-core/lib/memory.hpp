// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
// SPDX-FileCopyrightText: Copyright 2018 Intel Corporation
#pragma once

#include "bmcweb_config.h"

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/memory.hpp"
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

#include <asm-generic/errno.h>

#include <boost/beast/http/verb.hpp>
#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/message/native_types.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

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
    if (memoryType == "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType.HBM3")
    {
        return "HBM3";
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

inline void dimmPropToHex(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const char* key, const uint16_t* value,
                          const nlohmann::json::json_pointer& jsonPtr)
{
    if (value == nullptr)
    {
        return;
    }
    asyncResp->res.jsonValue[jsonPtr][key] = "0x" + intToHexString(*value, 4);
}

inline void getPersistentMemoryProperties(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const dbus::utility::DBusPropertiesMap& properties,
    const nlohmann::json::json_pointer& jsonPtr)
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
    const std::vector<uint32_t>* maxAveragePowerLimitmW = nullptr;
    const bool* configurationLocked = nullptr;
    const std::string* allowedMemoryModes = nullptr;
    const std::string* memoryMedia = nullptr;
    const bool* configurationLockCapable = nullptr;
    const bool* dataLockCapable = nullptr;
    const bool* passphraseCapable = nullptr;
    const uint64_t* maxPassphraseCount = nullptr;
    const uint64_t* passphraseLockLimit = nullptr;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
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

    if (!success)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    dimmPropToHex(asyncResp, "ModuleManufacturerID", moduleManufacturerID,
                  jsonPtr);
    dimmPropToHex(asyncResp, "ModuleProductID", moduleProductID, jsonPtr);
    dimmPropToHex(asyncResp, "MemorySubsystemControllerManufacturerID",
                  subsystemVendorID, jsonPtr);
    dimmPropToHex(asyncResp, "MemorySubsystemControllerProductID",
                  subsystemDeviceID, jsonPtr);

    if (volatileRegionSizeLimitInKiB != nullptr)
    {
        asyncResp->res.jsonValue[jsonPtr]["VolatileRegionSizeLimitMiB"] =
            (*volatileRegionSizeLimitInKiB) >> 10;
    }

    if (pmRegionSizeLimitInKiB != nullptr)
    {
        asyncResp->res.jsonValue[jsonPtr]["PersistentRegionSizeLimitMiB"] =
            (*pmRegionSizeLimitInKiB) >> 10;
    }

    if (volatileSizeInKiB != nullptr)
    {
        asyncResp->res.jsonValue[jsonPtr]["VolatileSizeMiB"] =
            (*volatileSizeInKiB) >> 10;
    }

    if (pmSizeInKiB != nullptr)
    {
        asyncResp->res.jsonValue[jsonPtr]["NonVolatileSizeMiB"] =
            (*pmSizeInKiB) >> 10;
    }

    if (cacheSizeInKB != nullptr)
    {
        asyncResp->res.jsonValue[jsonPtr]["CacheSizeMiB"] =
            (*cacheSizeInKB >> 10);
    }

    if (voltaileRegionMaxSizeInKib != nullptr)
    {
        asyncResp->res.jsonValue[jsonPtr]["VolatileRegionSizeMaxMiB"] =
            (*voltaileRegionMaxSizeInKib) >> 10;
    }

    if (pmRegionMaxSizeInKiB != nullptr)
    {
        asyncResp->res.jsonValue[jsonPtr]["PersistentRegionSizeMaxMiB"] =
            (*pmRegionMaxSizeInKiB) >> 10;
    }

    if (allocationIncrementInKiB != nullptr)
    {
        asyncResp->res.jsonValue[jsonPtr]["AllocationIncrementMiB"] =
            (*allocationIncrementInKiB) >> 10;
    }

    if (allocationAlignmentInKiB != nullptr)
    {
        asyncResp->res.jsonValue[jsonPtr]["AllocationAlignmentMiB"] =
            (*allocationAlignmentInKiB) >> 10;
    }

    if (volatileRegionNumberLimit != nullptr)
    {
        asyncResp->res.jsonValue[jsonPtr]["VolatileRegionNumberLimit"] =
            *volatileRegionNumberLimit;
    }

    if (pmRegionNumberLimit != nullptr)
    {
        asyncResp->res.jsonValue[jsonPtr]["PersistentRegionNumberLimit"] =
            *pmRegionNumberLimit;
    }

    if (spareDeviceCount != nullptr)
    {
        asyncResp->res.jsonValue[jsonPtr]["SpareDeviceCount"] =
            *spareDeviceCount;
    }

    if (isSpareDeviceInUse != nullptr)
    {
        asyncResp->res.jsonValue[jsonPtr]["IsSpareDeviceEnabled"] =
            *isSpareDeviceInUse;
    }

    if (isRankSpareEnabled != nullptr)
    {
        asyncResp->res.jsonValue[jsonPtr]["IsRankSpareEnabled"] =
            *isRankSpareEnabled;
    }

    if (maxAveragePowerLimitmW != nullptr)
    {
        asyncResp->res.jsonValue[jsonPtr]["MaxTDPMilliWatts"] =
            *maxAveragePowerLimitmW;
    }

    if (configurationLocked != nullptr)
    {
        asyncResp->res.jsonValue[jsonPtr]["ConfigurationLocked"] =
            *configurationLocked;
    }

    if (allowedMemoryModes != nullptr)
    {
        constexpr const std::array<const char*, 3> values{"Volatile", "PMEM",
                                                          "Block"};

        for (const char* v : values)
        {
            if (allowedMemoryModes->ends_with(v))
            {
                asyncResp->res.jsonValue[jsonPtr]["OperatingMemoryModes"]
                    .push_back(v);
                break;
            }
        }
    }

    if (memoryMedia != nullptr)
    {
        constexpr const std::array<const char*, 3> values{"DRAM", "NAND",
                                                          "Intel3DXPoint"};

        for (const char* v : values)
        {
            if (memoryMedia->ends_with(v))
            {
                asyncResp->res.jsonValue[jsonPtr]["MemoryMedia"].push_back(v);
                break;
            }
        }
    }

    if (configurationLockCapable != nullptr)
    {
        asyncResp->res.jsonValue[jsonPtr]["SecurityCapabilities"]
                                ["ConfigurationLockCapable"] =
            *configurationLockCapable;
    }

    if (dataLockCapable != nullptr)
    {
        asyncResp->res
            .jsonValue[jsonPtr]["SecurityCapabilities"]["DataLockCapable"] =
            *dataLockCapable;
    }

    if (passphraseCapable != nullptr)
    {
        asyncResp->res
            .jsonValue[jsonPtr]["SecurityCapabilities"]["PassphraseCapable"] =
            *passphraseCapable;
    }

    if (maxPassphraseCount != nullptr)
    {
        asyncResp->res
            .jsonValue[jsonPtr]["SecurityCapabilities"]["MaxPassphraseCount"] =
            *maxPassphraseCount;
    }

    if (passphraseLockLimit != nullptr)
    {
        asyncResp->res
            .jsonValue[jsonPtr]["SecurityCapabilities"]["PassphraseLockLimit"] =
            *passphraseLockLimit;
    }
}

inline void assembleDimmProperties(
    std::string_view dimmId,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const dbus::utility::DBusPropertiesMap& properties,
    const nlohmann::json::json_pointer& jsonPtr)
{
    asyncResp->res.jsonValue[jsonPtr]["Id"] = dimmId;
    asyncResp->res.jsonValue[jsonPtr]["Name"] = "DIMM Slot";
    asyncResp->res.jsonValue[jsonPtr]["Status"]["State"] =
        resource::State::Enabled;
    asyncResp->res.jsonValue[jsonPtr]["Status"]["Health"] =
        resource::Health::OK;

    const uint16_t* memoryDataWidth = nullptr;
    const size_t* memorySizeInKB = nullptr;
    const std::string* partNumber = nullptr;
    const std::string* serialNumber = nullptr;
    const std::string* manufacturer = nullptr;
    const uint16_t* revisionCode = nullptr;
    const bool* present = nullptr;
    const uint16_t* memoryTotalWidth = nullptr;
    const std::string* ecc = nullptr;
    const std::string* formFactor = nullptr;
    const std::vector<uint16_t>* allowedSpeedsMT = nullptr;
    const size_t* memoryAttributes = nullptr;
    const uint16_t* memoryConfiguredSpeedInMhz = nullptr;
    const std::string* memoryType = nullptr;
    const std::uint8_t* channel = nullptr;
    const std::uint8_t* memoryController = nullptr;
    const std::uint8_t* slot = nullptr;
    const std::uint8_t* socket = nullptr;
    const std::string* sparePartNumber = nullptr;
    const std::string* model = nullptr;
    const std::string* locationCode = nullptr;
    const bool* functional = nullptr;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), properties, "MemoryDataWidth",
        memoryDataWidth, "MemorySizeInKB", memorySizeInKB, "PartNumber",
        partNumber, "SerialNumber", serialNumber, "Manufacturer", manufacturer,
        "RevisionCode", revisionCode, "Present", present, "MemoryTotalWidth",
        memoryTotalWidth, "ECC", ecc, "FormFactor", formFactor,
        "AllowedSpeedsMT", allowedSpeedsMT, "MemoryAttributes",
        memoryAttributes, "MemoryConfiguredSpeedInMhz",
        memoryConfiguredSpeedInMhz, "MemoryType", memoryType, "Channel",
        channel, "MemoryController", memoryController, "Slot", slot, "Socket",
        socket, "SparePartNumber", sparePartNumber, "Model", model,
        "LocationCode", locationCode, "Functional", functional);

    if (!success)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    if (memoryDataWidth != nullptr)
    {
        asyncResp->res.jsonValue[jsonPtr]["DataWidthBits"] = *memoryDataWidth;
    }

    if (memorySizeInKB != nullptr)
    {
        asyncResp->res.jsonValue[jsonPtr]["CapacityMiB"] =
            (*memorySizeInKB >> 10);
    }

    if (partNumber != nullptr)
    {
        asyncResp->res.jsonValue[jsonPtr]["PartNumber"] = *partNumber;
    }

    if (serialNumber != nullptr)
    {
        asyncResp->res.jsonValue[jsonPtr]["SerialNumber"] = *serialNumber;
    }

    if (manufacturer != nullptr)
    {
        asyncResp->res.jsonValue[jsonPtr]["Manufacturer"] = *manufacturer;
    }

    if (revisionCode != nullptr)
    {
        asyncResp->res.jsonValue[jsonPtr]["FirmwareRevision"] =
            std::to_string(*revisionCode);
    }

    if (present != nullptr && !*present)
    {
        asyncResp->res.jsonValue[jsonPtr]["Status"]["State"] =
            resource::State::Absent;
    }

    if (functional != nullptr)
    {
        if (!*functional)
        {
            asyncResp->res.jsonValue[jsonPtr]["Status"]["Health"] =
                resource::Health::Critical;
        }
    }

    if (memoryTotalWidth != nullptr)
    {
        asyncResp->res.jsonValue[jsonPtr]["BusWidthBits"] = *memoryTotalWidth;
    }

    if (ecc != nullptr)
    {
        constexpr const std::array<const char*, 4> values{
            "NoECC", "SingleBitECC", "MultiBitECC", "AddressParity"};

        for (const char* v : values)
        {
            if (ecc->ends_with(v))
            {
                asyncResp->res.jsonValue[jsonPtr]["ErrorCorrection"] = v;
                break;
            }
        }
    }

    if (formFactor != nullptr)
    {
        constexpr const std::array<const char*, 11> values{
            "RDIMM",       "UDIMM",       "SO_DIMM",      "LRDIMM",
            "Mini_RDIMM",  "Mini_UDIMM",  "SO_RDIMM_72b", "SO_UDIMM_72b",
            "SO_DIMM_16b", "SO_DIMM_32b", "Die"};

        for (const char* v : values)
        {
            if (formFactor->ends_with(v))
            {
                asyncResp->res.jsonValue[jsonPtr]["BaseModuleType"] = v;
                break;
            }
        }
    }

    if (allowedSpeedsMT != nullptr)
    {
        nlohmann::json& jValue =
            asyncResp->res.jsonValue[jsonPtr]["AllowedSpeedsMHz"];
        jValue = nlohmann::json::array();
        for (uint16_t subVal : *allowedSpeedsMT)
        {
            jValue.push_back(subVal);
        }
    }

    if (memoryAttributes != nullptr)
    {
        asyncResp->res.jsonValue[jsonPtr]["RankCount"] = *memoryAttributes;
    }

    if (memoryConfiguredSpeedInMhz != nullptr)
    {
        asyncResp->res.jsonValue[jsonPtr]["OperatingSpeedMhz"] =
            *memoryConfiguredSpeedInMhz;
    }

    if (memoryType != nullptr)
    {
        std::string memoryDeviceType =
            translateMemoryTypeToRedfish(*memoryType);
        // Values like "Unknown" or "Other" will return empty
        // so just leave off
        if (!memoryDeviceType.empty())
        {
            asyncResp->res.jsonValue[jsonPtr]["MemoryDeviceType"] =
                memoryDeviceType;
        }
        if (memoryType->find("DDR") != std::string::npos)
        {
            asyncResp->res.jsonValue[jsonPtr]["MemoryType"] =
                memory::MemoryType::DRAM;
        }
        else if (memoryType->ends_with("Logical"))
        {
            asyncResp->res.jsonValue[jsonPtr]["MemoryType"] =
                memory::MemoryType::IntelOptane;
        }
    }

    if (channel != nullptr)
    {
        asyncResp->res.jsonValue[jsonPtr]["MemoryLocation"]["Channel"] =
            *channel;
    }

    if (memoryController != nullptr)
    {
        asyncResp->res
            .jsonValue[jsonPtr]["MemoryLocation"]["MemoryController"] =
            *memoryController;
    }

    if (slot != nullptr)
    {
        asyncResp->res.jsonValue[jsonPtr]["MemoryLocation"]["Slot"] = *slot;
    }

    if (socket != nullptr)
    {
        asyncResp->res.jsonValue[jsonPtr]["MemoryLocation"]["Socket"] = *socket;
    }

    if (sparePartNumber != nullptr)
    {
        asyncResp->res.jsonValue[jsonPtr]["SparePartNumber"] = *sparePartNumber;
    }

    if (model != nullptr)
    {
        asyncResp->res.jsonValue[jsonPtr]["Model"] = *model;
    }

    if (locationCode != nullptr)
    {
        asyncResp->res
            .jsonValue[jsonPtr]["Location"]["PartLocation"]["ServiceLabel"] =
            *locationCode;
    }

    getPersistentMemoryProperties(asyncResp, properties, jsonPtr);
}

inline void getDimmDataByService(
    std::shared_ptr<bmcweb::AsyncResp> asyncResp, const std::string& dimmId,
    const std::string& service, const std::string& objPath)
{
    BMCWEB_LOG_DEBUG("Get available system components.");
    dbus::utility::getAllProperties(
        service, objPath, "",
        [dimmId, asyncResp{std::move(asyncResp)}](
            const boost::system::error_code& ec,
            const dbus::utility::DBusPropertiesMap& properties) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG("DBUS response error");
                messages::internalError(asyncResp->res);
                return;
            }
            assembleDimmProperties(dimmId, asyncResp, properties,
                                   ""_json_pointer);
        });
}

inline void assembleDimmPartitionData(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const dbus::utility::DBusPropertiesMap& properties,
    const nlohmann::json::json_pointer& regionPtr)
{
    const std::string* memoryClassification = nullptr;
    const uint64_t* offsetInKiB = nullptr;
    const std::string* partitionId = nullptr;
    const bool* passphraseState = nullptr;
    const uint64_t* sizeInKiB = nullptr;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), properties, "MemoryClassification",
        memoryClassification, "OffsetInKiB", offsetInKiB, "PartitionId",
        partitionId, "PassphraseState", passphraseState, "SizeInKiB",
        sizeInKiB);

    if (!success)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    nlohmann::json::object_t partition;

    if (memoryClassification != nullptr)
    {
        partition["MemoryClassification"] = *memoryClassification;
    }

    if (offsetInKiB != nullptr)
    {
        partition["OffsetMiB"] = (*offsetInKiB >> 10);
    }

    if (partitionId != nullptr)
    {
        partition["RegionId"] = *partitionId;
    }

    if (passphraseState != nullptr)
    {
        partition["PassphraseEnabled"] = *passphraseState;
    }

    if (sizeInKiB != nullptr)
    {
        partition["SizeMiB"] = (*sizeInKiB >> 10);
    }

    asyncResp->res.jsonValue[regionPtr].emplace_back(std::move(partition));
}

inline void getDimmPartitionData(std::shared_ptr<bmcweb::AsyncResp> asyncResp,
                                 const std::string& service,
                                 const std::string& path)
{
    dbus::utility::getAllProperties(
        service, path,
        "xyz.openbmc_project.Inventory.Item.PersistentMemory.Partition",
        [asyncResp{std::move(asyncResp)}](
            const boost::system::error_code& ec,
            const dbus::utility::DBusPropertiesMap& properties) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG("DBUS response error");
                messages::internalError(asyncResp->res);

                return;
            }
            nlohmann::json::json_pointer regionPtr = "/Regions"_json_pointer;
            assembleDimmPartitionData(asyncResp, properties, regionPtr);
        }

    );
}

inline void afterGetDimmData(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& dimmId, const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec)
    {
        if (ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR("DBUS response error: {}", ec.value());
            messages::internalError(asyncResp->res);
        }
        return;
    }

    bool found = false;
    for (const auto& [objectPath, serviceMap] : subtree)
    {
        sdbusplus::message::object_path path(objectPath);

        bool dimmInterface = false;
        bool associationInterface = false;
        /* Note: Multiple D-Bus objects can provide details for the Memory
         * object: 1) Dimm is the primary object 2) Additional partitions could
         * exist per Dimm. Only consider the object found if the Dimm is found.
         */
        for (const auto& [serviceName, interfaceList] : serviceMap)
        {
            for (const auto& interface : interfaceList)
            {
                if (interface == "xyz.openbmc_project.Inventory.Item.Dimm" &&
                    path.filename() == dimmId)
                {
                    // Found the single Dimm
                    getDimmDataByService(asyncResp, dimmId, serviceName,
                                         objectPath);
                    dimmInterface = true;
                    found = true;
                }
                else if (interface ==
                         "xyz.openbmc_project.Association.Definitions")
                {
                    /* Object has associations. If this object is also a Dimm
                     * then the association might provide the LED state
                     * information. After all interfaces for this object have
                     * been checked the LED information will be gathered if the
                     * object was a Dimm
                     */
                    associationInterface = true;
                }
                else if (
                    interface ==
                        "xyz.openbmc_project.Inventory.Item.PersistentMemory.Partition" &&
                    path.parent_path().filename() == dimmId)
                {
                    // partitions are separate as there can be multiple per
                    // device, i.e.
                    // /xyz/openbmc_project/Inventory/Item/Dimm1/Partition1
                    // /xyz/openbmc_project/Inventory/Item/Dimm1/Partition2
                    getDimmPartitionData(asyncResp, serviceName, objectPath);
                }
            }
        }

        /* If a Dimm has an Association check if it has a LED */
        if (associationInterface && dimmInterface)
        {
            getLocationIndicatorActive(asyncResp, objectPath);
        }
    }

    if (!found)
    {
        // Dimm object not found
        messages::resourceNotFound(asyncResp->res, "Memory", dimmId);
        return;
    }
    // Set @odata only if object is found
    asyncResp->res.jsonValue["@odata.type"] = "#Memory.v1_11_0.Memory";
    asyncResp->res.jsonValue["@odata.id"] =
        boost::urls::format("/redfish/v1/Systems/{}/Memory/{}",
                            BMCWEB_REDFISH_SYSTEM_URI_NAME, dimmId);
}

inline void getDimmData(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& dimmId)
{
    BMCWEB_LOG_DEBUG("Get dimm path for {}", dimmId);
    constexpr std::array<std::string_view, 2> interfaces = {
        "xyz.openbmc_project.Inventory.Item.Dimm",
        "xyz.openbmc_project.Inventory.Item.PersistentMemory.Partition"};

    dbus::utility::getSubTree(
        "/xyz/openbmc_project/inventory", 0, interfaces,
        [asyncResp,
         dimmId](const boost::system::error_code& ec,
                 const dbus::utility::MapperGetSubTreeResponse& subtree) {
            afterGetDimmData(asyncResp, dimmId, ec, subtree);
        });
}

inline void handleSetDimmData(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    bool locationIndicatorActive, const std::string& dimmPath)
{
    setLocationIndicatorActive(asyncResp, dimmPath, locationIndicatorActive);
}

inline void afterGetValidDimmPath(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& dimmId, const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreePathsResponse& subtree,
    const std::function<void(const std::string& dimmPath)>& callback)
{
    if (ec)
    {
        if (ec.value() == EBADR)
        {
            /* Need to report error for PATCH */
            BMCWEB_LOG_WARNING("Dimm not found in inventory");
            messages::resourceNotFound(asyncResp->res, "Memory", dimmId);
        }
        else
        {
            BMCWEB_LOG_ERROR("DBUS response error: {}", ec.value());
            messages::internalError(asyncResp->res);
        }
        return;
    }

    for (const auto& objectPath : subtree)
    {
        // Ignore any objects which don't end with our desired dimm name
        sdbusplus::message::object_path path(objectPath);
        if (path.filename() == dimmId)
        {
            callback(path);
            return;
        }
    }

    // Object not found
    messages::resourceNotFound(asyncResp->res, "Memory", dimmId);
}

inline void getValidDimmPath(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& dimmId,
    std::function<void(const std::string& dimmPath)>&& callback)
{
    BMCWEB_LOG_DEBUG("Get dimm path for {}", dimmId);
    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Inventory.Item.Dimm"};

    dbus::utility::getSubTreePaths(
        "/xyz/openbmc_project/inventory", 0, interfaces,
        [asyncResp, dimmId, callback{std::move(callback)}](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreePathsResponse& subtree) {
            afterGetValidDimmPath(asyncResp, dimmId, ec, subtree, callback);
        });
}

inline void handleMemoryPatch(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& dimmId)
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

    std::optional<bool> locationIndicatorActive;
    if (!json_util::readJsonPatch(                             //
            req, asyncResp->res,                               //
            "LocationIndicatorActive", locationIndicatorActive //
            ))
    {
        return;
    }

    if (locationIndicatorActive)
    {
        getValidDimmPath(asyncResp, dimmId,
                         std::bind_front(handleSetDimmData, asyncResp,
                                         *locationIndicatorActive));
    }
}

inline void handleMemoryGet(App& app, const crow::Request& req,
                            const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& systemName,
                            const std::string& dimmId)
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

    getDimmData(asyncResp, dimmId);
}

inline void requestRoutesMemoryCollection(App& app)
{
    /**
     * Functions triggers appropriate requests on DBus
     */
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Memory/")
        .privileges(redfish::privileges::getMemoryCollection)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemName) {
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

                asyncResp->res.jsonValue["@odata.type"] =
                    "#MemoryCollection.MemoryCollection";
                asyncResp->res.jsonValue["Name"] = "Memory Module Collection";
                asyncResp->res.jsonValue["@odata.id"] =
                    boost::urls::format("/redfish/v1/Systems/{}/Memory",
                                        BMCWEB_REDFISH_SYSTEM_URI_NAME);

                constexpr std::array<std::string_view, 1> interfaces{
                    "xyz.openbmc_project.Inventory.Item.Dimm"};
                collection_util::getCollectionMembers(
                    asyncResp,
                    boost::urls::format("/redfish/v1/Systems/{}/Memory",
                                        BMCWEB_REDFISH_SYSTEM_URI_NAME),
                    interfaces, "/xyz/openbmc_project/inventory");
            });
}

inline void requestRoutesMemory(App& app)
{
    /**
     * Functions triggers appropriate requests on DBus
     */
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Memory/<str>/")
        .privileges(redfish::privileges::getMemory)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleMemoryGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Memory/<str>/")
        .privileges(redfish::privileges::patchMemory)
        .methods(boost::beast::http::verb::patch)(
            std::bind_front(handleMemoryPatch, std::ref(app)));
}

} // namespace redfish
