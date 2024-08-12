#pragma once

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "human_sort.hpp"

#include <boost/url/format.hpp>

#include <string_view>

namespace redfish
{

inline void doHypervisorCollection(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec, const std::string& /*hostName*/)
{
    if (ec)
    {
        return;
    }
    auto val = asyncResp->res.jsonValue.find("Members@odata.count");
    if (val == asyncResp->res.jsonValue.end())
    {
        BMCWEB_LOG_CRITICAL("Count wasn't found??");
        return;
    }
    int64_t* count = val->get_ptr<int64_t*>();
    if (count == nullptr)
    {
        BMCWEB_LOG_CRITICAL("Count wasn't found??");
        return;
    }
    *count = *count + 1;
    BMCWEB_LOG_DEBUG("Hypervisor is available");
    nlohmann::json& membersArray = asyncResp->res.jsonValue["Members"];
    nlohmann::json::object_t hypervisor;
    hypervisor["@odata.id"] = "/redfish/v1/Systems/hypervisor";
    membersArray.emplace_back(std::move(hypervisor));
}

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

        sdbusplus::asio::getProperty<std::string>(
            *crow::connections::systemBus, "xyz.openbmc_project.Settings",
            "/xyz/openbmc_project/network/hypervisor",
            "xyz.openbmc_project.Network.SystemConfiguration", "HostName",
            std::bind_front(doHypervisorCollection, asyncResp));

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
} // namespace redfish
