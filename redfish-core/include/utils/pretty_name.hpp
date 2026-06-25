// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "async_resp.hpp"
#include "dbus_utility.hpp"

#include <memory>
#include <string>

namespace redfish::utils
{

inline void setPrettyName(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& service,
    const std::string& path,
    const std::string& defaultName)
{
    asyncResp->res.jsonValue["Name"] = defaultName;

    dbus::utility::getProperty<std::string>(
        service, path,
        "xyz.openbmc_project.Inventory.Item",
        "PrettyName",
        [asyncResp](const boost::system::error_code& ec,
                    const std::string& prettyName) {
            if (!ec && !prettyName.empty())
            {
                asyncResp->res.jsonValue["Name"] = prettyName;
            }
        });
}

} // namespace redfish::utils
