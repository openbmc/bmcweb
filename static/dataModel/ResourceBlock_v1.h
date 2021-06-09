#ifndef RESOURCEBLOCK_V1
#define RESOURCEBLOCK_V1

#include "NavigationReference__.h"
#include "ResourceBlock_v1.h"
#include "Resource_v1.h"

enum class ResourceBlock_v1_CompositionState
{
    Composing,
    ComposedAndAvailable,
    Composed,
    Unused,
    Failed,
    Unavailable,
};
enum class ResourceBlock_v1_ResourceBlockType
{
    Compute,
    Processor,
    Memory,
    Network,
    Storage,
    ComputerSystem,
    Expansion,
};
struct ResourceBlock_v1_Actions
{
    ResourceBlock_v1_OemActions oem;
};
struct ResourceBlock_v1_CompositionStatus
{
    bool reserved;
    ResourceBlock_v1_CompositionState compositionState;
    bool sharingCapable;
    bool sharingEnabled;
    int64_t maxCompositions;
    int64_t numberOfCompositions;
};
struct ResourceBlock_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference__ computerSystems;
    NavigationReference__ chassis;
    NavigationReference__ zones;
};
struct ResourceBlock_v1_OemActions
{};
struct ResourceBlock_v1_ResourceBlock
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Resource_v1_Resource status;
    ResourceBlock_v1_CompositionStatus compositionStatus;
    ResourceBlock_v1_ResourceBlockType resourceBlockType;
    ResourceBlock_v1_Links links;
    ResourceBlock_v1_Actions actions;
    NavigationReference__ processors;
    NavigationReference__ memory;
    NavigationReference__ storage;
    NavigationReference__ simpleStorage;
    NavigationReference__ ethernetInterfaces;
    NavigationReference__ networkInterfaces;
    NavigationReference__ computerSystems;
    NavigationReference__ drives;
};
struct ResourceBlock_v1_ResourceBlockLimits
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
