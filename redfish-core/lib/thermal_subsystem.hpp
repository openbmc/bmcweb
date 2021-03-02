#pragma once

#include "redfish_util.hpp"

#include <app.hpp>
#include <registries/privilege_registry.hpp>
#include <utils/json_utils.hpp>

namespace redfish
{

inline void doThermalSubsystemCollection(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId)
{

    asyncResp->res.jsonValue["@odata.type"] =
        "#ThermalSubsystem.v1_0_0.ThermalSubsystem";
    asyncResp->res.jsonValue["Name"] = "Thermal Subsystem";
    asyncResp->res.jsonValue["Id"] = "ThermalSubsystem";

    asyncResp->res.jsonValue["@odata.id"] =
        crow::utility::urlFromPieces("redfish", "v1", "Chassis", chassisId,
                                     "ThermalSubsystem")
            .string();

    asyncResp->res.jsonValue["Status"]["State"] = "Enabled";
    asyncResp->res.jsonValue["Status"]["Health"] = "OK";
}

inline void handleThermalSubsystemCollectionGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp, const std::string&)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    getMainChassisId(asyncResp,
                     [](const std::string& chassisId,
                        const std::shared_ptr<bmcweb::AsyncResp>& aResp) {
        doThermalSubsystemCollection(aResp, chassisId /*, chassisPath*/);
    });
}

inline void requestRoutesThermalSubsystem(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/ThermalSubsystem/")
        .privileges(redfish::privileges::getThermalSubsystem)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleThermalSubsystemCollectionGet, std::ref(app)));
}

} // namespace redfish
