// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "logging.hpp"
#include "utils/dbus_utils.hpp"

#include <asm-generic/errno.h>

#include <boost/system/error_code.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <functional>
#include <memory>
#include <ranges>
#include <string>

namespace redfish
{
namespace asset_utils
{

inline void extractAssetInfo(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const nlohmann::json::json_pointer& jsonKeyName,
    const dbus::utility::DBusPropertiesMap& assetList,
    bool includeSparePartNumber = false, bool includeManufacturer = true)
{
    const std::string* manufacturer = nullptr;
    const std::string* model = nullptr;
    const std::string* partNumber = nullptr;
    const std::string* serialNumber = nullptr;
    const std::string* sparePartNumber = nullptr;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), assetList, "Manufacturer",
        manufacturer, "Model", model, "PartNumber", partNumber, "SerialNumber",
        serialNumber, "SparePartNumber", sparePartNumber);
    if (!success)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    nlohmann::json& assetData = asyncResp->res.jsonValue[jsonKeyName];

    if (includeManufacturer && manufacturer != nullptr)
    {
        assetData["Manufacturer"] = *manufacturer;
    }
    if (model != nullptr)
    {
        assetData["Model"] = *model;
    }
    if (partNumber != nullptr)
    {
        assetData["PartNumber"] = *partNumber;
    }
    if (serialNumber != nullptr)
    {
        assetData["SerialNumber"] = *serialNumber;
    }
    // SparePartNumber is optional on D-Bus so skip if it is empty
    if (includeSparePartNumber && sparePartNumber != nullptr &&
        !sparePartNumber->empty())
    {
        assetData["SparePartNumber"] = *sparePartNumber;
    }
}

inline void getAssetInfo(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& serviceName, const std::string& dbusPath,
    const nlohmann::json::json_pointer& jsonKeyName,
    bool includeSparePartNumber = false, bool includeManufacturer = true)
{
    dbus::utility::getAllProperties(
        serviceName, dbusPath, "xyz.openbmc_project.Inventory.Decorator.Asset",
        [asyncResp, jsonKeyName, includeSparePartNumber, includeManufacturer](
            const boost::system::error_code& ec,
            const dbus::utility::DBusPropertiesMap& assetList) {
            if (ec)
            {
                if (ec.value() != EBADR)
                {
                    BMCWEB_LOG_ERROR("DBUS response error for Properties {}",
                                     ec.value());
                    messages::internalError(asyncResp->res);
                }
                return;
            }
            extractAssetInfo(asyncResp, jsonKeyName, assetList,
                             includeSparePartNumber, includeManufacturer);
        });
}

} // namespace asset_utils
} // namespace redfish
