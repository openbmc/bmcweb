
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
 * @param[i]   path  Redfish collection path which is used for the
 *             Members Redfish Path
 * @param[i]   services  List of interfaces to constrain the GetSubTree search
 *
 * @return void
 */
inline void getPrettyName(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& path,
    const std::vector<std::pair<std::string, std::vector<std::string>>>&
        services)
{
    BMCWEB_LOG_DEBUG << "Get PrettyName for: " << path;

    bool foundName = false;
    for (const auto& service : services)
    {
        crow::connections::systemBus->async_method_call(
            [asyncResp,
             &foundName](const boost::system::error_code ec,
                         const std::variant<std::string>& prettyName) {
                if (ec)
                {
                    BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                    return;
                }

                const std::string* s = std::get_if<std::string>(&prettyName);
                BMCWEB_LOG_DEBUG << "Pretty Name: " << *s;
                if (s != nullptr)
                {
                    asyncResp->res.jsonValue["Name"] = *s;
                    foundName = true;
                }
            },
            service.first, path, "org.freedesktop.DBus.Properties", "Get",
            "xyz.openbmc_project.Inventory.Item", "PrettyName");
        if (foundName)
        {
            break;
        }
    }
}

} // namespace name_util
} // namespace redfish
