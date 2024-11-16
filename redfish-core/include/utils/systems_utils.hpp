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
    const std::string& systemName, const std::string& systemId,
    const std::string& serviceName,
    std::function<void(const uint64_t computerSystemIndex)> callback)
{
    // consider empty result as single-host and fallback to index 0
    if (systemId.empty())
    {
        BMCWEB_LOG_DEBUG(
            "ManagedHost interface not found, fallback to HostIndex 0");
        callback(0);
        return;
    }

    sdbusplus::asio::getProperty<uint64_t>(
        *crow::connections::systemBus, serviceName, systemId,
        "xyz.openbmc_project.Inventory.Decorator.ManagedHost", "HostIndex",
        [asyncResp, systemName, systemId, serviceName,
         callback = std::move(callback)](const boost::system::error_code& ec,
                                         const uint64_t hostIndex) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("DBUS response error {}", ec);
                messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                           systemName);
                return;
            }
            BMCWEB_LOG_DEBUG("Got index {} for path {}", hostIndex, systemId);
            callback(hostIndex);
        });
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
    constexpr std::array<std::string_view, 1> interfaces{
        "xyz.openbmc_project.Inventory.Decorator.ManagedHost"};
    dbus::utility::getSubTree(
        "/xyz/openbmc_project/inventory", 0, interfaces,
        [asyncResp, systemName, callback = std::move(callback)](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreeResponse& subtree) {
            std::string systemId;
            std::string serviceName;
            if (ec)
            {
                if (ec.value() == boost::system::errc::io_error)
                {
                    BMCWEB_LOG_DEBUG("EIO - System not found");
                    messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                               systemName);
                    return;
                }

                BMCWEB_LOG_ERROR("DBus method call failed with error {}",
                                 ec.value());
                messages::internalError(asyncResp->res);
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

            getManagedHostProperty(asyncResp, systemName, systemId, serviceName,
                                   callback);
        });
}
} // namespace redfish
