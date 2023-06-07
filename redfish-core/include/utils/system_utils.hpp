#pragma once

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"

#include <array>
#include <string_view>

namespace redfish
{

namespace system_utils
{
/**
 * @brief Retrieves valid system path
 * @param asyncResp   Pointer to object holding response data
 * @param callback  Callback for next step to get valid system path
 * Callback should take in asyncResp pointer and full object path of Item.System
 * Single Host Systems will return empty string as the object path of
 * Item.System.
 */
template <typename Callback>
void getSystemInformation(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& systemName, Callback&& callback)
{
    crow::connections::systemBus->async_method_call(
        [callback{std::forward<Callback>(callback)}, asyncResp,
         systemName](const boost::system::error_code ec,
                     const std::vector<std::string>& objects) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG << "DBUS response error";
            messages::internalError(asyncResp->res);
            return;
        }

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
            std::string objectName = object.substr(object.rfind("/") + 1);
            // Found system object
            if (systemName == objectName)
            {
                callback(asyncResp, object);
                return;
            }
        }

        // Could not find system object
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths", "/", 0,
        std::array<const char*, 1>{
            "xyz.openbmc_project.Inventory.Item.System"});
}

} // namespace system_utils
} // namespace redfish
