// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "persistent_data.hpp"
#include "utils/collection.hpp"
#include "utils/dbus_utils.hpp"

#include <phosphor-logging/audit/audit.hpp>

namespace redfish
{

enum pathIds
{
    REDFISH_V1_ACCOUNT_SERVICE_ACCOUNTS = 0
};

static std::optional<std::vector<std::tuple<uint32_t, uint32_t>>>
    bmcwebAllowList;
static std::optional<bool> bmcwebAuditEnabled;

void bmcwebAuEvent(uint32_t pathId, int32_t retval,
                   persistent_data::UserSession session)
{
    auto addr = boost::asio::ip::make_address(session.clientIp);
    boost::asio::ip::tcp::endpoint ep(addr, 443);

    phosphor::logging::audit::bmcwebAuditEvent(
        pathId, retval, ep, session.username, bmcwebAuditEnabled,
        bmcwebAllowList);
}

} // namespace redfish
