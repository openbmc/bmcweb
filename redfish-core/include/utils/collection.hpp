// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "http/utility.hpp"
#include "json_utils.hpp"
#include "logging.hpp"

#include <boost/url/url.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/message/native_types.hpp>

#include <algorithm>
#include <functional>
#include <memory>
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
namespace details
{
inline nlohmann::json::array_t& getJsonArrayAt(nlohmann::json& v)
{
    auto* arr = v.get_ptr<nlohmann::json::array_t*>();
    if (arr == nullptr)
    {
        v = nlohmann::json::array();
        arr = v.get_ptr<nlohmann::json::array_t*>();
    }
    return *arr;
}

} // namespace details

inline nlohmann::json::array_t& getJsonArray(nlohmann::json& parent,
                                             std::string_view key)
{
    return details::getJsonArrayAt(parent[key]);
}

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
        nlohmann::json::array_t& membersArr =
            details::getJsonArrayAt(asyncResp->res.jsonValue[jsonKeyName]);
        asyncResp->res.jsonValue[jsonCountKeyName] = membersArr.size();
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
        sdbusplus::object_path path(object);
        std::string leaf = path.filename();
        if (leaf.empty())
        {
            continue;
        }
        pathNames.push_back(leaf);
    }

    nlohmann::json::array_t& membersArr =
        details::getJsonArrayAt(asyncResp->res.jsonValue[jsonKeyName]);
    for (const std::string& leaf : pathNames)
    {
        boost::urls::url url = collectionPath;
        crow::utility::appendUrlPieces(url, leaf);
        nlohmann::json::object_t member;
        member["@odata.id"] = std::move(url);
        membersArr.emplace_back(std::move(member));
    }
    json_util::sortJsonArrayByOData(membersArr);
    asyncResp->res.jsonValue[jsonCountKeyName] = membersArr.size();
}

/**
 * @brief Populate the collection members from a GetSubTreePaths search of
 *        inventory
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
inline void getCollectionToKey(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::urls::url& collectionPath,
    std::span<const std::string_view> interfaces, const std::string& subtree,
    const nlohmann::json::json_pointer& jsonKeyName)
{
    BMCWEB_LOG_DEBUG("Get collection members for: {}", collectionPath.buffer());
    dbus::utility::getSubTreePaths(
        subtree, 0, interfaces,
        std::bind_front(handleCollectionMembers, asyncResp, collectionPath,
                        jsonKeyName));
}
inline void getCollectionMembers(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::urls::url& collectionPath,
    std::span<const std::string_view> interfaces, const std::string& subtree)
{
    getCollectionToKey(asyncResp, collectionPath, interfaces, subtree,
                       nlohmann::json::json_pointer("/Members"));
}

} // namespace collection_util
} // namespace redfish
