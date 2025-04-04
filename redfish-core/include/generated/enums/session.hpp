// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <nlohmann/json.hpp>

namespace session
{
// clang-format off

enum class SessionTypes{
    Invalid,
    HostConsole,
    ManagerConsole,
    IPMI,
    KVMIP,
    OEM,
    Redfish,
    VirtualMedia,
    WebUI,
    OutboundConnection,
};

NLOHMANN_JSON_SERIALIZE_ENUM(SessionTypes, {
    {SessionTypes::Invalid, "Invalid"},
    {SessionTypes::HostConsole, "HostConsole"},
    {SessionTypes::ManagerConsole, "ManagerConsole"},
    {SessionTypes::IPMI, "IPMI"},
    {SessionTypes::KVMIP, "KVMIP"},
    {SessionTypes::OEM, "OEM"},
    {SessionTypes::Redfish, "Redfish"},
    {SessionTypes::VirtualMedia, "VirtualMedia"},
    {SessionTypes::WebUI, "WebUI"},
    {SessionTypes::OutboundConnection, "OutboundConnection"},
});

}
// clang-format on
