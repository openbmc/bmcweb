// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <nlohmann/json.hpp>

namespace battery
{
// clang-format off

enum class ChargeState{
    Invalid,
    Idle,
    Charging,
    Discharging,
};

enum class BatteryChemistryType{
    Invalid,
    LeadAcid,
    LithiumIon,
    NickelCadmium,
};

enum class EnergyStorageType{
    Invalid,
    Battery,
    Supercapacitor,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ChargeState, {
    {ChargeState::Invalid, "Invalid"},
    {ChargeState::Idle, "Idle"},
    {ChargeState::Charging, "Charging"},
    {ChargeState::Discharging, "Discharging"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(BatteryChemistryType, {
    {BatteryChemistryType::Invalid, "Invalid"},
    {BatteryChemistryType::LeadAcid, "LeadAcid"},
    {BatteryChemistryType::LithiumIon, "LithiumIon"},
    {BatteryChemistryType::NickelCadmium, "NickelCadmium"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(EnergyStorageType, {
    {EnergyStorageType::Invalid, "Invalid"},
    {EnergyStorageType::Battery, "Battery"},
    {EnergyStorageType::Supercapacitor, "Supercapacitor"},
});

}
// clang-format on
