#pragma once

#include "utils/query_param.hpp"

#include <bmcweb_config.h>

namespace redfish
{

inline bool setUpRedfishRoute(crow::App& app, const crow::Request& req,
                              crow::Response& res)
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
    std::optional<query_param::Query> query =
        query_param::parseParameters(req.urlView.params(), res);
    if (query == std::nullopt)
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

    BMCWEB_LOG_DEBUG << "Setting completion handler for query";

    res.setCompleteRequestHandler([&app, handler(std::move(handler)),
                                   query](crow::Response& res) mutable {
        BMCWEB_LOG_DEBUG << "Starting query params handling";
        processAllParams(app, query, handler, res);
    });
    return true;
}
} // namespace redfish