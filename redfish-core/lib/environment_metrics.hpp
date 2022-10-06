#pragma once

#include "app.hpp"
#include "utils/chassis_utils.hpp"

#include <memory>
#include <optional>
#include <string>

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
                      const std::string& chassisPath,
                      const std::string& chassisId,
                      const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    for (const auto& [fanPath, serviceMap] : subtree)
    {
        for (const auto& [service, interfaces] : serviceMap)
        {
            if (std::find_if(interfaces.begin(), interfaces.end(),
                             [](const auto& i) {
                return i == "xyz.openbmc_project.Association.Definitions";
                }) == interfaces.end())
            {
                continue;
            }

            sdbusplus::asio::getProperty<
                dbus::utility::MapperGetAssociationResponse>(
                *crow::connections::systemBus, service, fanPath,
                "xyz.openbmc_project.Association.Definitions", "Associations",
                [asyncResp, chassisPath,
                 chassisId](const boost::system::error_code ec,
                            const dbus::utility::MapperGetAssociationResponse&
                                associations) {
                if (ec)
                {
                    BMCWEB_LOG_DEBUG << "DBUS response error " << ec.message();
                    messages::internalError(asyncResp->res);
                    return;
                }

                std::vector<std::string> fanSensorPaths;
                bool found = false;
                for (const auto& assoc : associations)
                {
                    const auto& [rType, tType, endpoint] = assoc;
                    if (rType == "sensors")
                    {
                        fanSensorPaths.push_back(endpoint);
                    }
                    else if (rType == "chassis" && endpoint == chassisPath)
                    {
                        found = true;
                    }
                }

                if (found)
                {
                    for (const auto& fanSensorPath : fanSensorPaths)
                    {
                        auto respHandler =
                            [asyncResp, chassisId,
                             fanSensorPath](const std::string& name) {
                            getFanSensors(asyncResp, chassisId, name,
                                          fanSensorPath);
                        };
                        std::vector<std::string> ifs{
                            "xyz.openbmc_project.Sensor.Value"};
                        dbus::utility::getDbusObject(asyncResp, fanSensorPath,
                                                     ifs,
                                                     std::move(respHandler));
                    }
                }
                });
        }
    }
}

inline void
    getFanSpeedsPercent(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& chassisPath,
                        const std::string& chassisId)
{
    auto respHandler =
        [asyncResp, chassisPath,
         chassisId](const boost::system::error_code ec,
                    const dbus::utility::MapperGetSubTreeResponse& subtree) {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "respHandler DBUS error: " << ec.message();
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

        getFanSensorPaths(asyncResp, chassisPath, chassisId, subtree);
    };

    crow::connections::systemBus->async_method_call(
        respHandler, "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", 0,
        std::array<const char*, 1>{"xyz.openbmc_project.Inventory.Item.Fan"});
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
