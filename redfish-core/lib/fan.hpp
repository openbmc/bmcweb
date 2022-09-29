#pragma once

#include "app.hpp"
#include "error_messages.hpp"
#include "utils/chassis_utils.hpp"

#include <memory>
#include <optional>
#include <string>

namespace redfish
{

inline void
    updateFanList(const std::shared_ptr<bmcweb::AsyncResp>& /* asyncResp */,
                  const std::string& /* chassisId */,
                  const std::string& /* fanPath */)
{
    // TODO In order for the validator to pass, the Members property will be
    // implemented on the next commit
}

inline void fanCollection(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& fanPath,
                          const std::string& chassisId)
{
    std::string sensorPath = fanPath + "/sensors";
    auto respHandler =
        [asyncResp, fanPath, chassisId](
            const std::optional<dbus::utility::MapperEndPoints>& endpointsPtr) {
        if (!endpointsPtr || endpointsPtr->empty())
        {
            updateFanList(asyncResp, chassisId, fanPath);
            return;
        }

        for (const auto& endpoint : *endpointsPtr)
        {
            updateFanList(asyncResp, chassisId, endpoint);
        }
    };
    dbus::utility::getAssociationEndPoints(asyncResp, sensorPath,
                                           std::move(respHandler));
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
        [asyncResp, chassisId](
            const std::optional<dbus::utility::MapperEndPoints>& endpointsPtr) {
        if (!endpointsPtr || endpointsPtr->empty())
        {
            return;
        }

        for (const auto& endpoint : *endpointsPtr)
        {
            fanCollection(asyncResp, endpoint, chassisId);
        }
    };
    dbus::utility::getAssociationEndPoints(asyncResp, fanPath,
                                           std::move(respHandler));
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

} // namespace redfish
