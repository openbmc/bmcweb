#pragma once

#include "app.hpp"
#include "dbus_utility.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "sensors.hpp"
#include "utils/chassis_utils.hpp"
#include "utils/json_utils.hpp"
#include "utils/sensor_utils.hpp"

#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>

#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace redfish
{
inline void afterGetTemperatureValue(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& path,
    const boost::system::error_code& ec,
    const dbus::utility::DBusPropertiesMap& valuesDict)
{
    if (ec)
    {
        if (ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR("DBUS response error for getAllProperties {}",
                             ec.value());
            messages::internalError(asyncResp->res);
        }
        return;
    }

    nlohmann::json item = nlohmann::json::object();

    /* Don't return an error for a failure to fill in properties from any of
     * the sensors in the list. Just skip it.
     */
    if (sensor_utils::objectExcerptToJson(
            path, chassisId, sensor_utils::ChassisSubNode::thermalMetricsNode,
            "temperature", valuesDict, item))
    {
        nlohmann::json& temperatureReadings =
            asyncResp->res.jsonValue["TemperatureReadingsCelsius"];
        nlohmann::json::array_t* temperatureArray =
            temperatureReadings.get_ptr<nlohmann::json::array_t*>();
        if (temperatureArray == nullptr)
        {
            BMCWEB_LOG_ERROR("Missing TemperatureReadingsCelsius Json array");
            messages::internalError(asyncResp->res);
            return;
        }

        temperatureArray->emplace_back(std::move(item));
        asyncResp->res.jsonValue["TemperatureReadingsCelsius@odata.count"] =
            temperatureArray->size();

        json_util::sortJsonArrayByKey(*temperatureArray, "DataSourceUri");
    }
}

inline void handleTemperatureReadingsCelsius(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const boost::system::error_code& ec,
    const sensor_utils::SensorServicePathList& sensorsServiceAndPath)
{
    if (ec)
    {
        if (ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR("DBUS response error for getAssociatedSubTree {}",
                             ec.value());
            messages::internalError(asyncResp->res);
        }
        return;
    }

    asyncResp->res.jsonValue["TemperatureReadingsCelsius"] =
        nlohmann::json::array_t();
    asyncResp->res.jsonValue["TemperatureReadingsCelsius@odata.count"] = 0;

    for (const auto& [service, sensorPath] : sensorsServiceAndPath)
    {
        sdbusplus::asio::getAllProperties(
            *crow::connections::systemBus, service, sensorPath,
            "xyz.openbmc_project.Sensor.Value",
            [asyncResp, chassisId,
             sensorPath](const boost::system::error_code& ec1,
                         const dbus::utility::DBusPropertiesMap& properties) {
            afterGetTemperatureValue(asyncResp, chassisId, sensorPath, ec1,
                                     properties);
        });
    }
}

inline void getTemperatureReadingsCelsius(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& validChassisPath, const std::string& chassisId)
{
    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Sensor.Value"};

    sensor_utils::getAllSensorObjects(
        validChassisPath, "/xyz/openbmc_project/sensors/temperature",
        interfaces, 1,
        std::bind_front(handleTemperatureReadingsCelsius, asyncResp,
                        chassisId));
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
