#pragma once

#include "app.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/resource.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/chassis_utils.hpp"
#include "utils/dbus_utils.hpp"

#include <boost/url/format.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

namespace redfish
{

inline void getPowerSubsystemAllocation(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    // Get max from CapLimit
    sdbusplus::asio::getProperty<uint32_t>(
        *crow::connections::systemBus, "org.open_power.OCC.Control",
        "/xyz/openbmc_project/control/host0/power_cap_limits",
        "xyz.openbmc_project.Control.Power.CapLimits", "MaxPowerCapValue",
        [asyncResp](const boost::system::error_code& ec, uint32_t maxCap) {
            if (ec)
            {
                if (ec.value() != EBADR)
                {
                    BMCWEB_LOG_ERROR("DBUS response error: {}", ec.value());
                    messages::internalError(asyncResp->res);
                }
                return;
            }
            asyncResp->res.jsonValue["Allocation"]["AllocatedWatts"] = maxCap;
            asyncResp->res.jsonValue["Allocation"]["RequestedWatts"] = maxCap;
        });

    // Get power cap enable status and the cap (if set)
    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, "xyz.openbmc_project.Settings",
        "/xyz/openbmc_project/control/host0/power_cap",
        "xyz.openbmc_project.Control.Power.Cap",
        [asyncResp](const boost::system::error_code& ec,
                    const dbus::utility::DBusPropertiesMap& propertiesList) {
            if (ec)
            {
                if (ec.value() != EBADR)
                {
                    BMCWEB_LOG_ERROR("DBUS response error: {}", ec.value());
                    messages::internalError(asyncResp->res);
                }
                return;
            }

            const uint32_t* powerCap = nullptr;
            const bool* powerCapEnable = nullptr;
            const bool success = sdbusplus::unpackPropertiesNoThrow(
                dbus_utils::UnpackErrorPrinter(), propertiesList, "PowerCap",
                powerCap, "PowerCapEnable", powerCapEnable);

            if (!success)
            {
                messages::internalError(asyncResp->res);
                return;
            }

            if ((powerCap != nullptr) && (powerCapEnable != nullptr))
            {
                if (*powerCapEnable)
                {
                    asyncResp->res.jsonValue["Allocation"]["AllocatedWatts"] =
                        *powerCap;
                }
            }
        });
}

inline void doPowerSubsystemCollection(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
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
        "</redfish/v1/JsonSchemas/PowerSubsystem/PowerSubsystem.json>; rel=describedby");
    asyncResp->res.jsonValue["@odata.type"] =
        "#PowerSubsystem.v1_1_0.PowerSubsystem";
    asyncResp->res.jsonValue["Name"] = "Power Subsystem";
    asyncResp->res.jsonValue["Id"] = "PowerSubsystem";
    asyncResp->res.jsonValue["@odata.id"] =
        boost::urls::format("/redfish/v1/Chassis/{}/PowerSubsystem", chassisId);
    asyncResp->res.jsonValue["Status"]["State"] = resource::State::Enabled;
    asyncResp->res.jsonValue["Status"]["Health"] = resource::Health::OK;
    asyncResp->res.jsonValue["PowerSupplies"]["@odata.id"] =
        boost::urls::format(
            "/redfish/v1/Chassis/{}/PowerSubsystem/PowerSupplies", chassisId);

    getPowerSubsystemAllocation(asyncResp);
}

inline void handlePowerSubsystemCollectionHead(
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
            "</redfish/v1/JsonSchemas/PowerSubsystem/PowerSubsystem.json>; rel=describedby");
    };
    redfish::chassis_utils::getValidChassisPath(asyncResp, chassisId,
                                                std::move(respHandler));
}

inline void handlePowerSubsystemCollectionGet(
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
        std::bind_front(doPowerSubsystemCollection, asyncResp, chassisId));
}

inline void requestRoutesPowerSubsystem(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/PowerSubsystem/")
        .privileges(redfish::privileges::headPowerSubsystem)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handlePowerSubsystemCollectionHead, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/PowerSubsystem/")
        .privileges(redfish::privileges::getPowerSubsystem)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handlePowerSubsystemCollectionGet, std::ref(app)));
}

} // namespace redfish
