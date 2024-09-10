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

NLOHMANN_JSON_SERIALIZE_ENUM(ChargeState, {
    {ChargeState::Invalid, "Invalid"},
    {ChargeState::Idle, "Idle"},
    {ChargeState::Charging, "Charging"},
    {ChargeState::Discharging, "Discharging"},
});

}
// clang-format on
