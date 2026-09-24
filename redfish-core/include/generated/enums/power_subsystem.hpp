// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <nlohmann/json.hpp>

namespace power_subsystem
{
// clang-format off

enum class ThresholdLevel{
    Invalid,
    Disabled,
    UpperCaution,
    UpperCritical,
};

enum class PowerBrakeReason{
    Invalid,
    LoadPercent,
    Temperature,
    LossOfComms,
    ExternalSignal,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ThresholdLevel, {
    {ThresholdLevel::Invalid, "Invalid"},
    {ThresholdLevel::Disabled, "Disabled"},
    {ThresholdLevel::UpperCaution, "UpperCaution"},
    {ThresholdLevel::UpperCritical, "UpperCritical"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(PowerBrakeReason, {
    {PowerBrakeReason::Invalid, "Invalid"},
    {PowerBrakeReason::LoadPercent, "LoadPercent"},
    {PowerBrakeReason::Temperature, "Temperature"},
    {PowerBrakeReason::LossOfComms, "LossOfComms"},
    {PowerBrakeReason::ExternalSignal, "ExternalSignal"},
});

// clang-format on
} // namespace power_subsystem
