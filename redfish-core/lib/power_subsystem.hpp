// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/resource.hpp"
#include "http_request.hpp"
#include "logging.hpp"
#include "power_supply.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/chassis_utils.hpp"

#include <asm-generic/errno.h>

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>

#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace redfish
{

/**
 * @brief Sums the PowerCapacityWatts of the power supplies in a chassis.
 *
 * One instance is shared by every asynchronous per-PSU D-Bus read. When the
 * last read completes and releases its reference, the destructor stores the
 * total in the response.
 */
struct PowerSubsystemCapacity
{
    explicit PowerSubsystemCapacity(
        const std::shared_ptr<bmcweb::AsyncResp>& asyncRespIn) :
        asyncResp(asyncRespIn)
    {}

    PowerSubsystemCapacity(const PowerSubsystemCapacity&) = delete;
    PowerSubsystemCapacity& operator=(const PowerSubsystemCapacity&) = delete;
    PowerSubsystemCapacity(PowerSubsystemCapacity&&) = delete;
    PowerSubsystemCapacity& operator=(PowerSubsystemCapacity&&) = delete;

    ~PowerSubsystemCapacity()
    {
        // Do not add to a response that already holds an error
        if (found && asyncResp->res.result() == boost::beast::http::status::ok)
        {
            asyncResp->res.jsonValue["CapacityWatts"] = totalWatts;
        }
    }

    std::shared_ptr<bmcweb::AsyncResp> asyncResp;
    uint64_t totalWatts = 0;
    bool found = false;
};

inline void afterGetPowerSupplyCapacity(
    const std::shared_ptr<PowerSubsystemCapacity>& capacity,
    const boost::system::error_code& ec, uint64_t value)
{
    if (ec)
    {
        if (ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR("DBUS response error for PowerCapacityWatts {}",
                             ec.value());
            messages::internalError(capacity->asyncResp->res);
        }
        return;
    }
    // The PDI default is maxint (unknown); do not include it in the total
    if (value == std::numeric_limits<uint64_t>::max())
    {
        return;
    }
    capacity->totalWatts += value;
    capacity->found = true;
}

inline void afterGetPowerSupplies(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec)
    {
        // io_error means no power supplies are associated with the chassis
        if (ec.value() != EBADR && ec.value() != boost::system::errc::io_error)
        {
            BMCWEB_LOG_ERROR("DBUS response error for PowerSupplies {}",
                             ec.value());
            messages::internalError(asyncResp->res);
        }
        return;
    }

    auto capacity = std::make_shared<PowerSubsystemCapacity>(asyncResp);
    for (const auto& [path, serviceMap] : subtree)
    {
        if (serviceMap.empty())
        {
            continue;
        }
        dbus::utility::getProperty<uint64_t>(
            serviceMap.begin()->first, path,
            "xyz.openbmc_project.Inventory.Decorator.PowerAttributes",
            "PowerCapacityWatts",
            std::bind_front(afterGetPowerSupplyCapacity, capacity));
    }
}

inline void getPowerSubsystemCapacity(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId)
{
    dbus::utility::getAssociatedSubTreeById(
        chassisId, "/xyz/openbmc_project/inventory", chassisInterfaces,
        "powered_by", powerSupplyInterface,
        std::bind_front(afterGetPowerSupplies, asyncResp));
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
}

inline void doPowerSubsystemCollectionHead(
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

    redfish::chassis_utils::getValidChassisPath(
        asyncResp, chassisId,
        std::bind_front(doPowerSubsystemCollectionHead, asyncResp, chassisId));
}

inline void afterGetValidChassisPathForGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId,
    const std::optional<std::string>& validChassisPath)
{
    doPowerSubsystemCollection(asyncResp, chassisId, validChassisPath);
    if (!validChassisPath)
    {
        return;
    }
    // D-Bus lookups live here, not in doPowerSubsystemCollection, so that the
    // unit test for the static properties does not need a D-Bus connection.
    getPowerSubsystemCapacity(asyncResp, chassisId);
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
        std::bind_front(afterGetValidChassisPathForGet, asyncResp, chassisId));
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
