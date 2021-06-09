#ifndef CAPACITY_V1
#define CAPACITY_V1

#include "Capacity_v1.h"
#include "DriveCollection_v1.h"
#include "MemoryChunksCollection_v1.h"
#include "MemoryCollection_v1.h"
#include "NavigationReference__.h"
#include "Resource_v1.h"
#include "StoragePoolCollection_v1.h"
#include "VolumeCollection_v1.h"

struct Capacity_v1_Actions
{
    Capacity_v1_OemActions oem;
};
struct Capacity_v1_Capacity
{
    Capacity_v1_CapacityInfo data;
    Capacity_v1_CapacityInfo metadata;
    Capacity_v1_CapacityInfo snapshot;
    bool isThinProvisioned;
};
struct Capacity_v1_CapacityInfo
{
    int64_t consumedBytes;
    int64_t allocatedBytes;
    int64_t guaranteedBytes;
    int64_t provisionedBytes;
};
struct Capacity_v1_CapacitySource
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Capacity_v1_Capacity providedCapacity;
    NavigationReference__ providedClassOfService;
    VolumeCollection_v1_VolumeCollection providingVolumes;
    StoragePoolCollection_v1_StoragePoolCollection providingPools;
    DriveCollection_v1_DriveCollection providingDrives;
    MemoryChunksCollection_v1_MemoryChunksCollection providingMemoryChunks;
    MemoryCollection_v1_MemoryCollection providingMemory;
    Capacity_v1_Actions actions;
};
struct Capacity_v1_OemActions
{
};
#endif
