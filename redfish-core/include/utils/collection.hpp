#pragma once

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "http/utility.hpp"
#include "human_sort.hpp"

#include <boost/url/url.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/asio/property.hpp>

#include <algorithm>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace redfish
{
namespace collection_util
{

/**
 * @brief Populate the collection "Members" from a GetSubTreePaths search of
 *        inventory
 *
 * @param[i,o] aResp  Async response object
 * @param[i]   collectionPath  Redfish collection path which is used for the
 *             Members Redfish Path
 * @param[i]   interfaces  List of interfaces to constrain the GetSubTree search
 * @param[in]  subtree     D-Bus base path to constrain search to.
 *
 * @return void
 */
inline void
    getCollectionMembers(std::shared_ptr<bmcweb::AsyncResp> aResp,
                         const boost::urls::url& collectionPath,
                         std::span<const std::string_view> interfaces,
                         const char* subtree = "/xyz/openbmc_project/inventory")
{
    BMCWEB_LOG_DEBUG << "Get collection members for: "
                     << collectionPath.buffer();
    dbus::utility::getSubTreePaths(
        subtree, 0, interfaces,
        [collectionPath, aResp{std::move(aResp)}](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreePathsResponse& objects) {
        if (ec == boost::system::errc::io_error)
        {
            aResp->res.jsonValue["Members"] = nlohmann::json::array();
            aResp->res.jsonValue["Members@odata.count"] = 0;
            return;
        }

        if (ec)
        {
            BMCWEB_LOG_DEBUG << "DBUS response error " << ec.value();
            messages::internalError(aResp->res);
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
        std::sort(pathNames.begin(), pathNames.end(),
                  AlphanumLess<std::string>());

        nlohmann::json& members = aResp->res.jsonValue["Members"];
        members = nlohmann::json::array();
        for (const std::string& leaf : pathNames)
        {
            boost::urls::url url = collectionPath;
            crow::utility::appendUrlPieces(url, leaf);
            nlohmann::json::object_t member;
            member["@odata.id"] = std::move(url);
            members.push_back(std::move(member));
        }
        aResp->res.jsonValue["Members@odata.count"] = members.size();
        });
}

/**
 * @brief Sort the associated collection "Members" and set the collection
 *        response
 * @param[in, out] aResp  Async response object
 * @param[in]      objects Collection of associated objects
 * @param[in]      collectionPath  Redfish collection path which is used for the
 *                 Members Redfish Path
 * @param[in]      ec Error code
 * @param[in]      subTreePaths Collection of inventory objects constrained by
 *                 interfaces
 * @return void
 */

inline void sortAssociatedCollectionMembersAndSetResponse(
    const std::shared_ptr<bmcweb::AsyncResp>& aResp,
    const dbus::utility::MapperGetSubTreePathsResponse& objects,
    const boost::urls::url& collectionPath, const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreePathsResponse& subTreePaths)
{
    if (ec == boost::system::errc::io_error)
    {
        aResp->res.jsonValue["Members"] = nlohmann::json::array();
        aResp->res.jsonValue["Members@odata.count"] = 0;
        return;
    }

    if (ec)
    {
        BMCWEB_LOG_DEBUG << "DBUS response error " << ec.value();
        messages::internalError(aResp->res);
        return;
    }

    // Sort the vectors of object paths, before getting their subset
    // vector
    std::vector<std::string> sortedObjects(objects.size());
    std::partial_sort_copy(objects.begin(), objects.end(),
                           sortedObjects.begin(), sortedObjects.end(),
                           AlphanumLess<std::string>());

    std::vector<std::string> sortedSubTreePaths(subTreePaths.size());
    std::partial_sort_copy(subTreePaths.begin(), subTreePaths.end(),
                           sortedSubTreePaths.begin(), sortedSubTreePaths.end(),
                           AlphanumLess<std::string>());

    std::vector<std::string> objectPaths;

    // For a given association endpoint path, there could be associated
    // members with different interface types. So filter out the
    // required members.
    std::set_intersection(sortedObjects.begin(), sortedObjects.end(),
                          sortedSubTreePaths.begin(), sortedSubTreePaths.end(),
                          std::back_inserter(objectPaths));

    std::vector<std::string> pathNames;
    for (const auto& objectPath : objectPaths)
    {
        std::string leaf =
            (sdbusplus::message::object_path(objectPath)).filename();
        if (leaf.empty())
        {
            continue;
        }
        pathNames.push_back(leaf);
    }

    nlohmann::json& members = aResp->res.jsonValue["Members"];
    members = nlohmann::json::array();
    for (const std::string& leaf : pathNames)
    {
        boost::urls::url url = collectionPath;
        crow::utility::appendUrlPieces(url, leaf);
        nlohmann::json::object_t member;
        member["@odata.id"] = url;
        members.emplace_back(std::move(member));
    }
    aResp->res.jsonValue["Members@odata.count"] = members.size();
}

/**
 * @brief Populate the collection "Members" from the Associations endpoint
 *        path
 *
 * @param[in, out] aResp  Async response object
 * @param[in]      collectionPath  Redfish collection path which is used for the
 *                 Members Redfish Path
 * @param[in]      interfaces  List of interfaces to constrain the associated
 *                 members
 * @param[in]      associationEndPointPath  Path that points to the association
 *                 end point
 *
 * @return void
 */
inline void getAssociatedCollectionMembers(
    const std::shared_ptr<bmcweb::AsyncResp>& aResp,
    const boost::urls::url& collectionPath,
    std::vector<std::string_view>& interfaces,
    const std::string& associationEndPointPath)
{
    BMCWEB_LOG_DEBUG << "Get collection members for: " << collectionPath;

    sdbusplus::asio::getProperty<std::vector<std::string>>(
        *crow::connections::systemBus, "xyz.openbmc_project.ObjectMapper",
        associationEndPointPath, "xyz.openbmc_project.Association", "endpoints",
        [collectionPath, interfaces,
         aResp](const boost::system::error_code& ec,
                const dbus::utility::MapperGetSubTreePathsResponse& objects) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG << "Error in association path " << ec.value();
            // Return empty collection since association is not setup
            aResp->res.jsonValue["Members"] = nlohmann::json::array();
            aResp->res.jsonValue["Members@odata.count"] = 0;
            return;
        }

        dbus::utility::getSubTreePaths(
            "/xyz/openbmc_project/inventory", 0, interfaces,
            std::bind_front(sortAssociatedCollectionMembersAndSetResponse,
                            aResp, objects, collectionPath));
        });
}

} // namespace collection_util
} // namespace redfish
