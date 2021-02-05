#pragma once

#include <app.hpp>
#include <registries/privilege_registry.hpp>
#include <utils/chassis_utils.hpp>
#include <utils/json_utils.hpp>

namespace redfish
{

inline void
    getPowerSubsystem(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const std::string& chassisID)
{
    BMCWEB_LOG_DEBUG
        << "Get properties for PowerSubsystem associated to chassis = "
        << chassisID;

    asyncResp->res.jsonValue = {
        {"@odata.type", "#PowerSubsystem.v1_0_0.PowerSubsystem"},
        {"Name", "Power Subsystem for Chassis"}};
    asyncResp->res.jsonValue["Id"] = "PowerSubsystem";
    asyncResp->res.jsonValue["@odata.id"] =
        "/redfish/v1/Chassis/" + chassisID + "/PowerSubsystem";
    asyncResp->res.jsonValue["PowerSupplies"]["@odata.id"] =
        "/redfish/v1/Chassis/" + chassisID + "/PowerSubsystem/PowerSupplies";
}

inline void requestRoutesPowerSubsystem(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/PowerSubsystem/")
        .privileges(redfish::privileges::getPowerSubsystem)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& chassisID) {
        auto getChassisID =
            [asyncResp,
             chassisID](const std::optional<std::string>& validChassisID) {
            if (!validChassisID)
            {
                BMCWEB_LOG_ERROR << "Not a valid chassis ID:" << chassisID;
                messages::resourceNotFound(asyncResp->res, "Chassis",
                                           chassisID);
                return;
            }

            getPowerSubsystem(asyncResp, chassisID);
        };
        redfish::chassis_utils::getValidChassisID(asyncResp, chassisID,
                                                  std::move(getChassisID));
        });
}

} // namespace redfish
