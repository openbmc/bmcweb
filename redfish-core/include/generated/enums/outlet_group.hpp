#pragma once
#include <boost/describe/enum.hpp>
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

BOOST_DESCRIBE_ENUM(PowerState,

    Invalid,
    On,
    Off,
    PowerCycle,
);

NLOHMANN_JSON_SERIALIZE_ENUM(OutletGroupType, {
    {OutletGroupType::Invalid, "Invalid"},
    {OutletGroupType::HardwareDefined, "HardwareDefined"},
    {OutletGroupType::UserDefined, "UserDefined"},
});

BOOST_DESCRIBE_ENUM(OutletGroupType,

    Invalid,
    HardwareDefined,
    UserDefined,
);

}
// clang-format on
