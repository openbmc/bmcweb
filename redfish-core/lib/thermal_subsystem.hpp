#pragma once

#include <app.hpp>
#include <registries/privilege_registry.hpp>
#include <utils/json_utils.hpp>

namespace redfish
{

inline void doThermalSubsystemCollection(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId,
    const std::optional<std::string>& validChassisPath)
{
    if (!validChassisPath)
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
        crow::utility::urlFromPieces("redfish", "v1", "Chassis", chassisId,
                                     "ThermalSubsystem")
            .string();

    asyncResp->res.jsonValue["Status"]["State"] = "Enabled";
    asyncResp->res.jsonValue["Status"]["Health"] = "OK";
}

inline void handleThermalSubsystemCollectionGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& param)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    const std::string& chassisId = param;
    auto sensorAsyncResp = std::make_shared<SensorsAsyncResp>(
        asyncResp, chassisId, sensors::dbus::sensorPaths,
        sensors::node::sensors);

    getValidChassisPath(
        sensorAsyncResp,
        [asyncResp,
         chassisId](const std::optional<std::string>& validChassisPath) {
        doThermalSubsystemCollection(asyncResp, chassisId, validChassisPath);
        });
}

inline void requestRoutesThermalSubsystem(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/ThermalSubsystem/")
        .privileges(redfish::privileges::getThermalSubsystem)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleThermalSubsystemCollectionGet, std::ref(app)));
}

} // namespace redfish
