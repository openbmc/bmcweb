#pragma once
#include <boost/describe/enum.hpp>
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

BOOST_DESCRIBE_ENUM(ChargeState,

    Invalid,
    Idle,
    Charging,
    Discharging,
);

}
// clang-format on
