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
#include <vector>

namespace redfish
{
namespace collection_util
{

/**
 * @brief Populate the collection members from a GetSubTreePaths search of
 *        inventory
 *
 * @param[i,o] asyncResp  Async response object
 * @param[i]   collectionPath  Redfish collection path which is used for the
 *             Members Redfish Path
 * @param[i]   interfaces  List of interfaces to constrain the GetSubTree search
 * @param[in]  subtree     D-Bus base path to constrain search to.
 * @param[in]  arrayName   Array name in which the collection members will be
 *             stored.
 *
 * @return void
 */
inline void getCollectionMembersArray(
    std::shared_ptr<bmcweb::AsyncResp> asyncResp,
    const boost::urls::url& collectionPath,
    std::span<const std::string_view> interfaces, const std::string& arrayName,
    const char* subtree = "/xyz/openbmc_project/inventory")
{
    BMCWEB_LOG_DEBUG("Get collection members for: {}", collectionPath.buffer());
    dbus::utility::getSubTreePaths(
        subtree, 0, interfaces,
        [collectionPath, asyncResp{std::move(asyncResp)}, arrayName](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreePathsResponse& objects) {
        if (ec == boost::system::errc::io_error)
        {
            asyncResp->res.jsonValue[arrayName] = nlohmann::json::array();
            asyncResp->res.jsonValue[arrayName + "@odata.count"] = 0;
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
            sdbusplus::message::object_path path(object);
            std::string leaf = path.filename();
            if (leaf.empty())
            {
                continue;
            }
            pathNames.push_back(leaf);
        }
        std::ranges::sort(pathNames, AlphanumLess<std::string>());

        nlohmann::json& members = asyncResp->res.jsonValue[arrayName];
        members = nlohmann::json::array();
        for (const std::string& leaf : pathNames)
        {
            boost::urls::url url = collectionPath;
            crow::utility::appendUrlPieces(url, leaf);
            nlohmann::json::object_t member;
            member["@odata.id"] = std::move(url);
            members.emplace_back(std::move(member));
        }
        asyncResp->res.jsonValue[arrayName + "@odata.count"] = members.size();
        });
}

inline void
    getCollectionMembers(std::shared_ptr<bmcweb::AsyncResp> asyncResp,
                         const boost::urls::url& collectionPath,
                         std::span<const std::string_view> interfaces,
                         const char* subtree = "/xyz/openbmc_project/inventory")
{
    getCollectionMembersArray(std::move(asyncResp), collectionPath, interfaces,
                              "Members", subtree);
}

} // namespace collection_util
} // namespace redfish
