// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include "dbus_singleton.hpp"
#include "sessions.hpp"

#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/message/native_types.hpp>

#include <string>

namespace bmcweb
{

inline void onUserRemoved(sdbusplus::message_t& msg)
{
    auto p = msg.unpack<sdbusplus::object_path>();

    std::string username = p.filename();
    persistent_data::SessionStore::getInstance().removeSessionsByUsername(
        username);
}

inline void registerUserRemovedSignal()
{
    std::string userRemovedMatchStr =
        sdbusplus::bus::match::rules::interfacesRemoved(
            "/xyz/openbmc_project/user");

    static sdbusplus::bus::match_t userRemovedMatch(
        *crow::connections::systemBus, userRemovedMatchStr, onUserRemoved);
}

// Drop a user's Redfish sessions whenever User.Attributes.UserEnabled
// transitions to false. Covers Redfish PATCH, IPMI `user disable`, and any
// other writer of the property.
inline void onUserPropertiesChanged(sdbusplus::message_t& msg)
{
    std::string interface;
    dbus::utility::DBusPropertiesMap propertiesMap;
    try
    {
        msg.read(interface, propertiesMap);
    }
    catch (const sdbusplus::exception_t& e)
    {
        BMCWEB_LOG_ERROR("Failed to read PropertiesChanged signal: {}",
                         e.what());
        return;
    }

    if (interface != "xyz.openbmc_project.User.Attributes")
    {
        return;
    }

    const bool* userEnabled = nullptr;
    const bool success = sdbusplus::unpackPropertiesNoThrow(
        redfish::dbus_utils::UnpackErrorPrinter(), propertiesMap, "UserEnabled",
        userEnabled);
    if (!success || userEnabled == nullptr || *userEnabled)
    {
        // Property wasn't in this change, or user is being (re-)enabled.
        return;
    }

    sdbusplus::message::object_path path(msg.get_path());
    std::string username = path.filename();
    if (username.empty())
    {
        return;
    }

    BMCWEB_LOG_INFO("User {} disabled; clearing active sessions", username);
    persistent_data::SessionStore::getInstance().removeSessionsByUsername(
        username);
}

inline void registerUserPropertiesChangedSignal()
{
    std::string matchStr =
        sdbusplus::bus::match::rules::propertiesChangedNamespace(
            "/xyz/openbmc_project/user", "xyz.openbmc_project.User.Attributes");

    static sdbusplus::bus::match_t userPropertiesChangedMatch(
        *crow::connections::systemBus, matchStr, onUserPropertiesChanged);
}
} // namespace bmcweb
