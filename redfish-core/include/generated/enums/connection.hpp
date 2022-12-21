#pragma once
#include <nlohmann/json.hpp>

namespace connection
{
// clang-format off

enum class ConnectionType{
    Invalid,
    Storage,
    Memory,
};

enum class AccessCapability{
    Invalid,
    Read,
    Write,
};

enum class AccessState{
    Invalid,
    Optimized,
    NonOptimized,
    Standby,
    Unavailable,
    Transitioning,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ConnectionType, {
    {ConnectionType::Invalid, "Invalid"},
    {ConnectionType::Storage, "Storage"},
    {ConnectionType::Memory, "Memory"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(AccessCapability, {
    {AccessCapability::Invalid, "Invalid"},
    {AccessCapability::Read, "Read"},
    {AccessCapability::Write, "Write"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(AccessState, {
    {AccessState::Invalid, "Invalid"},
    {AccessState::Optimized, "Optimized"},
    {AccessState::NonOptimized, "NonOptimized"},
    {AccessState::Standby, "Standby"},
    {AccessState::Unavailable, "Unavailable"},
    {AccessState::Transitioning, "Transitioning"},
});

}
// clang-format on
