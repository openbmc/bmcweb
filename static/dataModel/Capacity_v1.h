#ifndef CAPACITY_V1
#define CAPACITY_V1

#include "Capacity_v1.h"
#include "DriveCollection_v1.h"
#include "MemoryChunksCollection_v1.h"
#include "MemoryCollection_v1.h"
#include "NavigationReference_.h"
#include "Resource_v1.h"
#include "StoragePoolCollection_v1.h"
#include "VolumeCollection_v1.h"

struct CapacityV1OemActions
{};
struct CapacityV1Actions
{
    CapacityV1OemActions oem;
};
struct CapacityV1CapacityInfo
{
    int64_t consumedBytes;
    int64_t allocatedBytes;
    int64_t guaranteedBytes;
    int64_t provisionedBytes;
};
struct CapacityV1Capacity
{
    CapacityV1CapacityInfo data;
    CapacityV1CapacityInfo metadata;
    CapacityV1CapacityInfo snapshot;
    bool isThinProvisioned;
};
struct CapacityV1CapacitySource
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    CapacityV1Capacity providedCapacity;
    NavigationReference_ providedClassOfService;
    VolumeCollectionV1VolumeCollection providingVolumes;
    StoragePoolCollectionV1StoragePoolCollection providingPools;
    DriveCollectionV1DriveCollection providingDrives;
    MemoryChunksCollectionV1MemoryChunksCollection providingMemoryChunks;
    MemoryCollectionV1MemoryCollection providingMemory;
    CapacityV1Actions actions;
};
#endif
