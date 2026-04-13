// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "logging.hpp"

#include <boost/system/error_code.hpp>
#include <sdbusplus/message/native_types.hpp>

#include <array>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>

namespace redfish
{

inline void afterGetAssociatedSubResourceById(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    std::string_view resourceType, const std::string& resourceId,
    const std::function<void(const std::string& path,
                             const std::string& serviceName)>& callback,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& object)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("DBus response error on GetAssociatedSubTree {}", ec);
        messages::internalError(asyncResp->res);
        return;
    }

    for (const auto& [path, services] : object)
    {
        if (sdbusplus::object_path(path).filename() != resourceId)
        {
            continue;
        }
        if (services.size() != 1)
        {
            BMCWEB_LOG_ERROR("Expected exactly one service for {}, found {}",
                             path, services.size());
            messages::internalError(asyncResp->res);
            return;
        }
        callback(path, services.begin()->first);
        return;
    }

    BMCWEB_LOG_DEBUG("No associated {} with id '{}' found", resourceType,
                     resourceId);
    messages::resourceNotFound(asyncResp->res, resourceType, resourceId);
}

// Resolve a single associated sub-resource of a parent inventory object.
//
// Follows associationName from parentPath, then searches the associated
// inventory objects (matching interfaces) for the one whose object-path leaf
// name equals resourceId and invokes callback(objectPath, serviceName) for it.
// Responds with ResourceNotFound(resourceType, resourceId) when none match.
inline void getAssociatedSubResourceById(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    std::string_view associationName,
    std::span<const std::string_view> interfaces, std::string_view resourceType,
    const std::string& resourceId,
    std::function<void(const std::string& path,
                       const std::string& serviceName)>&& callback,
    const std::string& parentPath)
{
    sdbusplus::object_path associationPath =
        sdbusplus::object_path(parentPath) / associationName;
    dbus::utility::getAssociatedSubTree(
        associationPath,
        sdbusplus::object_path{"/xyz/openbmc_project/inventory"}, 0, interfaces,
        std::bind_front(afterGetAssociatedSubResourceById, asyncResp,
                        resourceType, resourceId, std::move(callback)));
}

// Resolve an associated Connector.Port by id, via the "connecting"
// association. Thin wrapper over getAssociatedSubResourceById for the common
// port case.
inline void getAssociatedPortPath(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& portId,
    std::function<void(const std::string& portPath,
                       const std::string& serviceName)>&& callback,
    const std::string& parentPath)
{
    static constexpr std::array<std::string_view, 1> connectorPortInterface{
        "xyz.openbmc_project.Inventory.Connector.Port"};
    getAssociatedSubResourceById(asyncResp, "connecting",
                                 connectorPortInterface, "Port", portId,
                                 std::move(callback), parentPath);
}

} // namespace redfish
