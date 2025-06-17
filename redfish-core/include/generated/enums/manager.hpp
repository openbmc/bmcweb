// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <nlohmann/json.hpp>

namespace manager
{
// clang-format off

enum class ManagerType{
    Invalid,
    ManagementController,
    EnclosureManager,
    BMC,
    RackManager,
    AuxiliaryController,
    Service,
    FabricManager,
};

enum class SerialConnectTypesSupported{
    Invalid,
    SSH,
    Telnet,
    IPMI,
    Oem,
};

enum class CommandConnectTypesSupported{
    Invalid,
    SSH,
    Telnet,
    IPMI,
    Oem,
};

enum class GraphicalConnectTypesSupported{
    Invalid,
    KVMIP,
    Oem,
};

enum class ResetToDefaultsType{
    Invalid,
    ResetAll,
    PreserveNetworkAndUsers,
    PreserveNetwork,
};

enum class DateTimeSource{
    Invalid,
    RTC,
    Firmware,
    Host,
    NTP,
    PTP,
};

enum class SecurityModeTypes{
    Invalid,
    FIPS_140_2,
    FIPS_140_3,
    CNSA_1_0,
    CNSA_2_0,
    SuiteB,
    OEM,
    Default,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ManagerType, {
    {ManagerType::Invalid, "Invalid"},
    {ManagerType::ManagementController, "ManagementController"},
    {ManagerType::EnclosureManager, "EnclosureManager"},
    {ManagerType::BMC, "BMC"},
    {ManagerType::RackManager, "RackManager"},
    {ManagerType::AuxiliaryController, "AuxiliaryController"},
    {ManagerType::Service, "Service"},
    {ManagerType::FabricManager, "FabricManager"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(SerialConnectTypesSupported, {
    {SerialConnectTypesSupported::Invalid, "Invalid"},
    {SerialConnectTypesSupported::SSH, "SSH"},
    {SerialConnectTypesSupported::Telnet, "Telnet"},
    {SerialConnectTypesSupported::IPMI, "IPMI"},
    {SerialConnectTypesSupported::Oem, "Oem"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(CommandConnectTypesSupported, {
    {CommandConnectTypesSupported::Invalid, "Invalid"},
    {CommandConnectTypesSupported::SSH, "SSH"},
    {CommandConnectTypesSupported::Telnet, "Telnet"},
    {CommandConnectTypesSupported::IPMI, "IPMI"},
    {CommandConnectTypesSupported::Oem, "Oem"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(GraphicalConnectTypesSupported, {
    {GraphicalConnectTypesSupported::Invalid, "Invalid"},
    {GraphicalConnectTypesSupported::KVMIP, "KVMIP"},
    {GraphicalConnectTypesSupported::Oem, "Oem"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(ResetToDefaultsType, {
    {ResetToDefaultsType::Invalid, "Invalid"},
    {ResetToDefaultsType::ResetAll, "ResetAll"},
    {ResetToDefaultsType::PreserveNetworkAndUsers, "PreserveNetworkAndUsers"},
    {ResetToDefaultsType::PreserveNetwork, "PreserveNetwork"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(DateTimeSource, {
    {DateTimeSource::Invalid, "Invalid"},
    {DateTimeSource::RTC, "RTC"},
    {DateTimeSource::Firmware, "Firmware"},
    {DateTimeSource::Host, "Host"},
    {DateTimeSource::NTP, "NTP"},
    {DateTimeSource::PTP, "PTP"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(SecurityModeTypes, {
    {SecurityModeTypes::Invalid, "Invalid"},
    {SecurityModeTypes::FIPS_140_2, "FIPS_140_2"},
    {SecurityModeTypes::FIPS_140_3, "FIPS_140_3"},
    {SecurityModeTypes::CNSA_1_0, "CNSA_1_0"},
    {SecurityModeTypes::CNSA_2_0, "CNSA_2_0"},
    {SecurityModeTypes::SuiteB, "SuiteB"},
    {SecurityModeTypes::OEM, "OEM"},
    {SecurityModeTypes::Default, "Default"},
});

}
// clang-format on
