// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <nlohmann/json.hpp>

// clang-format off

namespace coolant_connector
{
enum class CoolantConnectorType{
    Invalid,
    Pair,
    Supply,
    Return,
    Inline,
    Closed,
};

enum class ValveState{
    Invalid,
    Open,
    Closed,
};

enum class ValveStateReason{
    Invalid,
    Normal,
    NotInUse,
    LeakDetected,
};

NLOHMANN_JSON_SERIALIZE_ENUM(CoolantConnectorType, {
    {CoolantConnectorType::Invalid, "Invalid"},
    {CoolantConnectorType::Pair, "Pair"},
    {CoolantConnectorType::Supply, "Supply"},
    {CoolantConnectorType::Return, "Return"},
    {CoolantConnectorType::Inline, "Inline"},
    {CoolantConnectorType::Closed, "Closed"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(ValveState, {
    {ValveState::Invalid, "Invalid"},
    {ValveState::Open, "Open"},
    {ValveState::Closed, "Closed"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(ValveStateReason, {
    {ValveStateReason::Invalid, "Invalid"},
    {ValveStateReason::Normal, "Normal"},
    {ValveStateReason::NotInUse, "NotInUse"},
    {ValveStateReason::LeakDetected, "LeakDetected"},
});

// clang-format on
}
