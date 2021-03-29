#pragma once

#include <app.hpp>

namespace crow
{
namespace redfish
{
inline void requestRoutes(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/")
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                asyncResp->res.jsonValue = {{"v1", "/redfish/v1/"}};
            });
}
} // namespace redfish
} // namespace crow
