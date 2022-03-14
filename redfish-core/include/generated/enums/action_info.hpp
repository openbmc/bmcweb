#pragma once
#include <nlohmann/json.hpp>

namespace action_info
{
// clang-format off

enum class ParameterTypes{
    Invalid,
    Boolean,
    Number,
    NumberArray,
    String,
    StringArray,
    Object,
    ObjectArray,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ParameterTypes, {
    {ParameterTypes::Invalid, "Invalid"},
    {ParameterTypes::Boolean, "Boolean"},
    {ParameterTypes::Number, "Number"},
    {ParameterTypes::NumberArray, "NumberArray"},
    {ParameterTypes::String, "String"},
    {ParameterTypes::StringArray, "StringArray"},
    {ParameterTypes::Object, "Object"},
    {ParameterTypes::ObjectArray, "ObjectArray"},
});

}
// clang-format on
