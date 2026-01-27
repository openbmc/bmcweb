// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "bmcweb_config.h"

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "human_sort.hpp"
#include "logging.hpp"

#include <boost/url/format.hpp>
#include <sdbusplus/message/native_types.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <format>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace redfish
{

namespace systems_utils
{

inline void handleSystemCollectionMembers(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreePathsResponse& objects)
{
    if (ec == boost::system::errc::io_error)
    {
        asyncResp->res.jsonValue["Members"] = nlohmann::json::array();
        asyncResp->res.jsonValue["Members@odata.count"] = 0;
        return;
    }

    if (ec)
    {
        BMCWEB_LOG_ERROR("DBUS response error {}", ec.value());
        messages::internalError(asyncResp->res);
        return;
    }

    nlohmann::json& membersArray = asyncResp->res.jsonValue["Members"];
    membersArray = nlohmann::json::array();

    // consider an empty result as single-host, since single-host systems
    // do not populate the ManagedHost dbus interface
    if (objects.empty())
    {
        asyncResp->res.jsonValue["Members@odata.count"] = 1;
        nlohmann::json::object_t system;
        system["@odata.id"] = boost::urls::format(
            "/redfish/v1/Systems/{}", BMCWEB_REDFISH_SYSTEM_URI_NAME);
        membersArray.emplace_back(std::move(system));

        if constexpr (BMCWEB_HYPERVISOR_COMPUTER_SYSTEM)
        {
            BMCWEB_LOG_DEBUG("Hypervisor is available");
            asyncResp->res.jsonValue["Members@odata.count"] = 2;

            nlohmann::json::object_t hypervisor;
            hypervisor["@odata.id"] = "/redfish/v1/Systems/hypervisor";
            membersArray.emplace_back(std::move(hypervisor));
        }

        return;
    }

    std::vector<std::string> pathNames;
    for (const auto& object : objects)
    {
        sdbusplus::message::object_path path(object);
        std::string leaf = path.filename();
        if (leaf.empty())
        {
            continue;
        }
        pathNames.emplace_back(leaf);
    }
    std::ranges::sort(pathNames, AlphanumLess<std::string>());

    for (const std::string& systemName : pathNames)
    {
        nlohmann::json::object_t member;
        member["@odata.id"] =
            boost::urls::format("/redfish/v1/Systems/{}", systemName);
        membersArray.emplace_back(std::move(member));
    }
    asyncResp->res.jsonValue["Members@odata.count"] = membersArray.size();
}

/**
 * @brief Populate the system collection members from a GetSubTreePaths search
 * of the inventory based of the ManagedHost dbus interface
 *
 * @param[i] asyncResp  Async response object
 *
 * @return None
 */
inline void getSystemCollectionMembers(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    constexpr std::array<std::string_view, 1> interfaces{
        "xyz.openbmc_project.Inventory.Decorator.ManagedHost",
    };

    BMCWEB_LOG_DEBUG("Get system collection members for /redfish/v1/Systems");

    dbus::utility::getSubTreePaths(
        "/xyz/openbmc_project/inventory", 0, interfaces,
        std::bind_front(handleSystemCollectionMembers, asyncResp));
}

inline void getManagedHostProperty(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& systemPath,
    std::function<void(const uint64_t computerSystemIndex)> callback)
{
    dbus::utility::getProperty<uint64_t>(
        "xyz.openbmc_project.EntityManager", systemPath,
        "xyz.openbmc_project.Inventory.Decorator.ManagedHost", "HostIndex",
        [asyncResp, systemName, systemPath, callback = std::move(callback)](
            const boost::system::error_code& ec, const uint64_t hostIndex) {
            if (ec)
            {
                BMCWEB_LOG_WARNING("DBUS response error {}", ec);
                messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                           systemName);
                return;
            }
            BMCWEB_LOG_DEBUG("Got index {} for path {}", hostIndex, systemPath);
            callback(hostIndex);
        });
}

inline void afterGetComputerSystemSubTreePaths(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName,
    std::function<void(const uint64_t computerSystemIndex)>& callback,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreePathsResponse& objects)
{
    sdbusplus::message::object_path systemPath;
    if (ec)
    {
        if (ec.value() == boost::system::errc::io_error)
        {
            BMCWEB_LOG_WARNING("EIO - System not found");
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }

        BMCWEB_LOG_ERROR("DBus method call failed with error {}", ec.value());
        messages::internalError(asyncResp->res);
        return;
    }

    const auto& found = std::ranges::find_if(
        objects, [systemName](const sdbusplus::message::object_path& path) {
            return path.filename() == systemName;
        });

    if (found == objects.end())
    {
        BMCWEB_LOG_WARNING("Failed to match systemName: {}", systemName);
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    systemPath = *found;

    getManagedHostProperty(asyncResp, systemName, systemPath,
                           std::move(callback));
}

/**
 * @brief Retrieve the index associated with the requested system based on the
 * ManagedHost interface
 *
 * @param[i] asyncResp  Async response object
 * @param[i] systemName The requested system
 * @param[i] callback   Callback to call once the index has been found
 *
 * @return None
 */
inline void getComputerSystemIndex(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName,
    std::function<void(const uint64_t computerSystemIndex)>&& callback)
{
    if constexpr (BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
        constexpr std::array<std::string_view, 1> interfaces{
            "xyz.openbmc_project.Inventory.Decorator.ManagedHost"};
        dbus::utility::getSubTreePaths(
            "/xyz/openbmc_project/inventory", 0, interfaces,
            std::bind_front(afterGetComputerSystemSubTreePaths, asyncResp,
                            systemName, std::move(callback)));
    }
    else
    {
        // on single-host, fallback to index 0
        BMCWEB_LOG_DEBUG(
            "Single-host detected, fallback to computerSystemIndex 0");
        callback(0);
    }
}

inline sdbusplus::message::object_path getHostStateObjectPath(
    const uint64_t computerSystemIndex)
{
    sdbusplus::message::object_path hostPath("/xyz/openbmc_project/state");
    hostPath /= std::format("host{}", computerSystemIndex);
    return hostPath;
}

inline std::string getHostStateServiceName(const uint64_t computerSystemIndex)
{
    if constexpr (BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
        return std::format("xyz.openbmc_project.State.Host{}",
                           computerSystemIndex);
    }

    return "xyz.openbmc_project.State.Host";
}

inline sdbusplus::message::object_path getChassisStateObjectPath(
    const uint64_t computerSystemIndex)
{
    sdbusplus::message::object_path chassisPath("/xyz/openbmc_project/state");
    chassisPath /= std::format("chassis{}", computerSystemIndex);
    return chassisPath;
}

inline std::string getChassisStateServiceName(
    const uint64_t computerSystemIndex)
{
    if constexpr (BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
        return std::format("xyz.openbmc_project.State.Chassis{}",
                           computerSystemIndex);
    }

    return "xyz.openbmc_project.State.Chassis";
}

inline sdbusplus::message::object_path getControlObjectPath(
    const uint64_t computerSystemIndex)
{
    sdbusplus::message::object_path controlPath("/xyz/openbmc_project/control");
    controlPath /= std::format("host{}", computerSystemIndex);
    return controlPath;
}

inline void afterGetValidSystemsPath(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemId,
    const std::function<void(const std::optional<std::string>&)>& callback,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreePathsResponse& systemsPaths)
{
    if (ec)
    {
        if (ec == boost::system::errc::io_error)
        {
            BMCWEB_LOG_DEBUG("No systems found");
            callback(std::nullopt);
            return;
        }
        BMCWEB_LOG_ERROR("DBUS response error: {}", ec.value());
        messages::internalError(asyncResp->res);
        return;
    }

    for (const std::string& system : systemsPaths)
    {
        sdbusplus::message::object_path path(system);
        if (path.filename() == systemId)
        {
            callback(path);
            return;
        }
    }
    BMCWEB_LOG_DEBUG("No system named {} found", systemId);
    callback(std::nullopt);
}

inline void getValidSystemsPath(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemId,
    std::function<void(const std::optional<std::string>&)>&& callback)
{
    BMCWEB_LOG_DEBUG("Get path for {}", systemId);

    constexpr std::array<std::string_view, 2> interfaces = {
        "xyz.openbmc_project.Inventory.Decorator.ManagedHost",
        "xyz.openbmc_project.Inventory.Item.System"};
    dbus::utility::getSubTreePaths(
        "/xyz/openbmc_project/inventory", 0, interfaces,
        [asyncResp, systemId, callback{std::move(callback)}](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreePathsResponse& systemsPaths) {
            afterGetValidSystemsPath(asyncResp, systemId, callback, ec,
                                     systemsPaths);
        });
}

/**
 * @brief Match computerSystemIndex with index contained by an object path
 *        i.e 1 in /xyz/openbmc/project/control/host1/policy/TPMEnable
 *
 * @param[i] asyncResp           Shared pointer for generating response
 * @param[i] computerSystemIndex The index to match against
 * @param[i] subtree             Mapper response object
 * @param[o] objectPath          Buffer for matched object path
 * @param[o] service             Buffer for service of matched object
 *                               path
 *
 * @return true if match found, else false
 */
inline bool indexMatchingSubTreeMapObjectPath(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const uint64_t computerSystemIndex,
    const dbus::utility::MapperGetSubTreeResponse& subtree,
    std::string& objectPath, std::string& service)
{
    const std::string host = std::format("host{}", computerSystemIndex);

    for (const auto& obj : subtree)
    {
        std::string tmp = host;
        const sdbusplus::message::object_path path{obj.first};
        const std::string serv = obj.second.begin()->first;

        if (path.str.empty() || obj.second.size() != 1)
        {
            BMCWEB_LOG_DEBUG("Error finding index in object path");
            messages::internalError(asyncResp->res);
            return false;
        }

        objectPath = path;
        service = serv;

        if (path.filename() == host)
        {
            return true;
        }

        tmp.insert(0, 1, '/');
        tmp.append("/");
        if (path.str.find(host) != std::string::npos)
        {
            return true;
        }
    }

    return false;
}

inline bool checkSingleHostSystemNotFound(
    const std::string& systemName,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if constexpr (!BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
        if (systemName != BMCWEB_REDFISH_SYSTEM_URI_NAME)
        {
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return true;
        }
    }
    return false;
}
} // namespace systems_utils
} // namespace redfish
