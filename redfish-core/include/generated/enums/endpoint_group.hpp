#pragma once
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

NLOHMANN_JSON_SERIALIZE_ENUM(GroupType, {
    {GroupType::Invalid, "Invalid"},
    {GroupType::Client, "Client"},
    {GroupType::Server, "Server"},
    {GroupType::Initiator, "Initiator"},
    {GroupType::Target, "Target"},
});

}
// clang-format on
