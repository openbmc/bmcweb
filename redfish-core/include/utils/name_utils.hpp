// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#pragma once

#include "async_resp.hpp"
#include "dbus_utility.hpp"

#include <nlohmann/json.hpp>

#include <memory>
#include <string>

namespace redfish
{

namespace name_utils
{

inline void afterGetPrettyName(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const nlohmann::json::json_pointer& jsonPtr,
    const boost::system::error_code& ec, const std::string& prettyName)
{
    if (!ec && !prettyName.empty())
    {
        asyncResp->res.jsonValue[jsonPtr] = prettyName;
    }
}
/**
 * @brief Get PrettyName from D-Bus Inventory.Item interface and set it
 *        at the specified JSON pointer location.
 *
 * @param asyncResp AsyncResp object to update
 * @param service D-Bus service name
 * @param path D-Bus object path
 * @param defaultName Fallback name if PrettyName is not available
 * @param jsonPtr JSON pointer where to set the name (default: "/Name")
 */
inline void getPrettyName(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& service, const std::string& path,
    const std::string& defaultName,
    const nlohmann::json::json_pointer& jsonPtr = "/Name"_json_pointer)
{
    asyncResp->res.jsonValue[jsonPtr] = defaultName;

    dbus::utility::getProperty<std::string>(
        service, path, "xyz.openbmc_project.Inventory.Item", "PrettyName",
        std::bind_front(afterGetPrettyName, asyncResp, jsonPtr));
}

/**
 * @brief Get PrettyName from the first service that implements Inventory.Item
 * otherwise defaults to defaultName
 *
 * @param asyncResp AsyncResp object to update
 * @param services D-Bus service names to try, in priority order
 * @param path D-Bus object path
 * @param defaultName Fallback name if no service returns a valid PrettyName
 * @param jsonPtr JSON pointer where to set the name (default: "/Name")
 */
inline void getPrettyName(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const dbus::utility::MapperServiceMap& services, const std::string& path,
    const std::string& defaultName,
    const nlohmann::json::json_pointer& jsonPtr = "/Name"_json_pointer)
{
    for (const auto& [serviceName, interfaces] : services)
    {
        for (const auto& interface : interfaces)
        {
            if (interface == "xyz.openbmc_project.Inventory.Item")
            {
                getPrettyName(asyncResp, serviceName, path, defaultName,
                              jsonPtr);
                return;
            }
        }
    }
    // If the Inventory.Item interface is missing, set to default
    asyncResp->res.jsonValue[jsonPtr] = defaultName;
}

} // namespace name_utils
} // namespace redfish
