#pragma once

#include <app.hpp>
#include <utils/chassis_utils.hpp>

namespace redfish
{

using MapperGetAssociationResponse =
    std::vector<std::tuple<std::string, std::string, std::string>>;

const static std::string assIntf =
    "xyz.openbmc_project.Association.Definitions";

template <typename Callback>
inline void getObject(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const std::string& path, Callback&& callback)
{
    if (path.empty())
    {
        return;
    }

    crow::connections::systemBus->async_method_call(
        [asyncResp, path, callback{std::forward<Callback>(callback)}](
            const boost::system::error_code ec,
            const std::vector<std::pair<std::string, std::vector<std::string>>>&
                object) {
        if (ec)
        {
            return;
        }

        for (const auto& obj : object)
        {
            const std::string& service = obj.first;
            callback(service, path);
        }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetObject", path,
        std::array<std::string, 0>());
}

template <typename Callback>
inline void
    fanSensorPath(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                  const std::string& chassisPath, const std::string& chassisId,
                  const dbus::utility::MapperGetSubTreeResponse& subtree,
                  Callback&& callback)
{
    for (const auto& [fanPath, serviceMap] : subtree)
    {
        for (const auto& [service, interfaces] : serviceMap)
        {
            if (std::find_if(interfaces.begin(), interfaces.end(),
                             [](const auto& i) { return i == assIntf; }) ==
                interfaces.end())
            {
                continue;
            }

            sdbusplus::asio::getProperty<MapperGetAssociationResponse>(
                *crow::connections::systemBus, service, fanPath, assIntf,
                "Associations",
                [asyncResp, chassisPath, chassisId,
                 callback](const boost::system::error_code ec,
                           const MapperGetAssociationResponse& associations) {
                if (ec)
                {
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
                        getObject(asyncResp, fanSensorPath, callback);
                    }
                }
                });
        }
    }
}

template <typename Callback>
inline void
    getFanSensorPath(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     const std::string& chassisPath,
                     const std::string& chassisId, Callback&& callback)
{
    auto respHandler =
        [callback{std::forward<Callback>(callback)}, asyncResp, chassisPath,
         chassisId](const boost::system::error_code ec,
                    const dbus::utility::MapperGetSubTreeResponse& subtree) {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "respHandler DBUS error: " << ec.message();
            return;
        }

        fanSensorPath(asyncResp, chassisPath, chassisId, subtree, callback);
    };

    crow::connections::systemBus->async_method_call(
        respHandler, "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project", 0,
        std::array<const char*, 1>{"xyz.openbmc_project.Inventory.Item.Fan"});
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
    item["DeviceName"] = value;

    nlohmann::json& fanSensorList =
        asyncResp->res.jsonValue["FanSpeedsPercent"];
    fanSensorList.emplace_back(std::move(item));
    asyncResp->res.jsonValue["FanSpeedsPercent@odata.count"] =
        fanSensorList.size();
}

inline void
    getFanSpeedsPercent(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& chassisPath,
                        const std::string& chassisId)
{
    auto respHandler = [asyncResp, chassisId](const std::string& service,
                                              const std::string& path) {
        if (path.empty())
        {
            return;
        }

        sdbusplus::asio::getProperty<double>(
            *crow::connections::systemBus, service, path,
            "xyz.openbmc_project.Sensor.Value", "Value",
            [asyncResp, chassisId, path](const boost::system::error_code ec,
                                         const double value) {
            if (ec)
            {
                return;
            }

            updateFanSensorList(asyncResp, chassisId, path, value);
            });
    };

    getFanSensorPath(asyncResp, chassisPath, chassisId, std::move(respHandler));
}

inline void getPowerWatts(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& chassisPath,
                          const std::string& chassisId)
{
    auto respHandler =
        [asyncResp, chassisPath, chassisId](const std::string& service,
                                            const std::string& path) {
        sdbusplus::asio::getProperty<MapperGetAssociationResponse>(
            *crow::connections::systemBus, service, path, assIntf,
            "Associations",
            [asyncResp, chassisPath, chassisId, service,
             path](const boost::system::error_code ec,
                   const MapperGetAssociationResponse& associations) {
            if (ec)
            {
                return;
            }

            for (const auto& assoc : associations)
            {
                const auto& [rType, tType, endpoint] = assoc;
                if (rType == "chassis" && endpoint == chassisPath)
                {
                    sdbusplus::asio::getProperty<double>(
                        *crow::connections::systemBus, service, path,
                        "xyz.openbmc_project.Sensor.Value", "Value",
                        [asyncResp,
                         chassisId](const boost::system::error_code ec1,
                                    const uint32_t value) {
                        if (ec1)
                        {
                            return;
                        }

                        asyncResp->res
                            .jsonValue["PowerWatts"]["DataSourceUri"] =
                            crow::utility::urlFromPieces(
                                "redfish", "v1", "Chassis", chassisId,
                                "Sensors", "total_power");
                        asyncResp->res.jsonValue["PowerWatts"]["Reading"] =
                            value;
                        });
                    break;
                }
            }
            });
    };

    getObject(asyncResp, "/xyz/openbmc_project/sensors/power/total_power",
              std::move(respHandler));
}

inline void
    doEnvironmentMetrics(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::string& chassisId,
                         const std::optional<std::string>& validChassisPath)
{
    if (!validChassisPath)
    {
        BMCWEB_LOG_ERROR << "Not a valid chassis ID" << chassisId;
        messages::resourceNotFound(asyncResp->res, "Chassis", chassisId);
        return;
    }

    asyncResp->res.jsonValue["@odata.type"] =
        "#EnvironmentMetrics.v1_3_0.EnvironmentMetrics";
    asyncResp->res.jsonValue["Name"] = "Chassis Environment Metrics";
    asyncResp->res.jsonValue["Id"] = "EnvironmentMetrics";
    asyncResp->res.jsonValue["@odata.id"] = crow::utility::urlFromPieces(
        "redfish", "v1", "Chassis", chassisId, "EnvironmentMetrics");

    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/EnvironmentMetrics/EnvironmentMetrics.json>; rel=describedby");

    getFanSpeedsPercent(asyncResp, *validChassisPath, chassisId);
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
        std::bind_front(doEnvironmentMetrics, asyncResp, chassisId));
}

inline void requestRoutesEnvironmentMetrics(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/EnvironmentMetrics/")
        .privileges(redfish::privileges::getEnvironmentMetrics)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleEnvironmentMetricsGet, std::ref(app)));
}

} // namespace redfish
