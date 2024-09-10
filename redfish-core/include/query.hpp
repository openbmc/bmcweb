// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
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
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

// IWYU pragma: no_forward_declare crow::App

#include "redfish_aggregator.hpp"

namespace redfish
{
inline void afterIfMatchRequest(
    crow::App& app, const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::shared_ptr<crow::Request>& req, const std::string& ifMatchHeader,
    const crow::Response& resIn)
{
    std::string computedEtag = resIn.computeEtag();
    BMCWEB_LOG_DEBUG("User provided if-match etag {} computed etag {}",
                     ifMatchHeader, computedEtag);
    if (computedEtag != ifMatchHeader)
    {
        messages::preconditionFailed(asyncResp->res);
        return;
    }
    // Restart the request without if-match
    req->clearHeader(boost::beast::http::field::if_match);
    BMCWEB_LOG_DEBUG("Restarting request");
    app.handle(req, asyncResp);
}

inline bool handleIfMatch(crow::App& app, const crow::Request& req,
                          const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (req.session == nullptr)
    {
        // If the user isn't authenticated, don't even attempt to parse match
        // parameters
        return true;
    }

    std::string ifMatch{
        req.getHeaderValue(boost::beast::http::field::if_match)};
    if (ifMatch.empty())
    {
        // No If-Match header.  Nothing to do
        return true;
    }
    if (ifMatch == "*")
    {
        // Representing any resource
        // https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/If-Match
        return true;
    }
    if (req.method() != boost::beast::http::verb::patch &&
        req.method() != boost::beast::http::verb::post &&
        req.method() != boost::beast::http::verb::delete_)
    {
        messages::preconditionFailed(asyncResp->res);
        return false;
    }
    boost::system::error_code ec;

    // Try to GET the same resource
    auto getReq = std::make_shared<crow::Request>(
        crow::Request::Body{boost::beast::http::verb::get,
                            req.url().encoded_path(), 11},
        ec);

    if (ec)
    {
        messages::internalError(asyncResp->res);
        return false;
    }

    // New request has the same credentials as the old request
    getReq->session = req.session;

    // Construct a new response object to fill in, and check the hash of before
    // we modify the Resource.
    std::shared_ptr<bmcweb::AsyncResp> getReqAsyncResp =
        std::make_shared<bmcweb::AsyncResp>();

    // Ideally we would have a shared_ptr to the original Request which we could
    // modify to remove the If-Match and restart it. But instead we have to make
    // a full copy to restart it.
    getReqAsyncResp->res.setCompleteRequestHandler(std::bind_front(
        afterIfMatchRequest, std::ref(app), asyncResp,
        std::make_shared<crow::Request>(req), std::move(ifMatch)));

    app.handle(getReq, getReqAsyncResp);
    return false;
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
    BMCWEB_LOG_DEBUG("setup redfish route");

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
        query_param::parseParameters(req.url().params(), asyncResp->res);
    if (!queryOpt)
    {
        return false;
    }

    if (!handleIfMatch(app, req, asyncResp))
    {
        return false;
    }

    bool needToCallHandlers = true;

    if constexpr (BMCWEB_REDFISH_AGGREGATION)
    {
        needToCallHandlers =
            RedfishAggregator::beginAggregation(req, asyncResp) ==
            Result::LocalHandle;

        // If the request should be forwarded to a satellite BMC then we don't
        // want to write anything to the asyncResp since it will get overwritten
        // later.
    }

    // If this isn't a get, no need to do anything with parameters
    if (req.method() != boost::beast::http::verb::get)
    {
        return needToCallHandlers;
    }

    delegated = query_param::delegate(queryCapabilities, *queryOpt);
    std::function<void(crow::Response&)> handler =
        asyncResp->res.releaseCompleteRequestHandler();

    asyncResp->res.setCompleteRequestHandler(
        [&app, handler(std::move(handler)), query{std::move(*queryOpt)},
         delegated{delegated}](crow::Response& resIn) mutable {
            processAllParams(app, query, delegated, handler, resIn);
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
