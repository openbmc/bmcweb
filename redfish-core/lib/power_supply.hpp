#pragma once

#include "app.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/chassis_utils.hpp"

#include <memory>
#include <optional>
#include <string>

namespace redfish
{

inline void updatePowerSupplyList(
    const std::shared_ptr<bmcweb::AsyncResp>& /* asyncResp */,
    const std::string& /* chassisId */, const std::string& powerSupplyPath)
{
    std::string powerSupplyName =
        sdbusplus::message::object_path(powerSupplyPath).filename();
    if (powerSupplyName.empty())
    {
        return;
    }

    // TODO In order for the validator to pass, the Members property will be
    // implemented on the next commit
}

inline void getPowerEndPoints(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& path,
    std::function<void(const std::vector<std::string>&)>&& callback)
{
    sdbusplus::asio::getProperty<std::vector<std::string>>(
        *crow::connections::systemBus, "xyz.openbmc_project.ObjectMapper", path,
        "xyz.openbmc_project.Association", "endpoints",
        [asyncResp, callback{std::move(callback)}](
            const boost::system::error_code ec,
            const std::vector<std::string>& endpoints) {
        if (ec)
        {
            if (ec.value() ==
                boost::system::linux_error::bad_request_descriptor)
            {
                messages::resourceNotFound(
                    asyncResp->res, "PowerSupplyCollection", "PowerSubsystem");
                return;
            }
            messages::internalError(asyncResp->res);
            return;
        }

        callback(endpoints);
        });
}

inline void
    doPowerSupplyCollection(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
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
        "</redfish/v1/JsonSchemas/PowerSupplyCollection/PowerSupplyCollection.json>; rel=describedby");
    asyncResp->res.jsonValue["@odata.type"] =
        "#PowerSupplyCollection.PowerSupplyCollection";
    asyncResp->res.jsonValue["Name"] = "Power Supply Collection";
    asyncResp->res.jsonValue["@odata.id"] =
        crow::utility::urlFromPieces("redfish", "v1", "Chassis", chassisId,
                                     "PowerSubsystem", "PowerSupplies");
    asyncResp->res.jsonValue["Description"] =
        "The collection of PowerSupply resource instances.";
    asyncResp->res.jsonValue["Members"] = nlohmann::json::array();
    asyncResp->res.jsonValue["Members@odata.count"] = 0;

    std::string powerPath = *validChassisPath + "/powered_by";
    auto respHandler =
        [asyncResp, chassisId](const std::vector<std::string>& endpoints) {
        for (const auto& endpoint : endpoints)
        {
            updatePowerSupplyList(asyncResp, chassisId, endpoint);
        }
    };
    getPowerEndPoints(asyncResp, powerPath, std::move(respHandler));
}

inline void handlePowerSupplyCollectionHead(
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
            "</redfish/v1/JsonSchemas/PowerSupplyCollection/PowerSupplyCollection.json>; rel=describedby");
    };
    redfish::chassis_utils::getValidChassisPath(asyncResp, chassisId,
                                                std::bind_front(respHandler));
}

inline void handlePowerSupplyCollectionGet(
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
        std::bind_front(doPowerSupplyCollection, asyncResp, chassisId));
}

inline void requestRoutesPowerSupplyCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/PowerSubsystem/PowerSupplies/")
        .privileges(redfish::privileges::headPowerSupplyCollection)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handlePowerSupplyCollectionHead, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/PowerSubsystem/PowerSupplies/")
        .privileges(redfish::privileges::getPowerSupplyCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handlePowerSupplyCollectionGet, std::ref(app)));
}

} // namespace redfish
