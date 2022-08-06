#pragma once

#include "bmcweb_config.h"

#include "app.hpp"
#include "async_resp.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "logging.hpp"
#include "utils/query_param.hpp"

#include <boost/beast/http/verb.hpp>
#include <boost/url/params_view.hpp>
#include <boost/url/url_view.hpp>
#include <valijson/adapters/nlohmann_json_adapter.hpp>
#include <valijson/schema.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/validator.hpp>

#include <fstream>
#include <functional>
#include <memory>
#include <new>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

// IWYU pragma: no_forward_declare crow::App
// IWYU pragma: no_include <boost/url/impl/params_view.hpp>
// IWYU pragma: no_include <boost/url/impl/url_view.hpp>

namespace redfish
{

inline std::map<std::string, nlohmann::json> jsonSchemas;

inline const nlohmann::json* fetchDocument(const std::string& uri)
{
    if (!boost::starts_with(uri, "http://redfish.dmtf.org/schemas/v1/"))
    {
        std::cout << "Not a DMTF schema???\n";
        return nullptr;
    }
    std::string folder = "/build/schemas/json-schema/";
    std::filesystem::path filename(uri.substr(35));

    folder += filename.string();
    std::cout << "Fetching \"" << folder << "\"\n";
    auto it = jsonSchemas.find(folder);
    if (it == jsonSchemas.end())
    {
        std::cout << "Couldn't find " << folder << "\n";
        return nullptr;
    }
    nlohmann::json* ret = &it->second;
    return ret;
}

inline void freeDocument(const nlohmann::json* /*ptr*/)
{}

inline bool validateJson(const nlohmann::json& input)
{
    auto it = input.find("@odata.type");
    if (it == input.end())
    {
        return false;
    }
    valijson::Schema schema;
    valijson::SchemaParser parser;
    std::cout << "Parsing schema\n";
    nlohmann::json schemafile;
    {
        std::ifstream ifs("/build/schemas/json-schema/ServiceRoot.json");
        schemafile = nlohmann::json::parse(ifs);
    }
    valijson::adapters::NlohmannJsonAdapter schemaAdapter(schemafile);
    parser.populateSchema(schemaAdapter, schema, fetchDocument, freeDocument);

    for (auto& file : std::filesystem::directory_iterator(
             std::filesystem::path("/build/schemas/json-schema")))
    {
        std::cout << "Checking " << file.path().string() << "\n";
        std::error_code ec;
        if (!std::filesystem::is_regular_file(file.path(), ec))
        {
            continue;
        }
        std::ifstream inputFile(file.path().string());
        std::cout << "Adding " << file.path().string() << "\n";

        jsonSchemas[file.path().string()] =
            nlohmann::json::parse(inputFile, nullptr, false);
    }

    valijson::Validator validator;
    valijson::adapters::NlohmannJsonAdapter targetAdapter(input);
    if (!validator.validate(schema, targetAdapter, nullptr))
    {
        std::cout << "Parsing failed\n";
        return false;
    }
    std::cout << "Parsing succeeded\n";
    return true;
}

inline void validateResponse(crow::Response& res)
{
    if (!validateJson(res.jsonValue))
    {
        messages::internalError(res);
    }
}

// Sets up the Redfish Route and delegates some of the query parameter
// processing. |queryCapabilities| stores which query parameters will be
// handled by redfish-core/lib codes, then default query parameter handler won't
// process these parameters.
[[nodiscard]] inline bool setUpRedfishRouteWithDelegation(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    query_param::Query& delegated,
    const query_param::QueryCapabilities& queryCapabilities)
{
    BMCWEB_LOG_DEBUG << "setup redfish route";

    // Section 7.4 of the redfish spec "Redfish Services shall process the
    // [OData-Version header] in the following table as defined by the HTTP 1.1
    // specification..."
    // Required to pass redfish-protocol-validator REQ_HEADERS_ODATA_VERSION
    std::string_view odataHeader = req.getHeaderValue("OData-Version");
    if (!odataHeader.empty() && odataHeader != "4.0")
    {
        messages::preconditionFailed(asyncResp->res);
        return false;
    }

    asyncResp->res.addHeader("OData-Version", "4.0");

    std::optional<query_param::Query> queryOpt =
        query_param::parseParameters(req.urlView.params(), asyncResp->res);
    if (queryOpt == std::nullopt)
    {
        return false;
    }

    // If this isn't a get, no need to do anything with parameters
    if (req.method() != boost::beast::http::verb::get)
    {
        return true;
    }

    delegated = query_param::delegate(queryCapabilities, *queryOpt);
    std::function<void(crow::Response&)> handler =
        asyncResp->res.releaseCompleteRequestHandler();

    asyncResp->res.setCompleteRequestHandler(
        [&app, handler(std::move(handler)),
         query{std::move(*queryOpt)}](crow::Response& resIn) mutable {
        validateResponse(resIn);
        processAllParams(app, query, handler, resIn);
    });

    return true;
}

// Sets up the Redfish Route. All parameters are handled by the default handler.
[[nodiscard]] inline bool
    setUpRedfishRoute(crow::App& app, const crow::Request& req,
                      const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    // This route |delegated| is never used
    query_param::Query delegated;
    return setUpRedfishRouteWithDelegation(app, req, asyncResp, delegated,
                                           query_param::QueryCapabilities{});
}
} // namespace redfish
