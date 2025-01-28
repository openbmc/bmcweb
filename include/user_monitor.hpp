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
    sdbusplus::message::object_path p;
    msg.read(p);
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
} // namespace bmcweb
