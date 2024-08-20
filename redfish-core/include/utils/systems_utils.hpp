#pragma once

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "human_sort.hpp"
#include "str_utility.hpp"

#include <boost/url/format.hpp>

#include <string_view>

namespace redfish
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
        BMCWEB_LOG_DEBUG("DBUS response error {}", ec.value());
        messages::internalError(asyncResp->res);
        return;
    }

    nlohmann::json& membersArray = asyncResp->res.jsonValue["Members"];
    membersArray = nlohmann::json::array();

    if (objects.size() == 1)
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
        pathNames.push_back(leaf);
    }
    std::ranges::sort(pathNames, AlphanumLess<std::string>());

    for (const std::string& l : pathNames)
    {
        boost::urls::url url("/redfish/v1/Systems");
        crow::utility::appendUrlPieces(url, l);
        nlohmann::json::object_t member;
        member["@odata.id"] = std::move(url);
        membersArray.emplace_back(std::move(member));
    }
    asyncResp->res.jsonValue["Members@odata.count"] = membersArray.size();
}

/**
 * @brief Populate the system collection members from a GetSubTreePaths search
 * of inventory
 *
 * @param[i,o] asyncResp  Async response object
 * @param[i]   collectionPath  Redfish collection path which is used for the
 *             Members Redfish Path
 * @param[i]   interfaces  List of interfaces to constrain the GetSubTree search
 * @param[in]  subtree     D-Bus base path to constrain search to.
 * @param[in]  jsonKeyName Key name in which the collection members will be
 *             stored.
 *
 * @return void
 */
inline void getSystemCollectionMembers(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    constexpr std::array<std::string_view, 1> interfaces{
        "xyz.openbmc_project.Inventory.Decorator.ManagedHost"};

    BMCWEB_LOG_DEBUG("Get system collection members for /redfish/v1/Systems");

    dbus::utility::getSubTreePaths(
        "/xyz/openbmc_project/inventory", 0, interfaces,
        std::bind_front(handleSystemCollectionMembers, asyncResp));
}

inline void getValidServiceName(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& interface,
    const size_t computerSystemIndex,
    std::function<void(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& systemName,
                       const sdbusplus::message::object_path& path,
                       const std::string& service)>& callback,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreePathsResponse& paths)
{
    // match the found object paths against the index returned by
    // getManagedHostProperty. Then getObject call on found dbus object path and
    // the requested interface to find correct service name and finally call
    // callback with found object path and service

    BMCWEB_LOG_DEBUG("in getValidServiceName..");
    sdbusplus::message::object_path path;

    if (ec)
    {
        BMCWEB_LOG_ERROR("DBUS response error {}", ec);
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    BMCWEB_LOG_DEBUG("Got index: {}; Got paths:\n[", computerSystemIndex);
    for (const auto& p : paths)
    {
        BMCWEB_LOG_DEBUG("  {}", p);
    }
    BMCWEB_LOG_DEBUG("]; determining dbus path..");

    if (paths.size() == 1)
    {
        // single host should only return a single path containing "host0" when
        // we run host specific queries
        path = paths.front();
    }
    else
    {
        // multi host will return multiple paths, find the correct one
        // naive approach, assuming that the chassis is fully slotted
        path = paths[computerSystemIndex - 1];
    }

    BMCWEB_LOG_DEBUG("found path: {} .. calling getDbusObject",
                     std::string(path));
    std::array<std::string_view, 1> interfaces{interface};
    dbus::utility::getDbusObject(
        path, interfaces,
        [asyncResp, systemName, callback = std::move(callback),
         path](const boost::system::error_code& ec2,
               const dbus::utility::MapperGetObject& object) {
            if (ec2 || object.empty())
            {
                BMCWEB_LOG_ERROR("DBUS response error on getDbusObject {}",
                                 ec2.value());
                messages::internalError(asyncResp->res);
                return;
            }
            callback(asyncResp, systemName, path, object.begin()->first);
        });
}

inline void getValidObjectPaths(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& interface,
    std::function<void(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& systemName,
                       const sdbusplus::message::object_path& path,
                       const std::string& service)>& callback,
    const boost::system::error_code& ec, const size_t computerSystemIndex)

{
    // get all object paths that implement the requested interface

    BMCWEB_LOG_DEBUG("in getValidObjectPaths.. got index {}",
                     computerSystemIndex);
    // getSubTreePaths search on interface to get all possible paths
    // afterwards match against the found index
    if (ec)
    {
        BMCWEB_LOG_ERROR("DBUS response error {}", ec);
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    BMCWEB_LOG_DEBUG(
        "calling getSubTreePaths for interface: {} using getValidServiceName callback",
        interface);
    std::array<std::string_view, 1> interfaces{interface};
    dbus::utility::getSubTreePaths(
        "/", 0, interfaces,
        std::bind_front(getValidServiceName, asyncResp, systemName, interface,
                        computerSystemIndex, std::move(callback)));
}

inline void getManagedHostProperty(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& interface,
    const boost::system::error_code& ec, const std::string& systemId,
    const std::string& serviceName,
    std::function<void(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& systemName,
                       const sdbusplus::message::object_path& path,
                       const std::string& service)>&& callback)
{
    // get HostIndex property associated with found system path

    BMCWEB_LOG_DEBUG("in getManagedHostProperty..");
    if (ec)
    {
        if (ec.value() == boost::system::errc::io_error)
        {
            BMCWEB_LOG_DEBUG("EIO - System not found");
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }

        BMCWEB_LOG_ERROR("DBus method call failed with error {}", ec.value());
        messages::internalError(asyncResp->res);
        return;
    }
    if (systemId.empty() || serviceName.empty())
    {
        BMCWEB_LOG_WARNING("System not found");
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    BMCWEB_LOG_DEBUG(
        "calling getProperty for HostIndex and binding getValidObjectPaths");
    sdbusplus::asio::getProperty<uint64_t>(
        *crow::connections::systemBus, serviceName, systemId,
        "xyz.openbmc_project.Inventory.Decorator.ManagedHost", "HostIndex",
        std::bind_front(getValidObjectPaths, asyncResp, systemName, interface,
                        std::move(callback)));
}

inline void afterGetValidSystemPaths(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& interface,
    std::function<void(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& systemName,
                       const sdbusplus::message::object_path& path,
                       const std::string& service)>& callback,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    // match systemName against found system paths

    BMCWEB_LOG_DEBUG("in afterGetValidSystemPaths");
    std::string systemId;
    std::string serviceName;
    if (ec)
    {
        getManagedHostProperty(asyncResp, systemName, interface, ec, systemId,
                               serviceName, std::move(callback));
        return;
    }

    for (const auto& [path, serviceMap] : subtree)
    {
        systemId = sdbusplus::message::object_path(path).filename();

        if (systemId == systemName)
        {
            serviceName = serviceMap.begin()->first;
            systemId = path;
            break;
        }
    }
    BMCWEB_LOG_DEBUG(
        "found systemId: {}, serviceName: {} .. calling getManagedHostProperty",
        systemId, serviceName);

    getManagedHostProperty(asyncResp, systemName, interface, ec, systemId,
                           serviceName, std::move(callback));
}

inline void getValidSystemPaths(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& interface,
    std::function<void(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& systemName,
                       const sdbusplus::message::object_path& path,
                       const std::string& service)>&& callback)
{
    // getSubTree call on ManagedHost dbus interface to retrieve all
    // available system paths
    BMCWEB_LOG_DEBUG(
        "in getValidSystemPaths: running getSubTree and binding afterGetValidSystemPaths");
    constexpr std::array<std::string_view, 1> interfaces{
        "xyz.openbmc_project.Inventory.Decorator.ManagedHost"};
    dbus::utility::getSubTree(
        "/xyz/openbmc_project/inventory", 0, interfaces,
        std::bind_front(afterGetValidSystemPaths, asyncResp, systemName,
                        interface, std::move(callback)));
}

/**
 * @brief Wrapper - Retrieve object path and associated service that implement
 *                  passed in interface
 *
 * @param[in] asyncResp   Shared pointer for completing asynchronous calls
 * @param[in] systemName  Name of the requested system
 * @param[in] interface   D-Bus interface, to base search for path and service
 * on
 * @param[in] callback    Function to call with the found path and service
 *
 * @return None.
 */
inline void getComputerSystemDBusResources(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& interface,
    std::function<void(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& systemName,
                       const sdbusplus::message::object_path& path,
                       const std::string& service)>&& callback)
{
    BMCWEB_LOG_DEBUG("calling getValidSystemPaths");
    getValidSystemPaths(asyncResp, systemName, interface, std::move(callback));
}
} // namespace redfish
