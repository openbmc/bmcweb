#pragma once

#include "error_messages.hpp"
#include "utility.hpp"

#include <app.hpp>
#include <http_request.hpp>
#include <http_response.hpp>
#include <schemas.hpp>

#include <string>

namespace redfish
{

inline void redfishGet(App& app, const crow::Request& req,
                       const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp->res))
    {
        return;
    }
    asyncResp->res.jsonValue["v1"] = "/redfish/v1/";
}

inline void
    jsonSchemaIndexGet(App& app, const crow::Request& req,
                       const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp->res))
    {
        return;
    }
    nlohmann::json& json = asyncResp->res.jsonValue;
    json["@odata.id"] = "/redfish/v1/JsonSchemas";
    json["@odata.context"] =
        "/redfish/v1/$metadata#JsonSchemaFileCollection.JsonSchemaFileCollection";
    json["@odata.type"] = "#JsonSchemaFileCollection.JsonSchemaFileCollection";
    json["Name"] = "JsonSchemaFile Collection";
    json["Description"] = "Collection of JsonSchemaFiles";
    nlohmann::json::array_t members;
    for (const std::string_view schema : schemas)
    {
        nlohmann::json::object_t member;
        member["@odata.id"] = crow::utility::urlFromPieces(
            "redfish", "v1", "JsonSchemas", schema);
        members.push_back(std::move(member));
    }
    json["Members"] = std::move(members);
    json["Members@odata.count"] = schemas.size();
}

inline void jsonSchemaGet(App& app, const crow::Request& req,
                          const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& schema)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp->res))
    {
        return;
    }

    if (std::find(schemas.begin(), schemas.end(), schema) == schemas.end())
    {
        messages::resourceNotFound(asyncResp->res,
                                   "JsonSchemaFile.JsonSchemaFile", schema);
        return;
    }

    nlohmann::json& json = asyncResp->res.jsonValue;
    json["@odata.context"] =
        "/redfish/v1/$metadata#JsonSchemaFile.JsonSchemaFile";
    json["@odata.id"] =
        crow::utility::urlFromPieces("redfish", "v1", "JsonSchemas", schema);
    json["@odata.type"] = "#JsonSchemaFile.v1_0_2.JsonSchemaFile";
    json["Name"] = schema + " Schema File";
    json["Description"] = schema + " Schema File Location";
    json["Id"] = schema;
    std::string schemaName = "#";
    schemaName += schema;
    schemaName += ".";
    schemaName += schema;
    json["Schema"] = std::move(schemaName);
    constexpr std::array<std::string_view, 1> languages{"en"};
    json["Languages"] = languages;
    json["Languages@odata.count"] = languages.size();

    nlohmann::json::array_t locationArray;
    nlohmann::json::object_t locationEntry;
    locationEntry["Language"] = "en";
    locationEntry["PublicationUri"] =
        "http://redfish.dmtf.org/schemas/v1/" + schema + ".json";
    locationEntry["Uri"] = crow::utility::urlFromPieces(
        "redfish", "v1", "JsonSchemas", schema, std::string(schema) + ".json");

    locationArray.emplace_back(locationEntry);

    json["Location"] = std::move(locationArray);
    json["Location@odata.count"] = 1;
}

inline void requestRoutesRedfish(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/")
        .methods(boost::beast::http::verb::get)(
            std::bind_front(redfishGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/JsonSchemas/<str>/")
        .methods(boost::beast::http::verb::get)(
            std::bind_front(jsonSchemaGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/JsonSchemas/")
        .methods(boost::beast::http::verb::get)(
            std::bind_front(jsonSchemaIndexGet, std::ref(app)));
}

} // namespace redfish
