#pragma once

#include <app.hpp>
#include <registries/privilege_registry.hpp>
#include <utils/chassis_utils.hpp>
#include <utils/json_utils.hpp>

namespace redfish
{

static constexpr const std::array<const char*, 1> fanInterfaces = {
    "xyz.openbmc_project.Configuration.Fan"};
static constexpr const std::array<const char*, 1> sensorInterfaces = {
    "xyz.openbmc_project.Sensor.Value"};

inline void getFanSensorValue(std::shared_ptr<bmcweb::AsyncResp> asyncResp,
                              const std::string& chassisId,
                              const std::string& sensorName,
                              const std::string& sensorType)
{
    if (sensorType != "fan_tach" && sensorType != "fan_pwm")
    {
        BMCWEB_LOG_DEBUG << "Unsupported sensor type";
        messages::internalError(asyncResp->res);
        return;
    }
    crow::connections::systemBus->async_method_call(
        [asyncResp, chassisId, sensorName, sensorType](
            const boost::system::error_code ec,
            const std::vector<std::pair<
                std::string,
                std::vector<std::pair<std::string, std::vector<std::string>>>>>&
                sensorsubtree) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG << "D-Bus response error on GetSubTree " << ec;
            messages::internalError(asyncResp->res);
            return;
        }
        for (const auto& [objectPath, serviceNames] : sensorsubtree)
        {
            if (objectPath.empty() || serviceNames.size() != 1)
            {
                BMCWEB_LOG_DEBUG << "Error getting Fan D-Bus object!";
                messages::internalError(asyncResp->res);
                return;
            }
            sdbusplus::message::object_path path(objectPath);
            const std::string& leaf = path.filename();
            if (leaf.empty())
            {
                continue;
            }
            if (leaf != sensorName)
            {
                continue;
            }

            const std::string& connectionName = serviceNames[0].first;
            sdbusplus::asio::getProperty<std::vector<std::string>>(
                *crow::connections::systemBus,
                "xyz.openbmc_project.ObjectMapper", objectPath + "/chassis",
                "xyz.openbmc_project.Association", "endpoints",
                [asyncResp, objectPath, connectionName, chassisId, sensorName,
                 sensorType](const boost::system::error_code ec,
                             const std::vector<std::string>& resp) {
                BMCWEB_LOG_DEBUG << "Getting chassis association for sensor "
                                 << sensorName;
                if (ec)
                {
                    BMCWEB_LOG_DEBUG
                        << "Error getting chassis association for the sensor!";
                    messages::internalError(asyncResp->res);
                    return;
                }
                if (resp.size() != 1)
                {
                    BMCWEB_LOG_DEBUG
                        << "The number of associated chassis should be 1";
                    messages::internalError(asyncResp->res);
                    return;
                }
                sdbusplus::message::object_path path(resp[0]);
                std::string leaf = path.filename();
                if (leaf != chassisId)
                {
                    BMCWEB_LOG_DEBUG
                        << "The sensor doesn't belong to the chassis "
                        << chassisId;
                    messages::internalError(asyncResp->res);
                    return;
                }
                sdbusplus::asio::getProperty<double>(
                    *crow::connections::systemBus, connectionName, objectPath,
                    "xyz.openbmc_project.Sensor.Value", "Value",
                    [asyncResp, chassisId, sensorName,
                     sensorType](const boost::system::error_code ec,
                                 const double& resp) {
                    if (ec)
                    {
                        BMCWEB_LOG_DEBUG
                            << "Error getting sensor value for sensor:"
                            << sensorName;
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    if (sensorType == "fan_pwm")
                    {
                        asyncResp->res
                            .jsonValue["SpeedPercent"]["DataSourceUri"] =
                            "/redfish/v1/Chassis/" + chassisId + "/Sensors/" +
                            sensorName + "/";
                        asyncResp->res.jsonValue["SpeedPercent"]["Reading"] =
                            resp;
                    }
                    else
                    {
                        asyncResp->res.jsonValue["SpeedPercent"]["SpeedRPM"] =
                            resp;
                    }
                    });
                });
        }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/sensors", 0, sensorInterfaces);
}

inline void
    fillFanProperties(std::shared_ptr<bmcweb::AsyncResp> asyncResp,
                      const boost::system::error_code ec,
                      const dbus::utility::DBusPropertiesMap& properties,
                      const std::string& chassisId)
{
    if (ec)
    {
        BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
        messages::internalError(asyncResp->res);
        return;
    }
    for (const auto& [propKey, propVariant] : properties)
    {
        if (propKey == "HotPluggable")
        {
            const bool* hotPluggable = std::get_if<bool>(&propVariant);
            if (hotPluggable == nullptr)
            {
                messages::internalError(asyncResp->res);
                return;
            }
            asyncResp->res.jsonValue["HotPluggable"] = *hotPluggable;
        }
        else if (propKey == "LocationType")
        {
            const std::string* locationType =
                std::get_if<std::string>(&propVariant);
            if (locationType == nullptr)
            {
                messages::internalError(asyncResp->res);
                return;
            }
            asyncResp->res
                .jsonValue["Location"]["PartLocation"]["LocationType"] =
                *locationType;
        }
        else if (propKey == "ServiceLabel")
        {
            const std::string* serviceLabel =
                std::get_if<std::string>(&propVariant);
            if (serviceLabel == nullptr)
            {
                messages::internalError(asyncResp->res);
                return;
            }
            asyncResp->res
                .jsonValue["Location"]["PartLocation"]["ServiceLabel"] =
                *serviceLabel;
        }
        else if (propKey == "TachSensor")
        {
            const std::string* tachSensor =
                std::get_if<std::string>(&propVariant);
            if (tachSensor == nullptr)
            {
                messages::internalError(asyncResp->res);
                return;
            }
            getFanSensorValue(asyncResp, chassisId, *tachSensor, "fan_tach");
        }
        else if (propKey == "PWMSensor")
        {
            const std::string* pwmSensor =
                std::get_if<std::string>(&propVariant);
            if (pwmSensor == nullptr)
            {
                messages::internalError(asyncResp->res);
                return;
            }
            getFanSensorValue(asyncResp, chassisId, *pwmSensor, "fan_pwm");
        }
    }
}

inline void
    getFanProperties(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     const std::string& fanObjectPath,
                     const dbus::utility::MapperServiceMap& serviceMap,
                     const std::string& chassisId)
{
    BMCWEB_LOG_DEBUG << "Get Properties for Fan " << fanObjectPath;
    for (const auto& [service, interfaces] : serviceMap)
    {
        for (const auto& interface : interfaces)
        {
            if (interface != fanInterfaces[0])
            {
                continue;
            }

            crow::connections::systemBus->async_method_call(
                [asyncResp, chassisId](
                    const boost::system::error_code ec,
                    const dbus::utility::DBusPropertiesMap& properties) {
                fillFanProperties(asyncResp, ec, properties, chassisId);
                },
                service, fanObjectPath, "org.freedesktop.DBus.Properties",
                "GetAll", interface);
        }
    }
}

inline void getValidFan(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& chassisId)
{
    BMCWEB_LOG_DEBUG << "Get fan list associated to chassis = " << chassisId;
    asyncResp->res.jsonValue["@odata.type"] = "#FanCollection.FanCollection";
    asyncResp->res.jsonValue["@odata.id"] =
        "/redfish/v1/Chassis/" + chassisId + "/ThermalSubsystem/Fans/";
    asyncResp->res.jsonValue["Name"] = "Fan Collection";
    asyncResp->res.jsonValue["Description"] =
        "The collection of Fan resource instances " + chassisId;
    crow::connections::systemBus->async_method_call(
        [asyncResp, chassisId](
            const boost::system::error_code ec,
            const std::vector<std::pair<
                std::string,
                std::vector<std::pair<std::string, std::vector<std::string>>>>>&
                fansubtree) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG << "DBUS response error";
            messages::internalError(asyncResp->res);
            return;
        }

        nlohmann::json& fanList = asyncResp->res.jsonValue["Members"];
        fanList = nlohmann::json::array();

        std::string newPath =
            "/redfish/v1/Chassis/" + chassisId + "/ThermalSubsystem/Fans/";

        for (const auto& [objectPath, serviceName] : fansubtree)
        {
            if (objectPath.empty() || serviceName.size() != 1)
            {
                BMCWEB_LOG_DEBUG << "Error getting D-Bus object!";
                messages::internalError(asyncResp->res);
                return;
            }
            const std::string& validPath = objectPath;
            // const std::string& connectionName = serviceName[0].first;
            std::string chassisName;
            std::string fanName;

            // 5 and 6 below comes from
            // /xyz/openbmc_project/inventory/system/chassis/chassisName/fanName
            //   0      1             2         3         4         5      6
            if (!dbus::utility::getNthStringFromPath(validPath, 5,
                                                     chassisName) ||
                !dbus::utility::getNthStringFromPath(validPath, 6, fanName))
            {
                BMCWEB_LOG_ERROR << "Got invalid path " << validPath;
                messages::invalidObject(asyncResp->res,
                                        crow::utility::urlFromPieces(
                                            "xyz", "openbmc_project",
                                            "inventory", "system", validPath));
                continue;
            }
            if (chassisName != chassisId)
            {
                BMCWEB_LOG_ERROR << "The fan obtained at this time "
                                    "does not belong to this chassis ";
                continue;
            }
            fanList.push_back({{"@odata.id", newPath + fanName + "/"}});
            asyncResp->res.jsonValue["Members@odata.count"] = fanList.size();
        }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", 0, fanInterfaces);
}

inline void requestRoutesFanCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/ThermalSubsystem/Fans/")
        .privileges(redfish::privileges::getFanCollection)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& chassisId) {
        auto getChassisId = [asyncResp, chassisId](
                                const std::optional<std::string>& chassisPath) {
            if (!chassisPath)
            {
                BMCWEB_LOG_ERROR << "Not a valid chassis ID" << chassisId;
                messages::resourceNotFound(asyncResp->res, "Chassis",
                                           chassisId);
                return;
            }
            getValidFan(asyncResp, chassisId);
        };
        redfish::chassis_utils::getValidChassisID(asyncResp, chassisId,
                                                  std::move(getChassisId));
        });
}

inline void requestRoutesFan(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/ThermalSubsystem/Fans/<str>/")
        .privileges(redfish::privileges::getFan)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& chassisId, const std::string& fanId) {
        auto getChassisId = [asyncResp, chassisId, fanId](
                                const std::optional<std::string>& chassisPath) {
            if (!chassisPath)
            {
                BMCWEB_LOG_ERROR << "Not a valid chassis ID" << chassisId;
                messages::resourceNotFound(asyncResp->res, "Chassis",
                                           chassisId);
                return;
            }
            auto respHandler =
                [asyncResp, chassisId, fanId](
                    const boost::system::error_code ec,
                    const dbus::utility::MapperGetSubTreeResponse& subtree) {
                if (ec)
                {
                    BMCWEB_LOG_DEBUG << "DBUS response error";
                    messages::internalError(asyncResp->res);
                    return;
                }
                for (const auto& [objectPath, serviceMap] : subtree)
                {
                    sdbusplus::message::object_path path(objectPath);
                    if (path.filename() != fanId)
                    {
                        continue;
                    }
                    std::string newPath = "/redfish/v1/Chassis/" + chassisId +
                                          "/ThermalSubsystem/Fans/";
                    asyncResp->res.jsonValue["@odata.type"] = "#Fan.v1_2_0.Fan";
                    asyncResp->res.jsonValue["Name"] = fanId;
                    asyncResp->res.jsonValue["Id"] = fanId;
                    asyncResp->res.jsonValue["@odata.id"] =
                        newPath + fanId + "/";
                    getFanProperties(asyncResp, objectPath, serviceMap,
                                     chassisId);
                    return;
                }
            };
            crow::connections::systemBus->async_method_call(
                respHandler, "xyz.openbmc_project.ObjectMapper",
                "/xyz/openbmc_project/object_mapper",
                "xyz.openbmc_project.ObjectMapper", "GetSubTree",
                "/xyz/openbmc_project/inventory", 0, fanInterfaces);
        };
        redfish::chassis_utils::getValidChassisID(asyncResp, chassisId,
                                                  std::move(getChassisId));
        });
}

} // namespace redfish
