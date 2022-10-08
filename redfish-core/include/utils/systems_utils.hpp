#pragma once

#include "async_resp.hpp"

#include <boost/system/error_code.hpp>

#include <array>
#include <functional>
#include <optional>
#include <string>
#include <utility>

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
static void getValidSystemsPath(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemId,
    std::function<void(const std::optional<std::string>&)>&& callback)
{
    BMCWEB_LOG_DEBUG << "checkSystemsId enter";

    auto respHandler =
        [callback{std::move(callback)}, asyncResp, systemId](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreePathsResponse& systemsPaths) {
        BMCWEB_LOG_DEBUG << "getValidSystemsPath respHandler enter";
        if (ec)
        {
            BMCWEB_LOG_ERROR << "getValidSystemsPath respHandler DBUS error: "
                             << ec;
            messages::internalError(asyncResp->res);
            return;
        }

        std::optional<std::string> systemsPath;
        std::string systemName;
        for (const std::string& system : systemsPaths)
        {
            sdbusplus::message::object_path path(system);
            systemName = path.filename();
            if (systemName.empty())
            {
                BMCWEB_LOG_ERROR << "Failed to find '/' in " << system;
                continue;
            }
            if (systemName == systemId)
            {
                systemsPath = system;
                break;
            }
        }
        callback(systemsPath);
    };

    // Get the Systems Collection
    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Inventory.Item.System"};
    dbus::utility::getSubTreePaths("/xyz/openbmc_project/inventory", 0,
                                   interfaces, std::move(respHandler));
    BMCWEB_LOG_DEBUG << "checkSystemsId exit";
}

} // namespace systems_utils
} // namespace redfish
