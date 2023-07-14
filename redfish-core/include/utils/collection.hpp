#pragma once

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "http/utility.hpp"
#include "human_sort.hpp"

#include <boost/url/url.hpp>
#include <nlohmann/json.hpp>

#include <span>
#include <string>
#include <string_view>
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
    for (const auto& object : objects)
    {
        std::string leaf = sdbusplus::message::object_path(object).filename();
        if (leaf.empty())
        {
            continue;
        }
        pathNames.emplace_back(leaf);
    }
    std::sort(pathNames.begin(), pathNames.end(), AlphanumLess<std::string>());

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
    getCollectionMembers(std::shared_ptr<bmcweb::AsyncResp> asyncResp,
                         const boost::urls::url& collectionPath,
                         std::span<const std::string_view> interfaces,
                         const char* subtree = "/xyz/openbmc_project/inventory")
{
    BMCWEB_LOG_DEBUG("Get collection members for: {}", collectionPath.buffer());
    dbus::utility::getSubTreePaths(
        subtree, 0, interfaces,
        [collectionPath, asyncResp{std::move(asyncResp)}](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreePathsResponse& objects) {
        handleCollectionMembers(asyncResp, collectionPath, ec, objects);
        });
}

/**
 * @brief Populate the collection "Members" from the Associations endpoint
 *        path
 *
 * @param[in, out] asyncResp  Async response object
 * @param[in]      collectionPath  Redfish collection path which is used for the
 *                 Members Redfish Path
 * @param[in]      interfaces  List of interfaces to constrain the associated
 *                 members
 * @param[in]      associationEndPointPath  Path that points to the association
 *                 end point
 * @param[in]      path  Base path to search for the subtree
 *
 * @return void
 */
inline void
    getAssociatedCollectionMembers(std::shared_ptr<bmcweb::AsyncResp> asyncResp,
                                   const boost::urls::url& collectionPath,
                                   std::span<const std::string_view> interfaces,
                                   const std::string& associationEndPointPath,
                                   const sdbusplus::message::object_path& path)
{
    BMCWEB_LOG_DEBUG("Get associated collection members for: {}",
                     collectionPath);

    dbus::utility::getAssociatedSubTreePaths(
        associationEndPointPath, path, 0, interfaces,
        [collectionPath, asyncResp{std::move(asyncResp)}](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreePathsResponse& objects) {
        handleCollectionMembers(asyncResp, collectionPath, ec, objects);
        });
}

} // namespace collection_util
} // namespace redfish
