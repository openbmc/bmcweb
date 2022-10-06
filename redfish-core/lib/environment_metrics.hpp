#pragma once

#include "app.hpp"
#include "utils/chassis_utils.hpp"

#include <memory>
#include <optional>
#include <string>

namespace redfish
{

inline void getEndPoints(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& path, const std::string& chassisId,
    std::function<void(const std::vector<std::string>&)>&& callback)
{
    sdbusplus::asio::getProperty<std::vector<std::string>>(
        *crow::connections::systemBus, "xyz.openbmc_project.ObjectMapper", path,
        "xyz.openbmc_project.Association", "endpoints",
        [asyncResp, chassisId, callback{std::move(callback)}](
            const boost::system::error_code ec,
            const std::vector<std::string>& endpoints) {
        if (ec)
        {
            if (ec.value() ==
                boost::system::linux_error::bad_request_descriptor)
            {
                messages::resourceNotFound(asyncResp->res, "Chassis",
                                           chassisId);
                return;
            }
            messages::internalError(asyncResp->res);
            return;
        }

        callback(endpoints);
        });
}

inline void
    updateFanSensorList(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& chassisId,
                        const std::string& fanSensorPath, double value)
{
    std::string fanSensorName =
        sdbusplus::message::object_path(fanSensorPath).filename();
    if (fanSensorName.empty())
    {
        return;
    }

    nlohmann::json item;
    item["DataSourceUri"] = crow::utility::urlFromPieces(
        "redfish", "v1", "Chassis", chassisId, "Sensors", fanSensorName);
    item["DeviceName"] = "Chassis Fan #" + fanSensorName;
    item["SpeedRPM"] = value;

    nlohmann::json& fanSensorList =
        asyncResp->res.jsonValue["FanSpeedsPercent"];
    fanSensorList.emplace_back(std::move(item));
    asyncResp->res.jsonValue["FanSpeedsPercent@odata.count"] =
        fanSensorList.size();
}

inline void getFanSensors(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& chassisId,
                          const std::string& service, const std::string& path)
{
    sdbusplus::asio::getProperty<double>(
        *crow::connections::systemBus, service, path,
        "xyz.openbmc_project.Sensor.Value", "Value",
        [asyncResp, chassisId, path](const boost::system::error_code ec,
                                     const double value) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG << "DBUS response error " << ec.message();
            messages::internalError(asyncResp->res);
            return;
        }

        updateFanSensorList(asyncResp, chassisId, path, value);
        });
}

inline void
    getFanSensorPaths(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const std::string& fanSensorPath,
                      const std::string& chassisId)
{
    auto respHandler =
        [asyncResp, chassisId, fanSensorPath](const std::string& service) {
        getFanSensors(asyncResp, chassisId, service, fanSensorPath);
    };
    std::vector<std::string> ifs{"xyz.openbmc_project.Sensor.Value"};
    dbus::utility::getDbusObject(asyncResp, fanSensorPath, ifs,
                                 std::move(respHandler));
}

inline void
    getFanSpeedsPercent(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& chassisPath,
                        const std::string& chassisId)
{
    std::string fanPath = chassisPath + "/cooled_by";
    auto respInventoryHandler =
        [asyncResp,
         chassisId](const std::vector<std::string>& inventoryEndpoints) {
        for (const auto& ivPoint : inventoryEndpoints)
        {
            std::string sensorPath = ivPoint + "/sensors";
            auto respSensorHandler =
                [asyncResp,
                 chassisId](const std::vector<std::string>& sensorEndpoints) {
                for (const auto& sensorPint : sensorEndpoints)
                {
                    getFanSensorPaths(asyncResp, sensorPint, chassisId);
                }
            };
            getEndPoints(asyncResp, sensorPath, chassisId,
                         std::move(respSensorHandler));
        }
    };
    getEndPoints(asyncResp, fanPath, chassisId,
                 std::move(respInventoryHandler));
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
        asyncResp->res.jsonValue["@odata.id"] = crow::utility::urlFromPieces(
            "redfish", "v1", "Chassis", chassisId, "EnvironmentMetrics");

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
