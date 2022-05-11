#pragma once

#include <app.hpp>
#include <http_request.hpp>
#include <http_response.hpp>

#include <string>

namespace redfish
{

inline void requestRoutes(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/")
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                if (!redfish::setUpRedfishRoute(app, req, asyncResp->res))
                {
                    return;
                }
                asyncResp->res.jsonValue["v1"] = "/redfish/v1/";
            });
}
} // namespace redfish
