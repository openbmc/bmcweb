#pragma once

#include <utils/fw_utils.hpp>
namespace redfish
{
/**
 * BiosService class supports handle get method for bios.
 */
inline void requestRoutesBiosService(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/Bios/")
        .privileges({"Login"})
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/Systems/system/Bios";
                asyncResp->res.jsonValue["@odata.type"] = "#Bios.v1_1_0.Bios";
                asyncResp->res.jsonValue["Name"] = "BIOS Configuration";
                asyncResp->res.jsonValue["Description"] =
                    "BIOS Configuration Service";
                asyncResp->res.jsonValue["Id"] = "BIOS";
                asyncResp->res.jsonValue["Actions"]["#Bios.ResetBios"] = {
                    {"target",
                     "/redfish/v1/Systems/system/Bios/Actions/Bios.ResetBios"}};

                // Get the ActiveSoftwareImage and SoftwareImages
                fw_util::populateFirmwareInformation(
                    asyncResp, fw_util::biosPurpose, "", true);
            });
}
/**
 * BiosReset class supports handle POST method for Reset bios.
 * The class retrieves and sends data directly to D-Bus.
 *
 * Function handles POST method request.
 * Analyzes POST body message before sends Reset request data to D-Bus.
 */

inline void requestRoutesBiosReset(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/Bios/Actions/Bios.ResetBios/")
        .privileges({"ConfigureManager"})
        .methods(boost::beast::http::verb::post)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                crow::connections::systemBus->async_method_call(
                    [asyncResp](const boost::system::error_code ec) {
                        if (ec)
                        {
                            BMCWEB_LOG_ERROR << "Failed to reset bios: " << ec;
                            messages::internalError(asyncResp->res);
                            return;
                        }
                    },
                    "org.open_power.Software.Host.Updater",
                    "/xyz/openbmc_project/software",
                    "xyz.openbmc_project.Common.FactoryReset", "Reset");
            });
}
} // namespace redfish
