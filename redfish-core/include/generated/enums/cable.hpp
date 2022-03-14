#pragma once
#include <nlohmann/json.hpp>

namespace cable
{
// clang-format off

enum class CableClass{
    Invalid,
    Power,
    Network,
    Storage,
    Fan,
    PCIe,
    USB,
    Video,
    Fabric,
    Serial,
    General,
};

enum class ConnectorType{
    Invalid,
    ACPower,
    DB9,
    DCPower,
    DisplayPort,
    HDMI,
    ICI,
    IPASS,
    PCIe,
    Proprietary,
    RJ45,
    SATA,
    SCSI,
    SlimSAS,
    SFP,
    SFPPlus,
    USBA,
    USBC,
    QSFP,
    CDFP,
    OSFP,
};

enum class CableStatus{
    Invalid,
    Normal,
    Degraded,
    Failed,
    Testing,
    Disabled,
    SetByService,
};

NLOHMANN_JSON_SERIALIZE_ENUM(CableClass, {
    {CableClass::Invalid, "Invalid"},
    {CableClass::Power, "Power"},
    {CableClass::Network, "Network"},
    {CableClass::Storage, "Storage"},
    {CableClass::Fan, "Fan"},
    {CableClass::PCIe, "PCIe"},
    {CableClass::USB, "USB"},
    {CableClass::Video, "Video"},
    {CableClass::Fabric, "Fabric"},
    {CableClass::Serial, "Serial"},
    {CableClass::General, "General"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(ConnectorType, {
    {ConnectorType::Invalid, "Invalid"},
    {ConnectorType::ACPower, "ACPower"},
    {ConnectorType::DB9, "DB9"},
    {ConnectorType::DCPower, "DCPower"},
    {ConnectorType::DisplayPort, "DisplayPort"},
    {ConnectorType::HDMI, "HDMI"},
    {ConnectorType::ICI, "ICI"},
    {ConnectorType::IPASS, "IPASS"},
    {ConnectorType::PCIe, "PCIe"},
    {ConnectorType::Proprietary, "Proprietary"},
    {ConnectorType::RJ45, "RJ45"},
    {ConnectorType::SATA, "SATA"},
    {ConnectorType::SCSI, "SCSI"},
    {ConnectorType::SlimSAS, "SlimSAS"},
    {ConnectorType::SFP, "SFP"},
    {ConnectorType::SFPPlus, "SFPPlus"},
    {ConnectorType::USBA, "USBA"},
    {ConnectorType::USBC, "USBC"},
    {ConnectorType::QSFP, "QSFP"},
    {ConnectorType::CDFP, "CDFP"},
    {ConnectorType::OSFP, "OSFP"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(CableStatus, {
    {CableStatus::Invalid, "Invalid"},
    {CableStatus::Normal, "Normal"},
    {CableStatus::Degraded, "Degraded"},
    {CableStatus::Failed, "Failed"},
    {CableStatus::Testing, "Testing"},
    {CableStatus::Disabled, "Disabled"},
    {CableStatus::SetByService, "SetByService"},
});

}
// clang-format on
