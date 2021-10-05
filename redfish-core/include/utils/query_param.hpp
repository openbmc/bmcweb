#pragma once
#include "app.hpp"
#include "async_resp.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "routing.hpp"

#include <string_view>
#include <vector>

namespace redfish
{
namespace query_param
{

struct Query
{
    bool isOnly = false;
};

inline std::optional<Query>
    parseParameters(const boost::urls::query_params_view& urlParams,
                    crow::Response& res)
{
    Query ret{};
    for (const boost::urls::query_params_view::value_type& it : urlParams)
    {
        if (it.key() == "only")
        {
            // Only cannot be combined with any other query
            if (urlParams.size() != 1)
            {
                messages::queryCombinationInvalid(res);
                return std::nullopt;
            }
            if (!it.value().empty())
            {
                messages::queryParameterValueFormatError(res, it.value(),
                                                         it.key());
                return std::nullopt;
            }
            ret.isOnly = true;
        }
    }
    return ret;
}

inline bool processOnly(crow::App& app, crow::Response& res,
                        std::function<void(crow::Response&)>& completionHandler)
{
    BMCWEB_LOG_DEBUG << "Processing only query param";
    auto itMembers = res.jsonValue.find("Members");
    if (itMembers == res.jsonValue.end())
    {
        messages::queryNotSupportedOnResource(res);
        completionHandler(res);
        return false;
    }
    auto itMemBegin = itMembers->begin();
    if (itMemBegin == itMembers->end() || itMembers->size() != 1)
    {
        BMCWEB_LOG_DEBUG << "Members contains " << itMembers->size()
                         << " element, returning full collection.";
        completionHandler(res);
        return false;
    }

    auto itUrl = itMemBegin->find("@odata.id");
    if (itUrl == itMemBegin->end())
    {
        BMCWEB_LOG_DEBUG << "No found odata.id";
        messages::internalError(res);
        completionHandler(res);
        return false;
    }
    const std::string* url = itUrl->get_ptr<const std::string*>();
    if (!url)
    {
        BMCWEB_LOG_DEBUG << "@odata.id wasn't a string????";
        messages::internalError(res);
        completionHandler(res);
        return false;
    }
    // TODO(Ed) copy request headers?
    // newReq.session = req.session;
    std::error_code ec;
    crow::Request newReq({boost::beast::http::verb::get, *url, 11}, ec);
    if (ec)
    {
        messages::internalError(res);
        completionHandler(res);
        return false;
    }

    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    BMCWEB_LOG_DEBUG << "setting completion handler on " << &asyncResp->res;
    asyncResp->res.setCompleteRequestHandler(std::move(completionHandler));
    asyncResp->res.setIsAliveHelper(res.releaseIsAliveHelper());
    app.handle(newReq, asyncResp);
    return true;
}

void processAllParams(crow::App& app, const std::optional<Query>& query,
                      crow::Response& intermediateResponse,
                      std::function<void(crow::Response&)>& completionHandler)
{
    if (!completionHandler)
    {
        BMCWEB_LOG_DEBUG << "Function was invalid?";
        return;
    }

    BMCWEB_LOG_DEBUG << "Processing query params";
    if (!query)
    {
        BMCWEB_LOG_DEBUG << "No query params to process";
        // Query params weren't valid, no need to continue;
        completionHandler(intermediateResponse);
        return;
    }

    // If the request failed, there's no reason to even try to run query
    // params.
    if (intermediateResponse.resultInt() < 200 ||
        intermediateResponse.resultInt() >= 400)
    {
        completionHandler(intermediateResponse);
        return;
    }
    if (query->isOnly)
    {
        processOnly(app, intermediateResponse, completionHandler);
        return;
    }
    completionHandler(intermediateResponse);
}

} // namespace query_param
} // namespace redfish
