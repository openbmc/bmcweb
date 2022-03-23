#pragma once

#include "utils/query_param.hpp"

#include <bmcweb_config.h>

namespace redfish
{

inline bool setUpRedfishRoute(crow::App& app, const crow::Request& req,
                              crow::Response& res)
{
    BMCWEB_LOG_DEBUG << "setup redfish route";

    // If query parameters aren't enabled, do nothing.
    if constexpr (bmcwebInsecureEnableQueryParams)
    {
        return true;
    }
    std::optional<query_param::Query> query =
        query_param::parseParameters(req.urlView.params(), res);

    // If this isn't a get, no need to do anything with parameters
    if (req.method() != boost::beast::http::verb::get)
    {
        return true;
    }

    std::function<void(crow::Response&)> handler =
        res.releaseCompleteRequestHandler();

    BMCWEB_LOG_DEBUG << "Setting completion handler for query";

    res.setCompleteRequestHandler([&app, handler(std::move(handler)),
                                   query](crow::Response& res) mutable {
        BMCWEB_LOG_DEBUG << "Starting query params handling";
        processAllParams(app, query, handler, res);
    });
    return true;
}
} // namespace redfish