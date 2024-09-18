#pragma once

#include "app.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utility.hpp"

#include <boost/url/format.hpp>

#include <ranges>
#include <string>

namespace redfish
{

inline void redfishGet(App& app, const crow::Request& req,
                       const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    asyncResp->res.jsonValue["v1"] = "/redfish/v1/";
}

inline void redfish404(App& app, const crow::Request& req,
                       const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& path)
{
    asyncResp->res.addHeader(boost::beast::http::field::allow, "");

    // If we fall to this route, we didn't have a more specific route, so return
    // 404
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    BMCWEB_LOG_WARNING("404 on path {}", path);

    std::string name = req.url().segments().back();
    // Note, if we hit the wildcard route, we don't know the "type" the user was
    // actually requesting, but giving them a return with an empty string is
    // still better than nothing.
    messages::resourceNotFound(asyncResp->res, "", name);
}

inline void redfish405(App& app, const crow::Request& req,
                       const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& path)
{
    // If we fall to this route, we didn't have a more specific route, so return
    // 405
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    BMCWEB_LOG_WARNING("405 on path {}", path);
    asyncResp->res.result(boost::beast::http::status::method_not_allowed);
    if (req.method() == boost::beast::http::verb::delete_)
    {
        messages::resourceCannotBeDeleted(asyncResp->res);
    }
    else
    {
        messages::operationNotAllowed(asyncResp->res);
    }
}

inline void
    jsonSchemaIndexGet(App& app, const crow::Request& req,
                       const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    nlohmann::json& json = asyncResp->res.jsonValue;
    json["@odata.id"] = "/redfish/v1/JsonSchemas";
    json["@odata.type"] = "#JsonSchemaFileCollection.JsonSchemaFileCollection";
    json["Name"] = "JsonSchemaFile Collection";
    json["Description"] = "Collection of JsonSchemaFiles";
    nlohmann::json::array_t members;

    std::error_code ec;
    std::filesystem::directory_iterator dirList(
        "/usr/share/www/redfish/v1/JsonSchemas", ec);
    if (ec)
    {
        messages::internalError(asyncResp->res);
        return;
    }
    for (const std::filesystem::path& file : dirList)
    {
        std::string filename = file.filename();
        std::vector<std::string> split;
        bmcweb::split(split, filename, '.');
        if (split.empty())
        {
            continue;
        }
        nlohmann::json::object_t member;
        member["@odata.id"] = boost::urls::format("/redfish/v1/JsonSchemas/{}",
                                                  split[0]);
        members.emplace_back(std::move(member));
    }

    json["Members@odata.count"] = members.size();
    json["Members"] = std::move(members);
}

inline void jsonSchemaGet(App& app, const crow::Request& req,
                          const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& schema)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    std::error_code ec;
    std::filesystem::directory_iterator dirList(
        "/usr/share/www/redfish/v1/JsonSchemas", ec);
    if (ec)
    {
        messages::resourceNotFound(asyncResp->res, "JsonSchemaFile", schema);
        return;
    }
    for (const std::filesystem::path& file : dirList)
    {
        std::string filename = file.filename();
        std::vector<std::string> split;
        bmcweb::split(split, filename, '.');
        if (split.empty())
        {
            continue;
        }
        BMCWEB_LOG_DEBUG("Checking {}", split[0]);
        if (split[0] != schema)
        {
            continue;
        }

        nlohmann::json& json = asyncResp->res.jsonValue;
        json["@odata.id"] = boost::urls::format("/redfish/v1/JsonSchemas/{}",
                                                schema);
        json["@odata.type"] = "#JsonSchemaFile.v1_0_2.JsonSchemaFile";
        json["Name"] = schema + " Schema File";
        json["Description"] = schema + " Schema File Location";
        json["Id"] = schema;
        std::string schemaName = std::format("#{}.{}", schema, schema);
        json["Schema"] = std::move(schemaName);
        constexpr std::array<std::string_view, 1> languages{"en"};
        json["Languages"] = languages;
        json["Languages@odata.count"] = languages.size();

        nlohmann::json::array_t locationArray;
        nlohmann::json::object_t locationEntry;
        locationEntry["Language"] = "en";

        locationEntry["PublicationUri"] = boost::urls::format(
            "http://redfish.dmtf.org/schemas/v1/{}", filename);
        locationEntry["Uri"] = boost::urls::format(
            "/redfish/v1/JsonSchemas/{}/{}", schema, filename);

        locationArray.emplace_back(locationEntry);

        json["Location"] = std::move(locationArray);
        json["Location@odata.count"] = 1;
        return;
    }
    messages::resourceNotFound(asyncResp->res, "JsonSchemaFile", schema);
}

inline void
    jsonSchemaGetFile(const crow::Request& /*req*/,
                      const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const std::string& schema, const std::string& schemaFile)
{
    // Sanity check the filename
    if (schemaFile.find_first_not_of(
            "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-.") !=
        std::string::npos)
    {
        messages::resourceNotFound(asyncResp->res, "JsonSchemaFile", schema);
        return;
    }
    // Schema path should look like /redfish/v1/JsonSchemas/Foo/Foo.x.json
    // Make sure the two paths match.
    if (!schemaFile.starts_with(schema))
    {
        messages::resourceNotFound(asyncResp->res, "JsonSchemaFile", schema);
        return;
    }
    std::filesystem::path filepath("/usr/share/www/redfish/v1/JsonSchemas");
    filepath /= schemaFile;
    if (filepath.is_relative())
    {
        messages::resourceNotFound(asyncResp->res, "JsonSchemaFile", schema);
        return;
    }

    crow::OpenCode ec = asyncResp->res.openFile(filepath);
    if (ec == crow::OpenCode::FileDoesNotExist)
    {
        messages::resourceNotFound(asyncResp->res, "JsonSchemaFile", schema);
        return;
    }
    if (ec == crow::OpenCode::InternalError)
    {
        BMCWEB_LOG_DEBUG("failed to read file");
        messages::internalError(asyncResp->res);
        return;
    }
}

inline void requestRoutesRedfish(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/")
        .methods(boost::beast::http::verb::get)(
            std::bind_front(redfishGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/JsonSchemas/<str>/<str>")
        .privileges(redfish::privileges::getJsonSchemaFile)
        .methods(boost::beast::http::verb::get)(jsonSchemaGetFile);

    BMCWEB_ROUTE(app, "/redfish/v1/JsonSchemas/<str>/")
        .privileges(redfish::privileges::getJsonSchemaFileCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(jsonSchemaGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/JsonSchemas/")
        .privileges(redfish::privileges::getJsonSchemaFile)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(jsonSchemaIndexGet, std::ref(app)));

    // Note, this route must always be registered last
    BMCWEB_ROUTE(app, "/redfish/<path>")
        .notFound()
        .privileges(redfish::privileges::privilegeSetLogin)(
            std::bind_front(redfish404, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/<path>")
        .methodNotAllowed()
        .privileges(redfish::privileges::privilegeSetLogin)(
            std::bind_front(redfish405, std::ref(app)));
}

} // namespace redfish
