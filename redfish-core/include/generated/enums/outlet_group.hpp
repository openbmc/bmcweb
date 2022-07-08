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

NLOHMANN_JSON_SERIALIZE_ENUM(PowerState, {
    {PowerState::Invalid, "Invalid"},
    {PowerState::On, "On"},
    {PowerState::Off, "Off"},
    {PowerState::PowerCycle, "PowerCycle"},
});

}
// clang-format on
