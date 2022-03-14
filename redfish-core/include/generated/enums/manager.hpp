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

NLOHMANN_JSON_SERIALIZE_ENUM(ManagerType, {
    {ManagerType::Invalid, "Invalid"},
    {ManagerType::ManagementController, "ManagementController"},
    {ManagerType::EnclosureManager, "EnclosureManager"},
    {ManagerType::BMC, "BMC"},
    {ManagerType::RackManager, "RackManager"},
    {ManagerType::AuxiliaryController, "AuxiliaryController"},
    {ManagerType::Service, "Service"},
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

}
// clang-format on
