#pragma once

#include <app.hpp>
#include <rf_async_resp.hpp>
#include <http_request.hpp>
#include <http_response.hpp>

#include <string>

namespace redfish
{

inline void requestRoutes(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/")
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&, crow::Response& res) {
                std::shared_ptr<AsyncResp> asyncResp =
                    std::make_shared<AsyncResp>(res);
                res.jsonValue = {{"v1", "/redfish/v1/"}};
            });
}
} // namespace redfish
