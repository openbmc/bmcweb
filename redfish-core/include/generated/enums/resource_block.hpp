#pragma once
#include <boost/describe/enum.hpp>
#include <nlohmann/json.hpp>

namespace resource_block
{
// clang-format off

enum class ResourceBlockType{
    Invalid,
    Compute,
    Processor,
    Memory,
    Network,
    Storage,
    ComputerSystem,
    Expansion,
    IndependentResource,
};

enum class CompositionState{
    Invalid,
    Composing,
    ComposedAndAvailable,
    Composed,
    Unused,
    Failed,
    Unavailable,
};

enum class PoolType{
    Invalid,
    Free,
    Active,
    Unassigned,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ResourceBlockType, {
    {ResourceBlockType::Invalid, "Invalid"},
    {ResourceBlockType::Compute, "Compute"},
    {ResourceBlockType::Processor, "Processor"},
    {ResourceBlockType::Memory, "Memory"},
    {ResourceBlockType::Network, "Network"},
    {ResourceBlockType::Storage, "Storage"},
    {ResourceBlockType::ComputerSystem, "ComputerSystem"},
    {ResourceBlockType::Expansion, "Expansion"},
    {ResourceBlockType::IndependentResource, "IndependentResource"},
});

BOOST_DESCRIBE_ENUM(ResourceBlockType,

    Invalid,
    Compute,
    Processor,
    Memory,
    Network,
    Storage,
    ComputerSystem,
    Expansion,
    IndependentResource,
);

NLOHMANN_JSON_SERIALIZE_ENUM(CompositionState, {
    {CompositionState::Invalid, "Invalid"},
    {CompositionState::Composing, "Composing"},
    {CompositionState::ComposedAndAvailable, "ComposedAndAvailable"},
    {CompositionState::Composed, "Composed"},
    {CompositionState::Unused, "Unused"},
    {CompositionState::Failed, "Failed"},
    {CompositionState::Unavailable, "Unavailable"},
});

BOOST_DESCRIBE_ENUM(CompositionState,

    Invalid,
    Composing,
    ComposedAndAvailable,
    Composed,
    Unused,
    Failed,
    Unavailable,
);

NLOHMANN_JSON_SERIALIZE_ENUM(PoolType, {
    {PoolType::Invalid, "Invalid"},
    {PoolType::Free, "Free"},
    {PoolType::Active, "Active"},
    {PoolType::Unassigned, "Unassigned"},
});

BOOST_DESCRIBE_ENUM(PoolType,

    Invalid,
    Free,
    Active,
    Unassigned,
);

}
// clang-format on
