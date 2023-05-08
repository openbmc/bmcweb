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

constexpr std::array<std::string_view, 2> chassisInterfaces = {
    "xyz.openbmc_project.Inventory.Item.Board",
    "xyz.openbmc_project.Inventory.Item.Chassis"};

/**
 * @brief Retrieves valid chassis path
 * @param asyncResp   Pointer to object holding response data
 * @param callback  Callback for next step to get valid chassis path
 */
template <typename Callback>
void getValidChassisPath(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::string& chassisId, Callback&& callback)
{
    BMCWEB_LOG_DEBUG << "checkChassisId enter";

    // Get the Chassis Collection
    dbus::utility::getSubTreePaths(
        "/xyz/openbmc_project/inventory", 0, chassisInterfaces,
        [callback{std::forward<Callback>(callback)}, asyncResp,
         chassisId](const boost::system::error_code& ec,
                    const dbus::utility::MapperGetSubTreePathsResponse&
                        chassisPaths) mutable {
        BMCWEB_LOG_DEBUG << "getValidChassisPath respHandler enter";
        if (ec)
        {
            BMCWEB_LOG_ERROR << "getValidChassisPath respHandler DBUS error: "
                             << ec;
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
                BMCWEB_LOG_ERROR << "Failed to find '/' in " << chassis;
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
    BMCWEB_LOG_DEBUG << "checkChassisId exit";
}

/**
 * @brief Get subtree for associated endpoint with chassis interfaces.
 * @param asyncResp         Pointer to object holding response data
 * @param associationPath   Path used to associate with chassis endpoints
 * @param callback          Callback to process subtree response
 */
template <typename Callback>
inline void getAssociatedChassisSubtree(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& associationPath, Callback&& callback)
{
    sdbusplus::message::object_path association(associationPath);
    sdbusplus::message::object_path root("/xyz/openbmc_project/inventory");
    dbus::utility::getAssociatedSubTree(
        association, root, 0, chassisInterfaces,
        [asyncResp, associationPath,
         callback{std::forward<Callback>(callback)}](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreeResponse& subtree) {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "Failed to get chassis associations for "
                             << associationPath << " ec: " << ec.message();
            messages::internalError(asyncResp->res);
            return;
        }
        callback(asyncResp, subtree);
        });
}

} // namespace chassis_utils
} // namespace redfish
