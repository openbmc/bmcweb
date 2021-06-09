
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
 * @param[i,o] asyncResp    Async response object
 * @param[i]   serviceName  Service exporting the PrettyName
 * @param[i]   objectPath   Dbus path of the resource
 * @param[i]   namePath     Json pointer to the name field to update.
 *
 * @return void
 */
inline void
    getPrettyName(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                  const std::string& serviceName, const std::string& objectPath,
                  const nlohmann::json_pointer<nlohmann::json>& namePath)
{
    BMCWEB_LOG_DEBUG << "Get PrettyName for: " << objectPath;

    crow::connections::systemBus->async_method_call(
        [asyncResp, objectPath,
         namePath](const boost::system::error_code ec,
                   const std::variant<std::string>& prettyName) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                return;
            }

            const std::string* value = std::get_if<std::string>(&prettyName);
            if (!value)
            {
                BMCWEB_LOG_ERROR << "Failed to get Pretty Name for "
                                 << objectPath;
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
        serviceName, objectPath, "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Inventory.Item", "PrettyName");
}

} // namespace name_util
} // namespace redfish
