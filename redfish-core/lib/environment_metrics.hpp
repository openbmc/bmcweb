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

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace redfish
{
inline void
    updateFanSensorList(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& chassisId,
                        const std::string& fanSensorPath, double value)
{
    std::string fanSensorName =
        sdbusplus::message::object_path(fanSensorPath).filename();
    if (fanSensorName.empty())
    {
        BMCWEB_LOG_ERROR("Fan Sensor name is empty and invalid");
        messages::internalError(asyncResp->res);
        return;
    }

    nlohmann::json::object_t item;
    item["DataSourceUri"] = boost::urls::format(
        "/redfish/v1/Chassis/{}/Sensors/{}", chassisId, fanSensorName);
    item["DeviceName"] = "Chassis #" + fanSensorName;
    item["SpeedRPM"] = value;

    nlohmann::json& fanSensorList =
        asyncResp->res.jsonValue["FanSpeedsPercent"];
    fanSensorList.emplace_back(std::move(item));
    std::sort(fanSensorList.begin(), fanSensorList.end(),
              [](const nlohmann::json& c1, const nlohmann::json& c2) {
        return c1["DataSourceUri"] < c2["DataSourceUri"];
    });
    asyncResp->res.jsonValue["FanSpeedsPercent@odata.count"] =
        fanSensorList.size();
}

inline void
    getFanSensorsProperties(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& chassisId,
                            const std::string& path,
                            const boost::system::error_code& ec,
                            const dbus::utility::MapperGetObject& object)
{
    if (ec || object.empty())
    {
        BMCWEB_LOG_ERROR("DBUS response error {}", ec.value());
        messages::internalError(asyncResp->res);
        return;
    }

    const std::string& service = object.begin()->first;
    sdbusplus::asio::getProperty<double>(
        *crow::connections::systemBus, service, path,
        "xyz.openbmc_project.Sensor.Value", "Value",
        [asyncResp, chassisId, path](const boost::system::error_code& ec1,
                                     const double value) {
        if (ec1)
        {
            if (ec1.value() != EBADR)
            {
                BMCWEB_LOG_ERROR("DBUS response error {}", ec1.value());
                messages::internalError(asyncResp->res);
            }
            return;
        }

        updateFanSensorList(asyncResp, chassisId, path, value);
    });
}

inline void
    getFanSensorValues(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& chassisId,
                       const boost::system::error_code& ec,
                       const dbus::utility::MapperEndPoints& fanSensorPaths)
{
    constexpr std::array<std::string_view, 1> sensorInterface = {
        "xyz.openbmc_project.Sensor.Value"};

    if (ec)
    {
        if (ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR("DBUS response error {}", ec.value());
            messages::internalError(asyncResp->res);
            return;
        }
    }

    for (const auto& fanSensorPath : fanSensorPaths)
    {
        dbus::utility::getDbusObject(fanSensorPath, sensorInterface,
                                     std::bind_front(getFanSensorsProperties,
                                                     asyncResp, chassisId,
                                                     fanSensorPath));
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
        endpointPath /= "sensors";

        dbus::utility::getAssociationEndPoints(
            endpointPath,
            std::bind_front(getFanSensorValues, asyncResp, chassisId));
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
