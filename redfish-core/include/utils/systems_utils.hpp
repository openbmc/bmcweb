#pragma once

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "human_sort.hpp"

#include <boost/url/format.hpp>

#include <string_view>

namespace redfish
{

namespace systems_utils
{

inline void handleCollectionMembers(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::urls::url& collectionPath,
    const nlohmann::json::json_pointer& jsonKeyName,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreePathsResponse& objects)
{
    if (jsonKeyName.empty())
    {
        messages::internalError(asyncResp->res);
        BMCWEB_LOG_ERROR("Json Key called empty.  Did you mean /Members?");
        return;
    }
    nlohmann::json::json_pointer jsonCountKeyName = jsonKeyName;
    std::string back = jsonCountKeyName.back();
    jsonCountKeyName.pop_back();
    jsonCountKeyName /= back + "@odata.count";

    if (ec == boost::system::errc::io_error)
    {
        asyncResp->res.jsonValue[jsonKeyName] = nlohmann::json::array();
        asyncResp->res.jsonValue[jsonCountKeyName] = 0;
        return;
    }

    if (ec)
    {
        BMCWEB_LOG_DEBUG("DBUS response error {}", ec.value());
        messages::internalError(asyncResp->res);
        return;
    }

    std::vector<std::string> pathNames;
    nlohmann::json& members = asyncResp->res.jsonValue[jsonKeyName];
    members = nlohmann::json::array();

    if (objects.size() == 1)
    {
        nlohmann::json::object_t system;
        system["@odata.id"] = boost::urls::format(
            "/redfish/v1/Systems/{}", BMCWEB_REDFISH_SYSTEM_URI_NAME);
        members.emplace_back(std::move(system));
        asyncResp->res.jsonValue[jsonCountKeyName] = 1;
        return;
    }

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
        boost::urls::url url = collectionPath;
        crow::utility::appendUrlPieces(url, l);
        nlohmann::json::object_t member;
        member["@odata.id"] = std::move(url);
        members.emplace_back(std::move(member));
    }
    asyncResp->res.jsonValue[jsonCountKeyName] = members.size();
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
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::urls::url& collectionPath,
    std::span<const std::string_view> interfaces, const std::string& subtree)
{
    BMCWEB_LOG_DEBUG("Get system collection members for: {}",
                     collectionPath.buffer());
    dbus::utility::getSubTreePaths(
        subtree, 0, interfaces,
        std::bind_front(handleCollectionMembers, asyncResp, collectionPath,
                        nlohmann::json::json_pointer("/Members")));
}
} // namespace systems_utils
} // namespace redfish
