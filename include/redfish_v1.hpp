#pragma once

#include <app.hpp>

namespace redfish
{
inline void requestRoutes(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/")
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&, crow::Response& res) {
                auto asyncResp = std::make_shared<AsyncResp>(res);
                res.jsonValue = {{"v1", "/redfish/v1/"}};
            });
}
} // namespace redfish

