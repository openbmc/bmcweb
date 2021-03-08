#pragma once

#include "app.hpp"
#include "utils/chassis_utils.hpp"

#include <memory>
#include <optional>
#include <string>

namespace redfish
{
inline void
    doThermalMetrics(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
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
        "#ThermalMetrics.v1_0_1.ThermalMetrics";
    asyncResp->res.jsonValue["@odata.id"] =
        crow::utility::urlFromPieces("redfish", "v1", "Chassis", chassisId,
                                     "ThermalSubsystem", "ThermalMetrics");
    asyncResp->res.jsonValue["Id"] = "ThermalMetrics";
    asyncResp->res.jsonValue["Name"] = "Chassis Thermal Metrics";
    asyncResp->res.jsonValue["TemperatureReadingsCelsius"] =
        nlohmann::json::array();
}

inline void
    handleThermalMetricsGet(App& app, const crow::Request& req,
                            const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& chassisId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    redfish::chassis_utils::getValidChassisPath(
        asyncResp, chassisId,
        std::bind_front(doThermalMetrics, asyncResp, chassisId));
}

inline void requestRoutesThermalMetrics(App& app)
{
    BMCWEB_ROUTE(app,
                 "/redfish/v1/Chassis/<str>/ThermalSubsystem/ThermalMetrics/")
        .privileges(redfish::privileges::getThermalMetrics)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleThermalMetricsGet, std::ref(app)));
}
} // namespace redfish