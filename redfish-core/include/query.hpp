#pragma once

#include "utils/query_param.hpp"

#include <bmcweb_config.h>

#include <redfish_aggregator.hpp>

namespace redfish
{

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
#ifdef BMCWEB_ENABLE_REDFISH_AGGREGATION
    // Asynchronously determine if this request needs to be routed to a
    // satellite BMC.  We will locally process the request at the same time.
    // If the request should be forwarded to a satellite, then local handling
    // won't recognize the path due to the prefix and set res to return a 404.
    // That error will get overwritten by the actual response from the
    // satellite BMC.

    // TODO: There is a chance of a race condition where the local BMC does
    // not finish until after the response from the satellite BMC has already
    // been processed.  In that instance, the valid response from the
    // satellite would get clobbered by us locally writing a 404.  Future
    // changes should be made to remove the possibility of this occurring.

    RedfishAggregator::getInstance().beginAggregation(req, asyncResp);
#endif

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
         query{*queryOpt}](crow::Response& resIn) mutable {
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
