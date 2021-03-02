#pragma once

#include <app.hpp>
#include <utils/chassis_utils.hpp>
#include <utils/json_utils.hpp>

namespace redfish
{

inline void requestRoutesThermalSubsystem(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/ThermalSubsystem/")
        .privileges({"Login"})
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& param) -> void {
                if (param.empty())
                {
                    messages::internalError(asyncResp->res);
                    return;
                }
                const std::string& chassisId = param;

                auto getChassisPath =
                    [asyncResp,
                     chassisId](const std::optional<std::string>& chassisPath) {
                        if (!chassisPath)
                        {
                            BMCWEB_LOG_ERROR << "Not a valid chassis ID"
                                             << chassisId;
                            messages::resourceNotFound(asyncResp->res,
                                                       "Chassis", chassisId);
                            return;
                        }
                        asyncResp->res.jsonValue["@odata.type"] =
                            "#ThermalSubsystem.v1_0_0.ThermalSubsystem";
                        asyncResp->res.jsonValue["Name"] =
                            "Thermal Subsystem for Chassis";
                        asyncResp->res.jsonValue["Id"] = "ThermalSubsystem";

                        asyncResp->res.jsonValue["@odata.id"] =
                            "/redfish/v1/Chassis/" + chassisId +
                            "/ThermalSubsystem";

                        asyncResp->res.jsonValue["Status"] = {
                            {"State", "Enabled"}, {"Health", "OK"}};
                    };
                redfish::chassis_utils::getValidChassisID(
                    asyncResp, chassisId, std::move(getChassisPath));
            });
}

} // namespace redfish
