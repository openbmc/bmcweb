#pragma once
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

}
// clang-format on
