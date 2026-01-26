// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <nlohmann/json.hpp>

namespace cooling_unit
{
// clang-format off

enum class CoolingEquipmentType{
    Invalid,
    CDU,
    HeatExchanger,
    ImmersionUnit,
    RPU,
};

enum class CoolingUnitMode{
    Invalid,
    Enabled,
    Disabled,
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
    CoolingUnit,
};

enum class ExportSecurity{
    Invalid,
    IncludeSensitiveData,
    HashedDataOnly,
    ExcludeSensitiveData,
};

NLOHMANN_JSON_SERIALIZE_ENUM(CoolingEquipmentType, {
    {CoolingEquipmentType::Invalid, "Invalid"},
    {CoolingEquipmentType::CDU, "CDU"},
    {CoolingEquipmentType::HeatExchanger, "HeatExchanger"},
    {CoolingEquipmentType::ImmersionUnit, "ImmersionUnit"},
    {CoolingEquipmentType::RPU, "RPU"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(CoolingUnitMode, {
    {CoolingUnitMode::Invalid, "Invalid"},
    {CoolingUnitMode::Enabled, "Enabled"},
    {CoolingUnitMode::Disabled, "Disabled"},
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
    {Component::CoolingUnit, "CoolingUnit"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(ExportSecurity, {
    {ExportSecurity::Invalid, "Invalid"},
    {ExportSecurity::IncludeSensitiveData, "IncludeSensitiveData"},
    {ExportSecurity::HashedDataOnly, "HashedDataOnly"},
    {ExportSecurity::ExcludeSensitiveData, "ExcludeSensitiveData"},
});

// clang-format on
} // namespace cooling_unit
