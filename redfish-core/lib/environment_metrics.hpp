#pragma once

#include <app.hpp>
#include <utils/chassis_utils.hpp>
#include <utils/json_utils.hpp>

namespace redfish
{
inline void
    getfanSpeedsPercent(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& chassisID)
{
    BMCWEB_LOG_DEBUG << "Get properties for getFan associated to chassis = "
                     << chassisID;
    const std::array<std::string, 1> sensorInterfaces = {
        "xyz.openbmc_project.Sensor.Value"};
    crow::connections::systemBus->async_method_call(
        [asyncResp, chassisID](
            const boost::system::error_code ec,
            const std::vector<std::pair<
                std::string,
                std::vector<std::pair<std::string, std::vector<std::string>>>>>&
                sensorsubtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "D-Bus response error on GetSubTree " << ec;
                if (ec.value() == boost::system::errc::io_error)
                {
                    messages::resourceNotFound(asyncResp->res, "Chassis",
                                               chassisID);
                    return;
                }
                messages::internalError(asyncResp->res);
                return;
            }
            nlohmann::json& fanList =
                asyncResp->res.jsonValue["FanSpeedsPercent"];
            fanList = nlohmann::json::array();

            for (const auto& [objectPath, serviceName] : sensorsubtree)
            {
                if (objectPath.empty() || serviceName.size() != 1)
                {
                    BMCWEB_LOG_DEBUG << "Error getting D-Bus object!";
                    messages::internalError(asyncResp->res);
                    return;
                }
                const std::string& validPath = objectPath;
                const std::string& connectionName = serviceName[0].first;
                std::string sensorType;
                std::string sensorName;

                // 4 below comes from
                // /xyz/openbmc_project/sensors/sensorType/sensorName
                //   0      1             2        3        4
                if (!dbus::utility::getNthStringFromPath(validPath, 3,
                                                         sensorType) ||
                    !dbus::utility::getNthStringFromPath(validPath, 4,
                                                         sensorName))
                {
                    BMCWEB_LOG_ERROR << "Got invalid path " << validPath;
                    messages::invalidObject(asyncResp->res, validPath);
                    return;
                }

                if (sensorType == "fan" || sensorType == "fan_tach" ||
                    sensorType == "fan_pwm")
                {
                    crow::connections::systemBus->async_method_call(
                        [asyncResp, chassisID, &fanList,
                         sensorName](const boost::system::error_code ec,
                                     const std::variant<double>& value) {
                            if (ec)
                            {
                                BMCWEB_LOG_DEBUG << "Can't get Fan speed!";
                                messages::internalError(asyncResp->res);
                                return;
                            }

                            const double* attributeValue =
                                std::get_if<double>(&value);
                            if (attributeValue == nullptr)
                            {
                                // illegal property
                                messages::internalError(asyncResp->res);
                                return;
                            }
                            std::string tempPath = "/redfish/v1/Chassis/" +
                                                   chassisID + "/Sensors/";
                            fanList.push_back(
                                {{"DeviceName", "Chassis Fan #" + sensorName},
                                 {"SpeedRPM", *attributeValue},
                                 {"DataSourceUri", tempPath + sensorName},
                                 {"@odata.id", tempPath + sensorName}});
                        },
                        connectionName, validPath,
                        "org.freedesktop.DBus.Properties", "Get",
                        "xyz.openbmc_project.Sensor.Value", "Value");
                }
                else
                {
                    BMCWEB_LOG_DEBUG
                        << "This is not a fan-related sensor,sensortype = "
                        << sensorType;
                    continue;
                }
            }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/sensors", 0, sensorInterfaces);
}

inline void getPowerWatts(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& chassisID)
{
    const std::string sensorPath = "/xyz/openbmc_project/sensors";
    const std::array<std::string, 1> sensorInterfaces = {
        "xyz.openbmc_project.Sensor.Value"};
    crow::connections::systemBus->async_method_call(
        [asyncResp, chassisID](const boost::system::error_code ec,
                               const std::vector<std::string>& paths) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(asyncResp->res);
                return;
            }
            bool judgment = false;
            for (const auto& tempPath : paths)
            {
                sdbusplus::message::object_path path(tempPath);
                std::string leaf = path.filename();
                if (!leaf.compare("total_power"))
                {
                    judgment = true;
                    break;
                }
            }
            if (judgment)
            {
                const std::array<const char*, 1> totalPowerInterfaces = {
                    "xyz.openbmc_project.Sensor.Value"};
                const std::string& totalPowerPath =
                    "/xyz/openbmc_project/sensors/power/total_power";
                crow::connections::systemBus->async_method_call(
                    [asyncResp, chassisID, totalPowerPath](
                        const boost::system::error_code ec,
                        const std::vector<std::pair<
                            std::string, std::vector<std::string>>>& object) {
                        if (ec)
                        {
                            BMCWEB_LOG_DEBUG << "DBUS response error";
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        for (const auto& tempObject : object)
                        {
                            const std::string& connectionName =
                                tempObject.first;
                            crow::connections::systemBus->async_method_call(
                                [asyncResp,
                                 chassisID](const boost::system::error_code ec,
                                            const std::variant<double>& value) {
                                    if (ec)
                                    {
                                        BMCWEB_LOG_DEBUG
                                            << "Can't get Power Watts!";
                                        messages::internalError(asyncResp->res);
                                        return;
                                    }

                                    const double* attributeValue =
                                        std::get_if<double>(&value);
                                    if (attributeValue == nullptr)
                                    {
                                        // illegal property
                                        messages::internalError(asyncResp->res);
                                        return;
                                    }
                                    std::string tempPath =
                                        "/redfish/v1/Chassis/" + chassisID +
                                        "/Sensors/";
                                    asyncResp->res.jsonValue["PowerWatts"] = {
                                        {"Reading", *attributeValue},
                                        {"DataSourceUri",
                                         tempPath + "total_power"},
                                        {"@odata.id",
                                         tempPath + "total_power"}};
                                },
                                connectionName, totalPowerPath,
                                "org.freedesktop.DBus.Properties", "Get",
                                "xyz.openbmc_project.Sensor.Value", "Value");
                        }
                    },
                    "xyz.openbmc_project.ObjectMapper",
                    "/xyz/openbmc_project/object_mapper",
                    "xyz.openbmc_project.ObjectMapper", "GetObject",
                    totalPowerPath, totalPowerInterfaces);
            }
            else
            {
                BMCWEB_LOG_DEBUG << "There is not total_power";
                return;
            }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths", sensorPath, 0,
        sensorInterfaces);
}

inline void
    getPowerLimitWatts(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    const std::array<std::string, 1> powerCapInterfaces = {
        "xyz.openbmc_project.Control.Power.Cap"};
    crow::connections::systemBus->async_method_call(
        [asyncResp](
            const boost::system::error_code ec,
            const std::vector<std::pair<
                std::string,
                std::vector<std::pair<std::string, std::vector<std::string>>>>>&
                powercapsubtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "D-Bus response error on GetSubTree " << ec;
                messages::internalError(asyncResp->res);
                return;
            }

            for (const auto& [objectPath, serviceName] : powercapsubtree)
            {
                if (objectPath.empty() || serviceName.size() != 1)
                {
                    BMCWEB_LOG_DEBUG << "Error getting D-Bus object!";
                    messages::internalError(asyncResp->res);
                    return;
                }
                const std::string& validPath = objectPath;
                const std::string& connectionName = serviceName[0].first;

                asyncResp->res.jsonValue["PowerLimitWatts"]["SetPoint"] = 0;
                asyncResp->res.jsonValue["PowerLimitWatts"]["ControlMode"] =
                    "automatic";

                crow::connections::systemBus->async_method_call(
                    [asyncResp](const boost::system::error_code ec2,
                                const std::vector<std::pair<
                                    std::string, std::variant<uint32_t, bool>>>&
                                    properties) {
                        if (ec2)
                        {
                            BMCWEB_LOG_DEBUG << "Power Limit Set: Dbus error: "
                                             << ec2;
                            messages::internalError(asyncResp->res);
                            return;
                        }

                        for (const std::pair<std::string,
                                             std::variant<uint32_t, bool>>&
                                 property : properties)
                        {
                            if (property.first == "PowerCap")
                            {
                                const uint32_t* powerCap =
                                    std::get_if<uint32_t>(&property.second);

                                if (powerCap == nullptr)
                                {

                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                                asyncResp->res
                                    .jsonValue["PowerLimitWatts"]["SetPoint"] =
                                    *powerCap;
                            }
                            else if (property.first == "PowerCapEnable")
                            {
                                const bool* powerCapEnable =
                                    std::get_if<bool>(&property.second);

                                if (powerCapEnable == nullptr)
                                {

                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                                if (!*powerCapEnable)
                                {
                                    asyncResp->res.jsonValue["PowerLimitWatts"]
                                                            ["ControlMode"] =
                                        "disabled";
                                }
                            }
                        }
                    },
                    connectionName, validPath,
                    "org.freedesktop.DBus.Properties", "GetAll",
                    "xyz.openbmc_project.Control.Power.Cap");
            }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree", "/", 0,
        powerCapInterfaces);
}

inline void
    setPowerSetPoint(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     uint32_t powerCap)
{
    BMCWEB_LOG_DEBUG << "Set Power Limit Watts Set Point";

    const std::array<std::string, 1> powerCapInterfaces = {
        "xyz.openbmc_project.Control.Power.Cap"};
    crow::connections::systemBus->async_method_call(
        [asyncResp, powerCap](
            const boost::system::error_code ec,
            const std::vector<std::pair<
                std::string,
                std::vector<std::pair<std::string, std::vector<std::string>>>>>&
                powercapsubtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "D-Bus response error on GetSubTree " << ec;
                messages::internalError(asyncResp->res);
                return;
            }

            for (const auto& [objectPath, serviceName] : powercapsubtree)
            {
                if (objectPath.empty() || serviceName.size() != 1)
                {
                    BMCWEB_LOG_DEBUG << "Error getting D-Bus object!";
                    messages::internalError(asyncResp->res);
                    return;
                }
                const std::string& validPath = objectPath;
                const std::string& connectionName = serviceName[0].first;

                crow::connections::systemBus->async_method_call(
                    [asyncResp](const boost::system::error_code ec) {
                        if (ec)
                        {
                            BMCWEB_LOG_DEBUG << "Power Limit Set: Dbus error: "
                                             << ec;
                            messages::internalError(asyncResp->res);
                            return;
                        }
                    },
                    connectionName, validPath,
                    "org.freedesktop.DBus.Properties", "Set",
                    "xyz.openbmc_project.Control.Power.Cap", "PowerCap",
                    std::variant<uint32_t>(powerCap));
            }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree", "/", 0,
        powerCapInterfaces);
}

inline void
    setPowerControlMode(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& controlMode)
{
    BMCWEB_LOG_DEBUG << "Set Power Limit Watts Control Mode";

    const std::array<std::string, 1> powerCapInterfaces = {
        "xyz.openbmc_project.Control.Power.Cap"};
    crow::connections::systemBus->async_method_call(
        [asyncResp, controlMode](
            const boost::system::error_code ec,
            const std::vector<std::pair<
                std::string,
                std::vector<std::pair<std::string, std::vector<std::string>>>>>&
                powercapsubtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "D-Bus response error on GetSubTree " << ec;
                messages::internalError(asyncResp->res);
                return;
            }

            bool powerCapEnable;
            if (controlMode == "disabled")
            {
                powerCapEnable = false;
            }
            else if (controlMode == "automatic")
            {
                powerCapEnable = true;
            }
            else
            {
                BMCWEB_LOG_DEBUG
                    << "Power Control Mode  does not support this mode :"
                    << controlMode;
                messages::propertyValueNotInList(asyncResp->res, controlMode,
                                                 "ControlMode");
                return;
            }

            for (const auto& [objectPath, serviceName] : powercapsubtree)
            {
                if (objectPath.empty() || serviceName.size() != 1)
                {
                    BMCWEB_LOG_DEBUG << "Error getting D-Bus object!";
                    messages::internalError(asyncResp->res);
                    return;
                }
                const std::string& validPath = objectPath;
                const std::string& connectionName = serviceName[0].first;

                crow::connections::systemBus->async_method_call(
                    [asyncResp](const boost::system::error_code ec) {
                        if (ec)
                        {
                            messages::internalError(asyncResp->res);
                            BMCWEB_LOG_ERROR
                                << "powerCapEnable Get handler: Dbus error "
                                << ec;
                            return;
                        }
                    },
                    connectionName, validPath,
                    "org.freedesktop.DBus.Properties", "Set",
                    "xyz.openbmc_project.Control.Power.Cap", "PowerCapEnable",
                    std::variant<bool>(powerCapEnable));
            }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree", "/", 0,
        powerCapInterfaces);
}

inline void
    getEnvironmentMetrics(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& chassisID)
{
    BMCWEB_LOG_DEBUG
        << "Get properties for EnvironmentMetrics associated to chassis = "
        << chassisID;
    asyncResp->res.jsonValue["@odata.type"] =
        "#EnvironmentMetrics.v1_1_0.EnvironmentMetrics";
    asyncResp->res.jsonValue["Name"] = "Chassis Environment Metrics";
    asyncResp->res.jsonValue["Id"] = "EnvironmentMetrics";
    asyncResp->res.jsonValue["@odata.id"] =
        "/redfish/v1/Chassis/" + chassisID + "/EnvironmentMetrics";
    getfanSpeedsPercent(asyncResp, chassisID);
    getPowerWatts(asyncResp, chassisID);
    getPowerLimitWatts(asyncResp);
}

inline void requestRoutesEnvironmentMetrics(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/EnvironmentMetrics/")
        .privileges({{"Login"}})
        // TODO: Use automated PrivilegeRegistry
        // Need to wait for Redfish to release a new registry
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& chassisID) {
                auto getChassisID =
                    [asyncResp, chassisID](
                        const std::optional<std::string>& validChassisID) {
                        if (!validChassisID)
                        {
                            BMCWEB_LOG_ERROR << "Not a valid chassis ID:"
                                             << chassisID;
                            messages::resourceNotFound(asyncResp->res,
                                                       "Chassis", chassisID);
                            return;
                        }

                        getEnvironmentMetrics(asyncResp, *validChassisID);
                    };
                redfish::chassis_utils::getValidChassisID(
                    asyncResp, chassisID, std::move(getChassisID));
            });

    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/EnvironmentMetrics/")
        .privileges({{"ConfigureComponents"}})
        // TODO: Use automated PrivilegeRegistry
        // Need to wait for Redfish to release a new registry
        .methods(boost::beast::http::verb::patch)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& chassisID) {
                std::optional<nlohmann::json> powerLimitWatts;
                if (!json_util::readJson(req, asyncResp->res, "PowerLimitWatts",
                                         powerLimitWatts))
                {
                    return;
                }

                if (powerLimitWatts)
                {
                    std::optional<uint32_t> setPoint;
                    std::optional<std::string> controlMode;
                    if (!redfish::json_util::readJson(
                            *powerLimitWatts, asyncResp->res, "SetPoint",
                            setPoint, "ControlMode", controlMode))
                    {
                        return;
                    }

                    if (setPoint)
                    {
                        auto getChassisID =
                            [asyncResp, chassisID,
                             setPoint](const std::optional<std::string>&
                                           validChassisID) {
                                if (!validChassisID)
                                {
                                    BMCWEB_LOG_ERROR
                                        << "Not a valid chassis ID:"
                                        << chassisID;
                                    messages::resourceNotFound(
                                        asyncResp->res, "Chassis", chassisID);
                                    return;
                                }
                                setPowerSetPoint(asyncResp, *setPoint);
                            };
                        redfish::chassis_utils::getValidChassisID(
                            asyncResp, chassisID, std::move(getChassisID));
                    }

                    if (controlMode)
                    {
                        auto getChassisID =
                            [asyncResp, chassisID,
                             controlMode](const std::optional<std::string>&
                                              validChassisID) {
                                if (!validChassisID)
                                {
                                    BMCWEB_LOG_ERROR
                                        << "Not a valid chassis ID:"
                                        << chassisID;
                                    messages::resourceNotFound(
                                        asyncResp->res, "Chassis", chassisID);
                                    return;
                                }
                                setPowerControlMode(asyncResp, *controlMode);
                            };
                        redfish::chassis_utils::getValidChassisID(
                            asyncResp, chassisID, std::move(getChassisID));
                    }
                }
            });
}

} // namespace redfish