#pragma once

#include "utils/query_param.hpp"

#include <bmcweb_config.h>

namespace redfish
{

[[nodiscard]] inline bool setUpRedfishRoute(crow::App& app,
                                            const crow::Request& req,
                                            crow::Response& res)
{
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

    std::function<void(crow::Response&)> handler =
        res.releaseCompleteRequestHandler();

    res.setCompleteRequestHandler(
        [&app, handler(std::move(handler)),
         query{*queryOpt}](crow::Response& res) mutable {
            processAllParams(app, query, handler, res);
        });
    return true;
}
} // namespace redfish
