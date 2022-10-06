// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_singleton.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/chassis_utils.hpp"
#include "utils/fan_utils.hpp"
#include "utils/sensor_utils.hpp"

#include <asm-generic/errno.h>

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/message/native_types.hpp>

#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace redfish
{

inline void afterGetPowerWatts(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& path,
    const boost::system::error_code& ec,
    const dbus::utility::DBusPropertiesMap& valuesDict)
{
    if (ec)
    {
        if (ec != boost::system::errc::io_error)
        {
            BMCWEB_LOG_ERROR("DBUS response error for PowerWatts {}", ec);
            messages::internalError(asyncResp->res);
        }
        return;
    }

    nlohmann::json item = nlohmann::json::object();

    /* Don't return an error for a failure to fill in properties from the
     * single sensor. Just skip adding it.
     */
    if (sensor_utils::objectExcerptToJson(
            path, chassisId,
            sensor_utils::ChassisSubNode::environmentMetricsNode, "power",
            valuesDict, item))
    {
        asyncResp->res.jsonValue["PowerWatts"] = std::move(item);
    }
}

inline void handleTotalPowerList(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const boost::system::error_code& ec,
    const std::shared_ptr<sensor_utils::SensorServicePathList>& sensorList)
{
    BMCWEB_LOG_DEBUG("handleTotalPowerList: {}", sensorList->size());

    if (ec)
    {
        if (ec != boost::system::errc::io_error)
        {
            BMCWEB_LOG_ERROR("D-Bus response error {}", ec);
            messages::internalError(asyncResp->res);
        }
        return;
    }

    // TotalPower cannot be supplied by multiple sensors
    if (sensorList->size() != 1)
    {
        if (sensorList->empty())
        {
            // None found, not an error
            return;
        }
        BMCWEB_LOG_ERROR("Too many total power sensors found {}. Expected 1.",
                         sensorList->size());
        messages::internalError(asyncResp->res);
        return;
    }

    const std::string& serviceName = (*sensorList)[0].first;
    const std::string& sensorPath = (*sensorList)[0].second;
    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, serviceName, sensorPath,
        "xyz.openbmc_project.Sensor.Value",
        [asyncResp, chassisId,
         sensorPath](const boost::system::error_code& ec1,
                     const dbus::utility::DBusPropertiesMap& propertiesList) {
            afterGetPowerWatts(asyncResp, chassisId, sensorPath, ec1,
                               propertiesList);
        });
}

inline void getTotalPowerSensor(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const boost::system::error_code& ec,
    const sensor_utils::SensorServicePathList& sensorsServiceAndPath)
{
    BMCWEB_LOG_DEBUG("getTotalPowerSensor {}", sensorsServiceAndPath.size());

    if (ec)
    {
        if (ec != boost::system::errc::io_error)
        {
            BMCWEB_LOG_ERROR("DBUS response error {}", ec);
            messages::internalError(asyncResp->res);
        }
        // None found, not an error
        return;
    }

    if (sensorsServiceAndPath.empty())
    {
        // No power sensors implement Sensor.Purpose, not an error
        return;
    }

    // Create vector to hold list of sensors with totalPower purpose
    std::shared_ptr<sensor_utils::SensorServicePathList> sensorList =
        std::make_shared<sensor_utils::SensorServicePathList>();

    sensor_utils::getSensorsByPurpose(
        asyncResp, sensorsServiceAndPath,
        sensor_utils::SensorPurpose::totalPower, sensorList,
        std::bind_front(handleTotalPowerList, asyncResp, chassisId));
}

/**
 * @brief Find sensor providing totalPower and fill in response
 *
 * Multiple D-Bus calls are needed to find the sensor providing the totalPower
 * details:
 *
 * 1. Retrieve list of power sensors associated with specified chassis which
 * implement the Sensor.Purpose interface.
 *
 * 2. For each of those power sensors retrieve the actual purpose of the sensor
 * to find the sensor implementing totalPower purpose. Expect no more than
 * one sensor to implement this purpose.
 *
 * 3. If a totalPower sensor is found then retrieve its properties to fill in
 * PowerWatts in the response.
 *
 * @param asyncResp Response data
 * @param validChassisPath Path to chassis, caller confirms path is valid
 * @param chassisId Chassis id matching <validChassisPath>
 */
inline void getPowerWatts(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& validChassisPath,
                          const std::string& chassisId)
{
    BMCWEB_LOG_DEBUG("getPowerWatts: {}", validChassisPath);

    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Sensor.Purpose"};
    sensor_utils::getAllSensorObjects(
        validChassisPath, "/xyz/openbmc_project/sensors/power", interfaces, 1,
        std::bind_front(getTotalPowerSensor, asyncResp, chassisId));
}

inline void handleGetFanSensorsExcerpt(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& fanSensorPath,
    const boost::system::error_code& ec,
    const dbus::utility::DBusPropertiesMap& propertiesList)
{
    if (ec)
    {
        if (ec.value() != boost::system::errc::io_error && ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR("DBUS response error {}", ec);
            messages::internalError(asyncResp->res);
        }
        return;
    }

    nlohmann::json item = nlohmann::json::object();

    /* Don't return an error for a failure to fill in properties from this
     * sensor. Just skip adding it.
     */
    if (sensor_utils::objectExcerptToJson(
            fanSensorPath, chassisId,
            sensor_utils::ChassisSubNode::environmentMetricsNode, "fan_tach",
            propertiesList, item))
    {
        nlohmann::json& fanSensorList =
            asyncResp->res.jsonValue["FanSpeedsPercent"];
        fanSensorList.emplace_back(std::move(item));
        asyncResp->res.jsonValue["FanSpeedsPercent@odata.count"] =
            fanSensorList.size();
    }
}

inline void getFanSensorsExcerpt(
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
                handleGetFanSensorsExcerpt(asyncResp, chassisId, sensorPath, ec,
                                           propertiesList);
            });
    }
}

inline void afterGetFanSpeedsPercent(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId,
    const dbus::utility::MapperGetSubTreePathsResponse& fanPaths)
{
    if (!fanPaths.empty())
    {
        // Have fans, initialize the array
        asyncResp->res.jsonValue["FanSpeedsPercent"] = nlohmann::json::array();
        asyncResp->res.jsonValue["FanSpeedsPercent@odata.count"] = 0;
    }

    for (const std::string& fanPath : fanPaths)
    {
        sdbusplus::message::object_path endpointPath{fanPath};

        fan_utils::getFanItemsSensors(
            asyncResp, endpointPath,
            std::bind_front(getFanSensorsExcerpt, asyncResp, chassisId));
    }
}

inline void getFanSpeedsPercent(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& validChassisPath, const std::string& chassisId)
{
    fan_utils::getFanPaths(
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

inline void doEnvironmentMetricsGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
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
        "</redfish/v1/JsonSchemas/EnvironmentMetrics/EnvironmentMetrics.json>; rel=describedby");
    asyncResp->res.jsonValue["@odata.type"] =
        "#EnvironmentMetrics.v1_3_0.EnvironmentMetrics";
    asyncResp->res.jsonValue["Name"] = "Chassis Environment Metrics";
    asyncResp->res.jsonValue["Id"] = "EnvironmentMetrics";
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Chassis/{}/EnvironmentMetrics", chassisId);

    getPowerWatts(asyncResp, *validChassisPath, chassisId);
    getFanSpeedsPercent(asyncResp, *validChassisPath, chassisId);
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

    redfish::chassis_utils::getValidChassisPath(
        asyncResp, chassisId,
        std::bind_front(doEnvironmentMetricsGet, asyncResp, chassisId));
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
