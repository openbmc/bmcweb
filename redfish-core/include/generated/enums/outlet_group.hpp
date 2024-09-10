SPDX-License-Identifier: Apache-2.0
SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <nlohmann/json.hpp>

namespace outlet_group
{
// clang-format off

enum class PowerState{
    Invalid,
    On,
    Off,
    PowerCycle,
};

enum class OutletGroupType{
    Invalid,
    HardwareDefined,
    UserDefined,
};

NLOHMANN_JSON_SERIALIZE_ENUM(PowerState, {
    {PowerState::Invalid, "Invalid"},
    {PowerState::On, "On"},
    {PowerState::Off, "Off"},
    {PowerState::PowerCycle, "PowerCycle"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(OutletGroupType, {
    {OutletGroupType::Invalid, "Invalid"},
    {OutletGroupType::HardwareDefined, "HardwareDefined"},
    {OutletGroupType::UserDefined, "UserDefined"},
});

}
// clang-format on
