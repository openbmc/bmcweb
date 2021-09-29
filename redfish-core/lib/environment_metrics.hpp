#pragma once

#include "app.hpp"
#include "dbus_utility.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/chassis_utils.hpp"
#include "utils/json_utils.hpp"

#include <boost/url/format.hpp>

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace redfish
{

/**
 * @brief Returns the single path and service for PowerLimits
 * @param subtree Response from getSubTree()
 * @return Pair of path and service. On any error an empty pair is returned.
 */
inline std::pair<std::string, std::string>
    getPowerLimitsPath(const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    std::pair<std::string, std::string> pathAndService;

    /* This is a system-wide setting. Should be at most one path and service. */
    if (subtree.size() != 1)
    {
        BMCWEB_LOG_DEBUG("Unexpected number paths found {}", subtree.size());
    }
    else
    {
        const auto& object = subtree[0];
        const auto& serviceMap = object.second;

        if (serviceMap.size() != 1)
        {
            BMCWEB_LOG_DEBUG("Unexpected number serviceMaps found {}",
                             serviceMap.size());
        }
        else
        {
            pathAndService.first = object.first;
            pathAndService.second = serviceMap[0].first;
        }
    }

    return pathAndService;
}

inline void handleSetPowerSetPoint(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp, uint32_t powerCap,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec)
    {
        if (ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR("DBUS response error for PowerLimitWatts {}", ec);
            messages::internalError(asyncResp->res);
        }
        return;
    }

    auto pathAndService = getPowerLimitsPath(subtree);
    if (pathAndService.first.empty() || pathAndService.second.empty())
    {
        if (subtree.empty())
        {
            messages::propertyValueNotInList(asyncResp->res, "SetPoint",
                                             "EnvironmentMetrics");
            return;
        }
        BMCWEB_LOG_ERROR("Failed to get path and service for Power SetPoint");
        messages::internalError(asyncResp->res);
        return;
    }

    sdbusplus::asio::setProperty(
        *crow::connections::systemBus, pathAndService.second,
        pathAndService.first, "xyz.openbmc_project.Control.Power.Cap",
        "PowerCap", powerCap,
        [asyncResp](const boost::system::error_code& ec1) {
            if (ec1)
            {
                if (ec1.value() != EBADR)
                {
                    BMCWEB_LOG_ERROR("Failed to set PowerCap {}", ec1);
                    messages::internalError(asyncResp->res);
                }
                return;
            }
        });
}

inline void setPowerSetPoint(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp, uint32_t powerCap)
{
    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Control.Power.Cap"};

    BMCWEB_LOG_DEBUG("Set Power Limit Watts Set Point");
    dbus::utility::getSubTree(
        "/xyz/openbmc_project/control", 0, interfaces,
        std::bind_front(handleSetPowerSetPoint, asyncResp, powerCap));
}

inline void handleSetPowerControlMode(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& controlMode, const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec)
    {
        if (ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR("DBUS response error for PowerLimitWatts {}", ec);
            messages::internalError(asyncResp->res);
        }
        return;
    }

    auto pathAndService = getPowerLimitsPath(subtree);
    if (pathAndService.first.empty() || pathAndService.second.empty())
    {
        if (subtree.empty())
        {
            messages::propertyValueNotInList(asyncResp->res, "ControlMode",
                                             "EnvironmentMetrics");
            return;
        }
        BMCWEB_LOG_ERROR(
            "Failed to get path and service for Power ControlMode");
        messages::internalError(asyncResp->res);
        return;
    }

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
        BMCWEB_LOG_DEBUG("Power Control Mode does not support this mode : {}",
                         controlMode);
        messages::propertyValueNotInList(asyncResp->res, controlMode,
                                         "ControlMode");
        return;
    }

    sdbusplus::asio::setProperty(
        *crow::connections::systemBus, pathAndService.second,
        pathAndService.first, "xyz.openbmc_project.Control.Power.Cap",
        "PowerCapEnable", powerCapEnable,
        [asyncResp](const boost::system::error_code& ec1) {
            if (ec1)
            {
                if (ec1.value() != EBADR)
                {
                    BMCWEB_LOG_ERROR("Failed to set PowerCapEnable {}", ec1);
                    messages::internalError(asyncResp->res);
                }
                return;
            }
        });
}

inline void
    setPowerControlMode(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& controlMode)
{
    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Control.Power.Cap"};

    BMCWEB_LOG_DEBUG("Set Power Limit Watts Control Mode");
    dbus::utility::getSubTree(
        "/xyz/openbmc_project/control", 0, interfaces,
        std::bind_front(handleSetPowerControlMode, asyncResp, controlMode));
}

inline void afterGetPowerLimitWatts(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const dbus::utility::DBusPropertiesMap& propertiesList)
{
    if (ec)
    {
        if (ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR("Failed to get PowerLimitWatts {}", ec);
            messages::internalError(asyncResp->res);
        }
        return;
    }

    const uint32_t* powerCap = nullptr;
    bool powerCapEnable = false;
    const uint32_t* minCap = nullptr;
    const uint32_t* maxCap = nullptr;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), propertiesList, "PowerCap", powerCap,
        "PowerCapEnable", powerCapEnable, "MinPowerCapValue", minCap,
        "MaxPowerCapValue", maxCap);

    if (!success)
    {
        BMCWEB_LOG_ERROR("Failed to get properties for PowerLimitWatts");
        messages::internalError(asyncResp->res);
        return;
    }

    if (powerCap != nullptr)
    {
        asyncResp->res.jsonValue["PowerLimitWatts"]["SetPoint"] = *powerCap;
    }

    if (powerCapEnable)
    {
        asyncResp->res.jsonValue["PowerLimitWatts"]["ControlMode"] =
            "Automatic";
    }
    else
    {
        asyncResp->res.jsonValue["PowerLimitWatts"]["ControlMode"] = "Disabled";
    }

    if (minCap != nullptr)
    {
        asyncResp->res.jsonValue["PowerLimitWatts"]["AllowableMin"] = *minCap;
    }

    if (maxCap != nullptr)
    {
        asyncResp->res.jsonValue["PowerLimitWatts"]["AllowableMax"] = *maxCap;
    }
}

inline void handleGetPowerLimitWatts(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec)
    {
        if (ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR("DBUS response error for PowerLimitWatts {}", ec);
            messages::internalError(asyncResp->res);
        }
        return;
    }

    auto pathAndService = getPowerLimitsPath(subtree);
    if (pathAndService.first.empty() || pathAndService.second.empty())
    {
        BMCWEB_LOG_DEBUG("Failed to get path and service for Power Cap");
        return;
    }

    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, pathAndService.second,
        pathAndService.first, "xyz.openbmc_project.Control.Power.Cap",
        [asyncResp](const boost::system::error_code& ec1,
                    const dbus::utility::DBusPropertiesMap& propertiesList) {
            afterGetPowerLimitWatts(asyncResp, ec1, propertiesList);
        });
}

inline void
    getPowerLimitWatts(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Control.Power.Cap"};

    dbus::utility::getSubTree(
        "/xyz/openbmc_project/control", 0, interfaces,
        std::bind_front(handleGetPowerLimitWatts, asyncResp));
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

    std::optional<nlohmann::json> powerLimitWatts;
    if (!json_util::readJsonPatch(req, asyncResp->res, "PowerLimitWatts",
                                  powerLimitWatts))
    {
        return;
    }

    if (!powerLimitWatts)
    {
        return;
    }

    std::optional<uint32_t> setPoint;
    std::optional<std::string> controlMode;
    if (!json_util::readJson(*powerLimitWatts, asyncResp->res, "SetPoint",
                             setPoint, "ControlMode", controlMode))
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
