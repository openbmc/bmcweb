#pragma once
#include "dbus_singleton.hpp"
#include "dbus_utility.hpp"
#include "persistent_data.hpp"

#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/message/types.hpp>

namespace bmcweb
{

inline void onUserRemoved(sdbusplus::message::message& msg)
{
    sdbusplus::message::object_path p;
    msg.read(p);
    std::string username = p.filename();
    if (username.empty())
    {
        BMCWEB_LOG_DEBUG << "Unable to remove user from path " p;
        return;
    }
    persistent_data::SessionStore::getInstance().removeSessionsByUsername(
        username);
}
inline void registerUserRemovedSignal()
{
    std::string userRemovedMatch =
        sdbusplus::bus::match::rules::interfacesRemoved(
            "/xyz/openbmc_project/user");

    static sdbusplus::bus::match_t userRemovedMatch(
        *crow::connections::systemBus, userRemovedMatch, onUserRemoved,
        nullptr);
}
} // namespace bmcweb
