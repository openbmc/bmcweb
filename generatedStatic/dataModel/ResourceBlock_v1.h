#ifndef RESOURCEBLOCK_V1
#define RESOURCEBLOCK_V1

#include "NavigationReferenceRedfish.h"
#include "ResourceBlock_v1.h"
#include "Resource_v1.h"

enum class ResourceBlockV1CompositionState
{
    Composing,
    ComposedAndAvailable,
    Composed,
    Unused,
    Failed,
    Unavailable,
};
enum class ResourceBlockV1PoolType
{
    Free,
    Active,
    Unassigned,
};
enum class ResourceBlockV1ResourceBlockType
{
    Compute,
    Processor,
    Memory,
    Network,
    Storage,
    ComputerSystem,
    Expansion,
    IndependentResource,
};
struct ResourceBlockV1OemActions
{};
struct ResourceBlockV1Actions
{
    ResourceBlockV1OemActions oem;
};
struct ResourceBlockV1CompositionStatus
{
    bool reserved;
    ResourceBlockV1CompositionState compositionState;
    bool sharingCapable;
    bool sharingEnabled;
    int64_t maxCompositions;
    int64_t numberOfCompositions;
};
struct ResourceBlockV1Links
{
    ResourceV1Resource oem;
    NavigationReferenceRedfish computerSystems;
    NavigationReferenceRedfish chassis;
    NavigationReferenceRedfish zones;
    NavigationReferenceRedfish consumingResourceBlocks;
    NavigationReferenceRedfish supplyingResourceBlocks;
};
struct ResourceBlockV1ResourceBlock
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ResourceV1Resource status;
    ResourceBlockV1CompositionStatus compositionStatus;
    ResourceBlockV1ResourceBlockType resourceBlockType;
    ResourceBlockV1Links links;
    ResourceBlockV1Actions actions;
    NavigationReferenceRedfish processors;
    NavigationReferenceRedfish memory;
    NavigationReferenceRedfish storage;
    NavigationReferenceRedfish simpleStorage;
    NavigationReferenceRedfish ethernetInterfaces;
    NavigationReferenceRedfish networkInterfaces;
    NavigationReferenceRedfish computerSystems;
    NavigationReferenceRedfish drives;
    ResourceBlockV1PoolType pool;
    std::string client;
};
struct ResourceBlockV1ResourceBlockLimits
{
    int64_t minCompute;
    int64_t maxCompute;
    int64_t minProcessor;
    int64_t maxProcessor;
    int64_t minMemory;
    int64_t maxMemory;
    int64_t minNetwork;
    int64_t maxNetwork;
    int64_t minStorage;
    int64_t maxStorage;
    int64_t minComputerSystem;
    int64_t maxComputerSystem;
    int64_t minExpansion;
    int64_t maxExpansion;
};
#endif
