#pragma once

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_singleton.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utility.hpp"
#include "utils/chassis_utils.hpp"
#include "utils/dbus_utils.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/system/error_code.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace redfish
{
static constexpr auto powerCapInterface =
    "xyz.openbmc_project.Control.Power.Cap";

inline void getPowerSubsystemAllocationProperties(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& service, const std::string& objectPath)
{
    // Get all properties of Power.Cap D-Bus interface
    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, service, objectPath, powerCapInterface,
        [asyncResp](const boost::system::error_code& ec,
                    const dbus::utility::DBusPropertiesMap& propertiesList) {
        if (ec)
        {
            if (ec.value() != EBADR)
            {
                messages::internalError(asyncResp->res);
            }
            return;
        }

        uint32_t powerCap{0};
        bool powerCapEnable{false};
        uint32_t maxPowerCapValue{0};
        const bool success = sdbusplus::unpackPropertiesNoThrow(
            dbus_utils::UnpackErrorPrinter(), propertiesList, "PowerCap",
            powerCap, "PowerCapEnable", powerCapEnable, "MaxPowerCapValue",
            maxPowerCapValue);

        if (!success)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        // If MaxPowerCapValue valid, store Allocation properties in JSON
        if ((maxPowerCapValue > 0) && (maxPowerCapValue < UINT32_MAX))
        {
            if (powerCapEnable)
            {
                asyncResp->res.jsonValue["Allocation"]["AllocatedWatts"] =
                    powerCap;
            }
            else
            {
                asyncResp->res.jsonValue["Allocation"]["AllocatedWatts"] =
                    maxPowerCapValue;
            }
            asyncResp->res.jsonValue["Allocation"]["RequestedWatts"] =
                maxPowerCapValue;
        }
    });
}

inline void getPowerSubsystemAllocation(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    // Find service and object path that implement PowerCap interface (if any)
    constexpr std::array<std::string_view, 1> interfaces = {powerCapInterface};
    dbus::utility::getSubTree(
        "/", 0, interfaces,
        [asyncResp](const boost::system::error_code& ec,
                    const dbus::utility::MapperGetSubTreeResponse& subTree) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG("D-Bus response error on GetSubTree: {}", ec);
            messages::internalError(asyncResp->res);
            return;
        }

        if (!subTree.empty())
        {
            const auto& [objectPath, serviceMap] = subTree[0];
            if (!serviceMap.empty())
            {
                const auto& service = serviceMap[0].first;

                // Get properties from PowerCap interface and store in JSON
                getPowerSubsystemAllocationProperties(asyncResp, service,
                                                      objectPath);
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
    asyncResp->res.jsonValue["Status"]["State"] = "Enabled";
    asyncResp->res.jsonValue["Status"]["Health"] = "OK";
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
