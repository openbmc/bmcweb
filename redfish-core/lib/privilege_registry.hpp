#include "privileges.hpp"

#include <app.hpp>
#include <registries/privilege_registry.hpp>
#include <verb.hpp>

#include <array>
#include <string>
#include <vector>

namespace redfish
{

using EntityTag = redfish::privileges::EntityTag;

constexpr std::array<HttpVerb, 6> registryMethods = {
    HttpVerb::Get, HttpVerb::Head,   HttpVerb::Patch,
    HttpVerb::Put, HttpVerb::Delete, HttpVerb::Post,
};

inline void handlePrivilegeRegistryGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    asyncResp->res.jsonValue["@odata.id"] =
        "/redfish/v1/AccountService/PrivilegeMap";
    asyncResp->res.jsonValue["@odata.type"] =
        "#PrivilegeRegistry.v1_1_4.PrivilegeRegistry";
    asyncResp->res.jsonValue["Id"] = "Redfish_1.3.0_PrivilegeRegistry";
    asyncResp->res.jsonValue["Name"] = "Privilege Mapping array collection";

    // Get all base privileges
    asyncResp->res.jsonValue["PrivilegesUsed"] = basePrivileges;

    asyncResp->res.jsonValue["OEMPrivilegesUsed"] = getAllOemPrivileges();

    asyncResp->res.jsonValue["Mappings"] = nlohmann::json::array();

    for (uint entityTag = 0; entityTag < redfish::privileges::entities.size();
         ++entityTag)
    {
        nlohmann::json mapping;
        mapping["Entity"] = redfish::privileges::entities[entityTag];

        nlohmann::json operationMap;
        for (HttpVerb verb : registryMethods)
        {
            EntityTag curTag = static_cast<EntityTag>(entityTag);

            nlohmann::json privilegeSetJson = nlohmann::json::array();
            std::vector<Privileges> privilegeSet =
                redfish::privileges::privilegeSetMap[curTag][verb];

            for (Privileges privilege : privilegeSet)
            {
                nlohmann::json privilegeJson;
                privilegeJson["Privilege"] =
                    privilege.getAllActivePrivilegeNames();
                privilegeSetJson.push_back(privilegeJson);
            }

            operationMap[httpVerbToString(verb)] = privilegeSetJson;
        }
        mapping["OperationMap"] = operationMap;
        asyncResp->res.jsonValue["Mappings"].push_back(mapping);
    }
}

inline void requestRoutesPrivilegeRegistry(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/AccountService/PrivilegeMap")
        .privileges(redfish::privileges::getPrivilegesFromUrlAndMethod(
            "/redfish/v1/AccountService/PrivilegeMap", HttpVerb::Get))
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handlePrivilegeRegistryGet, std::ref(app)));
}

} // namespace redfish