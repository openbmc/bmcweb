
#pragma once
#include <async_resp.hpp>

#include <algorithm>
#include <string>
#include <variant>
#include <vector>

namespace redfish
{
namespace name_util
{

/**
 * @brief Populate the collection "Members" from a GetSubTreePaths search of
 *        inventory
 *
 * @param[i,o] asyncResp  Async response object
 * @param[i]   path       D-bus object path to find the find pretty name
 * @param[i]   services   List of services to exporting the D-bus object path
 * @param[i]   namePath   Json pointer to the name field to update.
 *
 * @return void
 */
inline void getPrettyName(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& path,
    const std::vector<std::pair<std::string, std::vector<std::string>>>&
        services,
    const nlohmann::json_pointer<nlohmann::json>& namePath)
{
    BMCWEB_LOG_DEBUG << "Get PrettyName for: " << path;

    for (const std::pair<std::string, std::vector<std::string>>& service :
         services)
    {
        if (service.first != "xyz.openbmc_project.Inventory.Item")
        {
            BMCWEB_LOG_DEBUG << "No PrettyName property available for: "
                             << service.first;
            continue
        }

        crow::connections::systemBus->async_method_call(
            [asyncResp, path,
             namePath](const boost::system::error_code ec,
                       const std::variant<std::string>& prettyName) {
                if (ec)
                {
                    BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                    return;
                }

                const std::string* value =
                    std::get_if<std::string>(&prettyName);

                if (!value)
                {

                    BMCWEB_LOG_ERROR << "Failed to get Pretty Name for "
                                     << path;
                    messages::internalError(asyncResp->res);
                    return;
                }

                if (value->empty())
                {
                    return;
                }

                BMCWEB_LOG_DEBUG << "Pretty Name: " << *value;

                asyncResp->res.jsonValue[namePath] = *value;
            },
            service.first, path, "org.freedesktop.DBus.Properties", "Get",
            "xyz.openbmc_project.Inventory.Item", "PrettyName");

        return;
    }
}

} // namespace name_util
} // namespace redfish
