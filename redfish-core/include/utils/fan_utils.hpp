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

inline void getFanPaths(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& validChassisPath,
    const std::function<void(const dbus::utility::MapperGetSubTreePathsResponse&
                                 fanPaths)>& callback)
{
    sdbusplus::object_path endpointPath{validChassisPath};
    if constexpr (BMCWEB_REDFISH_FAN_LINKS)
    {
        endpointPath /= "containing";
    }
    else
    {
        endpointPath /= "cooled_by";
    }

    dbus::utility::getAssociatedSubTreePaths(
        endpointPath, sdbusplus::object_path("/xyz/openbmc_project/inventory"),
        0, fanInterface,
        // ast-grep-ignore: long-lambda
        [asyncResp, callback](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreePathsResponse& subtreePaths) {
            if (ec)
            {
                if (ec.value() != boost::system::errc::io_error &&
                    ec.value() != EBADR)
                {
                    BMCWEB_LOG_ERROR("DBUS response error {}", ec);
                    messages::internalError(asyncResp->res);
                }
                return;
            }
            callback(subtreePaths);
        });
}

using FanMap =
    std::map<std::string, std::pair<std::vector<sdbusplus::object_path>,
                                    std::vector<sdbusplus::object_path>>>;

// @returns both {'containing', 'cooled_by'} fans for the chassis
// and depending on which links or collection we are populating
// the caller can go with either one of them
inline FanMap fanPathsByAssociation(
    const dbus::utility::GetPathsByAssociationResult& result)
{
    FanMap res;

    for (const auto& item : result)
    {
        const sdbusplus::object_path chassisPath(std::get<1>(item));
        const std::string& assoc = std::get<2>(item);
        const sdbusplus::object_path fanPath = std::get<4>(item);

        auto& pair = res[chassisPath.filename()];
        auto& containingFans = std::get<0>(pair);
        auto& cooledByFans = std::get<1>(pair);

        if (assoc == "containing")
        {
            containingFans.push_back(fanPath);
        }
        if (assoc == "cooled_by")
        {
            cooledByFans.push_back(fanPath);
        }
    }

    return res;
}

// @param chassisId: known (existing) chassis id
inline void getFanPathsByChassisId(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId,
    const std::function<void(const FanMap& fanMap)>& callback)
{
    const sdbusplus::object_path path("/xyz/openbmc_project/inventory");

    const std::vector<std::string_view> associations{"containing", "cooled_by"};

    dbus::utility::getPathsByAssociation(
        path, chassisInterfaces, associations, path, fanInterface, 0,
        // ast-grep-ignore: long-lambda
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
                return;
            }

            const auto map = fanPathsByAssociation(res);
            callback(map);
        });
}

// @returns a map of: owning chassis -> {fan path}
// for any non-subordinate fan resources of the chassis provided as parameter
inline std::map<std::string, std::vector<sdbusplus::object_path>>
    getLinkedFanPaths(const std::string& chassisId, const FanMap& fanMap)
{
    std::map<std::string, std::vector<sdbusplus::object_path>> res;

    if constexpr (!BMCWEB_REDFISH_FAN_LINKS)
    {
        return res;
    }

    if (!fanMap.contains(chassisId))
    {
        return res;
    }

    const auto& fanPaths = fanMap.at(chassisId);

    const auto& fanPathsContaining = std::get<0>(fanPaths);
    const auto& fanPathsCooledBy = std::get<1>(fanPaths);

    std::set<sdbusplus::object_path> fpcontainSet(fanPathsContaining.begin(),
                                                  fanPathsContaining.end());
    std::set<sdbusplus::object_path> fpcoolSet(fanPathsCooledBy.begin(),
                                               fanPathsCooledBy.end());

    std::set<sdbusplus::object_path> linkedFans;

    // TODO: make object_path work with ranges algorithms
    auto pathLess = [](const sdbusplus::object_path& lhs,
                       const sdbusplus::object_path& rhs) {
        return lhs.string() < rhs.string();
    };

    // making sure all the linked fans are not also subordinate resources
    // (which would be misconfiguration anyways, but we can easily check it
    // here)
    std::ranges::set_difference(fpcoolSet, fpcontainSet,
                                std::inserter(linkedFans, linkedFans.begin()),
                                pathLess);

    for (const auto& fanPath : linkedFans)
    {
        for (const auto& [otherChassisId, pair] : fanMap)
        {
            if (otherChassisId == chassisId)
            {
                continue;
            }
            const auto& ownedFans = std::get<0>(pair);

            if (std::ranges::find(ownedFans, fanPath) != ownedFans.end())
            {
                res[otherChassisId].emplace_back(fanPath);
            }
        }
    }

    return res;
}

} // namespace fan_utils
} // namespace redfish
