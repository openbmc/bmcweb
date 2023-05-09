#pragma once
#include <boost/describe/enum.hpp>
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
    PCIe,
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
    QSFPDD,
    OSFP,
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

enum class LLDPSystemCapabilities{
    Invalid,
    None,
    Bridge,
    DOCSISCableDevice,
    Other,
    Repeater,
    Router,
    Station,
    Telephone,
    WLANAccessPoint,
};

enum class CurrentPortConfigurationState{
    Invalid,
    Disabled,
    BindInProgress,
    UnbindInProgress,
    DSP,
    USP,
    Reserved,
    FabricLink,
};

enum class ConnectedDeviceMode{
    Invalid,
    Disconnected,
    RCD,
    CXL68BFlitAndVH,
    Standard256BFlit,
    CXLLatencyOptimized256BFlit,
    PBR,
};

enum class ConnectedDeviceType{
    Invalid,
    None,
    PCIeDevice,
    Type1,
    Type2,
    Type3SLD,
    Type3MLD,
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

BOOST_DESCRIBE_ENUM(PortType,

    Invalid,
    UpstreamPort,
    DownstreamPort,
    InterswitchPort,
    ManagementPort,
    BidirectionalPort,
    UnconfiguredPort,
);

NLOHMANN_JSON_SERIALIZE_ENUM(PortMedium, {
    {PortMedium::Invalid, "Invalid"},
    {PortMedium::Electrical, "Electrical"},
    {PortMedium::Optical, "Optical"},
});

BOOST_DESCRIBE_ENUM(PortMedium,

    Invalid,
    Electrical,
    Optical,
);

NLOHMANN_JSON_SERIALIZE_ENUM(LinkState, {
    {LinkState::Invalid, "Invalid"},
    {LinkState::Enabled, "Enabled"},
    {LinkState::Disabled, "Disabled"},
});

BOOST_DESCRIBE_ENUM(LinkState,

    Invalid,
    Enabled,
    Disabled,
);

NLOHMANN_JSON_SERIALIZE_ENUM(LinkStatus, {
    {LinkStatus::Invalid, "Invalid"},
    {LinkStatus::LinkUp, "LinkUp"},
    {LinkStatus::Starting, "Starting"},
    {LinkStatus::Training, "Training"},
    {LinkStatus::LinkDown, "LinkDown"},
    {LinkStatus::NoLink, "NoLink"},
});

BOOST_DESCRIBE_ENUM(LinkStatus,

    Invalid,
    LinkUp,
    Starting,
    Training,
    LinkDown,
    NoLink,
);

NLOHMANN_JSON_SERIALIZE_ENUM(LinkNetworkTechnology, {
    {LinkNetworkTechnology::Invalid, "Invalid"},
    {LinkNetworkTechnology::Ethernet, "Ethernet"},
    {LinkNetworkTechnology::InfiniBand, "InfiniBand"},
    {LinkNetworkTechnology::FibreChannel, "FibreChannel"},
    {LinkNetworkTechnology::GenZ, "GenZ"},
    {LinkNetworkTechnology::PCIe, "PCIe"},
});

BOOST_DESCRIBE_ENUM(LinkNetworkTechnology,

    Invalid,
    Ethernet,
    InfiniBand,
    FibreChannel,
    GenZ,
    PCIe,
);

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

BOOST_DESCRIBE_ENUM(PortConnectionType,

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
);

NLOHMANN_JSON_SERIALIZE_ENUM(SupportedEthernetCapabilities, {
    {SupportedEthernetCapabilities::Invalid, "Invalid"},
    {SupportedEthernetCapabilities::WakeOnLAN, "WakeOnLAN"},
    {SupportedEthernetCapabilities::EEE, "EEE"},
});

BOOST_DESCRIBE_ENUM(SupportedEthernetCapabilities,

    Invalid,
    WakeOnLAN,
    EEE,
);

NLOHMANN_JSON_SERIALIZE_ENUM(FlowControl, {
    {FlowControl::Invalid, "Invalid"},
    {FlowControl::None, "None"},
    {FlowControl::TX, "TX"},
    {FlowControl::RX, "RX"},
    {FlowControl::TX_RX, "TX_RX"},
});

BOOST_DESCRIBE_ENUM(FlowControl,

    Invalid,
    None,
    TX,
    RX,
    TX_RX,
);

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

BOOST_DESCRIBE_ENUM(IEEE802IdSubtype,

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
);

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
    {SFPType::QSFPDD, "QSFPDD"},
    {SFPType::OSFP, "OSFP"},
});

BOOST_DESCRIBE_ENUM(SFPType,

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
    QSFPDD,
    OSFP,
);

NLOHMANN_JSON_SERIALIZE_ENUM(MediumType, {
    {MediumType::Invalid, "Invalid"},
    {MediumType::Copper, "Copper"},
    {MediumType::FiberOptic, "FiberOptic"},
});

BOOST_DESCRIBE_ENUM(MediumType,

    Invalid,
    Copper,
    FiberOptic,
);

NLOHMANN_JSON_SERIALIZE_ENUM(FiberConnectionType, {
    {FiberConnectionType::Invalid, "Invalid"},
    {FiberConnectionType::SingleMode, "SingleMode"},
    {FiberConnectionType::MultiMode, "MultiMode"},
});

BOOST_DESCRIBE_ENUM(FiberConnectionType,

    Invalid,
    SingleMode,
    MultiMode,
);

NLOHMANN_JSON_SERIALIZE_ENUM(LLDPSystemCapabilities, {
    {LLDPSystemCapabilities::Invalid, "Invalid"},
    {LLDPSystemCapabilities::None, "None"},
    {LLDPSystemCapabilities::Bridge, "Bridge"},
    {LLDPSystemCapabilities::DOCSISCableDevice, "DOCSISCableDevice"},
    {LLDPSystemCapabilities::Other, "Other"},
    {LLDPSystemCapabilities::Repeater, "Repeater"},
    {LLDPSystemCapabilities::Router, "Router"},
    {LLDPSystemCapabilities::Station, "Station"},
    {LLDPSystemCapabilities::Telephone, "Telephone"},
    {LLDPSystemCapabilities::WLANAccessPoint, "WLANAccessPoint"},
});

BOOST_DESCRIBE_ENUM(LLDPSystemCapabilities,

    Invalid,
    None,
    Bridge,
    DOCSISCableDevice,
    Other,
    Repeater,
    Router,
    Station,
    Telephone,
    WLANAccessPoint,
);

NLOHMANN_JSON_SERIALIZE_ENUM(CurrentPortConfigurationState, {
    {CurrentPortConfigurationState::Invalid, "Invalid"},
    {CurrentPortConfigurationState::Disabled, "Disabled"},
    {CurrentPortConfigurationState::BindInProgress, "BindInProgress"},
    {CurrentPortConfigurationState::UnbindInProgress, "UnbindInProgress"},
    {CurrentPortConfigurationState::DSP, "DSP"},
    {CurrentPortConfigurationState::USP, "USP"},
    {CurrentPortConfigurationState::Reserved, "Reserved"},
    {CurrentPortConfigurationState::FabricLink, "FabricLink"},
});

BOOST_DESCRIBE_ENUM(CurrentPortConfigurationState,

    Invalid,
    Disabled,
    BindInProgress,
    UnbindInProgress,
    DSP,
    USP,
    Reserved,
    FabricLink,
);

NLOHMANN_JSON_SERIALIZE_ENUM(ConnectedDeviceMode, {
    {ConnectedDeviceMode::Invalid, "Invalid"},
    {ConnectedDeviceMode::Disconnected, "Disconnected"},
    {ConnectedDeviceMode::RCD, "RCD"},
    {ConnectedDeviceMode::CXL68BFlitAndVH, "CXL68BFlitAndVH"},
    {ConnectedDeviceMode::Standard256BFlit, "Standard256BFlit"},
    {ConnectedDeviceMode::CXLLatencyOptimized256BFlit, "CXLLatencyOptimized256BFlit"},
    {ConnectedDeviceMode::PBR, "PBR"},
});

BOOST_DESCRIBE_ENUM(ConnectedDeviceMode,

    Invalid,
    Disconnected,
    RCD,
    CXL68BFlitAndVH,
    Standard256BFlit,
    CXLLatencyOptimized256BFlit,
    PBR,
);

NLOHMANN_JSON_SERIALIZE_ENUM(ConnectedDeviceType, {
    {ConnectedDeviceType::Invalid, "Invalid"},
    {ConnectedDeviceType::None, "None"},
    {ConnectedDeviceType::PCIeDevice, "PCIeDevice"},
    {ConnectedDeviceType::Type1, "Type1"},
    {ConnectedDeviceType::Type2, "Type2"},
    {ConnectedDeviceType::Type3SLD, "Type3SLD"},
    {ConnectedDeviceType::Type3MLD, "Type3MLD"},
});

BOOST_DESCRIBE_ENUM(ConnectedDeviceType,

    Invalid,
    None,
    PCIeDevice,
    Type1,
    Type2,
    Type3SLD,
    Type3MLD,
);

}
// clang-format on
