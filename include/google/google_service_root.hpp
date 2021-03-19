#pragma once

#include <app.hpp>
#include <async_resp.hpp>
#include <nlohmann/json.hpp>

namespace crow
{
namespace google_api
{

inline void requestRoutes(App& app)
{
    BMCWEB_ROUTE(app, "/google/v1/")
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                asyncResp->res.jsonValue["@odata.type"] =
                    "#GoogleServiceRoot.v1_0_0.GoogleServiceRoot";
                asyncResp->res.jsonValue["@odata.id"] = "/google/v1";
                asyncResp->res.jsonValue["Id"] = "Google Rest RootService";
                asyncResp->res.jsonValue["Name"] = "Google Service Root";
                asyncResp->res.jsonValue["Version"] = "1.0.0";
            });
}

} // namespace google_api
} // namespace crow
