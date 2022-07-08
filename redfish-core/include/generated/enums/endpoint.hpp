#pragma once
#include <nlohmann/json.hpp>

namespace endpoint
{
// clang-format off

enum class EntityType{
    Invalid,
    StorageInitiator,
    RootComplex,
    NetworkController,
    Drive,
    StorageExpander,
    DisplayController,
    Bridge,
    Processor,
    Volume,
    AccelerationFunction,
    MediaController,
    MemoryChunk,
    Switch,
    FabricBridge,
    Manager,
    StorageSubsystem,
};

enum class EntityRole{
    Invalid,
    Initiator,
    Target,
    Both,
};

NLOHMANN_JSON_SERIALIZE_ENUM(EntityType, {
    {EntityType::Invalid, "Invalid"},
    {EntityType::StorageInitiator, "StorageInitiator"},
    {EntityType::RootComplex, "RootComplex"},
    {EntityType::NetworkController, "NetworkController"},
    {EntityType::Drive, "Drive"},
    {EntityType::StorageExpander, "StorageExpander"},
    {EntityType::DisplayController, "DisplayController"},
    {EntityType::Bridge, "Bridge"},
    {EntityType::Processor, "Processor"},
    {EntityType::Volume, "Volume"},
    {EntityType::AccelerationFunction, "AccelerationFunction"},
    {EntityType::MediaController, "MediaController"},
    {EntityType::MemoryChunk, "MemoryChunk"},
    {EntityType::Switch, "Switch"},
    {EntityType::FabricBridge, "FabricBridge"},
    {EntityType::Manager, "Manager"},
    {EntityType::StorageSubsystem, "StorageSubsystem"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(EntityRole, {
    {EntityRole::Invalid, "Invalid"},
    {EntityRole::Initiator, "Initiator"},
    {EntityRole::Target, "Target"},
    {EntityRole::Both, "Both"},
});

}
// clang-format on
