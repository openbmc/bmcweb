#pragma once

#include "app.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "str_utility.hpp"
#include "utils/chassis_utils.hpp"
#include "utils/fan_utils.hpp"

#include <boost/url/format.hpp>
#include <nlohmann/json.hpp>

#include <array>
#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace redfish
{

inline void
    updateFanSensorList(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const boost::system::error_code& ec,
                        const std::string& chassisId,
                        const std::string& fanSensorPath,
                        const dbus::utility::DBusPropertiesMap& propertiesList)
{
    if (ec)
    {
        if (ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR("DBUS response error {}", ec.value());
            messages::internalError(asyncResp->res);
        }
        return;
    }

    std::string fanSensorId;
    redfish::fan_utils::getFanSensorId(fanSensorPath, std::ref(fanSensorId));
    if (fanSensorId.empty())
    {
        BMCWEB_LOG_DEBUG("Sensor path {} is invalid or not a fan",
                         fanSensorPath);
        return;
    }

    const double* value = nullptr;
    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), propertiesList, "Value", value);
    if (!success)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    if (value == nullptr)
    {
        BMCWEB_LOG_DEBUG("Fan Sensor {} missing RPM value", fanSensorPath);
        return;
    }
    if (*value < 0.0)
    {
        BMCWEB_LOG_ERROR("Incorrect D-Bus interface for sensor Value");
        messages::internalError(asyncResp->res);
        return;
    }

    nlohmann::json::object_t item;
    item["DataSourceUri"] = boost::urls::format(
        "/redfish/v1/Chassis/{}/Sensors/{}", chassisId, fanSensorId);

    if (!std::isfinite(*value))
    {
        // Readings are allowed to be NAN for unavailable;  coerce
        // them to null in the json response.
        item["SpeedRPM"] = nullptr;
    }
    else
    {
        item["SpeedRPM"] = *value;
    }

    nlohmann::json& fanSensorList =
        asyncResp->res.jsonValue["FanSpeedsPercent"];
    fanSensorList.emplace_back(std::move(item));
    asyncResp->res.jsonValue["FanSpeedsPercent@odata.count"] =
        fanSensorList.size();
}

inline void getFanSensorsProperties(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId,
    std::vector<std::pair<std::string, std::string>>& sensorsPathAndService)
{
    for (const auto& [service, sensorPath] : sensorsPathAndService)
    {
        sdbusplus::asio::getAllProperties(
            *crow::connections::systemBus, service, sensorPath,
            "xyz.openbmc_project.Sensor.Value",
            [asyncResp, chassisId, sensorPath](
                const boost::system::error_code& ec,
                const dbus::utility::DBusPropertiesMap& propertiesList) {
            updateFanSensorList(asyncResp, ec, chassisId, sensorPath,
                                propertiesList);
        });
    }
}

inline void afterGetFanSpeedsPercent(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId,
    const dbus::utility::MapperGetSubTreePathsResponse& fanPaths)
{
    for (const std::string& fanPath : fanPaths)
    {
        sdbusplus::message::object_path endpointPath{fanPath};

        redfish::fan_utils::getFanSensorObjects(
            asyncResp, endpointPath,
            std::bind_front(getFanSensorsProperties, asyncResp, chassisId));
    }
}

inline void
    getFanSpeedsPercent(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& validChassisPath,
                        const std::string& chassisId)
{
    redfish::fan_utils::getFanPaths(
        asyncResp, validChassisPath,
        std::bind_front(afterGetFanSpeedsPercent, asyncResp, chassisId));
}

inline void handleEnvironmentMetricsHead(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    auto respHandler = [asyncResp, chassisId](
                           const std::optional<std::string>& validChassisPath) {
        if (!validChassisPath)
        {
            messages::resourceNotFound(asyncResp->res, "Chassis", chassisId);
            return;
        }

        asyncResp->res.addHeader(
            boost::beast::http::field::link,
            "</redfish/v1/JsonSchemas/EnvironmentMetrics/EnvironmentMetrics.json>; rel=describedby");
    };

    redfish::chassis_utils::getValidChassisPath(asyncResp, chassisId,
                                                std::move(respHandler));
}

inline void handleEnvironmentMetricsGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    auto respHandler = [asyncResp, chassisId](
                           const std::optional<std::string>& validChassisPath) {
        if (!validChassisPath)
        {
            messages::resourceNotFound(asyncResp->res, "Chassis", chassisId);
            return;
        }

        asyncResp->res.addHeader(
            boost::beast::http::field::link,
            "</redfish/v1/JsonSchemas/EnvironmentMetrics/EnvironmentMetrics.json>; rel=describedby");
        asyncResp->res.jsonValue["@odata.type"] =
            "#EnvironmentMetrics.v1_3_0.EnvironmentMetrics";
        asyncResp->res.jsonValue["Name"] = "Chassis Environment Metrics";
        asyncResp->res.jsonValue["Id"] = "EnvironmentMetrics";
        asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
            "/redfish/v1/Chassis/{}/EnvironmentMetrics", chassisId);

        /* getValidFanSensorList - build sorted list of the fan sensors
         *      callback will process the list and get the property data for
         *      each sensor to add to member array
         */
        getFanSpeedsPercent(asyncResp, *validChassisPath, chassisId);
    };

    redfish::chassis_utils::getValidChassisPath(asyncResp, chassisId,
                                                std::move(respHandler));
}

inline void requestRoutesEnvironmentMetrics(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/EnvironmentMetrics/")
        .privileges(redfish::privileges::headEnvironmentMetrics)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handleEnvironmentMetricsHead, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/EnvironmentMetrics/")
        .privileges(redfish::privileges::getEnvironmentMetrics)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleEnvironmentMetricsGet, std::ref(app)));
}

} // namespace redfish
