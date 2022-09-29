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
    dbus::utility::getAssociationEndPoints(
        fanPath + "/sensors",
        [asyncResp, fanPath,
         chassisId](const boost::system::error_code& ec,
                    const dbus::utility::MapperEndPoints& endpoints) {
        if (ec.value() != EBADR)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        if (endpoints.empty())
        {
            updateFanList(asyncResp, chassisId, fanPath);
            return;
        }

        for (const auto& endpoint : endpoints)
        {
            updateFanList(asyncResp, chassisId, endpoint);
        }
        });
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

    dbus::utility::getAssociationEndPoints(
        *validChassisPath + "/cooled_by",
        [asyncResp,
         chassisId](const boost::system::error_code& ec,
                    const dbus::utility::MapperEndPoints& endpoints) {
        if (ec.value() != EBADR)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        for (const auto& endpoint : endpoints)
        {
            fanCollection(asyncResp, endpoint, chassisId);
        }
        });
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

    redfish::chassis_utils::getValidChassisPath(
        asyncResp, chassisId,
        [asyncResp,
         chassisId](const std::optional<std::string>& validChassisPath) {
        if (!validChassisPath)
        {
            messages::resourceNotFound(asyncResp->res, "Chassis", chassisId);
            return;
        }
        asyncResp->res.addHeader(
            boost::beast::http::field::link,
            "</redfish/v1/JsonSchemas/FanCollection/FanCollection.json>; rel=describedby");
        });
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
