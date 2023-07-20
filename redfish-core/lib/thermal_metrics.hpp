#pragma once

#include "app.hpp"
#include "dbus_utility.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "sensors.hpp"
#include "utils/chassis_utils.hpp"

#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>

#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace redfish
{
inline void afterGetTemperatureValue(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& path,
    const boost::system::error_code& ec, double val)
{
    if (ec)
    {
        if (ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR("handleTemperatureReadingsCelsius() failed: {}",
                             ec.value());
            messages::internalError(asyncResp->res);
        }
        return;
    }

    std::string sensorName = sdbusplus::message::object_path(path).filename();
    nlohmann::json& temperatureArray =
        asyncResp->res.jsonValue["TemperatureReadingsCelsius"];

    nlohmann::json::object_t item;
    item["@odata.id"] = boost::urls::format("/redfish/v1/Chassis/{}/Sensors/{}",
                                            chassisId, sensorName);
    item["DataSourceUri"] = boost::urls::format(
        "/redfish/v1/Chassis/{}/Sensors/{}", chassisId, sensorName);
    item["Reading"] = val;

    temperatureArray.emplace_back(std::move(item));
}

inline void handleTemperatureReadingsCelsius(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const dbus::utility::MapperGetSubTreeResponse& subtree,
    const std::string& chassisId)
{
    for (const auto& [path, serviceMap] : subtree)
    {
        for (const auto& [service, interfaces] : serviceMap)
        {
            sdbusplus::asio::getProperty<double>(
                *crow::connections::systemBus, service, path,
                "xyz.openbmc_project.Sensor.Value", "Value",
                std::bind_front(afterGetTemperatureValue, asyncResp, chassisId,
                                path));
        }
    }
}

inline void afterGetAssociatedSubTree(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& objects)
{
    if (ec == boost::system::errc::io_error)
    {
        asyncResp->res.jsonValue["TemperatureReadingsCelsius"] =
            nlohmann::json::array();
        return;
    }

    if (ec)
    {
        BMCWEB_LOG_ERROR("DBUS response error {}", ec.value());
        messages::internalError(asyncResp->res);
        return;
    }

    handleTemperatureReadingsCelsius(asyncResp, objects, chassisId);
}

inline void getTemperatureReadingsCelsius(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& validChassisPath, const std::string& chassisId)
{
    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Sensor.Value"};

    sdbusplus::message::object_path endpointPath{validChassisPath};
    endpointPath /= "all_sensors";

    dbus::utility::getAssociatedSubTree(
        endpointPath,
        sdbusplus::message::object_path(
            "/xyz/openbmc_project/sensors/temperature"),
        1, interfaces,
        std::bind_front(afterGetAssociatedSubTree, asyncResp, chassisId));
}

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

    getTemperatureReadingsCelsius(asyncResp, *validChassisPath, chassisId);
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
            messages::resourceNotFound(asyncResp->res, "Chassis", chassisId);
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
