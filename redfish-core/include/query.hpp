#pragma once

#include "utils/query_param.hpp"

#include <bmcweb_config.h>

namespace redfish
{

namespace internal
{

// Delegates query parameters according to the given |queryCapabilities|
// This function doesn't check query parameter conflicts since the parse
// function will take care of it.
inline void delegate(const query_param::QueryCapabilities& queryCapabilities,
                     query_param::Query& query)
{
    // delegate only
    if (query.isOnly && queryCapabilities.canDelegateOnly)
    {
        query.isOnly = false;
    }
    // delegate expand as much as we can
    if (query.expandType != query_param::ExpandType::None &&
        queryCapabilities.canDelegateType.contains(query.expandType))
    {
        if (query.expandLevel <= queryCapabilities.canDelegateExpandLevel)
        {
            query.expandLevel = 0;
            query.expandType = query_param::ExpandType::None;
        }
        else
        {
            query.expandLevel -= queryCapabilities.canDelegateExpandLevel;
        }
    }
}

} // namespace internal

// Sets up the Redfish Route and delegates some of the query parameter
// processing. |queryCapabilities| stores which query parameters will be
// handled by redfish-core/lib codes, then default query parameter handler won't
// process these parameters.
[[nodiscard]] inline bool
    setUpRedfishRoute(crow::App& app, const crow::Request& req,
                      crow::Response& res,
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
        messages::preconditionFailed(res);
        return false;
    }

    // If query parameters aren't enabled, do nothing.
    if constexpr (!bmcwebInsecureEnableQueryParams)
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

    internal::delegate(queryCapabilities, *queryOpt);
    std::function<void(crow::Response&)> handler =
        res.releaseCompleteRequestHandler();
    res.setCompleteRequestHandler(
        [&app, handler(std::move(handler)),
         query{std::move(*queryOpt)}](crow::Response& res) mutable {
            processAllParams(app, query, handler, res);
        });
    return true;
}

// Sets up the Redfish Route. All parameters are handled by the default handler.
[[nodiscard]] inline bool setUpRedfishRoute(crow::App& app,
                                            const crow::Request& req,
                                            crow::Response& res)
{
    return setUpRedfishRoute(app, req, res, query_param::QueryCapabilities{});
}
} // namespace redfish
