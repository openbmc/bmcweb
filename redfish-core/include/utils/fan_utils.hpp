// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "human_sort.hpp"
#include "logging.hpp"

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
    const sdbusplus::message::object_path& fanPath,
    const std::function<void(
        const std::vector<std::pair<std::string, std::string>>&)>& callback)
{
    sdbusplus::message::object_path endpointPath{fanPath};
    endpointPath /= "sensors";

    dbus::utility::getAssociatedSubTree(
        endpointPath,
        sdbusplus::message::object_path("/xyz/openbmc_project/sensors"), 0,
        sensorInterface,
        std::bind_front(afterGetFanSensorObjects, asyncResp, callback));
}

inline std::string getChassisFanAssociation()
{
    if constexpr (BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
        return "containing";
    }
    return "cooled_by";
}

inline void getFanPaths(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& validChassisPath,
    const std::function<void(const dbus::utility::MapperGetSubTreePathsResponse&
                                 fanPaths)>& callback)
{
    sdbusplus::message::object_path endpointPath{validChassisPath};

    endpointPath /= getChassisFanAssociation();

    dbus::utility::getAssociatedSubTreePaths(
        endpointPath,
        sdbusplus::message::object_path("/xyz/openbmc_project/inventory"), 0,
        fanInterface,
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

} // namespace fan_utils
} // namespace redfish
