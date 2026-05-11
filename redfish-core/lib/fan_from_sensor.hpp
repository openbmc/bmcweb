#pragma once

#include "app.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/chassis_utils.hpp"

#include <boost/url/format.hpp>
#include <dbus_utility.hpp>
#include <utils/collection.hpp>

#include <iostream>
#include <optional>
#include <string>

namespace redfish
{

inline void doFanCollection(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& chassisId,
                            const std::optional<std::string>& validChassisPath)
{
    if (!validChassisPath)
    {
        BMCWEB_LOG_ERROR("Not a valid chassis ID {}", chassisId);
        messages::resourceNotFound(asyncResp->res, "Chassis", chassisId);
        return;
    }

    asyncResp->res.jsonValue["@odata.type"] = "#FanCollection.FanCollection";
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Chassis/{}/ThermalSubsystem/Fans", chassisId);
    asyncResp->res.jsonValue["Name"] = "Fans Collection";
    asyncResp->res.jsonValue["Members"] = nlohmann::json::array();
    asyncResp->res.jsonValue["Members@odata.count"] = 0;

    constexpr std::array<std::string_view, 1> interfaces{
        "xyz.openbmc_project.Sensor.Value"};

    constexpr auto fanSensorPaths =
        std::array<const char*, 1>{"/xyz/openbmc_project/sensors/fan_pwm"};

    for (const char* subtree : fanSensorPaths)
    {
        dbus::utility::getSubTreePaths(
            subtree, 0, interfaces,
            [chassisId, asyncResp](
                const boost::system::error_code& ec,
                const dbus::utility::MapperGetSubTreePathsResponse& objects) {
                if (ec == boost::system::errc::io_error)
                {
                    asyncResp->res.jsonValue["Members"] =
                        nlohmann::json::array();
                    asyncResp->res.jsonValue["Members@odata.count"] = 0;
                    return;
                }

                if (ec)
                {
                    BMCWEB_LOG_DEBUG("DBUS response error {}", ec.value());
                    messages::internalError(asyncResp->res);
                    return;
                }

                std::vector<std::string> pathNames;

                for (const auto& object : objects)
                {
                    sdbusplus::message::object_path path(object);
                    std::string leaf = path.filename();
                    if (leaf.empty())
                    {
                        continue;
                    }
                    pathNames.push_back(leaf);
                }
                std::sort(pathNames.begin(), pathNames.end(),
                          AlphanumLess<std::string>());

                for (const std::string& leaf : pathNames)
                {
                    boost::urls::url url = boost::urls::format(
                        "/redfish/v1/Chassis/{}/ThermalSubsystem/Fans",
                        chassisId);
                    crow::utility::appendUrlPieces(url, leaf);
                    nlohmann::json::object_t member;
                    member["@odata.id"] = std::move(url);
                    asyncResp->res.jsonValue["Members"].push_back(
                        std::move(member));
                }
                asyncResp->res.jsonValue["Members@odata.count"] =
                    asyncResp->res.jsonValue["Members"].size();
            });
    }
}

inline bool checkFanId(const std::string& fanPath, const std::string& fanId)
{
    std::string fanName = sdbusplus::message::object_path(fanPath).filename();
    return !(fanName.empty() || fanName != fanId);
}

template <typename Callback>
inline void getValidfanId(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& chassisId,
                          const std::string& fanId, Callback&& callback)
{
    auto respHandler =
        [callback{std::forward<Callback>(callback)}, asyncResp, chassisId,
         fanId](const boost::system::error_code ec,
                const dbus::utility::MapperGetSubTreeResponse& subtree) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("respHandler DBUS error: {}", ec.message());
                messages::internalError(asyncResp->res);
                return;
            }

            bool resourceFound = false;
            for (const auto& [fanPath, serviceMap] : subtree)
            {
                for (const auto& [service, interfaces] : serviceMap)
                {
                    if (checkFanId(fanPath, fanId))
                    {
                        resourceFound = true;
                        callback(service, fanPath, interfaces);
                    }
                }
            }
            if (!resourceFound)
            {
                messages::resourceNotFound(asyncResp->res, "fan", fanId);
            }
        };

    crow::connections::systemBus->async_method_call(
        respHandler, "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/sensors", 0,
        std::array<const char*, 1>{"xyz.openbmc_project.Sensor.Value"});
}

inline void getFanValue(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& service, const std::string& path,
                        const std::string& intf)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code ec,
                    const dbus::utility::DBusPropertiesMap& propertiesList) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("Can't get Fan value!");
                messages::internalError(asyncResp->res);
                return;
            }
            for (const auto& property : propertiesList)
            {
                const std::string& propertyName = property.first;
                if (propertyName == "Value")
                {
                    const double* value = std::get_if<double>(&property.second);
                    if (value == nullptr)
                    {
                        std::cerr << "nullptr  " << std::endl;
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    asyncResp->res.jsonValue["SpeedPercent"]["Reading"] =
                        *value;
                }
            }
        },
        service, path, "org.freedesktop.DBus.Properties", "GetAll", intf);
}

inline void getFanState(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& service, const std::string& path,
                        const std::string& intf)
{
    sdbusplus::asio::getProperty<bool>(
        *crow::connections::systemBus, service, path, intf, "Available",
        [asyncResp](const boost::system::error_code ec, const bool value) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG("DBUS response error {}", ec.message());
                messages::internalError(asyncResp->res);
                return;
            }

            asyncResp->res.jsonValue["Status"]["State"] =
                value ? "Enabled" : "Absent";
        });
}

inline void getFanHealth(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::string& service, const std::string& path,
                         const std::string& intf)
{
    sdbusplus::asio::getProperty<bool>(
        *crow::connections::systemBus, service, path, intf, "Functional",
        [asyncResp](const boost::system::error_code ec, const bool value) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG("DBUS response error {}", ec.message());
                messages::internalError(asyncResp->res);
                return;
            }

            asyncResp->res.jsonValue["Status"]["Health"] =
                value ? "OK" : "Critical";
        });
}

inline void doFanGet(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     const std::string& chassisId, const std::string& fanId,
                     const std::optional<std::string>& validChassisPath)
{
    if (!validChassisPath)
    {
        BMCWEB_LOG_ERROR("Not a valid chassis ID {}", chassisId);
        messages::resourceNotFound(asyncResp->res, "Chassis", chassisId);
        return;
    }

    auto getFanIdFunc = [asyncResp, chassisId,
                         fanId](const std::string& service,
                                const std::string& fanPath,
                                const std::vector<std::string>& interfaces) {
        asyncResp->res.jsonValue["@odata.type"] = "#Fan.v1_3_0.Fan";
        asyncResp->res.jsonValue["Name"] = fanId;
        asyncResp->res.jsonValue["Id"] = fanId;
        asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
            "/redfish/v1/Chassis/{}/ThermalSubsystem/Fans/{}", chassisId,
            fanId);

        asyncResp->res.jsonValue["SpeedPercent"]["DataSourceUri"] =
            boost::urls::format("/redfish/v1/Chassis/{}/Sensors/fanpwm_{}",
                                chassisId, fanId);

        asyncResp->res.jsonValue["Status"]["State"] = "Enabled";
        asyncResp->res.jsonValue["Status"]["Health"] = "OK";

        asyncResp->res.addHeader(
            boost::beast::http::field::link,
            "</redfish/v1/JsonSchemas/Fan/Fan.json>; rel=describedby");
        for (const auto& intf : interfaces)
        {
            if (intf == "xyz.openbmc_project.Sensor.Value")
            {
                getFanValue(asyncResp, service, fanPath, intf);
            }
            if (intf == "xyz.openbmc_project.State.Decorator.Availability")
            {
                getFanState(asyncResp, service, fanPath, intf);
            }
            if (intf == "xyz.openbmc_project.State.Decorator.OperationalStatus")
            {
                getFanHealth(asyncResp, service, fanPath, intf);
            }
        }
    };

    getValidfanId(asyncResp, chassisId, fanId, std::move(getFanIdFunc));
}

inline void handleFanCollectionGet(
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
        std::bind_front(doFanCollection, asyncResp, chassisId));
}

inline void handleFanGet(App& app, const crow::Request& req,
                         const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::string& chassisId, const std::string& fanId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    redfish::chassis_utils::getValidChassisPath(
        asyncResp, chassisId,
        std::bind_front(doFanGet, asyncResp, chassisId, fanId));
}

inline void requestRoutesFanCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/ThermalSubsystem/Fans/")
        .privileges(redfish::privileges::getFanCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleFanCollectionGet, std::ref(app)));
}

inline void requestRoutesFan(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/ThermalSubsystem/Fans/<str>/")
        .privileges(redfish::privileges::getFan)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleFanGet, std::ref(app)));
}

} // namespace redfish
