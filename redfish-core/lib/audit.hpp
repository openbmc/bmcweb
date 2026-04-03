// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "utils/collection.hpp"
#include "utils/dbus_utils.hpp"
#include "persistent_data.hpp"

#include <phosphor-logging/audit/audit.hpp>

namespace redfish
{

static std::optional<std::vector<std::tuple<uint32_t, uint32_t>>>
    bmcwebAllowList;
static std::optional<bool> bmcwebAuditEnabled;


void bmcwebAuEvent(uint32_t pathId, int32_t retval, persistent_data::UserSession session)
{
    // if bmcweb enabled:
    //   if pathId in AllowList:
    //     audit_event();
    if (bmcwebAuditEnabled == std::nullopt)
    {
        // TODO: pack it in getProperty
        auto bus = sdbusplus::bus::new_default();

        constexpr auto objectName = "xyz.openbmc_project.Logging.Audit";
        constexpr auto objectPath =
            "/xyz/openbmc_project/logging/audit/manager";

        auto method = bus.new_method_call(
            objectName, objectPath, "org.freedesktop.DBus.Properties", "Get");

        constexpr auto propertyIntf =
            "xyz.openbmc_project.Logging.Audit.AuditBMCWEB";
        constexpr auto propertyName = "Enabled";

        method.append(propertyIntf, propertyName);
        auto reply = bus.call(method);
        std::variant<bool> prop;
        reply.read(prop);
        bmcwebAuditEnabled = std::get<bool>(prop);
    }

    if (*bmcwebAuditEnabled)
    {
        if (bmcwebAllowList == std::nullopt)
        {
            // TODO: pack it in getProperty
            auto bus = sdbusplus::bus::new_default();
            constexpr auto objectName = "xyz.openbmc_project.Logging.Audit";
            constexpr auto objectPath =
                "/xyz/openbmc_project/logging/audit/manager";

            auto method =
                bus.new_method_call(objectName, objectPath,
                                    "org.freedesktop.DBus.Properties", "Get");

            constexpr auto propertyIntf =
                "xyz.openbmc_project.Logging.Audit.AuditBMCWEB";
            constexpr auto propertyName = "AllowList";

            method.append(propertyIntf, propertyName);
            auto reply = bus.call(method);
            std::variant<std::vector<std::tuple<uint32_t, uint32_t>>> prop;
            reply.read(prop);
            bmcwebAllowList = std::move(
                std::get<std::vector<std::tuple<uint32_t, uint32_t>>>(prop));
        }

        for (auto& palEntry : *bmcwebAllowList)
        {
            if (pathId == std::get<0>(palEntry))
            {
                // TODO: change addr/port get
                auto addr = boost::asio::ip::make_address(session.clientIp);
                boost::asio::ip::tcp::endpoint ep(addr, 443);

                phosphor::logging::audit::audit_event(phosphor::logging::audit::BMCWEB, std::get<1>(palEntry), retval,
                                                      session.username, ep.data(), ep.size());
            }
        }
    }
}

} // namespace redfish
