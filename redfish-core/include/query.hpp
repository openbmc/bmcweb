#pragma once

#include "utils/query_param.hpp"

#include <bmcweb_config.h>

namespace redfish
{

// Sets up the Redfish Route and delegates some of the query parameter
// processing. |bypassDefaultQuery| takes a query object parsed from URLs, and
// returns a new query object indicating that which query parameters will be
// handled by other codes, then default query parameter handler won't process
// these parameters.
[[nodiscard]] inline bool setUpRedfishRoute(
    crow::App& app, const crow::Request& req, crow::Response& res,
    const std::function<query_param::Query(query_param::Query)>&
        bypassDefaultQuery)
{
    BMCWEB_LOG_DEBUG << "setup redfish route";

    // Section 7.4 of the redfish spec "Redfish Services shall process the
    // [OData-Version header] in the following table as defined by the HTTP 1.1
    // specification..."
    // Required to pass redfish-protocol-validator REQ_HEADERS_ODATA_VERSION
    std::string_view odataHeader = req.getHeaderValue("OData-Version");
    if (!odataHeader.empty() && odataHeader != "4.0")
    {
        messages::preconditionFailed(res);
        return false;
    }

    // If query parameters aren't enabled, do nothing.
    if constexpr (bmcwebInsecureEnableQueryParams)
    {
        return true;
    }
    std::optional<query_param::Query> queryOpt =
        query_param::parseParameters(req.urlView.params(), res);
    if (queryOpt == std::nullopt)
    {
        return false;
    }

    // If this isn't a get, no need to do anything with parameters
    if (req.method() != boost::beast::http::verb::get)
    {
        return true;
    }

    std::function<void(crow::Response&)> handler =
        res.releaseCompleteRequestHandler();

    res.setCompleteRequestHandler(
        [&app, handler(std::move(handler)),
         query{bypassDefaultQuery(*queryOpt)}](crow::Response& res) mutable {
            processAllParams(app, query, handler, res);
        });
    return true;
}

// Sets up the Redfish Route. All parameters are handled by the default handler.
[[nodiscard]] inline bool setUpRedfishRoute(crow::App& app,
                                            const crow::Request& req,
                                            crow::Response& res)
{
    return setUpRedfishRoute(app, req, res,
                             [](query_param::Query query) { return query; });
}
} // namespace redfish
