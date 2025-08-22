// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "bmcweb_config.h"

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "logging.hpp"
#include "persistent_data.hpp"

#include <nlohmann/json.hpp>

#include <array>
#include <cstddef>
#include <functional>
#include <memory>
#include <string_view>
#include <utility>

namespace redfish
{

constexpr std::array<std::string_view, 1> bmcInterfaces = {
    "xyz.openbmc_project.Inventory.Item.Bmc"};

namespace manager_utils
{

inline void setServiceIdentification(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    std::string_view serviceIdentification)
{
    constexpr const size_t maxStrSize = 99;
    constexpr const char* allowedChars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 _-";
    if (serviceIdentification.size() > maxStrSize)
    {
        messages::stringValueTooLong(asyncResp->res, "ServiceIdentification",
                                     maxStrSize);
        return;
    }
    if (serviceIdentification.find_first_not_of(allowedChars) !=
        std::string_view::npos)
    {
        messages::propertyValueError(asyncResp->res, "ServiceIdentification");
        return;
    }

    persistent_data::ConfigFile& config = persistent_data::getConfig();
    config.serviceIdentification = serviceIdentification;
    config.writeData();
    messages::success(asyncResp->res);
}

inline void getServiceIdentification(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const bool isServiceRoot)
{
    std::string_view serviceIdentification =
        persistent_data::getConfig().serviceIdentification;

    // This property shall not be present if its value is an empty string or
    // null: Redfish Data Model Specification 6.125.3
    if (isServiceRoot && serviceIdentification.empty())
    {
        return;
    }
    asyncResp->res.jsonValue["ServiceIdentification"] = serviceIdentification;
}

inline void afterGetValidManagerPath(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& managerId,
    const std::function<
        void(const boost::system::error_code&, const std::string& managerPath,
             const dbus::utility::MapperServiceMap& serviceMap)>& callback,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec || subtree.empty())
    {
        // Pass the empty managerPath & serviceMap
        callback(ec, {}, {});
        return;
    }

    // Assume only 1 bmc D-Bus object
    // Throw an error if there is more than 1
    if (subtree.size() > 1)
    {
        BMCWEB_LOG_ERROR("Found more than 1 bmc D-Bus object!");
        messages::internalError(asyncResp->res);
        return;
    }

    if (managerId != BMCWEB_REDFISH_MANAGER_URI_NAME)
    {
        // Treat it as NotFound
        callback(ec, {}, {});
        return;
    }

    callback(ec, subtree[0].first, subtree[0].second);
}

inline void getValidManagerPath(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& managerId,
    std::function<
        void(const boost::system::error_code&, const std::string& managerPath,
             const dbus::utility::MapperServiceMap& serviceMap)>&& callback)
{
    dbus::utility::getSubTree(
        "/xyz/openbmc_project/inventory", 0, bmcInterfaces,
        [asyncResp, managerId, callback = std::move(callback)](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreeResponse& subtree) {
            afterGetValidManagerPath(asyncResp, managerId, callback, ec,
                                     subtree);
        });
}

} // namespace manager_utils

} // namespace redfish
