#pragma once

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "http/utility.hpp"
#include "human_sort.hpp"

#include <boost/url/url.hpp>
#include <nlohmann/json.hpp>

#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace redfish
{
namespace collection_util
{

inline void handleCollectionMembers(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::urls::url& collectionPath, const boost::system::error_code& ec,
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

    std::vector<std::string> pathNames;

    // add case for SystemsCollection since we want to store Systems members
    // as "system0" to "systemN" instead of "chassis0" to "chassisN" or "host0"
    // to "hostN" depending on which dbus interface we are using (currently
    // using xyz.openbmc_project.State.Host, meaning "host0" to "hostN")
    if (collectionPath == boost::urls::url("/redfish/v1/Systems"))
    {
        for (const auto& object : objects)
        {
            sdbusplus::message::object_path path(object);
            /*BMCLOG*/ BMCWEB_LOG_DEBUG(
                "handleComputerCollectionMembers path {}", std::string(path));
            std::string leaf = path.filename();
            if (leaf.empty())
            {
                continue;
            }

            if (leaf.find(/*"chassis"*/ "host") != std::string::npos)
            {
                leaf.erase(remove(leaf.begin(), leaf.end(), '\"'), leaf.end());
                std::string computerSystemIndex = leaf.substr(
                    leaf.find(/*"chassis"*/ "host") + (leaf.length() - 1));

                if (!computerSystemIndex.empty())
                {
                    sdbusplus::message::object_path systemPath(
                        "/redfish/v1/Systems/system" + computerSystemIndex);
                    pathNames.push_back(systemPath.filename());
                }
            }
        }
    }
    else
    {
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
    }

    std::ranges::sort(pathNames, AlphanumLess<std::string>());

    nlohmann::json& members = asyncResp->res.jsonValue["Members"];
    members = nlohmann::json::array();
    for (const std::string& leaf : pathNames)
    {
        boost::urls::url url = collectionPath;
        crow::utility::appendUrlPieces(url, leaf);
        nlohmann::json::object_t member;
        member["@odata.id"] = std::move(url);
        members.emplace_back(std::move(member));
    }
    asyncResp->res.jsonValue["Members@odata.count"] = members.size();
}

/**
 * @brief Populate the collection "Members" from a GetSubTreePaths search of
 *        inventory
 *
 * @param[i,o] asyncResp  Async response object
 * @param[i]   collectionPath  Redfish collection path which is used for the
 *             Members Redfish Path
 * @param[i]   interfaces  List of interfaces to constrain the GetSubTree search
 * @param[in]  subtree     D-Bus base path to constrain search to.
 *
 * @return void
 */
inline void
    getCollectionMembers(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const boost::urls::url& collectionPath,
                         std::span<const std::string_view> interfaces,
                         const std::string& subtree)
{
    BMCWEB_LOG_DEBUG("Get collection members for: {}", collectionPath.buffer());
    dbus::utility::getSubTreePaths(
        subtree, 0, interfaces,
        std::bind_front(handleCollectionMembers, asyncResp, collectionPath));
}

} // namespace collection_util
} // namespace redfish
