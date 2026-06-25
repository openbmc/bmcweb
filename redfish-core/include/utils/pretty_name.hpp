// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "async_resp.hpp"
#include "dbus_utility.hpp"

#include <nlohmann/json.hpp>

#include <memory>
#include <string>

namespace redfish::utils
{

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
inline void getPrettyName(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& service, const std::string& path,
                          const std::string& defaultName,
                          const std::string& jsonPtr = "/Name")
{
    // Set default Name at the specified JSON pointer
    asyncResp->res.jsonValue[jsonPtr] = defaultName;

    // Try to get PrettyName
    dbus::utility::getProperty<std::string>(
        service, path, "xyz.openbmc_project.Inventory.Item", "PrettyName",
        [asyncResp, jsonPtr](const boost::system::error_code& ec,
                             const std::string& prettyName) {
            if (!ec && !prettyName.empty())
            {
                asyncResp->res.jsonValue[jsonPtr] = prettyName;
            }
        });
}

} // namespace redfish::utils
