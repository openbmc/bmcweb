#pragma once
#include <nlohmann/json.hpp>

namespace port
{
// clang-format off

enum class PortType{
    Invalid,
    UpstreamPort,
    DownstreamPort,
    InterswitchPort,
    ManagementPort,
    BidirectionalPort,
    UnconfiguredPort,
};

enum class PortMedium{
    Invalid,
    Electrical,
    Optical,
};

enum class LinkState{
    Invalid,
    Enabled,
    Disabled,
};

enum class LinkStatus{
    Invalid,
    LinkUp,
    Starting,
    Training,
    LinkDown,
    NoLink,
};

enum class LinkNetworkTechnology{
    Invalid,
    Ethernet,
    InfiniBand,
    FibreChannel,
    GenZ,
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
    FPort,
    EPort,
    TEPort,
    NPPort,
    GPort,
    NLPort,
    FLPort,
    EXPort,
    UPort,
    DPort,
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

enum class IEEE802IdSubtype{
    Invalid,
    ChassisComp,
    IfAlias,
    PortComp,
    MacAddr,
    NetworkAddr,
    IfName,
    AgentId,
    LocalAssign,
    NotTransmitted,
};

enum class SFPType{
    Invalid,
    SFP,
    SFPPlus,
    SFP28,
    cSFP,
    SFPDD,
    QSFP,
    QSFPPlus,
    QSFP14,
    QSFP28,
    QSFP56,
    MiniSASHD,
};

enum class MediumType{
    Invalid,
    Copper,
    FiberOptic,
};

enum class FiberConnectionType{
    Invalid,
    SingleMode,
    MultiMode,
};

NLOHMANN_JSON_SERIALIZE_ENUM(PortType, {
    {PortType::Invalid, "Invalid"},
    {PortType::UpstreamPort, "UpstreamPort"},
    {PortType::DownstreamPort, "DownstreamPort"},
    {PortType::InterswitchPort, "InterswitchPort"},
    {PortType::ManagementPort, "ManagementPort"},
    {PortType::BidirectionalPort, "BidirectionalPort"},
    {PortType::UnconfiguredPort, "UnconfiguredPort"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(PortMedium, {
    {PortMedium::Invalid, "Invalid"},
    {PortMedium::Electrical, "Electrical"},
    {PortMedium::Optical, "Optical"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(LinkState, {
    {LinkState::Invalid, "Invalid"},
    {LinkState::Enabled, "Enabled"},
    {LinkState::Disabled, "Disabled"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(LinkStatus, {
    {LinkStatus::Invalid, "Invalid"},
    {LinkStatus::LinkUp, "LinkUp"},
    {LinkStatus::Starting, "Starting"},
    {LinkStatus::Training, "Training"},
    {LinkStatus::LinkDown, "LinkDown"},
    {LinkStatus::NoLink, "NoLink"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(LinkNetworkTechnology, {
    {LinkNetworkTechnology::Invalid, "Invalid"},
    {LinkNetworkTechnology::Ethernet, "Ethernet"},
    {LinkNetworkTechnology::InfiniBand, "InfiniBand"},
    {LinkNetworkTechnology::FibreChannel, "FibreChannel"},
    {LinkNetworkTechnology::GenZ, "GenZ"},
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
    {PortConnectionType::FPort, "FPort"},
    {PortConnectionType::EPort, "EPort"},
    {PortConnectionType::TEPort, "TEPort"},
    {PortConnectionType::NPPort, "NPPort"},
    {PortConnectionType::GPort, "GPort"},
    {PortConnectionType::NLPort, "NLPort"},
    {PortConnectionType::FLPort, "FLPort"},
    {PortConnectionType::EXPort, "EXPort"},
    {PortConnectionType::UPort, "UPort"},
    {PortConnectionType::DPort, "DPort"},
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

NLOHMANN_JSON_SERIALIZE_ENUM(IEEE802IdSubtype, {
    {IEEE802IdSubtype::Invalid, "Invalid"},
    {IEEE802IdSubtype::ChassisComp, "ChassisComp"},
    {IEEE802IdSubtype::IfAlias, "IfAlias"},
    {IEEE802IdSubtype::PortComp, "PortComp"},
    {IEEE802IdSubtype::MacAddr, "MacAddr"},
    {IEEE802IdSubtype::NetworkAddr, "NetworkAddr"},
    {IEEE802IdSubtype::IfName, "IfName"},
    {IEEE802IdSubtype::AgentId, "AgentId"},
    {IEEE802IdSubtype::LocalAssign, "LocalAssign"},
    {IEEE802IdSubtype::NotTransmitted, "NotTransmitted"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(SFPType, {
    {SFPType::Invalid, "Invalid"},
    {SFPType::SFP, "SFP"},
    {SFPType::SFPPlus, "SFPPlus"},
    {SFPType::SFP28, "SFP28"},
    {SFPType::cSFP, "cSFP"},
    {SFPType::SFPDD, "SFPDD"},
    {SFPType::QSFP, "QSFP"},
    {SFPType::QSFPPlus, "QSFPPlus"},
    {SFPType::QSFP14, "QSFP14"},
    {SFPType::QSFP28, "QSFP28"},
    {SFPType::QSFP56, "QSFP56"},
    {SFPType::MiniSASHD, "MiniSASHD"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(MediumType, {
    {MediumType::Invalid, "Invalid"},
    {MediumType::Copper, "Copper"},
    {MediumType::FiberOptic, "FiberOptic"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(FiberConnectionType, {
    {FiberConnectionType::Invalid, "Invalid"},
    {FiberConnectionType::SingleMode, "SingleMode"},
    {FiberConnectionType::MultiMode, "MultiMode"},
});

}
// clang-format on
