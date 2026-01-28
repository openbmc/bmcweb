// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "human_sort.hpp"
#include "logging.hpp"
#include "utils/chassis_utils.hpp"

#include <asm-generic/errno.h>

#include <boost/system/error_code.hpp>
#include <sdbusplus/message/native_types.hpp>

#include <algorithm>
#include <array>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace redfish
{
constexpr std::array<std::string_view, 1> fanInterface = {
    "xyz.openbmc_project.Inventory.Item.Fan"};

namespace fan_utils
{
constexpr std::array<std::string_view, 1> sensorInterface = {
    "xyz.openbmc_project.Sensor.Value"};

inline void afterGetFanSensorObjects(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::function<void(
        const std::vector<std::pair<std::string, std::string>>&)>& callback,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec)
    {
        if (ec.value() != boost::system::errc::io_error && ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR("DBUS response error {}", ec);
            messages::internalError(asyncResp->res);
        }
        return;
    }

    std::vector<std::pair<std::string, std::string>> sensorsPathAndService;

    for (const auto& [sensorPath, serviceMaps] : subtree)
    {
        for (const auto& [service, interfaces] : serviceMaps)
        {
            sensorsPathAndService.emplace_back(service, sensorPath);
        }
    }

    /* Sort list by sensorPath */
    std::ranges::sort(
        sensorsPathAndService, [](const auto& c1, const auto& c2) {
            return AlphanumLess<std::string>()(c1.second, c2.second);
        });

    callback(sensorsPathAndService);
}

inline void getFanSensorObjects(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const sdbusplus::object_path& fanPath,
    const std::function<void(
        const std::vector<std::pair<std::string, std::string>>&)>& callback)
{
    sdbusplus::object_path endpointPath{fanPath};
    endpointPath /= "sensors";

    dbus::utility::getAssociatedSubTree(
        endpointPath, sdbusplus::object_path("/xyz/openbmc_project/sensors"), 0,
        sensorInterface,
        std::bind_front(afterGetFanSensorObjects, asyncResp, callback));
}

inline std::vector<std::string> fanPathsByAssociation(
    const std::string& chassisId,
    const dbus::utility::GetPathsByAssociationResult& result)
{
    // If there is a single 'containing' association,
    // we are using the newer code paths and the fan collection is formed
    // only with those contained fans.
    //
    // If only cooling/cooled_by association is present,
    // we are in the old code path where only that association is considered.
    //
    // If there are no fans, it does not matter, since the collection
    // will be empty either way.

    bool foundContaining = false;
    std::vector<std::string> containedFans;
    std::vector<std::string> coolingFans;

    for (const auto& item : result)
    {
        const sdbusplus::object_path chassisPath(std::get<0>(item));
        const auto& fanPath = std::get<2>(item);
        const auto& assoc = std::get<1>(item);

        const bool isContaining = assoc == "containing";
        const bool isCooledBy = assoc == "cooled_by";

        if (isContaining)
        {
            foundContaining = true;
        }

        if (chassisPath.filename() != chassisId)
        {
            continue;
        }

        if (isContaining)
        {
            containedFans.push_back(fanPath);
        }
        if (isCooledBy)
        {
            coolingFans.push_back(fanPath);
        }
    }

    std::vector<std::string>& res = containedFans;

    if (foundContaining)
    {
        BMCWEB_LOG_DEBUG("fan collection based on 'containing'");
    }
    else
    {
        BMCWEB_LOG_DEBUG("fan collection based on 'cooled_by' (legacy)");
        res = coolingFans;
    }

    BMCWEB_LOG_DEBUG("fan collection found {} fans", res.size());

    return res;
}

inline void getFanPathsByChassisId(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId,
    const std::function<void(const dbus::utility::MapperGetSubTreePathsResponse&
                                 fanPaths)>& callback)
{
    const sdbusplus::object_path path("/xyz/openbmc_project/inventory");

    const std::vector<std::string_view> associations{"containing", "cooled_by"};

    dbus::utility::getPathsByAssociation(
        path, chassisInterfaces, associations, path, fanInterface, 0,
        [asyncResp, callback,
         chassisId](const boost::system::error_code& ec,
                    const dbus::utility::GetPathsByAssociationResult& res) {
            if (ec)
            {
                if (ec.value() != boost::system::errc::io_error &&
                    ec.value() != EBADR)
                {
                    BMCWEB_LOG_ERROR("DBUS response error {}", ec);
                    messages::internalError(asyncResp->res);
                }
                else
                {
                    messages::resourceNotFound(asyncResp->res, "Chassis",
                                               chassisId);
                }
                return;
            }

            callback(fanPathsByAssociation(chassisId, res));
        });
}

inline void getFanPaths(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& validChassisPath,
    const std::function<void(const dbus::utility::MapperGetSubTreePathsResponse&
                                 fanPaths)>& callback)
{
    sdbusplus::object_path path(validChassisPath);
    getFanPathsByChassisId(asyncResp, path.filename(), callback);
}

} // namespace fan_utils
} // namespace redfish
