#pragma once

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"

#include <array>
#include <string_view>

namespace redfish
{

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
    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Inventory.Item.Chassis"};

    // Get the Chassis Collection
    dbus::utility::getSubTreePaths(
        "/xyz/openbmc_project/inventory", 0, interfaces,
        [callback{std::forward<Callback>(callback)}, asyncResp,
         chassisId](const boost::system::error_code& ec,
                    const dbus::utility::MapperGetSubTreePathsResponse&
                        chassisPaths) mutable {
        BMCWEB_LOG_DEBUG("getValidChassisPath respHandler enter");
        if (ec)
        {
            BMCWEB_LOG_ERROR("getValidChassisPath respHandler DBUS error: {}",
                             ec);
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
