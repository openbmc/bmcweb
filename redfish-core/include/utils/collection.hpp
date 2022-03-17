#pragma once

#include <human_sort.hpp>

#include <string>
#include <string_view>
#include <utility>
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
                         const std::vector<const char*>& interfaces,
                         const char* subtree = "/xyz/openbmc_project/inventory")
{
    BMCWEB_LOG_DEBUG << "Get collection members for: "
                     << collectionPath.buffer();
    crow::connections::systemBus->async_method_call(
        [collectionPath, aResp{std::move(aResp)}](
            const boost::system::error_code ec,
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
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths", subtree, 0,
        interfaces);
}
inline std::string
    getComputerSystemIndexString(const std::string_view systemNameStr)
{

#ifndef BMCWEB_ENABLE_MULTI_COMPUTERSYSTEM
    (void)systemNameStr;
    return "0";
#else
    static constexpr std::string_view systemToken = "system";
    static constexpr std::string_view zero = "0";
    auto location = systemNameStr.rfind(systemToken);
    if (location == std::string::npos)
    {
        BMCWEB_LOG_DEBUG << "invalid host number";
        return "";
    }

    auto index = systemNameStr.substr(location + systemToken.length());
    if (index.empty())
    {
        return std::string(zero);
    }
    // verify if the host number string has a valid number.
    // return empty string in case of error
    auto hostNumber = std::stoi(std::string(index));
    if (std::in_range<size_t>(hostNumber))
    {
        return std::string(index);
    }
    BMCWEB_LOG_DEBUG << "invalid host number";
    return "";
#endif
}
} // namespace collection_util
} // namespace redfish