#pragma once

#include "utils/query_param.hpp"

namespace redfish
{

inline bool setUpRedfishRoute(crow::App& app, const crow::Request& req,
                              crow::Response& res)
{
    BMCWEB_LOG_DEBUG << "setup redfish route";
    std::optional<query_param::Query> query =
        query_param::parseParameters(req.urlParams, res);

    std::function<void(crow::Response&)> handler =
        res.releaseCompleteRequestHandler();

    BMCWEB_LOG_DEBUG << "Function was valid: " << static_cast<bool>(handler);

    BMCWEB_LOG_DEBUG << "Setting completion handler for query";
    res.setCompleteRequestHandler([&app, handler(std::move(handler)),
                                   query](crow::Response& res) mutable {
        BMCWEB_LOG_DEBUG << "Starting query params handling";
        processAllParams(app, query, res, handler);
    });
    return true;
}
} // namespace redfish