#pragma once

#include <app.hpp>
#include <utils/chassis_utils.hpp>

namespace redfish
{

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
    BMCWEB_LOG_DEBUG << "getValidFanId enter";

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

        // Set the default value to resourceNotFound, and if we confirm that
        // fanId is correct, the error response will be cleared.
        messages::resourceNotFound(asyncResp->res, "fan", fanId);

        for (auto [objectPath, serviceMap] : subtree)
        {
            for (const auto& [service, interfaces] : serviceMap)
            {
                if (service == "xyz.openbmc_project.PLDM")
                {
                    sdbusplus::asio::getProperty<std::vector<std::string>>(
                        *crow::connections::systemBus,
                        "xyz.openbmc_project.ObjectMapper",
                        objectPath + "/chassis",
                        "xyz.openbmc_project.Association", "endpoints",
                        [callback{std::move(callback)}, asyncResp,
                         fanId](const boost::system::error_code ec1,
                                const std::vector<std::string>&
                                    fanChassisEndpoints) {
                        if (ec1)
                        {
                            if (ec1.value() != EBADR)
                            {
                                messages::internalError(asyncResp->res);
                            }
                            return;
                        }

                        for (const auto& fanChassisPath : fanChassisEndpoints)
                        {
                            if (checkFanId(fanChassisPath, fanId))
                            {
                                // Clear resourceNotFound response
                                asyncResp->res.clear();
                                callback();
                            }
                        }
                        });
                }
                else
                {
                    sdbusplus::asio::getProperty<std::vector<std::string>>(
                        *crow::connections::systemBus,
                        "xyz.openbmc_project.ObjectMapper",
                        objectPath + "/sensors",
                        "xyz.openbmc_project.Association", "endpoints",
                        [callback{std::move(callback)}, asyncResp,
                         fanId](const boost::system::error_code ec2,
                                const std::vector<std::string>&
                                    fanSensorsEndpoints) {
                        if (ec2)
                        {
                            if (ec2.value() != EBADR)
                            {
                                messages::internalError(asyncResp->res);
                            }
                            return;
                        }

                        for (const auto& fanSensorPath : fanSensorsEndpoints)
                        {
                            if (checkFanId(fanSensorPath, fanId))
                            {
                                // Clear resourceNotFound response
                                asyncResp->res.clear();
                                callback();
                            }
                        }
                        });
                }
            }
        }
    };

    // Get the fan Collection
    crow::connections::systemBus->async_method_call(
        respHandler, "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree", chassisPath, 0,
        std::array<const char*, 1>{"xyz.openbmc_project.Inventory.Item.Fan"});
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
        [asyncResp,
         chassisId](const boost::system::error_code ec,
                    const dbus::utility::MapperGetSubTreeResponse& subtree) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG << "DBUS response error: " << ec.message();
            messages::internalError(asyncResp->res);
            return;
        }

        for (const auto& [objectPath, serviceMap] : subtree)
        {
            for (const auto& [service, interfaces] : serviceMap)
            {
                if (service == "xyz.openbmc_project.PLDM")
                {
                    // get Mex Fan:
                    // /xyz/openbmc_project/inventory/system/chassisName/fanName
                    updateFanList(asyncResp, chassisId, objectPath);
                }
                else
                {
                    // get cec fan:
                    // /xyz/openbmc_project/inventory/system/chassisName/motherboard/fanName
                    sdbusplus::asio::getProperty<std::vector<std::string>>(
                        *crow::connections::systemBus,
                        "xyz.openbmc_project.ObjectMapper",
                        objectPath + "/sensors",
                        "xyz.openbmc_project.Association", "endpoints",
                        [asyncResp,
                         chassisId](const boost::system::error_code ec1,
                                    const std::vector<std::string>&
                                        fanSensorsEndpoints) {
                        if (ec1)
                        {
                            BMCWEB_LOG_DEBUG << "DBUS response error: "
                                             << ec1.message();
                            if (ec1.value() == boost::system::errc::io_error)
                            {
                                messages::resourceNotFound(
                                    asyncResp->res, "Chassis", chassisId);
                                return;
                            }

                            messages::internalError(asyncResp->res);
                            return;
                        }

                        for (const auto& fanSensorPath : fanSensorsEndpoints)
                        {
                            updateFanList(asyncResp, chassisId, fanSensorPath);
                        }
                        });
                }
            }
        }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree", *validChassisPath, 0,
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

    auto getFanIdFunc = [asyncResp, chassisId, fanId]() {
        std::string newPath =
            "/redfish/v1/Chassis/" + chassisId + "/ThermalSubsystem/Fans/";
        asyncResp->res.jsonValue["@odata.type"] = "#Fan.v1_3_0.Fan";
        asyncResp->res.jsonValue["Name"] = fanId;
        asyncResp->res.jsonValue["Id"] = fanId;
        asyncResp->res.jsonValue["@odata.id"] =
            crow::utility::urlFromPieces("redfish", "v1", "Chassis", chassisId,
                                         "ThermalSubsystem", "Fans", fanId);
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
