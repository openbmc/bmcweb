// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "app.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/chassis_utils.hpp"

#include <functional>
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
        messages::resourceNotFound(asyncResp->res, "Chassis", chassisId);
        return;
    }

    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/ThermalMetrics/ThermalMetrics.json>; rel=describedby");
    asyncResp->res.jsonValue["@odata.type"] =
        "#ThermalMetrics.v1_0_1.ThermalMetrics";
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Chassis/{}/ThermalSubsystem/ThermalMetrics", chassisId);
    asyncResp->res.jsonValue["Id"] = "ThermalMetrics";
    asyncResp->res.jsonValue["Name"] = "Thermal Metrics";
}

inline void handleThermalMetricsHead(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    redfish::chassis_utils::getValidChassisPath(
        asyncResp, chassisId,
        [asyncResp,
         chassisId](const std::optional<std::string>& validChassisPath) {
            if (!validChassisPath)
            {
                messages::resourceNotFound(asyncResp->res, "Chassis",
                                           chassisId);
                return;
            }
            asyncResp->res.addHeader(
                boost::beast::http::field::link,
                "</redfish/v1/JsonSchemas/ThermalMetrics/ThermalMetrics.json>; rel=describedby");
        });
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
        .privileges(redfish::privileges::headThermalMetrics)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handleThermalMetricsHead, std::ref(app)));

    BMCWEB_ROUTE(app,
                 "/redfish/v1/Chassis/<str>/ThermalSubsystem/ThermalMetrics/")
        .privileges(redfish::privileges::getThermalMetrics)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleThermalMetricsGet, std::ref(app)));
}
} // namespace redfish
