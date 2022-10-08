#pragma once

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "logging.hpp"

#include <boost/system/error_code.hpp>

#include <array>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace redfish
{

namespace systems_utils
{
/**
 * @brief Retrieves valid systems path
 * @param asyncResp     Pointer to object holding response data
 * @param systemId      System Id to be verified
 * @param callback      Callback for next step to get valid systems path
 */
inline void getValidSystemPath(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemId,
    std::function<void(const std::optional<std::string>&)>&& callback)
{
    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Inventory.Item.System"};

    dbus::utility::getSubTreePaths(
        "/xyz/openbmc_project/inventory", 0, interfaces,
        [callback{std::move(callback)}, asyncResp, systemId](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreePathsResponse& systemsPaths) {
        if (ec)
        {
            BMCWEB_LOG_ERROR("D-Bus response error on GetSubTree: {}", ec);
            messages::internalError(asyncResp->res);
            return;
        }

        std::optional<std::string> systemsPath;
        std::string systemName;
        for (const std::string& system : systemsPaths)
        {
            systemName = sdbusplus::message::object_path(system).filename();
            if (systemName.empty() || systemName != systemId)
            {
                continue;
            }
            systemsPath = system;
            break;
        }
        callback(systemsPath);
        });
}

} // namespace systems_utils
} // namespace redfish
