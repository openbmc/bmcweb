#pragma once

#include <app.hpp>
#include <utils/chassis_utils.hpp>

namespace redfish
{

using MapperGetAssociationResponse =
    std::vector<std::tuple<std::string, std::string, std::string>>;

inline void updateFanList(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& chassisId,
                          const std::string& fanPath)
{
    std::string fanName = sdbusplus::message::object_path(fanPath).filename();
    if (fanName.empty())
    {
        return;
    }

    nlohmann::json item;
    item["@odata.id"] =
        crow::utility::urlFromPieces("redfish", "v1", "Chassis", chassisId,
                                     "ThermalSubsystem", "Fans", fanName);

    nlohmann::json& fanList = asyncResp->res.jsonValue["Members"];
    fanList.emplace_back(std::move(item));
    asyncResp->res.jsonValue["Members@odata.count"] = fanList.size();
}

inline bool checkFanId(const std::string& fanPath, const std::string& fanId)
{
    std::string fanName = sdbusplus::message::object_path(fanPath).filename();

    return !(fanName.empty() || fanName != fanId);
}

template <typename Callback>
inline void getValidfanId(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& chassisPath,
                          const std::string& fanId, Callback&& callback)
{
    auto respHandler =
        [callback{std::forward<Callback>(callback)}, asyncResp,
         fanId](const boost::system::error_code ec,
                const dbus::utility::MapperGetSubTreeResponse& subtree) {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "respHandler DBUS error: " << ec.message();
            messages::internalError(asyncResp->res);
            return;
        }

        const static std::string assIntf =
            "xyz.openbmc_project.Association.Definitions";

        // Set the default value to resourceNotFound, and if we confirm that
        // fanId is correct, the error response will be cleared.
        messages::resourceNotFound(asyncResp->res, "fan", fanId);
        for (const auto& [fanPath, serviceMap] : subtree)
        {
            if (serviceMap.empty())
            {
                continue;
            }

            for (const auto& [service, interfaces] : serviceMap)
            {
                if (std::find_if(interfaces.begin(), interfaces.end(),
                                 [](const auto& i) { return i == assIntf; }) ==
                    interfaces.end())
                {
                    if (checkFanId(fanPath, fanId))
                    {
                        // Clear resourceNotFound response
                        asyncResp->res.clear();
                        callback(service, fanPath, interfaces);
                    }
                    continue;
                }

                sdbusplus::asio::getProperty<MapperGetAssociationResponse>(
                    *crow::connections::systemBus, service, fanPath, assIntf,
                    "Associations",
                    [callback, asyncResp, service{service}, fanId,
                     fanPath{fanPath}, interfaces{interfaces}](
                        const boost::system::error_code ec1,
                        const MapperGetAssociationResponse& associations) {
                    if (ec1)
                    {
                        return;
                    }

                    for (const auto& assoc : associations)
                    {
                        const auto& [rType, tType, endpoint] = assoc;
                        if (rType == "sensors")
                        {
                            if (!checkFanId(endpoint, fanId))
                            {
                                continue;
                            }
                            // Clear resourceNotFound response
                            asyncResp->res.clear();
                            callback(service, fanPath, interfaces, endpoint);
                            break;
                        }
                    }
                    });
            }
        }
    };

    crow::connections::systemBus->async_method_call(
        respHandler, "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree", chassisPath, 0,
        std::array<const char*, 1>{"xyz.openbmc_project.Inventory.Item.Fan"});
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
            return;
        }

        asyncResp->res.jsonValue["Status"]["Health"] =
            value ? "OK" : "Critical";
        });
}

inline void
    getFanSpeedPercent(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& service, const std::string& path,
                       const std::string& intf, const std::string& chassisId,
                       const std::string& fanId)
{
    sdbusplus::asio::getProperty<double>(
        *crow::connections::systemBus, service, path, intf, "Value",
        [asyncResp, chassisId, fanId](const boost::system::error_code ec,
                                      const double value) {
        if (ec)
        {
            return;
        }

        asyncResp->res.jsonValue["SpeedPercent"]["Reading"] = value;
        asyncResp->res.jsonValue["SpeedPercent"]["DataSourceUri"] =
            crow::utility::urlFromPieces("redfish", "v1", "Chassis", chassisId,
                                         "Sensors", "Fans", fanId);
        asyncResp->res.jsonValue["SpeedPercent"]["@odata.id"] =
            crow::utility::urlFromPieces("redfish", "v1", "Chassis", chassisId,
                                         "Sensors", "Fans", fanId);
        });
}

inline void
    getFanSensorStatus(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& service, const std::string& path,
                       const std::vector<std::string>& interfaces,
                       const std::string& chassisId, const std::string& fanId)
{
    for (const auto& intf : interfaces)
    {
        if (intf == "xyz.openbmc_project.State.Decorator.OperationalStatus")
        {
            getFanHealth(asyncResp, service, path, intf);
        }
        if (intf == "xyz.openbmc_project.Sensor.Value")
        {
            getFanSpeedPercent(asyncResp, service, path, intf, chassisId,
                               fanId);
        }
    }
}

inline void getFanState(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& service, const std::string& path,
                        const std::string& intf)
{
    sdbusplus::asio::getProperty<bool>(
        *crow::connections::systemBus, service, path, intf, "Present",
        [asyncResp](const boost::system::error_code ec, const bool value) {
        if (ec)
        {
            return;
        }

        asyncResp->res.jsonValue["Status"]["State"] =
            value ? "Enabled" : "Absent";
        });
}

inline void getFanAsset(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& service, const std::string& path,
                        const std::string& intf)
{
    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, service, path, intf,
        [asyncResp](const boost::system::error_code ec,
                    const dbus::utility::DBusPropertiesMap& propertiesList) {
        if (ec)
        {
            return;
        }

        const std::string* partNumber = nullptr;
        const std::string* serialNumber = nullptr;
        const std::string* manufacturer = nullptr;
        const std::string* model = nullptr;

        const bool success = sdbusplus::unpackPropertiesNoThrow(
            dbus_utils::UnpackErrorPrinter(), propertiesList, "PartNumber",
            partNumber, "SerialNumber", serialNumber, "Manufacturer",
            manufacturer, "Model", model);

        if (!success)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        if (partNumber != nullptr)
        {
            asyncResp->res.jsonValue["PartNumber"] = *partNumber;
        }

        if (serialNumber != nullptr)
        {
            asyncResp->res.jsonValue["SerialNumber"] = *serialNumber;
        }

        if (manufacturer != nullptr)
        {
            asyncResp->res.jsonValue["Manufacturer"] = *manufacturer;
        }

        if (model != nullptr)
        {
            asyncResp->res.jsonValue["Model"] = *model;
        }
        });
}

inline void getFanLocation(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const std::string& service, const std::string& path,
                           const std::string& intf)
{
    sdbusplus::asio::getProperty<std::string>(
        *crow::connections::systemBus, service, path, intf, "LocationCode",
        [asyncResp](const boost::system::error_code ec,
                    const std::string& value) {
        if (ec)
        {
            return;
        }

        asyncResp->res.jsonValue["Location"]["PartLocation"]["ServiceLabel"] =
            value;
        });
}

template <typename Callback>
inline void getObject(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const std::string& endpoint, const std::string& chassisId,
                      const std::string& fanId, Callback&& callback)
{
    if (endpoint.empty())
    {
        return;
    }

    crow::connections::systemBus->async_method_call(
        [asyncResp, endpoint, chassisId, fanId,
         callback{std::forward<Callback>(callback)}](
            const boost::system::error_code ec,
            const std::vector<std::pair<std::string, std::vector<std::string>>>&
                object) {
        if (ec)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        for (const auto& obj : object)
        {
            const std::string& service = obj.first;
            callback(asyncResp, service, endpoint, obj.second, chassisId,
                     fanId);
        }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetObject", endpoint,
        std::array<std::string, 0>());
}

inline void doFanCollection(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& chassisId,
                            const std::optional<std::string>& validChassisPath)
{
    if (!validChassisPath)
    {
        BMCWEB_LOG_ERROR << "Not a valid chassis ID" << chassisId;
        messages::resourceNotFound(asyncResp->res, "Chassis", chassisId);
        return;
    }

    asyncResp->res.jsonValue["@odata.type"] = "#FanCollection.FanCollection";
    asyncResp->res.jsonValue["@odata.id"] = crow::utility::urlFromPieces(
        "redfish", "v1", "Chassis", chassisId, "ThermalSubsystem", "Fans");
    asyncResp->res.jsonValue["Name"] = "Fan Collection";
    asyncResp->res.jsonValue["Description"] =
        "The collection of Fan resource instances " + chassisId;

    crow::connections::systemBus->async_method_call(
        [asyncResp, chassisId](
            const boost::system::error_code ec,
            const dbus::utility::MapperGetSubTreePathsResponse& fanPaths) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG << "DBUS response error: " << ec.message();
            messages::internalError(asyncResp->res);
            return;
        }

        for (const std::string& fanPath : fanPaths)
        {
            // get cec fan:
            // /xyz/openbmc_project/inventory/system/chassisName/motherboard/fanName
            sdbusplus::asio::getProperty<std::vector<std::string>>(
                *crow::connections::systemBus,
                "xyz.openbmc_project.ObjectMapper", fanPath + "/sensors",
                "xyz.openbmc_project.Association", "endpoints",
                [asyncResp, chassisId,
                 fanPath](const boost::system::error_code ec1,
                          const std::vector<std::string>& fanSensorsEndpoints) {
                if (ec1)
                {
                    // get Mex Fan:
                    // /xyz/openbmc_project/inventory/system/chassisName/fanName
                    updateFanList(asyncResp, chassisId, fanPath);
                    return;
                }

                for (const auto& fanSensorPath : fanSensorsEndpoints)
                {
                    updateFanList(asyncResp, chassisId, fanSensorPath);
                }
                });
        }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
        *validChassisPath, 0,
        std::array<const char*, 1>{"xyz.openbmc_project.Inventory.Item.Fan"});
}

inline void
    handleFanCollectionGet(App& app, const crow::Request& req,
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

inline void requestRoutesFanCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/ThermalSubsystem/Fans/")
        .privileges(redfish::privileges::getFanCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleFanCollectionGet, std::ref(app)));
}

inline void doFan(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                  const std::string& chassisId, const std::string& fanId,
                  const std::optional<std::string>& validChassisPath)
{
    if (!validChassisPath)
    {
        BMCWEB_LOG_ERROR << "Not a valid chassis ID" << chassisId;
        messages::resourceNotFound(asyncResp->res, "Chassis", chassisId);
        return;
    }

    auto getFanIdFunc = [asyncResp, chassisId,
                         fanId](const std::string& service,
                                const std::string& fanPath,
                                const std::vector<std::string>& interfaces,
                                const std::string& endpoint = "") {
        std::string newPath =
            "/redfish/v1/Chassis/" + chassisId + "/ThermalSubsystem/Fans/";
        asyncResp->res.jsonValue["@odata.type"] = "#Fan.v1_3_0.Fan";
        asyncResp->res.jsonValue["Name"] = fanId;
        asyncResp->res.jsonValue["Id"] = fanId;
        asyncResp->res.jsonValue["@odata.id"] =
            crow::utility::urlFromPieces("redfish", "v1", "Chassis", chassisId,
                                         "ThermalSubsystem", "Fans", fanId);

        for (const auto& intf : interfaces)
        {
            if (intf == "xyz.openbmc_project.Inventory.Item")
            {
                getFanState(asyncResp, service, fanPath, intf);
            }
            if (intf ==
                    "xyz.openbmc_project.State.Decorator.OperationalStatus" &&
                endpoint.empty())
            {
                getFanHealth(asyncResp, service, fanPath, intf);
            }
            if (intf == "xyz.openbmc_project.Inventory.Decorator.Asset")
            {
                getFanAsset(asyncResp, service, fanPath, intf);
            }
            if (intf == "xyz.openbmc_project.Inventory.Decorator.LocationCode")
            {
                getFanLocation(asyncResp, service, fanPath, intf);
            }
        }

        getObject(asyncResp, endpoint, chassisId, fanId,
                  std::move(getFanSensorStatus));
    };

    // Verify that the fan has the correct chassis and whether fan has a
    // chassis association
    getValidfanId(asyncResp, *validChassisPath, fanId, std::move(getFanIdFunc));
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
        std::bind_front(doFan, asyncResp, chassisId, fanId));
}

inline void requestRoutesFan(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/ThermalSubsystem/Fans/<str>/")
        .privileges(redfish::privileges::getFan)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleFanGet, std::ref(app)));
}

} // namespace redfish
