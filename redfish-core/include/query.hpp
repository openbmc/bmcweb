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

    bool needToCallHandlers = true;

#ifdef BMCWEB_ENABLE_REDFISH_AGGREGATION
    needToCallHandlers = RedfishAggregator::getInstance().beginAggregation(
                             req, asyncResp) == Result::LocalHandle;

    // If the request should be forwarded to a satellite BMC then we don't want
    // to write anything to the asyncResp since it will get overwritten later.
#endif

    // If this isn't a get, no need to do anything with parameters
    if (req.method() != boost::beast::http::verb::get)
    {
        return needToCallHandlers;
    }

    delegated = query_param::delegate(queryCapabilities, *queryOpt);
    std::function<void(crow::Response&)> handler =
        asyncResp->res.releaseCompleteRequestHandler();

    asyncResp->res.setCompleteRequestHandler(
        [&app, handler(std::move(handler)),
         query{std::move(*queryOpt)}](crow::Response& resIn) mutable {
        processAllParams(app, query, handler, resIn);
    });

    return needToCallHandlers;
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
