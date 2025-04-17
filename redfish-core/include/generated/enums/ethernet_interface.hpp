// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <nlohmann/json.hpp>

namespace ethernet_interface
{
// clang-format off

enum class MACAddress{
    Invalid,
};

enum class LinkStatus{
    Invalid,
    LinkUp,
    NoLink,
    LinkDown,
};

enum class DHCPv6OperatingMode{
    Invalid,
    Stateful,
    Stateless,
    Disabled,
    Enabled,
};

enum class DHCPFallback{
    Invalid,
    Static,
    AutoConfig,
    None,
};

enum class EthernetDeviceType{
    Invalid,
    Physical,
    Virtual,
};

enum class TeamMode{
    Invalid,
    None,
    RoundRobin,
    ActiveBackup,
    XOR,
    Broadcast,
    IEEE802_3ad,
    AdaptiveTransmitLoadBalancing,
    AdaptiveLoadBalancing,
};

enum class RoutingScope{
    Invalid,
    External,
    HostOnly,
    Internal,
    Limited,
};

NLOHMANN_JSON_SERIALIZE_ENUM(MACAddress, {
    {MACAddress::Invalid, "Invalid"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(LinkStatus, {
    {LinkStatus::Invalid, "Invalid"},
    {LinkStatus::LinkUp, "LinkUp"},
    {LinkStatus::NoLink, "NoLink"},
    {LinkStatus::LinkDown, "LinkDown"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(DHCPv6OperatingMode, {
    {DHCPv6OperatingMode::Invalid, "Invalid"},
    {DHCPv6OperatingMode::Stateful, "Stateful"},
    {DHCPv6OperatingMode::Stateless, "Stateless"},
    {DHCPv6OperatingMode::Disabled, "Disabled"},
    {DHCPv6OperatingMode::Enabled, "Enabled"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(DHCPFallback, {
    {DHCPFallback::Invalid, "Invalid"},
    {DHCPFallback::Static, "Static"},
    {DHCPFallback::AutoConfig, "AutoConfig"},
    {DHCPFallback::None, "None"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(EthernetDeviceType, {
    {EthernetDeviceType::Invalid, "Invalid"},
    {EthernetDeviceType::Physical, "Physical"},
    {EthernetDeviceType::Virtual, "Virtual"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(TeamMode, {
    {TeamMode::Invalid, "Invalid"},
    {TeamMode::None, "None"},
    {TeamMode::RoundRobin, "RoundRobin"},
    {TeamMode::ActiveBackup, "ActiveBackup"},
    {TeamMode::XOR, "XOR"},
    {TeamMode::Broadcast, "Broadcast"},
    {TeamMode::IEEE802_3ad, "IEEE802_3ad"},
    {TeamMode::AdaptiveTransmitLoadBalancing, "AdaptiveTransmitLoadBalancing"},
    {TeamMode::AdaptiveLoadBalancing, "AdaptiveLoadBalancing"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(RoutingScope, {
    {RoutingScope::Invalid, "Invalid"},
    {RoutingScope::External, "External"},
    {RoutingScope::HostOnly, "HostOnly"},
    {RoutingScope::Internal, "Internal"},
    {RoutingScope::Limited, "Limited"},
});

}
// clang-format on
