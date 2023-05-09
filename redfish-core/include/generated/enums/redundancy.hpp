#pragma once
#include <boost/describe/enum.hpp>
#include <nlohmann/json.hpp>

namespace redundancy
{
// clang-format off

enum class RedundancyType{
    Invalid,
    Failover,
    NPlusM,
    Sharing,
    Sparing,
    NotRedundant,
};

NLOHMANN_JSON_SERIALIZE_ENUM(RedundancyType, {
    {RedundancyType::Invalid, "Invalid"},
    {RedundancyType::Failover, "Failover"},
    {RedundancyType::NPlusM, "NPlusM"},
    {RedundancyType::Sharing, "Sharing"},
    {RedundancyType::Sparing, "Sparing"},
    {RedundancyType::NotRedundant, "NotRedundant"},
});

BOOST_DESCRIBE_ENUM(RedundancyType,

    Invalid,
    Failover,
    NPlusM,
    Sharing,
    Sparing,
    NotRedundant,
);

}
// clang-format on
