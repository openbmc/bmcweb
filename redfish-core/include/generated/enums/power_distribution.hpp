// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <nlohmann/json.hpp>

namespace power_distribution
{
    // clang-format off

enum class PowerEquipmentType{
    Invalid,
    RackPDU,
    FloorPDU,
    ManualTransferSwitch,
    AutomaticTransferSwitch,
    Switchgear,
    PowerShelf,
    Bus,
    BatteryShelf,
};

enum class TransferSensitivityType{
    Invalid,
    High,
    Medium,
    Low,
};

enum class ExportType{
    Invalid,
    Clone,
    Replacement,
};

enum class Component{
    Invalid,
    All,
    Manager,
    ManagerAccounts,
    PowerDistribution,
};

enum class ExportSecurity{
    Invalid,
    IncludeSensitiveData,
    HashedDataOnly,
    ExcludeSensitiveData,
};

NLOHMANN_JSON_SERIALIZE_ENUM(PowerEquipmentType, {
    {PowerEquipmentType::Invalid, "Invalid"},
    {PowerEquipmentType::RackPDU, "RackPDU"},
    {PowerEquipmentType::FloorPDU, "FloorPDU"},
    {PowerEquipmentType::ManualTransferSwitch, "ManualTransferSwitch"},
    {PowerEquipmentType::AutomaticTransferSwitch, "AutomaticTransferSwitch"},
    {PowerEquipmentType::Switchgear, "Switchgear"},
    {PowerEquipmentType::PowerShelf, "PowerShelf"},
    {PowerEquipmentType::Bus, "Bus"},
    {PowerEquipmentType::BatteryShelf, "BatteryShelf"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(TransferSensitivityType, {
    {TransferSensitivityType::Invalid, "Invalid"},
    {TransferSensitivityType::High, "High"},
    {TransferSensitivityType::Medium, "Medium"},
    {TransferSensitivityType::Low, "Low"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(ExportType, {
    {ExportType::Invalid, "Invalid"},
    {ExportType::Clone, "Clone"},
    {ExportType::Replacement, "Replacement"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(Component, {
    {Component::Invalid, "Invalid"},
    {Component::All, "All"},
    {Component::Manager, "Manager"},
    {Component::ManagerAccounts, "ManagerAccounts"},
    {Component::PowerDistribution, "PowerDistribution"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(ExportSecurity, {
    {ExportSecurity::Invalid, "Invalid"},
    {ExportSecurity::IncludeSensitiveData, "IncludeSensitiveData"},
    {ExportSecurity::HashedDataOnly, "HashedDataOnly"},
    {ExportSecurity::ExcludeSensitiveData, "ExcludeSensitiveData"},
});

}
// clang-format on
