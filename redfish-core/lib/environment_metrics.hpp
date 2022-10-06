#pragma once

#include "app.hpp"
#include "dbus_utility.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/chassis_utils.hpp"
#include "utils/json_utils.hpp"
#include "utils/sensor_utils.hpp"

#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/message/native_types.hpp>

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace redfish
{

namespace environment_metrics
{
struct SinglePath
{
    std::string path;
    std::string service;
};

/**
 * @brief Returns the single path and service from subtree response
 * @details This function should only be used when expecting a single path and
 * service in the subtree. If there are more than one of either an empty
 * structure is returned.
 * @param subtree Response from getSubTree()
 * @return Struct of path and service. On any error an empty struct is returned.
 */
inline SinglePath
    getSinglePath(const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    SinglePath pathAndService;

    if (subtree.size() != 1)
    {
        BMCWEB_LOG_DEBUG("Unexpected number of paths found {}", subtree.size());
        return pathAndService;
    }

    const auto& object = subtree[0];
    const auto& serviceMap = object.second;
    if (serviceMap.size() != 1)
    {
        BMCWEB_LOG_DEBUG("Unexpected number serviceMaps found {}",
                         serviceMap.size());
        return pathAndService;
    }
    pathAndService.path = object.first;
    pathAndService.service = serviceMap[0].first;
    return pathAndService;
}

} // namespace environment_metrics

inline void
    afterGetPowerWatts(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& chassisId, const std::string& path,
                       const boost::system::error_code& ec,
                       const dbus::utility::DBusPropertiesMap& valuesDict)
{
    if (ec)
    {
        if (ec.value() != EBADR)
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

inline void handlePowerWatts(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec)
    {
        if (ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR("DBUS response error for getAssociatedSubTree {}",
                             ec);
            messages::internalError(asyncResp->res);
        }
        return;
    }

    environment_metrics::SinglePath pathAndService =
        environment_metrics::getSinglePath(subtree);
    if (pathAndService.path.empty() || pathAndService.service.empty())
    {
        BMCWEB_LOG_DEBUG("No path and service found for PowerWatts");
        return;
    }

    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, pathAndService.service,
        pathAndService.path, "xyz.openbmc_project.Sensor.Value",
        [asyncResp, chassisId, path{pathAndService.path}](
            const boost::system::error_code& ec1,
            const dbus::utility::DBusPropertiesMap& propertiesList) {
            afterGetPowerWatts(asyncResp, chassisId, path, ec1, propertiesList);
        });
}

inline void getPowerWatts(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& validChassisPath,
                          const std::string& chassisId)
{
    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Sensor.Value"};
    sdbusplus::message::object_path endpointPath{validChassisPath};
    endpointPath /= "total_power";

    dbus::utility::getAssociatedSubTree(
        endpointPath,
        sdbusplus::message::object_path("/xyz/openbmc_project/sensors/power"),
        1, interfaces, std::bind_front(handlePowerWatts, asyncResp, chassisId));
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

inline void
    doEnvironmentMetricsGet(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
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
