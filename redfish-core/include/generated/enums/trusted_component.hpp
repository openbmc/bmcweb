#pragma once
#include <boost/describe/enum.hpp>
#include <nlohmann/json.hpp>

namespace trusted_component
{
// clang-format off

enum class TrustedComponentType{
    Invalid,
    Discrete,
    Integrated,
};

NLOHMANN_JSON_SERIALIZE_ENUM(TrustedComponentType, {
    {TrustedComponentType::Invalid, "Invalid"},
    {TrustedComponentType::Discrete, "Discrete"},
    {TrustedComponentType::Integrated, "Integrated"},
});

BOOST_DESCRIBE_ENUM(TrustedComponentType,

    Invalid,
    Discrete,
    Integrated,
);

}
// clang-format on
