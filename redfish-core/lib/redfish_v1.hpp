#pragma once

#include "error_messages.hpp"
#include "schema_common.hpp"
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

inline void metadataGet(const crow::Request& /*req*/,
                        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    // Note, because this is not technically a part of the redfish json tree, we
    // do not call setUpRedfishRoute on it
    std::string& body = asyncResp->res.body();
    body += "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    body +=
        "<edmx:Edmx xmlns:edmx=\"http://docs.oasis-open.org/odata/ns/edmx\" Version=\"4.0\">\n";

    for (const SchemaVersion* schema : schemas)
    {
        body += "<edmx:Reference Uri=\"/redfish/v1/schema/";
        body += schema->name;
        body += "_v1.xml\">";

        body += "<edmx:Include Namespace=\"";
        body += schema->toString();
        body += "\"/>\n";
    }
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
    for (const SchemaVersion* schema : schemas)
    {
        nlohmann::json::object_t member;
        member["@odata.id"] = crow::utility::urlFromPieces(
            "redfish", "v1", "JsonSchemas", schema->name);
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

    if (std::find_if(schemas.begin(), schemas.end(),
                     [&schema](const SchemaVersion* toCheck) {
                         return toCheck->name == schema;
                     }) == schemas.end())
    {
        messages::resourceNotFound(asyncResp->res, jsonSchemaFileType, schema);
        return;
    }

    nlohmann::json& json = asyncResp->res.jsonValue;
    json["@odata.context"] =
        "/redfish/v1/$metadata#JsonSchemaFile.JsonSchemaFile";
    json["@odata.id"] =
        crow::utility::urlFromPieces("redfish", "v1", "JsonSchemas", schema);
    json["@odata.type"] = jsonSchemaFileType;
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

inline void requestRoutes(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/")
        .methods(boost::beast::http::verb::get)(
            std::bind_front(redfishGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/$metadata")
        .methods(boost::beast::http::verb::get)(metadataGet);

#ifdef BMCWEB_ENABLE_REDFISH_JSON_SCHEMAS
    BMCWEB_ROUTE(app, "/redfish/v1/JsonSchemas/<str>/")
        .methods(boost::beast::http::verb::get)(
            std::bind_front(jsonSchemaGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/JsonSchemas/")
        .methods(boost::beast::http::verb::get)(
            std::bind_front(jsonSchemaIndexGet, std::ref(app)));
#endif
}

} // namespace redfish
