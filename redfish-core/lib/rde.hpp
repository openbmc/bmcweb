#pragma once

#include "bmcweb_config.h"

#include "app.hpp"
#include "async_resp.hpp"

#include <boost/beast/http/status.hpp>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <format>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace redfish
{

constexpr std::string_view redfishSystemsBase = "/redfish/v1/Systems";
constexpr std::string_view oemAmdRDEUri = "/oem/amd/RDE";

// GET handler
inline void rdeGetHandler(App& app, const crow::Request& req,
                          const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    BMCWEB_LOG_DEBUG("RDE : Get handler.");
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    // TODO: Implement the GET request handling logic here.
    asyncResp->res.result(boost::beast::http::status::ok);
    asyncResp->res.jsonValue["status"] = "success";
}

// PATCH handler
inline void rdePatchHandler(App& app, const crow::Request& req,
                            const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    BMCWEB_LOG_DEBUG("RDE : Patch handler.");

    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    // TODO: Implement the PATCH request handling logic here.
    asyncResp->res.result(boost::beast::http::status::ok);
    asyncResp->res.jsonValue["status"] = "success";
}

inline nlohmann::json loadRDESchemaMap()
{
    constexpr const char* rdeUriCachePath = "/var/lib/pldm/rde_uri_cache.json";

    nlohmann::json schemaMap = nlohmann::json::object();

    if (!std::filesystem::exists(rdeUriCachePath))
    {
        BMCWEB_LOG_ERROR("RDE: Schema file not found: {}", rdeUriCachePath);
        return schemaMap; // Return empty object
    }

    std::ifstream file(rdeUriCachePath);
    if (!file.is_open())
    {
        BMCWEB_LOG_ERROR("RDE: Failed to open schema file: {}",
                         rdeUriCachePath);
        return schemaMap; // Return empty object
    }

    try
    {
        if (file.peek() == std::ifstream::traits_type::eof())
        {
            BMCWEB_LOG_ERROR("RDE: Schema file is empty: {}", rdeUriCachePath);
            return schemaMap; // Return empty object
        }

        file >> schemaMap;

        if (!schemaMap.is_object())
        {
            BMCWEB_LOG_ERROR(
                "RDE: Schema file does not contain a valid JSON object.");
            return schemaMap; // Return empty object
        }
    }
    catch (const nlohmann::json::parse_error& e)
    {
        BMCWEB_LOG_ERROR("RDE: JSON parse error: {}", e.what());
        return schemaMap; // Return empty object
    }
    catch (const std::exception& e)
    {
        BMCWEB_LOG_ERROR("RDE: Unexpected error while reading schema: {}",
                         e.what());
        return schemaMap; // Return empty object
    }

    return schemaMap;
}

inline void handleRDE(crow::App& app, const crow::Request& req,
                      const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const std::string& systemName,
                      const std::string& resourceName,
                      const std::string& processorId,
                      const std::string& schemaPath)
{
    BMCWEB_LOG_DEBUG("RDE Request Handling[{} {} {} {}]", systemName,
                     resourceName, processorId, schemaPath);

    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    std::string subUri = std::format("{}/{}{}/{}", resourceName, processorId,
                                     oemAmdRDEUri, schemaPath);

    nlohmann::json schemaMap = loadRDESchemaMap();
    if (!schemaMap.contains("Resources") || !schemaMap["Resources"].is_object())
    {
        BMCWEB_LOG_ERROR("Invalid or missing 'Resources' in schemaMap");
        asyncResp->res.result(boost::beast::http::status::not_acceptable);
        asyncResp->res.jsonValue = {{"error", "Invalid or missing 'Resources"},
                                    {"status", "failed"}};
        return;
    }

    std::string expectedSubUri;

    for (const auto& [resourceId, schema] : schemaMap["Resources"].items())
    {
        std::string tmpResourceName;
        std::string tmpSubUri;
        std::vector<std::string> expectedSubUris;

        if (schema.contains("ProposedContainingResourceName") &&
            schema["ProposedContainingResourceName"].is_string())
        {
            tmpResourceName =
                schema["ProposedContainingResourceName"].get<std::string>();
        }
        if (schema.contains("SubURI") && schema["SubURI"].is_string())
        {
            tmpSubUri = schema["SubURI"].get<std::string>();
            if (!tmpSubUri.empty())
            {
                expectedSubUris.push_back(std::format(
                    "{}/{}/{}", tmpResourceName, processorId, tmpSubUri));
            }
        }
        if (schema.contains("OEMExtensions") &&
            schema["OEMExtensions"].is_array())
        {
            for (const auto& value : schema["OEMExtensions"])
            {
                if (value.is_string())
                {
                    const std::string& oemValue = value.get<std::string>();
                    expectedSubUris.push_back(std::format(
                        "{}/{}/{}", tmpResourceName, processorId, oemValue));
                }
            }
        }
        auto it = std::find(expectedSubUris.begin(), expectedSubUris.end(),
                            subUri);
        if (it != expectedSubUris.end())
        {
            expectedSubUri = *it;
        }
        if (!expectedSubUri.empty())
        {
            try
            {
                std::string majorSchemaName = "Unknown";
                std::string majorSchemaVersion = "0_0_0";
                if (schema.contains("MajorSchemaName") &&
                    schema["MajorSchemaName"].is_string())
                {
                    majorSchemaName =
                        schema["MajorSchemaName"].get<std::string>();
                }
                if (schema.contains("MajorSchemaVersion") &&
                    schema["MajorSchemaVersion"].is_string())
                {
                    majorSchemaVersion =
                        schema["MajorSchemaVersion"].get<std::string>();
                }

                asyncResp->res.jsonValue["@odata.type"] = "#RDE.v1_0_0.RDERoot";
                std::string fullPath = std::format("{}/{}", redfishSystemsBase,
                                                   subUri);
                asyncResp->res.jsonValue["@odata.id"] = fullPath;
                std::string odataType = "#" + majorSchemaName + ".v" +
                                        majorSchemaVersion + "." +
                                        majorSchemaName;
                asyncResp->res.jsonValue["@odata.type"] = odataType;
                asyncResp->res.jsonValue["ResourceID"] = resourceId;
            }
            catch (const std::exception& e)
            {
                BMCWEB_LOG_ERROR(
                    "RDE: Error populating response from schema: {}", e.what());
                asyncResp->res.result(
                    boost::beast::http::status::internal_server_error);
                asyncResp->res.jsonValue = {
                    {"error", "Failed to populate response"}};
                return;
            }

            // Handle HTTP methods
            if (req.method() == boost::beast::http::verb::get)
            {
                rdeGetHandler(app, req, asyncResp);
            }
            else if (req.method() == boost::beast::http::verb::patch)
            {
                rdePatchHandler(app, req, asyncResp);
            }
            else
            {
                asyncResp->res.result(
                    boost::beast::http::status::method_not_allowed);
                asyncResp->res.jsonValue = {
                    {"error", "Unsupported HTTP method"}};
            }
            return;
        }
    }
    // No matching suburi found
    BMCWEB_LOG_ERROR("RDE: No matching suburi found for input subUri: {}",
                     subUri);
    asyncResp->res.result(boost::beast::http::status::bad_request);
    asyncResp->res.jsonValue = {
        {"error", "Invalid subUri"}, {"subUri", subUri}, {"status", "failed"}};
    return;
}

inline void handleRDERoot([[maybe_unused]] crow::App& app,
                          [[maybe_unused]] const crow::Request& req,
                          const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& systemName)
{
    std::string baseUri = std::format("{}/{}", redfishSystemsBase, systemName);
    asyncResp->res.jsonValue["@odata.type"] = "#RDE.v1_0_0.RDERoot";
    asyncResp->res.jsonValue["@odata.id"] = baseUri;
    asyncResp->res.jsonValue["Name"] = "Redfish Dynamic Extensions";

    // Load schema map
    nlohmann::json schemaMap = loadRDESchemaMap();

    if (!schemaMap.contains("Resources") || !schemaMap["Resources"].is_object())
    {
        BMCWEB_LOG_ERROR("Invalid or missing 'Resources' in schemaMap");
        asyncResp->res.result(boost::beast::http::status::not_acceptable);
        asyncResp->res.jsonValue = {{"error", "Invalid or missing 'Resources"},
                                    {"status", "failed"}};
        return;
    }

    nlohmann::json resourceArray = nlohmann::json::array();

    for (const auto& [resourceId, schema] : schemaMap["Resources"].items())
    {
        try
        {
            if (!schema.is_object() || !schema.contains("SubURI") ||
                !schema["SubURI"].is_string() ||
                !schema.contains("ProposedContainingResourceName") ||
                !schema["ProposedContainingResourceName"].is_string())
            {
                continue;
            }

            const std::string subUri = schema["SubURI"].get<std::string>();
            const std::string resourceName =
                schema["ProposedContainingResourceName"].get<std::string>();

            std::string fullUri = std::format("{}/{}/{}/{}", baseUri,
                                              resourceName, "<id>", subUri);

            nlohmann::json resourceEntry = {{"@odata.id", fullUri}};
            // Optionally include Actions if present
            if (schema.contains("Actions") && schema["Actions"].is_array())
            {
                resourceEntry["Actions"] = schema["Actions"];
            }
            resourceArray.push_back(resourceEntry);
        }
        catch (const std::exception& e)
        {
            BMCWEB_LOG_ERROR("RDE: Exception processing resource {}: {}",
                             resourceId, e.what());
            continue;
        }
    }
    asyncResp->res.jsonValue["Resources"] = resourceArray;
}

inline void requestRoutesRDEService(crow::App& app)
{
    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/<str>/<str>/<str>/oem/amd/RDE/<path>")
        .privileges(redfish::privileges::privilegeSetConfigureManager)
        .methods(boost::beast::http::verb::head, boost::beast::http::verb::get,
                 boost::beast::http::verb::patch)(
            std::bind_front(handleRDE, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/oem/amd/RDE")
        .privileges(redfish::privileges::privilegeSetConfigureManager)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleRDERoot, std::ref(app)));
}

} // namespace redfish
