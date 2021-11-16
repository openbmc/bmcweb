#pragma once

#include <app.hpp>
#include <async_resp.hpp>
#include <http_request.hpp>
#include <nlohmann/json.hpp>
#include <privileges.hpp>
#include <routing.hpp>

#include <string>

namespace redfish
{

inline void requestRoutesManagerDiagnosticData(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Managers/bmc/ManagerDiagnosticData")
        .privileges(redfish::privileges::getManagerDiagnosticData)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request& /*req*/,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
        asyncResp->res.jsonValue["@odata.type"] =
            "#ManagerDiagnosticData.v1_0_0.ManagerDiagnosticData";
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Managers/bmc/ManagerDiagnosticData";
        const std::string& idAndName = "BMC's ManagerDiagnosticData";
        asyncResp->res.jsonValue["Id"] = idAndName;
        asyncResp->res.jsonValue["Name"] = idAndName;
        });
}

} // namespace redfish
