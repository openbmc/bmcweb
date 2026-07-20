// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/resource.hpp"
#include "logging.hpp"

#include <asm-generic/errno.h>

#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/message/native_types.hpp>

#include <functional>
#include <memory>
#include <string>

namespace redfish
{
namespace resource_utils
{

struct ResourceStatus
{
    std::optional<bool> present;
    std::optional<bool> available;
};

/**
 * @brief Fetches resource status.health from DBus interfaces
 *
 */
inline void determineResourceHealth(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const nlohmann::json::json_pointer& jsonPtr,
    const std::optional<bool>& functional)
{
    BMCWEB_LOG_DEBUG("determineResourceHealth");

    // Absent takes priority over unavailable
    if (!functional.value_or(true))
    {
        asyncResp->res.jsonValue[jsonPtr]["Status"]["Health"] =
            resource::Health::Critical;
    }
    else
    {
        asyncResp->res.jsonValue[jsonPtr]["Status"]["Health"] =
            resource::Health::OK;
    }
}

/**
 * @brief Fetches resource status.state from DBus interfaces
 *
 */
inline void determineResourceState(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::shared_ptr<ResourceStatus>& status,
    const nlohmann::json::json_pointer& jsonPtr)
{
    BMCWEB_LOG_DEBUG("determineResourceState");
    if (!status->present.has_value() || !status->available.has_value())
    {
        return;
    }

    // Absent takes priority over unavailable
    if (!status->present.value())
    {
        asyncResp->res.jsonValue[jsonPtr]["Status"]["State"] =
            resource::State::Absent;
    }
    else if (!status->available.value())
    {
        asyncResp->res.jsonValue[jsonPtr]["Status"]["State"] =
            resource::State::UnavailableOffline;
    }
    else
    {
        asyncResp->res.jsonValue[jsonPtr]["Status"]["State"] =
            resource::State::Enabled;
    }
}

inline void afterGetStatusProperty(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::shared_ptr<ResourceStatus>& status, const std::string& property,
    const nlohmann::json::json_pointer& jsonPtr,
    std::function<void(ResourceStatus&, bool)>& callback,
    const boost::system::error_code& ec, bool value)
{
    if (ec)
    {
        if (ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR("DBUS response error for {}, ec {}", property,
                             ec.value());
            messages::internalError(asyncResp->res);
            return;
        }
        // Default to true for Enabled/OK
        callback(*status, true);
    }
    else
    {
        callback(*status, value);
    }
    determineResourceState(asyncResp, status, jsonPtr);
}

/**
 * @brief Helper to fetch boolean property and update status
 *
 */
inline void getStatusProperty(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::shared_ptr<ResourceStatus>& status, const std::string& service,
    const std::string& path, const std::string& interface,
    const std::string& property, const nlohmann::json::json_pointer& jsonPtr,
    std::function<void(ResourceStatus&, bool)>&& callback)
{
    dbus::utility::getProperty<bool>(
        *crow::connections::systemBus, service, path, interface, property,
        std::bind_front(afterGetStatusProperty, asyncResp, status, property,
                        jsonPtr, std::move(callback)));
}

/*
 * @brief Retrieves the status of the resource's status.state
 *
 * Queries two interfaces:
 * - xyz.openbmc_project.Inventory.Item::Present
 * - xyz.openbmc_project.State.Decorator.Availability::Available
 */
inline void getResourceState(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& service, const std::string& path,
    const nlohmann::json::json_pointer& jsonPtr)
{
    BMCWEB_LOG_DEBUG("getResourceStatus");
    auto status = std::make_shared<ResourceStatus>();

    getStatusProperty(asyncResp, status, service, path,
                      "xyz.openbmc_project.Inventory.Item", "Present", jsonPtr,
                      [](ResourceStatus& s, bool val) { s.present = val; });
    getStatusProperty(asyncResp, status, service, path,
                      "xyz.openbmc_project.State.Decorator.Availability",
                      "Available", jsonPtr,
                      [](ResourceStatus& s, bool val) { s.available = val; });
}

inline void afterGetResourceHealth(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const nlohmann::json::json_pointer& jsonPtr,
    const boost::system::error_code& ec, const std::optional<bool>& functional)
{
    if (ec)
    {
        if (ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR("DBUS response error for {}, ec {}", "Available",
                             ec.value());
            messages::internalError(asyncResp->res);
            return;
        }
    }
    determineResourceHealth(asyncResp, jsonPtr, functional);
}

/*
 * @brief Retrieves the status of the resource's status.state
 *
 * Queries interface:
 * - xyz.openbmc_project.State.Decorator.OperationalStatus::Functional
 */
inline void getResourceHealth(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& service, const std::string& path,
    const nlohmann::json::json_pointer& jsonPtr)
{
    dbus::utility::getProperty<bool>(
        *crow::connections::systemBus, service, path,
        "xyz.openbmc_project.State.Decorator.OperationalStatus", "Functional",
        std::bind_front(afterGetResourceHealth, asyncResp, jsonPtr));
}

} // namespace resource_utils
} // namespace redfish
