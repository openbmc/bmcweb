#pragma once
#include <boost/describe/enum.hpp>
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

BOOST_DESCRIBE_ENUM(CollectionFunction,

    Invalid,
    Average,
    Maximum,
    Minimum,
    Summation,
);

}
// clang-format on
