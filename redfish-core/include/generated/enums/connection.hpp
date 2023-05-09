#pragma once
#include <boost/describe/enum.hpp>
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

BOOST_DESCRIBE_ENUM(ConnectionType,

    Invalid,
    Storage,
    Memory,
);

NLOHMANN_JSON_SERIALIZE_ENUM(AccessCapability, {
    {AccessCapability::Invalid, "Invalid"},
    {AccessCapability::Read, "Read"},
    {AccessCapability::Write, "Write"},
});

BOOST_DESCRIBE_ENUM(AccessCapability,

    Invalid,
    Read,
    Write,
);

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

}
// clang-format on
