#pragma once
#include <boost/describe/enum.hpp>
#include <nlohmann/json.hpp>

namespace endpoint_group
{
// clang-format off

enum class AccessState{
    Invalid,
    Optimized,
    NonOptimized,
    Standby,
    Unavailable,
    Transitioning,
};

enum class GroupType{
    Invalid,
    Client,
    Server,
    Initiator,
    Target,
};

NLOHMANN_JSON_SERIALIZE_ENUM(AccessState, {
    {AccessState::Invalid, "Invalid"},
    {AccessState::Optimized, "Optimized"},
    {AccessState::NonOptimized, "NonOptimized"},
    {AccessState::Standby, "Standby"},
    {AccessState::Unavailable, "Unavailable"},
    {AccessState::Transitioning, "Transitioning"},
});

BOOST_DESCRIBE_ENUM(AccessState,

    Invalid,
    Optimized,
    NonOptimized,
    Standby,
    Unavailable,
    Transitioning,
);

NLOHMANN_JSON_SERIALIZE_ENUM(GroupType, {
    {GroupType::Invalid, "Invalid"},
    {GroupType::Client, "Client"},
    {GroupType::Server, "Server"},
    {GroupType::Initiator, "Initiator"},
    {GroupType::Target, "Target"},
});

BOOST_DESCRIBE_ENUM(GroupType,

    Invalid,
    Client,
    Server,
    Initiator,
    Target,
);

}
// clang-format on
