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
#include <nlohmann/json.hpp>

#include <functional>
#include <memory>
#include <string>

namespace redfish
{
namespace resource_utils
{

/**
 * @brief Fetches resource status.health from DBus interfaces
 *
 */
inline void determineResourceHealth(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const nlohmann::json::json_pointer& jsonPtr, bool functional)
{
    BMCWEB_LOG_DEBUG("determineResourceHealth");

    if (!functional)
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

inline void determineResourceState(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp, bool present,
    bool available, const nlohmann::json::json_pointer& jsonPtr)
{
    BMCWEB_LOG_DEBUG("determineResourceState");

    // Absent takes priority over unavailable
    if (!present)
    {
        asyncResp->res.jsonValue[jsonPtr]["Status"]["State"] =
            resource::State::Absent;
    }
    else if (!available)
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

inline void getStatusAvailableState(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const nlohmann::json::json_pointer& jsonPtr, bool present,
    const boost::system::error_code& ec, bool available)
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
        available = true;
    }
    determineResourceState(asyncResp, present, available, jsonPtr);
}

inline void getStatusPresentState(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& service, const std::string& path,
    const nlohmann::json::json_pointer& jsonPtr,
    const boost::system::error_code& ec, bool present)
{
    if (ec)
    {
        if (ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR("DBUS response error for {}, ec {}", "Present",
                             ec.value());
            messages::internalError(asyncResp->res);
            return;
        }
        // Interface is missing, default to true for Enabled
        present = true;
    }
    dbus::utility::getProperty<bool>(
        *crow::connections::systemBus, service, path,
        "xyz.openbmc_project.State.Decorator.Availability", "Available",
        std::bind_front(getStatusAvailableState, asyncResp, jsonPtr, present));
}

inline void getResourceState(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& service, const std::string& path,
    const nlohmann::json::json_pointer& jsonPtr)
{
    BMCWEB_LOG_DEBUG("getResourceStatus");
    dbus::utility::getProperty<bool>(
        *crow::connections::systemBus, service, path,
        "xyz.openbmc_project.Inventory.Item", "Present",
        std::bind_front(getStatusPresentState, asyncResp, service, path,
                        jsonPtr));
}

inline void afterGetResourceHealth(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const nlohmann::json::json_pointer& jsonPtr,
    const boost::system::error_code& ec, bool functional)
{
    if (ec)
    {
        if (ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR("DBUS response error for {}, ec {}", "Functional",
                             ec.value());
            messages::internalError(asyncResp->res);
            return;
        }
        // Interface missing, default to OK
        functional = true;
    }
    determineResourceHealth(asyncResp, jsonPtr, functional);
}

/*
 * @brief Retrieves the status of the resource's status.health
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
