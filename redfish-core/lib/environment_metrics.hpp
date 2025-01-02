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

inline void getFanSensorsProperties(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& path,
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
        [asyncResp, chassisId,
         path](const boost::system::error_code& ec1, const double value) {
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

inline void getFanSensorValues(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const boost::system::error_code& ec,
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
        dbus::utility::getDbusObject(
            fanSensorPath, sensorInterface,
            std::bind_front(getFanSensorsProperties, asyncResp, chassisId,
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

inline void getFanSpeedsPercent(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& validChassisPath, const std::string& chassisId)
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

inline void afterGetPropertyForPowerWatts(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const boost::system::error_code& ec,
    double value)
{
    if (ec)
    {
        if (ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR("Can't get Power Watts! {}", ec.value());
            messages::internalError(asyncResp->res);
        }
        return;
    }
    asyncResp->res.jsonValue["PowerWatts"]["@odata.id"] = boost::urls::format(
        "/redfish/v1/Chassis/{}/Sensors/power_total_power", chassisId);
    asyncResp->res.jsonValue["PowerWatts"]["DataSourceUri"] =
        boost::urls::format("/redfish/v1/Chassis/{}/Sensors/power_total_power",
                            chassisId);
    asyncResp->res.jsonValue["PowerWatts"]["Reading"] = value;
}

inline void afterGetDbusObjectForPowerWatts(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const boost::system::error_code& ec,
    const dbus::utility::MapperGetObject& object)
{
    if (ec || object.empty())
    {
        BMCWEB_LOG_ERROR("DBUS response error: {}", ec.value());
        messages::internalError(asyncResp->res);
        return;
    }

    sdbusplus::asio::getProperty<double>(
        *crow::connections::systemBus, object.begin()->first,
        "/xyz/openbmc_project/sensors/power/total_power",
        "xyz.openbmc_project.Sensor.Value", "Value",
        std::bind_front(afterGetPropertyForPowerWatts, asyncResp, chassisId));
}

inline void afterGetPowerWatts(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreePathsResponse& paths)
{
    if (ec)
    {
        if (ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR("DBUS response error for getSubTreePaths: {}",
                             ec.value());
            messages::internalError(asyncResp->res);
        }
        return;
    }

    auto find = std::ranges::find_if(paths, [](const std::string& tempPath) {
        sdbusplus::message::object_path path(tempPath);
        std::string leaf = path.filename();
        return leaf == "total_power";
    });
    if (find == paths.end())
    {
        BMCWEB_LOG_DEBUG("There is no total_power");
        return;
    }

    dbus::utility::getDbusObject(
        "/xyz/openbmc_project/sensors/power/total_power", {},
        std::bind_front(afterGetDbusObjectForPowerWatts, asyncResp, chassisId));
}

inline void getPowerWatts(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& chassisId)
{
    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Sensor.Value"};
    dbus::utility::getSubTreePaths(
        "/xyz/openbmc_project/sensors", 0, interfaces,
        std::bind_front(afterGetPowerWatts, asyncResp, chassisId));
}

inline void setPowerSetPoint(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp, uint32_t powerCap)
{
    BMCWEB_LOG_DEBUG("Set Power Limit Watts Set Point");

    sdbusplus::asio::setProperty(
        *crow::connections::systemBus, "xyz.openbmc_project.Settings",
        "/xyz/openbmc_project/control/host0/power_cap",
        "xyz.openbmc_project.Control.Power.Cap", "PowerCap", powerCap,
        [asyncResp](const boost::system::error_code& ec) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("Failed to set PowerCap: {}", ec.value());
                messages::internalError(asyncResp->res);
                return;
            }
        });
}

inline void
    setPowerControlMode(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& controlMode)
{
    BMCWEB_LOG_DEBUG("Set Power Limit Watts Control Mode");
    bool powerCapEnable = false;
    if (controlMode == "Disabled")
    {
        powerCapEnable = false;
    }
    else if (controlMode == "Automatic")
    {
        powerCapEnable = true;
    }
    else
    {
        BMCWEB_LOG_WARNING("Power Control Mode  does not support this mode: {}",
                           controlMode);
        messages::propertyValueNotInList(asyncResp->res, controlMode,
                                         "ControlMode");
        return;
    }

    sdbusplus::asio::setProperty(
        *crow::connections::systemBus, "xyz.openbmc_project.Settings",
        "/xyz/openbmc_project/control/host0/power_cap",
        "xyz.openbmc_project.Control.Power.Cap", "PowerCapEnable",
        powerCapEnable, [asyncResp](const boost::system::error_code& ec) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("Failed to set PowerCapEnable: {}",
                                 ec.value());
                messages::internalError(asyncResp->res);
                return;
            }
        });
}

inline void
    getPowerLimitWatts(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, "org.open_power.OCC.Control",
        "/xyz/openbmc_project/control/host0/power_cap_limits",
        "xyz.openbmc_project.Control.Power.CapLimits",
        [asyncResp](const boost::system::error_code& ec,
                    const dbus::utility::DBusPropertiesMap& propertiesList) {
            if (ec)
            {
                if (ec.value() != EBADR)
                {
                    BMCWEB_LOG_ERROR("DBUS response error: {}", ec.value());
                    messages::internalError(asyncResp->res);
                }
                return;
            }

            const uint32_t* minCap = nullptr;
            const uint32_t* maxCap = nullptr;
            const bool success = sdbusplus::unpackPropertiesNoThrow(
                dbus_utils::UnpackErrorPrinter(), propertiesList,
                "MinPowerCapValue", minCap, "MaxPowerCapValue", maxCap);
            if (!success)
            {
                messages::internalError(asyncResp->res);
                return;
            }

            if (minCap != nullptr)
            {
                asyncResp->res.jsonValue["PowerLimitWatts"]["AllowableMin"] =
                    *minCap;
            }
            if (maxCap != nullptr)
            {
                asyncResp->res.jsonValue["PowerLimitWatts"]["AllowableMax"] =
                    *maxCap;
            }
        });

    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, "xyz.openbmc_project.Settings",
        "/xyz/openbmc_project/control/host0/power_cap",
        "xyz.openbmc_project.Control.Power.Cap",
        [asyncResp](const boost::system::error_code& ec,
                    const dbus::utility::DBusPropertiesMap& propertiesList) {
            if (ec)
            {
                if (ec.value() != EBADR)
                {
                    BMCWEB_LOG_ERROR("DBUS response error: {}", ec.value());
                    messages::internalError(asyncResp->res);
                }
                return;
            }

            asyncResp->res.jsonValue["PowerLimitWatts"]["SetPoint"] = 0;
            asyncResp->res.jsonValue["PowerLimitWatts"]["ControlMode"] =
                "Automatic";

            const uint32_t* powerCap = nullptr;
            const bool* powerCapEnable = nullptr;

            const bool success = sdbusplus::unpackPropertiesNoThrow(
                dbus_utils::UnpackErrorPrinter(), propertiesList, "PowerCap",
                powerCap, "PowerCapEnable", powerCapEnable);

            if (!success)
            {
                messages::internalError(asyncResp->res);
                return;
            }

            if (powerCap != nullptr)
            {
                asyncResp->res.jsonValue["PowerLimitWatts"]["SetPoint"] =
                    *powerCap;
            }

            if (powerCapEnable != nullptr && !*powerCapEnable)
            {
                asyncResp->res.jsonValue["PowerLimitWatts"]["ControlMode"] =
                    "Disabled";
            }
        });
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
        getPowerWatts(asyncResp, chassisId);
        getPowerLimitWatts(asyncResp);
    };

    redfish::chassis_utils::getValidChassisPath(asyncResp, chassisId,
                                                std::move(respHandler));
}

inline void handleEnvironmentMetricsPatch(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    std::optional<uint32_t> setPoint;
    std::optional<std::string> controlMode;
    if (!json_util::readJsonPatch(req, asyncResp->res,
                                  "PowerLimitWatts/SetPoint", setPoint,
                                  "PowerLimitWatts/ControlMode", controlMode))
    {
        return;
    }

    redfish::chassis_utils::getValidChassisPath(
        asyncResp, chassisId,
        [asyncResp, chassisId, setPoint,
         controlMode](const std::optional<std::string>& validChassisPath) {
            if (!validChassisPath)
            {
                messages::resourceNotFound(asyncResp->res, "Chassis",
                                           chassisId);
                return;
            }

            if (setPoint)
            {
                setPowerSetPoint(asyncResp, *setPoint);
            }

            if (controlMode)
            {
                setPowerControlMode(asyncResp, *controlMode);
            }
        });
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

    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/EnvironmentMetrics/")
        .privileges(redfish::privileges::patchEnvironmentMetrics)
        .methods(boost::beast::http::verb::patch)(
            std::bind_front(handleEnvironmentMetricsPatch, std::ref(app)));
}

} // namespace redfish
