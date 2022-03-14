#pragma once
#include <nlohmann/json.hpp>

namespace telemetry_service
{
// clang-format off

enum class CollectionFunction{
    Invalid,
    Average,
    Maximum,
    Minimum,
    Summation,
};

NLOHMANN_JSON_SERIALIZE_ENUM(CollectionFunction, {
    {CollectionFunction::Invalid, "Invalid"},
    {CollectionFunction::Average, "Average"},
    {CollectionFunction::Maximum, "Maximum"},
    {CollectionFunction::Minimum, "Minimum"},
    {CollectionFunction::Summation, "Summation"},
});

}
// clang-format on
