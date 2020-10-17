#pragma once

#include <boost/container/flat_map.hpp>

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
    getCollectionMembers(std::shared_ptr<AsyncResp> aResp,
                         const std::string& collectionPath,
                         const std::vector<const char*>& interfaces,
                         const char* subtree = "/xyz/openbmc_project/inventory")
{
    BMCWEB_LOG_DEBUG << "Get collection members for: " << collectionPath;
    crow::connections::systemBus->async_method_call(
        [collectionPath,
         aResp{std::move(aResp)}](const boost::system::error_code ec,
                                  const std::vector<std::string>& objects) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(aResp->res);
                return;
            }
            nlohmann::json& members = aResp->res.jsonValue["Members"];
            members = nlohmann::json::array();

            for (const auto& object : objects)
            {
                auto pos = object.rfind('/');
                if ((pos != std::string::npos) && (pos < (object.size() - 1)))
                {
                    members.push_back(
                        {{"@odata.id",
                          collectionPath + "/" + object.substr(pos + 1)}});
                }
            }
            aResp->res.jsonValue["Members@odata.count"] = members.size();
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths", subtree, 0,
        interfaces);
}

} // namespace collection_util
} // namespace redfish
