#pragma once

#include <human_sort.hpp>
#include <sdbusplus/asio/property.hpp>

#include <algorithm>
#include <string>
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
                     << collectionPath.string();
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

/**
 * @brief Populate the collection "Members" from the Associations endpoint
 *  path
 *
 * @param[i,o] aResp  Async response object
 * @param[i]   collectionPath  Redfish collection path which is used for the
 *             Members Redfish Path
 * @param[i]   interfaces  List of interfaces to constrain the associated
 *             members
 * @param[in]  associationEndPointPath  Path that points to the association end
 *             point
 *
 * @return void
 */
inline void getAssociatedCollectionMembers(
    const std::shared_ptr<bmcweb::AsyncResp>& aResp,
    const boost::urls::url& collectionPath,
    const std::vector<const char*>& interfaces,
    const char* associationEndPointPath)
{
    BMCWEB_LOG_DEBUG << "Get collection members for: " << collectionPath;

    sdbusplus::asio::getProperty<std::vector<std::string>>(
        *crow::connections::systemBus, "xyz.openbmc_project.ObjectMapper",
        associationEndPointPath, "xyz.openbmc_project.Association", "endpoints",
        [collectionPath, interfaces,
         aResp](const boost::system::error_code ec,
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

        crow::connections::systemBus->async_method_call(
            [aResp, objects{objects},
             collectionPath](const boost::system::error_code ec2,
                             const dbus::utility::MapperGetSubTreePathsResponse&
                                 subTreePaths) {
            if (ec2 == boost::system::errc::io_error)
            {
                aResp->res.jsonValue["Members"] = nlohmann::json::array();
                aResp->res.jsonValue["Members@odata.count"] = 0;
                return;
            }

            if (ec2)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error " << ec2.value();
                messages::internalError(aResp->res);
                return;
            }

            std::vector<std::string> objectPaths;

            // For a given association endpoint path, there could be associated
            // members with different interface types. So filter out the
            // required members.
            std::set_intersection(objects.begin(), objects.end(),
                                  subTreePaths.begin(), subTreePaths.end(),
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
            "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
            "/xyz/openbmc_project/inventory", 0, interfaces);
        });
}

} // namespace collection_util
} // namespace redfish
