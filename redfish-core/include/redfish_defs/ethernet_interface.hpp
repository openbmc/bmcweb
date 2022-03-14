#pragma once
#include <nlohmann/json.hpp>

namespace ethernet_interface
{
// clang-format off

enum class DHCPFallback{
    Invalid,
    Static,
    AutoConfig,
    None,
};

NLOHMANN_JSON_SERIALIZE_ENUM(DHCPFallback, {
    {DHCPFallback::Invalid, "Invalid"},
    {DHCPFallback::Static, "Static"},
    {DHCPFallback::AutoConfig, "AutoConfig"},
    {DHCPFallback::None, "None"},
});

enum class DHCPv6OperatingMode{
    Invalid,
    Stateful,
    Stateless,
    Disabled,
    Enabled,
};

NLOHMANN_JSON_SERIALIZE_ENUM(DHCPv6OperatingMode, {
    {DHCPv6OperatingMode::Invalid, "Invalid"},
    {DHCPv6OperatingMode::Stateful, "Stateful"},
    {DHCPv6OperatingMode::Stateless, "Stateless"},
    {DHCPv6OperatingMode::Disabled, "Disabled"},
    {DHCPv6OperatingMode::Enabled, "Enabled"},
});

enum class EthernetDeviceType{
    Invalid,
    Physical,
    Virtual,
};

NLOHMANN_JSON_SERIALIZE_ENUM(EthernetDeviceType, {
    {EthernetDeviceType::Invalid, "Invalid"},
    {EthernetDeviceType::Physical, "Physical"},
    {EthernetDeviceType::Virtual, "Virtual"},
});

enum class LinkStatus{
    Invalid,
    LinkUp,
    NoLink,
    LinkDown,
};

NLOHMANN_JSON_SERIALIZE_ENUM(LinkStatus, {
    {LinkStatus::Invalid, "Invalid"},
    {LinkStatus::LinkUp, "LinkUp"},
    {LinkStatus::NoLink, "NoLink"},
    {LinkStatus::LinkDown, "LinkDown"},
});

}
// clang-format on
