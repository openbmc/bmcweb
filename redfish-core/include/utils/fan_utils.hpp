#pragma once

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"

#include <array>
#include <string_view>

namespace redfish
{
namespace fan_utils
{
constexpr std::array<std::string_view, 1> fanInterface = {
    "xyz.openbmc_project.Inventory.Item.Fan"};
constexpr std::array<std::string_view, 1> sensorInterface = {
    "xyz.openbmc_project.Sensor.Value"};

template <typename Callback>
inline void
    getFanSensorsObject(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& fanPath, Callback&& callback)
{
    sdbusplus::message::object_path endpointPath{fanPath};
    endpointPath /= "all_sensors";

    dbus::utility::getAssociatedSubTree(
        endpointPath,
        sdbusplus::message::object_path("/xyz/openbmc_project/inventory"), 0,
        sensorInterface,
        [asyncResp, callback{std::forward<Callback>(callback)}](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreeResponse& subtree) {
        if (ec)
        {
            if (ec.value() != EBADR)
            {
                BMCWEB_LOG_ERROR(
                    "DBUS response error for getAssociatedSubTree {}",
                    ec.value());
                messages::internalError(asyncResp->res);
            }
            return;
        }

        for (const auto& [sensorPath, serviceMaps] : subtree)
        {
            for (const auto& [service, interfaces] : serviceMaps)
            {
                callback(service, sensorPath);
            }
        }
    });
}

template <typename Callback>
inline void getFanPaths(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::optional<std::string>& validChassisPath,
                        Callback&& callback)
{
    sdbusplus::message::object_path endpointPath{*validChassisPath};
    endpointPath /= "cooled_by";

    dbus::utility::getAssociatedSubTreePaths(
        endpointPath,
        sdbusplus::message::object_path("/xyz/openbmc_project/inventory"), 0,
        fanInterface,
        [asyncResp, callback{std::forward<Callback>(callback)}](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreePathsResponse& subtreePaths) {
        if (ec)
        {
            if (ec.value() != EBADR)
            {
                BMCWEB_LOG_ERROR(
                    "DBUS response error for getAssociatedSubTreePaths {}",
                    ec.value());
                messages::internalError(asyncResp->res);
            }
            return;
        }
        callback(subtreePaths);
    });
}

} // namespace fan_utils
} // namespace redfish
