#ifndef MEMORYCHUNKS_V1
#define MEMORYCHUNKS_V1

#include "MemoryChunks_v1.h"
#include "NavigationReference_.h"
#include "Resource_v1.h"

enum class MemoryChunksV1AddressRangeType
{
    Volatile,
    PMEM,
    Block,
};
struct MemoryChunksV1OemActions
{};
struct MemoryChunksV1Actions
{
    MemoryChunksV1OemActions oem;
};
struct MemoryChunksV1InterleaveSet
{
    NavigationReference_ memory;
    std::string regionId;
    int64_t offsetMiB;
    int64_t sizeMiB;
    int64_t memoryLevel;
};
struct MemoryChunksV1Links
{
    ResourceV1Resource oem;
    NavigationReference_ endpoints;
};
struct MemoryChunksV1MemoryChunks
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    int64_t memoryChunkSizeMiB;
    MemoryChunksV1AddressRangeType addressRangeType;
    bool isMirrorEnabled;
    bool isSpare;
    MemoryChunksV1InterleaveSet interleaveSets;
    MemoryChunksV1Actions actions;
    ResourceV1Resource status;
    int64_t addressRangeOffsetMiB;
    MemoryChunksV1Links links;
    std::string displayName;
};
#endif
