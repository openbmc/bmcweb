#pragma once

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"

#include <boost/system/error_code.hpp>

#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace redfish
{
namespace fan_utils
{
constexpr std::array<std::string_view, 1> fanInterface = {
    "xyz.openbmc_project.Inventory.Item.Fan"};

inline void getFanPaths(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::optional<std::string>& validChassisPath,
    std::function<void(const dbus::utility::MapperGetSubTreePathsResponse&
                           fanPaths)>&& callback)
{
    sdbusplus::message::object_path endpointPath{*validChassisPath};
    endpointPath /= "cooled_by";

    dbus::utility::getAssociatedSubTreePaths(
        endpointPath,
        sdbusplus::message::object_path("/xyz/openbmc_project/inventory"), 0,
        fanInterface,
        [asyncResp, callback{std::move(callback)}](
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
