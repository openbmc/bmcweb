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
        // ManagedHost interface is implemented on both, Board and Chassis
        // configuration for yv4. Filter out the Chassis ones.
        if (leaf.empty() || (leaf.find("Chassis") != std::string::npos))
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

inline void getManagedHostProperty(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const boost::system::error_code& ec,
    const std::string& systemId, const std::string& serviceName,
    std::function<void(const uint64_t computerSystemIndex)> callback)
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

    sdbusplus::asio::getProperty<uint64_t>(
        *crow::connections::systemBus, serviceName, systemId,
        "xyz.openbmc_project.Inventory.Decorator.ManagedHost", "HostIndex",
        [asyncResp, systemName, serviceName, callback = std::move(callback)](
            const boost::system::error_code& ec2, const uint64_t hostIndex) {
            if (ec2)
            {
                BMCWEB_LOG_ERROR("DBUS response error {}", ec2);
                messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                           systemName);
                return;
            }

            callback(hostIndex);
        });
}

inline void getComputerSystemIndex(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName,
    std::function<void(const uint64_t computerSystemIndex)>&& callback)
{
    constexpr std::array<std::string_view, 1> interfaces{
        "xyz.openbmc_project.Inventory.Decorator.ManagedHost"};
    dbus::utility::getSubTree(
        "/xyz/openbmc_project/inventory", 0, interfaces,
        [asyncResp, systemName, callback = std::move(callback)](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreeResponse& subtree) {
            BMCWEB_LOG_DEBUG("in afterGetValidSystemPaths");
            std::string systemId;
            std::string serviceName;
            if (ec)
            {
                getManagedHostProperty(asyncResp, systemName, ec, systemId,
                                       serviceName, callback);
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

            getManagedHostProperty(asyncResp, systemName, ec, systemId,
                                   serviceName, callback);
        });
}
} // namespace redfish
