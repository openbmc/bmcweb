
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
 * @param[i]   interfaces List of interfaces to constrain the GetSubTree search
 * @param[o]   name       Json object to output the pretty name
 *
 * @return void
 */
inline void getPrettyName(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& path,
                          const std::vector<const char*>& interfaces,
                          nlohmann::json* name)
{
    BMCWEB_LOG_DEBUG << "Get PrettyName for: " << path;

    crow::connections::systemBus->async_method_call(
        [asyncResp, path, &name](
            const boost::system::error_code ec,
            const std::vector<std::pair<std::string, std::vector<std::string>>>&
                objInfo) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "error_code = " << ec
                                 << ", error msg = " << ec.message();
                if (asyncResp)
                {
                    messages::internalError(asyncResp->res);
                }
                return;
            }

            // Ensure we only got one service back
            if (objInfo.size() != 1)
            {
                BMCWEB_LOG_ERROR << "Invalid Object Size " << objInfo.size();
                if (asyncResp)
                {
                    messages::internalError(asyncResp->res);
                }
                return;
            }

            crow::connections::systemBus->async_method_call(
                [asyncResp, path,
                 &name](const boost::system::error_code ec,
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

                    BMCWEB_LOG_DEBUG << "Pretty Name: " << *value;
                    *name = *value;
                },
                objInfo[0].first, path, "org.freedesktop.DBus.Properties",
                "Get", "xyz.openbmc_project.Inventory.Item", "PrettyName");
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetObject", path, interfaces);
}

} // namespace name_util
} // namespace redfish
