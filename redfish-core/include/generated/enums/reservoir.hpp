#pragma once
#include <nlohmann/json.hpp>

namespace reservoir
{
// clang-format off

enum class ReservoirType{
    Invalid,
    Reserve,
    Overflow,
    Inline,
    Immersion,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ReservoirType, {
    {ReservoirType::Invalid, "Invalid"},
    {ReservoirType::Reserve, "Reserve"},
    {ReservoirType::Overflow, "Overflow"},
    {ReservoirType::Inline, "Inline"},
    {ReservoirType::Immersion, "Immersion"},
});

}
// clang-format on
