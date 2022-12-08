#pragma once
#include <nlohmann/json.hpp>

namespace network_port
{
// clang-format off

enum class LinkStatus{
    Invalid,
    Down,
    Up,
    Starting,
    Training,
};

enum class LinkNetworkTechnology{
    Invalid,
    Ethernet,
    InfiniBand,
    FibreChannel,
};

enum class SupportedEthernetCapabilities{
    Invalid,
    WakeOnLAN,
    EEE,
};

enum class FlowControl{
    Invalid,
    None,
    TX,
    RX,
    TX_RX,
};

enum class PortConnectionType{
    Invalid,
    NotConnected,
    NPort,
    PointToPoint,
    PrivateLoop,
    PublicLoop,
    Generic,
    ExtenderFabric,
};

NLOHMANN_JSON_SERIALIZE_ENUM(LinkStatus, {
    {LinkStatus::Invalid, "Invalid"},
    {LinkStatus::Down, "Down"},
    {LinkStatus::Up, "Up"},
    {LinkStatus::Starting, "Starting"},
    {LinkStatus::Training, "Training"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(LinkNetworkTechnology, {
    {LinkNetworkTechnology::Invalid, "Invalid"},
    {LinkNetworkTechnology::Ethernet, "Ethernet"},
    {LinkNetworkTechnology::InfiniBand, "InfiniBand"},
    {LinkNetworkTechnology::FibreChannel, "FibreChannel"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(SupportedEthernetCapabilities, {
    {SupportedEthernetCapabilities::Invalid, "Invalid"},
    {SupportedEthernetCapabilities::WakeOnLAN, "WakeOnLAN"},
    {SupportedEthernetCapabilities::EEE, "EEE"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(FlowControl, {
    {FlowControl::Invalid, "Invalid"},
    {FlowControl::None, "None"},
    {FlowControl::TX, "TX"},
    {FlowControl::RX, "RX"},
    {FlowControl::TX_RX, "TX_RX"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(PortConnectionType, {
    {PortConnectionType::Invalid, "Invalid"},
    {PortConnectionType::NotConnected, "NotConnected"},
    {PortConnectionType::NPort, "NPort"},
    {PortConnectionType::PointToPoint, "PointToPoint"},
    {PortConnectionType::PrivateLoop, "PrivateLoop"},
    {PortConnectionType::PublicLoop, "PublicLoop"},
    {PortConnectionType::Generic, "Generic"},
    {PortConnectionType::ExtenderFabric, "ExtenderFabric"},
});

}
// clang-format on
