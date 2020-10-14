#pragma once

#include <boost/container/flat_map.hpp>

#include <string>
#include <vector>

namespace redfish
{
namespace collection_util
{

inline void getResourceList(std::shared_ptr<AsyncResp> aResp,
                            const std::string& subclass,
                            const std::vector<const char*>& collectionName)
{
    BMCWEB_LOG_DEBUG << "Get available system cpu/mem resources.";
    crow::connections::systemBus->async_method_call(
        [subclass, aResp{std::move(aResp)}](
            const boost::system::error_code ec,
            const boost::container::flat_map<
                std::string, boost::container::flat_map<
                                 std::string, std::vector<std::string>>>&
                subtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(aResp->res);
                return;
            }
            nlohmann::json& members = aResp->res.jsonValue["Members"];
            members = nlohmann::json::array();

            for (const auto& object : subtree)
            {
                auto iter = object.first.rfind("/");
                if ((iter != std::string::npos) && (iter < object.first.size()))
                {
                    members.push_back(
                        {{"@odata.id", "/redfish/v1/Systems/system/" +
                                           subclass + "/" +
                                           object.first.substr(iter + 1)}});
                }
            }
            aResp->res.jsonValue["Members@odata.count"] = members.size();
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", 0, collectionName);
}

} // namespace collection_util
} // namespace redfish
