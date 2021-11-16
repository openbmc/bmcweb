#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

namespace redfish
{

inline void requestRoutesManagerDiagnosticData(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Managers/bmc/ManagerDiagnosticData")
        .privileges(redfish::privileges::getManagerCollection)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                asyncResp->res.jsonValue["@odata.type"] =
                    "#ManagerDiagnosticData.ManagerDiagnosticData";
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/Managers/bmc/ManagerDiagnosticData";
            });
}

}