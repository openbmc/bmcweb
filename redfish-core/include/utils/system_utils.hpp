#pragma once

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"

#include <boost/system/error_code.hpp>
#include <sdbusplus/message/types.hpp>

#include <array>
#include <string_view>
#include <utility>

namespace redfish
{

namespace system_utils
{
template <typename Callback>
inline findSystemPathAndTriggerCallback(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName,
    const dbus::utility::MapperGetSubTreePathsResponse& objects,
    Callback&& callback)
{
    // If there are less than 2 Item.Systems assume singlehost
    if (objects.size() < 2)
    {
        if (systemName != "system")
        {
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }
        callback(asyncResp, "");
        return;
    }
    // Otherwise find system
    for (const auto& object : objects)
    {
        std::string objectName =
            sdbusplus::message::object_path(object).filename();
        // Found system object
        if (systemName == objectName)
        {
            callback(asyncResp, object);
            return;
        }
    }

    // Could not find system object
    messages::resourceNotFound(asyncResp->res, "ComputerSystem", systemName);
}
/**
 * @brief Retrieves valid system path
 * @param asyncResp   Pointer to object holding response data
 * @param callback  Callback for next step to get valid system path
 * Callback should take in asyncResp pointer and full object path of Item.System
 * Single Host Systems will return empty string as the object path of
 * Item.System.
 */
template <typename Callback>
void getValidSystemPath(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& systemName, Callback&& callback)
{
    dbus::utility::getSubTreePaths(
        "/", 0,
        std::array<const char*, 1>{"xyz.openbmc_project.Inventory.Item.System"},
        [callback{std::forward<Callback>(callback)}, asyncResp, systemName](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreePathsResponse& objects) {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "DBUS response error " << ec;
            messages::internalError(asyncResp->res);
            return;
        }
        findSystemPathAndTriggerCallback(asyncResp, objects, systemName,
                                         callback);
        });
}

} // namespace system_utils
} // namespace redfish
