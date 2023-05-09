#pragma once
#include <boost/describe/enum.hpp>
#include <nlohmann/json.hpp>

namespace cooling_loop
{
// clang-format off

enum class CoolantType{
    Invalid,
    Water,
    Hydrocarbon,
    Fluorocarbon,
    Dielectric,
};

NLOHMANN_JSON_SERIALIZE_ENUM(CoolantType, {
    {CoolantType::Invalid, "Invalid"},
    {CoolantType::Water, "Water"},
    {CoolantType::Hydrocarbon, "Hydrocarbon"},
    {CoolantType::Fluorocarbon, "Fluorocarbon"},
    {CoolantType::Dielectric, "Dielectric"},
});

BOOST_DESCRIBE_ENUM(CoolantType,

    Invalid,
    Water,
    Hydrocarbon,
    Fluorocarbon,
    Dielectric,
);

}
// clang-format on
