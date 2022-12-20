#include "privileges.hpp"

#include <app.hpp>
#include <nlohmann/json.hpp>
#include <query.hpp>
#include <registries/privilege_registry.hpp>
#include <verb.hpp>

#include <array>
#include <string>
#include <utility>
#include <vector>

namespace redfish
{

using EntityTag = redfish::privileges::EntityTag;

constexpr std::array<HttpVerb, 6> registryMethods = {
    HttpVerb::Get, HttpVerb::Head,   HttpVerb::Patch,
    HttpVerb::Put, HttpVerb::Delete, HttpVerb::Post,
};

inline void
    fillInPrivilegeRegistry(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
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

    for (size_t entityTag = 0; entityTag < redfish::privileges::entities.size();
         ++entityTag)
    {
        nlohmann::json::object_t mapping;
        mapping["Entity"] = redfish::privileges::entities[entityTag];

        nlohmann::json::object_t operationMap;
        for (HttpVerb verb : registryMethods)
        {
            EntityTag curTag = static_cast<EntityTag>(entityTag);

            nlohmann::json::array_t privilegeSetJson;
            const std::span<const Privileges> privilegeSet =
                redfish::privileges::getPrivilegeFromEntityAndMethod(curTag,
                                                                     verb);

            for (const Privileges& privilege : privilegeSet)
            {
                nlohmann::json::object_t privilegeJson;
                privilegeJson["Privilege"] =
                    privilege.getAllActivePrivilegeNames();

                // The base registry has "NoAuth" privileges to represent no
                // privileges needed
                if (privilegeJson["Privilege"].empty())
                {
                    privilegeJson["Privilege"].emplace_back("NoAuth");
                }
                privilegeSetJson.emplace_back(std::move(privilegeJson));
            }
            // Using string here because nlohmann [] operator dosent take
            // string_view
            const std::string methodString =
                std::string(httpVerbToString(verb));

            if (methodString.empty())
            {
                continue;
            }

            operationMap[methodString] = std::move(privilegeSetJson);
        }
        mapping["OperationMap"] = std::move(operationMap);
        asyncResp->res.jsonValue["Mappings"].emplace_back(std::move(mapping));
    }
}

inline void handlePrivilegeRegistryGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    fillInPrivilegeRegistry(asyncResp);
}

inline void requestPrivilegeRegistryRoutes(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/AccountService/PrivilegeMap")
        .privileges(redfish::privileges::getAccountService)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handlePrivilegeRegistryGet, std::ref(app)));
}

} // namespace redfish