#pragma once
#include <nlohmann/json.hpp>

namespace attribute_registry
{
// clang-format off

enum class AttributeType{
    Invalid,
    Enumeration,
    String,
    Integer,
    Boolean,
    Password,
};

enum class DependencyType{
    Invalid,
    Map,
};

enum class MapFromCondition{
    Invalid,
    EQU,
    NEQ,
    GTR,
    GEQ,
    LSS,
    LEQ,
};

enum class MapFromProperty{
    Invalid,
    CurrentValue,
    DefaultValue,
    ReadOnly,
    WriteOnly,
    GrayOut,
    Hidden,
    LowerBound,
    UpperBound,
    MinLength,
    MaxLength,
    ScalarIncrement,
};

enum class MapTerms{
    Invalid,
    AND,
    OR,
};

enum class MapToProperty{
    Invalid,
    CurrentValue,
    DefaultValue,
    ReadOnly,
    WriteOnly,
    GrayOut,
    Hidden,
    Immutable,
    HelpText,
    WarningText,
    DisplayName,
    DisplayOrder,
    LowerBound,
    UpperBound,
    MinLength,
    MaxLength,
    ScalarIncrement,
    ValueExpression,
};

NLOHMANN_JSON_SERIALIZE_ENUM(AttributeType, { //NOLINT
    {AttributeType::Invalid, "Invalid"},
    {AttributeType::Enumeration, "Enumeration"},
    {AttributeType::String, "String"},
    {AttributeType::Integer, "Integer"},
    {AttributeType::Boolean, "Boolean"},
    {AttributeType::Password, "Password"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(DependencyType, { //NOLINT
    {DependencyType::Invalid, "Invalid"},
    {DependencyType::Map, "Map"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(MapFromCondition, { //NOLINT
    {MapFromCondition::Invalid, "Invalid"},
    {MapFromCondition::EQU, "EQU"},
    {MapFromCondition::NEQ, "NEQ"},
    {MapFromCondition::GTR, "GTR"},
    {MapFromCondition::GEQ, "GEQ"},
    {MapFromCondition::LSS, "LSS"},
    {MapFromCondition::LEQ, "LEQ"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(MapFromProperty, { //NOLINT
    {MapFromProperty::Invalid, "Invalid"},
    {MapFromProperty::CurrentValue, "CurrentValue"},
    {MapFromProperty::DefaultValue, "DefaultValue"},
    {MapFromProperty::ReadOnly, "ReadOnly"},
    {MapFromProperty::WriteOnly, "WriteOnly"},
    {MapFromProperty::GrayOut, "GrayOut"},
    {MapFromProperty::Hidden, "Hidden"},
    {MapFromProperty::LowerBound, "LowerBound"},
    {MapFromProperty::UpperBound, "UpperBound"},
    {MapFromProperty::MinLength, "MinLength"},
    {MapFromProperty::MaxLength, "MaxLength"},
    {MapFromProperty::ScalarIncrement, "ScalarIncrement"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(MapTerms, { //NOLINT
    {MapTerms::Invalid, "Invalid"},
    {MapTerms::AND, "AND"},
    {MapTerms::OR, "OR"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(MapToProperty, { //NOLINT
    {MapToProperty::Invalid, "Invalid"},
    {MapToProperty::CurrentValue, "CurrentValue"},
    {MapToProperty::DefaultValue, "DefaultValue"},
    {MapToProperty::ReadOnly, "ReadOnly"},
    {MapToProperty::WriteOnly, "WriteOnly"},
    {MapToProperty::GrayOut, "GrayOut"},
    {MapToProperty::Hidden, "Hidden"},
    {MapToProperty::Immutable, "Immutable"},
    {MapToProperty::HelpText, "HelpText"},
    {MapToProperty::WarningText, "WarningText"},
    {MapToProperty::DisplayName, "DisplayName"},
    {MapToProperty::DisplayOrder, "DisplayOrder"},
    {MapToProperty::LowerBound, "LowerBound"},
    {MapToProperty::UpperBound, "UpperBound"},
    {MapToProperty::MinLength, "MinLength"},
    {MapToProperty::MaxLength, "MaxLength"},
    {MapToProperty::ScalarIncrement, "ScalarIncrement"},
    {MapToProperty::ValueExpression, "ValueExpression"},
});

}
// clang-format on
