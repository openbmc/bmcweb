#pragma once

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"

#include <array>
#include <functional>
#include <string>
#include <string_view>

namespace redfish
{

namespace port_utils
{

/**
 * @brief Retrieves Ports
 * @param asyncResp   Pointer to object holding response data
 * @param fabricAdapterPath Adapter dbus path
 * @param callback  Callback for next step to handle port list
 */

inline void getPortList(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& fabricAdapterPath,
    std::function<void(const dbus::utility::MapperEndPoints&)>&& callback)
{
    constexpr std::array<std::string_view, 1> portInterface{
        "xyz.openbmc_project.Inventory.Connector.Port"};

    dbus::utility::getAssociatedSubTreePaths(
        fabricAdapterPath + "/connecting",
        sdbusplus::message::object_path("/xyz/openbmc_project/inventory"), 0,
        portInterface,
        [asyncResp, callback{std::move(callback)}](
            const boost::system::error_code& ec,
            const dbus::utility::MapperEndPoints& portPaths) {
        if (ec)
        {
            if (ec.value() == EBADR)
            {
                BMCWEB_LOG_DEBUG("Port association not found");
                return;
            }
            BMCWEB_LOG_ERROR("DBUS response error {}", ec.value());
            messages::internalError(asyncResp->res);
            return;
        }
        // caller will handle the case of empty Ports
        callback(portPaths);
        });
}

} // namespace port_utils
} // namespace redfish
