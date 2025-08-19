// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include "dbus_singleton.hpp"
#include "dbus_utility.hpp"
#include "include/dbus_utility.hpp"
#include "logging.hpp"
#include "ssl_key_handler.hpp"

#include <systemd/sd-bus.h>

#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/message.hpp>

#include <array>
#include <cstddef>
#include <filesystem>
#include <iterator>
#include <memory>
#include <string_view>
#include <system_error>
#include <variant>

namespace crow
{
namespace hostname_monitor
{
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static std::unique_ptr<sdbusplus::bus::match_t> hostnameSignalMonitor;

inline int onPropertyUpdate(sd_bus_message* m, void* /* userdata */,
                            sd_bus_error* retError)
{
    if (retError == nullptr || (sd_bus_error_is_set(retError) != 0))
    {
        BMCWEB_LOG_ERROR("Got sdbus error on match");
        return 0;
    }

    sdbusplus::message_t message(m);
    std::string iface;
    dbus::utility::DBusPropertiesMap changedProperties;

    message.read(iface, changedProperties);
    const std::string* hostname = nullptr;
    for (const auto& propertyPair : changedProperties)
    {
        if (propertyPair.first == "HostName")
        {
            hostname = std::get_if<std::string>(&propertyPair.second);
        }
    }
    if (hostname == nullptr)
    {
        return 0;
    }

    BMCWEB_LOG_DEBUG("Read hostname from signal: {}", *hostname);
    const std::string certFile = "/etc/ssl/certs/https/server.pem";
    ensuressl::regenerateCertificateIfHostnameChanged(certFile, *hostname);

    return 0;
}

inline void registerHostnameSignal()
{
    BMCWEB_LOG_INFO("Register HostName PropertiesChanged Signal");
    std::string propertiesMatchString =
        ("type='signal',"
         "interface='org.freedesktop.DBus.Properties',"
         "path='/xyz/openbmc_project/network/config',"
         "arg0='xyz.openbmc_project.Network.SystemConfiguration',"
         "member='PropertiesChanged'");

    hostnameSignalMonitor = std::make_unique<sdbusplus::bus::match_t>(
        *crow::connections::systemBus, propertiesMatchString, onPropertyUpdate,
        nullptr);
}
} // namespace hostname_monitor
} // namespace crow
