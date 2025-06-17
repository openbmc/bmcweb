// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <nlohmann/json.hpp>

namespace control
{
// clang-format off

enum class ControlType{
    Invalid,
    Temperature,
    Power,
    Frequency,
    FrequencyMHz,
    Pressure,
    PressurekPa,
    Valve,
    Percent,
    DutyCycle,
    LinearPosition,
    LinearVelocity,
    LinearAcceleration,
    RotationalPosition,
    RotationalVelocity,
    RotationalAcceleration,
    LiquidFlowLPM,
};

enum class SetPointType{
    Invalid,
    Single,
    Range,
    Monitor,
};

enum class ControlMode{
    Invalid,
    Automatic,
    Override,
    Manual,
    Disabled,
};

enum class ImplementationType{
    Invalid,
    Programmable,
    Direct,
    Monitored,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ControlType, {
    {ControlType::Invalid, "Invalid"},
    {ControlType::Temperature, "Temperature"},
    {ControlType::Power, "Power"},
    {ControlType::Frequency, "Frequency"},
    {ControlType::FrequencyMHz, "FrequencyMHz"},
    {ControlType::Pressure, "Pressure"},
    {ControlType::PressurekPa, "PressurekPa"},
    {ControlType::Valve, "Valve"},
    {ControlType::Percent, "Percent"},
    {ControlType::DutyCycle, "DutyCycle"},
    {ControlType::LinearPosition, "LinearPosition"},
    {ControlType::LinearVelocity, "LinearVelocity"},
    {ControlType::LinearAcceleration, "LinearAcceleration"},
    {ControlType::RotationalPosition, "RotationalPosition"},
    {ControlType::RotationalVelocity, "RotationalVelocity"},
    {ControlType::RotationalAcceleration, "RotationalAcceleration"},
    {ControlType::LiquidFlowLPM, "LiquidFlowLPM"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(SetPointType, {
    {SetPointType::Invalid, "Invalid"},
    {SetPointType::Single, "Single"},
    {SetPointType::Range, "Range"},
    {SetPointType::Monitor, "Monitor"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(ControlMode, {
    {ControlMode::Invalid, "Invalid"},
    {ControlMode::Automatic, "Automatic"},
    {ControlMode::Override, "Override"},
    {ControlMode::Manual, "Manual"},
    {ControlMode::Disabled, "Disabled"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(ImplementationType, {
    {ImplementationType::Invalid, "Invalid"},
    {ImplementationType::Programmable, "Programmable"},
    {ImplementationType::Direct, "Direct"},
    {ImplementationType::Monitored, "Monitored"},
});

}
// clang-format on
