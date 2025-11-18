// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "async_resp.hpp"
#include "boost_formatters.hpp"
#include "dbus_utility.hpp"
#include "dbus_utils.hpp"
#include "error_messages.hpp"
#include "logging.hpp"

#include <sdbusplus/message/native_types.hpp>

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace redfish
{

static constexpr std::array<std::string_view, 2> chassisInterfaces = {
    "xyz.openbmc_project.Inventory.Item.Board",
    "xyz.openbmc_project.Inventory.Item.Chassis"};

namespace chassis_utils
{
/**
 * @brief Retrieves valid chassis path
 * @param asyncResp   Pointer to object holding response data
 * @param callback  Callback for next step to get valid chassis path
 */
template <typename Callback>
void getValidChassisPath(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::string& chassisId, Callback&& callback)
{
    BMCWEB_LOG_DEBUG("checkChassisId enter");

    // Get the Chassis Collection
    dbus::utility::getSubTreePaths(
        dbus_utils::inventoryPath, 0, chassisInterfaces,
        [callback = std::forward<Callback>(callback), asyncResp,
         chassisId](const boost::system::error_code& ec,
                    const dbus::utility::MapperGetSubTreePathsResponse&
                        chassisPaths) mutable {
            BMCWEB_LOG_DEBUG("getValidChassisPath respHandler enter");
            if (ec)
            {
                BMCWEB_LOG_ERROR(
                    "getValidChassisPath respHandler DBUS error: {}", ec);
                messages::internalError(asyncResp->res);
                return;
            }

            std::optional<std::string> chassisPath;
            for (const std::string& chassis : chassisPaths)
            {
                sdbusplus::message::object_path path(chassis);
                std::string chassisName = path.filename();
                if (chassisName.empty())
                {
                    BMCWEB_LOG_ERROR("Failed to find '/' in {}", chassis);
                    continue;
                }
                if (chassisName == chassisId)
                {
                    chassisPath = chassis;
                    break;
                }
            }
            callback(chassisPath);
        });
    BMCWEB_LOG_DEBUG("checkChassisId exit");
}

} // namespace chassis_utils
} // namespace redfish
