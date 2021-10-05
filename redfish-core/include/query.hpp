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
    std::optional<query_param::Query> queryOpt =
        query_param::parseParameters(req.urlView.params(), res);
    if (queryOpt == std::nullopt)
    {
        return false;
    }

    std::function<void(crow::Response&)> handler =
        res.releaseCompleteRequestHandler();

    BMCWEB_LOG_DEBUG << "Function was valid: " << static_cast<bool>(handler);

    BMCWEB_LOG_DEBUG << "Setting completion handler for query";
    res.setCompleteRequestHandler(
        [&app, handler(std::move(handler)),
         query{*queryOpt}](crow::Response& res) mutable {
            BMCWEB_LOG_DEBUG << "Starting query params handling";
            processAllParams(app, query, res, handler);
        });
    return true;
}
} // namespace redfish