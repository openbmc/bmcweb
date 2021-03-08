#pragma once

#include "sensors.hpp"

namespace redfish
{
inline void requestRoutesThermalMetrics(App& app)
{
    BMCWEB_ROUTE(app,
                 "/redfish/v1/Chassis/<str>/ThermalSubsystem/ThermalMetrics/")
        .privileges({"Login"})
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& param) -> void {
                const std::string& chassisId = param;
                auto sensorAsyncResp = std::make_shared<SensorsAsyncResp>(
                    asyncResp, chassisId,
                    sensors::dbus::paths.at(sensors::node::thermal),
                    sensors::node::thermal);

                // TODO Need to get Chassis Redundancy information.
                getThermalData(sensorAsyncResp);
            });
}
} // namespace redfish