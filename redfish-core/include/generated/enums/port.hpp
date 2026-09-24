// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
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
    CDFP,
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
    PBRComponent,
};

enum class HostDeviceType{
    Invalid,
    None,
    System,
    Manager,
};

enum class TransceiverManagementInterfaceType{
    Invalid,
    SFP,
    CMIS,
};

enum class UALinkGeneration{
    Invalid,
    UALink128G,
    UALink200G,
};

enum class PCIeReferenceClockMode{
    Invalid,
    CommonClock,
    SeparateClock,
};

enum class PoEPowerMode{
    Invalid,
    PSE,
    PD,
    None,
};

enum class PoEStandard{
    Invalid,
    IEEE8023af,
    IEEE8023at,
    IEEE8023bt,
};

enum class PoEDetectionStatus{
    Invalid,
    Disabled,
    Searching,
    DeliveringPower,
    Fault,
    Test,
    OtherFault,
};

enum class UALink128GExtendedSpeedMode{
    Invalid,
    NoESM,
    104G,
    112G,
    120G,
    128G,
};

enum class UALink200GSerialRate{
    Invalid,
    100G,
    200G,
};

enum class UALink200GCodewordInterleave{
    Invalid,
    OneWay,
    TwoWay,
    FourWay,
};

enum class UALinkPLState{
    Invalid,
    Idle,
    TrainingInProgress,
    TrainingFailed,
    TrainingTimeout,
    Up,
};

enum class UALinkDLState{
    Invalid,
    Idle,
    NOP,
    Fault,
    PowerDown,
    Up,
};

enum class UALinkTLState{
    Invalid,
    DropMode,
    Enabled,
};

enum class UALinkUPLIOriginatorState{
    Invalid,
    DropMode,
    IsolationMode,
    Enabled,
};

enum class UALinkUPLICompleterState{
    Invalid,
    DropMode,
    Enabled,
};

enum class PartnerDeviceType{
    Invalid,
    Switch,
    Endpoint,
};

enum class PartnerValidationState{
    Disabled,
    LinkDown,
    Discovering,
    Invalid,
    Validated,
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
    {LinkNetworkTechnology::PCIe, "PCIe"},
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
    {SFPType::QSFPDD, "QSFPDD"},
    {SFPType::OSFP, "OSFP"},
    {SFPType::CDFP, "CDFP"},
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

NLOHMANN_JSON_SERIALIZE_ENUM(ConnectedDeviceMode, {
    {ConnectedDeviceMode::Invalid, "Invalid"},
    {ConnectedDeviceMode::Disconnected, "Disconnected"},
    {ConnectedDeviceMode::RCD, "RCD"},
    {ConnectedDeviceMode::CXL68BFlitAndVH, "CXL68BFlitAndVH"},
    {ConnectedDeviceMode::Standard256BFlit, "Standard256BFlit"},
    {ConnectedDeviceMode::CXLLatencyOptimized256BFlit, "CXLLatencyOptimized256BFlit"},
    {ConnectedDeviceMode::PBR, "PBR"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(ConnectedDeviceType, {
    {ConnectedDeviceType::Invalid, "Invalid"},
    {ConnectedDeviceType::None, "None"},
    {ConnectedDeviceType::PCIeDevice, "PCIeDevice"},
    {ConnectedDeviceType::Type1, "Type1"},
    {ConnectedDeviceType::Type2, "Type2"},
    {ConnectedDeviceType::Type3SLD, "Type3SLD"},
    {ConnectedDeviceType::Type3MLD, "Type3MLD"},
    {ConnectedDeviceType::PBRComponent, "PBRComponent"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(HostDeviceType, {
    {HostDeviceType::Invalid, "Invalid"},
    {HostDeviceType::None, "None"},
    {HostDeviceType::System, "System"},
    {HostDeviceType::Manager, "Manager"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(TransceiverManagementInterfaceType, {
    {TransceiverManagementInterfaceType::Invalid, "Invalid"},
    {TransceiverManagementInterfaceType::SFP, "SFP"},
    {TransceiverManagementInterfaceType::CMIS, "CMIS"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(UALinkGeneration, {
    {UALinkGeneration::Invalid, "Invalid"},
    {UALinkGeneration::UALink128G, "UALink128G"},
    {UALinkGeneration::UALink200G, "UALink200G"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(PCIeReferenceClockMode, {
    {PCIeReferenceClockMode::Invalid, "Invalid"},
    {PCIeReferenceClockMode::CommonClock, "CommonClock"},
    {PCIeReferenceClockMode::SeparateClock, "SeparateClock"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(PoEPowerMode, {
    {PoEPowerMode::Invalid, "Invalid"},
    {PoEPowerMode::PSE, "PSE"},
    {PoEPowerMode::PD, "PD"},
    {PoEPowerMode::None, "None"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(PoEStandard, {
    {PoEStandard::Invalid, "Invalid"},
    {PoEStandard::IEEE8023af, "IEEE8023af"},
    {PoEStandard::IEEE8023at, "IEEE8023at"},
    {PoEStandard::IEEE8023bt, "IEEE8023bt"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(PoEDetectionStatus, {
    {PoEDetectionStatus::Invalid, "Invalid"},
    {PoEDetectionStatus::Disabled, "Disabled"},
    {PoEDetectionStatus::Searching, "Searching"},
    {PoEDetectionStatus::DeliveringPower, "DeliveringPower"},
    {PoEDetectionStatus::Fault, "Fault"},
    {PoEDetectionStatus::Test, "Test"},
    {PoEDetectionStatus::OtherFault, "OtherFault"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(UALink128GExtendedSpeedMode, {
    {UALink128GExtendedSpeedMode::Invalid, "Invalid"},
    {UALink128GExtendedSpeedMode::NoESM, "NoESM"},
    {UALink128GExtendedSpeedMode::104G, "104G"},
    {UALink128GExtendedSpeedMode::112G, "112G"},
    {UALink128GExtendedSpeedMode::120G, "120G"},
    {UALink128GExtendedSpeedMode::128G, "128G"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(UALink200GSerialRate, {
    {UALink200GSerialRate::Invalid, "Invalid"},
    {UALink200GSerialRate::100G, "100G"},
    {UALink200GSerialRate::200G, "200G"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(UALink200GCodewordInterleave, {
    {UALink200GCodewordInterleave::Invalid, "Invalid"},
    {UALink200GCodewordInterleave::OneWay, "OneWay"},
    {UALink200GCodewordInterleave::TwoWay, "TwoWay"},
    {UALink200GCodewordInterleave::FourWay, "FourWay"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(UALinkPLState, {
    {UALinkPLState::Invalid, "Invalid"},
    {UALinkPLState::Idle, "Idle"},
    {UALinkPLState::TrainingInProgress, "TrainingInProgress"},
    {UALinkPLState::TrainingFailed, "TrainingFailed"},
    {UALinkPLState::TrainingTimeout, "TrainingTimeout"},
    {UALinkPLState::Up, "Up"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(UALinkDLState, {
    {UALinkDLState::Invalid, "Invalid"},
    {UALinkDLState::Idle, "Idle"},
    {UALinkDLState::NOP, "NOP"},
    {UALinkDLState::Fault, "Fault"},
    {UALinkDLState::PowerDown, "PowerDown"},
    {UALinkDLState::Up, "Up"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(UALinkTLState, {
    {UALinkTLState::Invalid, "Invalid"},
    {UALinkTLState::DropMode, "DropMode"},
    {UALinkTLState::Enabled, "Enabled"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(UALinkUPLIOriginatorState, {
    {UALinkUPLIOriginatorState::Invalid, "Invalid"},
    {UALinkUPLIOriginatorState::DropMode, "DropMode"},
    {UALinkUPLIOriginatorState::IsolationMode, "IsolationMode"},
    {UALinkUPLIOriginatorState::Enabled, "Enabled"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(UALinkUPLICompleterState, {
    {UALinkUPLICompleterState::Invalid, "Invalid"},
    {UALinkUPLICompleterState::DropMode, "DropMode"},
    {UALinkUPLICompleterState::Enabled, "Enabled"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(PartnerDeviceType, {
    {PartnerDeviceType::Invalid, "Invalid"},
    {PartnerDeviceType::Switch, "Switch"},
    {PartnerDeviceType::Endpoint, "Endpoint"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(PartnerValidationState, {
    {PartnerValidationState::Disabled, "Disabled"},
    {PartnerValidationState::LinkDown, "LinkDown"},
    {PartnerValidationState::Discovering, "Discovering"},
    {PartnerValidationState::Invalid, "Invalid"},
    {PartnerValidationState::Validated, "Validated"},
});

// clang-format on
} // namespace port
