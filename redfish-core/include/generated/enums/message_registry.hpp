#pragma once
#include <nlohmann/json.hpp>

namespace message_registry
{
// clang-format off

enum class ParamType{
    Invalid,
    string,
    number,
};

enum class ClearingType{
    Invalid,
    SameOriginOfCondition,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ParamType, {
    {ParamType::Invalid, "Invalid"},
    {ParamType::string, "string"},
    {ParamType::number, "number"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(ClearingType, {
    {ClearingType::Invalid, "Invalid"},
    {ClearingType::SameOriginOfCondition, "SameOriginOfCondition"},
});

}
// clang-format on
