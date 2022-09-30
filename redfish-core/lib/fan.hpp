#pragma once

#include "app.hpp"
#include "utils/chassis_utils.hpp"

#include <memory>
#include <optional>
#include <string>

namespace redfish
{

inline void getEndPoints(
    const std::string& path,
    std::function<void(const std::vector<std::string>&)>&& callback)
{
    sdbusplus::asio::getProperty<std::vector<std::string>>(
        *crow::connections::systemBus, "xyz.openbmc_project.ObjectMapper", path,
        "xyz.openbmc_project.Association", "endpoints",
        [callback{std::move(callback)}](
            const boost::system::error_code ec,
            const std::vector<std::string>& endpoints) {
        if (ec)
        {
            callback(std::vector<std::string>{});
            return;
        }

        callback(endpoints);
        });
}

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

inline void fanCollection(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& fanPath,
                          const std::string& chassisId)
{
    std::string sensorPath = fanPath + "/sensors";
    auto respHandler = [asyncResp, fanPath,
                        chassisId](const std::vector<std::string>& endpoints) {
        if (endpoints.empty())
        {
            updateFanList(asyncResp, chassisId, fanPath);
            return;
        }

        for (const auto& endpoint : endpoints)
        {
            updateFanList(asyncResp, chassisId, endpoint);
        }
    };
    getEndPoints(sensorPath, std::move(respHandler));
}

inline void doFanCollection(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
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
        "</redfish/v1/JsonSchemas/FanCollection/FanCollection.json>; rel=describedby");
    asyncResp->res.jsonValue["@odata.type"] = "#FanCollection.FanCollection";
    asyncResp->res.jsonValue["@odata.id"] = crow::utility::urlFromPieces(
        "redfish", "v1", "Chassis", chassisId, "ThermalSubsystem", "Fans");
    asyncResp->res.jsonValue["Name"] = "Fan Collection";
    asyncResp->res.jsonValue["Description"] =
        "The collection of Fan resource instances " + chassisId;
    asyncResp->res.jsonValue["Members"] = nlohmann::json::array();
    asyncResp->res.jsonValue["Members@odata.count"] = 0;

    std::string fanPath = *validChassisPath + "/cooled_by";
    auto respHandler =
        [asyncResp, chassisId](const std::vector<std::string>& endpoints) {
        for (const auto& endpoint : endpoints)
        {
            fanCollection(asyncResp, endpoint, chassisId);
        }
    };
    getEndPoints(fanPath, std::move(respHandler));
}

inline void
    handleFanCollectionHead(App& app, const crow::Request& req,
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
            "</redfish/v1/JsonSchemas/FanCollection/FanCollection.json>; rel=describedby");
    };
    redfish::chassis_utils::getValidChassisPath(asyncResp, chassisId,
                                                std::move(respHandler));
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
        .privileges(redfish::privileges::headFanCollection)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handleFanCollectionHead, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/ThermalSubsystem/Fans/")
        .privileges(redfish::privileges::getFanCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleFanCollectionGet, std::ref(app)));
}

inline void getValidFanPath(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& validChassisPath,
                            const std::string& chassisId,
                            const std::string& fanId,
                            std::function<void()>&& callback)
{

    std::string fanPath = validChassisPath + "/cooled_by";
    auto respInventoryHandler =
        [asyncResp, chassisId, fanId, callback{std::move(callback)}](
            const std::vector<std::string>& ivEndpoints) {
        if (ivEndpoints.empty())
        {
            messages::resourceNotFound(asyncResp->res, "Fan", fanId);
            return;
        }

        for (const auto& ivEndpoint : ivEndpoints)
        {
            std::string sensorPath = ivEndpoint + "/sensors";
            auto respSensorHandler =
                [asyncResp, chassisId, fanId, ivEndpoint,
                 callback](const std::vector<std::string>& sensorEndpoints) {
                if (sensorEndpoints.empty())
                {
                    if (checkFanId(ivEndpoint, fanId))
                    {
                        callback();
                        return;
                    }
                    return;
                }

                for (const auto& sensorEndpoint : sensorEndpoints)
                {
                    if (checkFanId(sensorEndpoint, fanId))
                    {
                        callback();
                        return;
                    }
                }
            };
            getEndPoints(sensorPath, std::move(respSensorHandler));
        }

        return;
    };
    getEndPoints(fanPath, std::move(respInventoryHandler));
}

inline void doFanGet(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     const std::string& chassisId, const std::string& fanId,
                     const std::optional<std::string>& validChassisPath)
{
    if (!validChassisPath)
    {
        messages::resourceNotFound(asyncResp->res, "Chassis", chassisId);
        return;
    }

    auto getFanIdFunc = [asyncResp, chassisId, fanId]() {
        asyncResp->res.addHeader(
            boost::beast::http::field::link,
            "</redfish/v1/JsonSchemas/Fan/Fan.json>; rel=describedby");
        asyncResp->res.jsonValue["@odata.type"] = "#Fan.v1_3_0.Fan";
        asyncResp->res.jsonValue["Name"] = fanId;
        asyncResp->res.jsonValue["Id"] = fanId;
        asyncResp->res.jsonValue["@odata.id"] =
            crow::utility::urlFromPieces("redfish", "v1", "Chassis", chassisId,
                                         "ThermalSubsystem", "Fans", fanId);
    };

    getValidFanPath(asyncResp, *validChassisPath, chassisId, fanId,
                    std::move(getFanIdFunc));
}

inline void handleFanHead(App& app, const crow::Request& req,
                          const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& chassisId,
                          const std::string& fanId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    auto respValidChassisHandler =
        [asyncResp, chassisId,
         fanId](const std::optional<std::string>& validChassisPath) {
        if (!validChassisPath)
        {
            messages::resourceNotFound(asyncResp->res, "Fan", fanId);
            return;
        }

        auto respValidFanHandler = [asyncResp]() {
            asyncResp->res.addHeader(
                boost::beast::http::field::link,
                "</redfish/v1/JsonSchemas/Fan/Fan.json>; rel=describedby");
        };
        // Get the correct Path and Service that match the input parameters
        getValidFanPath(asyncResp, *validChassisPath, chassisId, fanId,
                        std::move(respValidFanHandler));
    };
    redfish::chassis_utils::getValidChassisPath(
        asyncResp, chassisId, std::move(respValidChassisHandler));
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

inline void requestRoutesFan(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/ThermalSubsystem/Fans/<str>/")
        .privileges(redfish::privileges::headFan)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handleFanHead, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/ThermalSubsystem/Fans/<str>/")
        .privileges(redfish::privileges::getFan)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleFanGet, std::ref(app)));
}

} // namespace redfish
