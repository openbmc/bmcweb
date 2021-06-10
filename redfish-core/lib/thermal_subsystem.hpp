#pragma once

#include <app.hpp>
#include <utils/chassis_utils.hpp>
#include <utils/json_utils.hpp>

namespace redfish
{

inline void doThermalSubsystemCollection(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::optional<std::string>& chassisPath)
{
    if (!chassisPath)
    {
        BMCWEB_LOG_ERROR << "Not a valid chassis ID" << chassisId;
        messages::resourceNotFound(asyncResp->res, "Chassis", chassisId);
        return;
    }
    asyncResp->res.jsonValue["@odata.type"] =
        "#ThermalSubsystem.v1_0_0.ThermalSubsystem";
    asyncResp->res.jsonValue["Name"] = "Thermal Subsystem";
    asyncResp->res.jsonValue["Id"] = "ThermalSubsystem";

    asyncResp->res.jsonValue["@odata.id"] =
        "/redfish/v1/Chassis/" + chassisId + "/ThermalSubsystem";

    asyncResp->res.jsonValue["Fans"] = {
        {"@odata.id",
         "/redfish/v1/Chassis/" + chassisId + "/ThermalSubsystem/Fans"}};

    asyncResp->res.jsonValue["Status"] = {{"State", "Enabled"},
                                          {"Health", "OK"}};
}

inline void requestRoutesThermalSubsystem(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/ThermalSubsystem/")
        .privileges({{"Login"}})
        // TODO: Use automated PrivilegeRegistry
        // Need to wait for Redfish to release a new registry
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& param) -> void {
                const std::string& chassisId = param;

                auto getChassisPath =
                    [asyncResp,
                     chassisId](const std::optional<std::string>& chassisPath) {
                        doThermalSubsystemCollection(asyncResp, chassisId,
                                                     chassisPath);
                    };

                redfish::chassis_utils::getValidChassisID(
                    asyncResp, chassisId, std::move(getChassisPath));
            });
}

} // namespace redfish
