#pragma once
#include <boost/describe/enum.hpp>
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

BOOST_DESCRIBE_ENUM(ReservoirType,

    Invalid,
    Reserve,
    Overflow,
    Inline,
    Immersion,
);

}
// clang-format on
