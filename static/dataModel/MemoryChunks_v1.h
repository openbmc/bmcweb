#ifndef MEMORYCHUNKS_V1
#define MEMORYCHUNKS_V1

#include "MemoryChunks_v1.h"
#include "NavigationReference__.h"
#include "Resource_v1.h"

enum class MemoryChunks_v1_AddressRangeType {
    Volatile,
    PMEM,
    Block,
};
struct MemoryChunks_v1_Actions
{
    MemoryChunks_v1_OemActions oem;
};
struct MemoryChunks_v1_InterleaveSet
{
    NavigationReference__ memory;
    std::string regionId;
    int64_t offsetMiB;
    int64_t sizeMiB;
    int64_t memoryLevel;
};
struct MemoryChunks_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference__ endpoints;
};
struct MemoryChunks_v1_MemoryChunks
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    int64_t memoryChunkSizeMiB;
    MemoryChunks_v1_AddressRangeType addressRangeType;
    bool isMirrorEnabled;
    bool isSpare;
    MemoryChunks_v1_InterleaveSet interleaveSets;
    MemoryChunks_v1_Actions actions;
    Resource_v1_Resource status;
    int64_t addressRangeOffsetMiB;
    MemoryChunks_v1_Links links;
    std::string displayName;
};
struct MemoryChunks_v1_OemActions
{
};
#endif
