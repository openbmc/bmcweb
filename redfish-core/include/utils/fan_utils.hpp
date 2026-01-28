// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "logging.hpp"

#include <asm-generic/errno.h>

#include <boost/system/error_code.hpp>
#include <sdbusplus/message/native_types.hpp>

#include <array>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace redfish
{
constexpr std::array<std::string_view, 1> fanInterface = {
    "xyz.openbmc_project.Inventory.Item.Fan"};

namespace fan_utils
{
inline std::string getChassisFanAssociation()
{
    if constexpr (BMCWEB_REDFISH_MULTI_HOST_FAN_INVENTORY)
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
