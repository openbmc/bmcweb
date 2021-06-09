#ifndef MEMORYDOMAIN_V1
#define MEMORYDOMAIN_V1

#include "MemoryChunksCollection_v1.h"
#include "MemoryDomain_v1.h"
#include "NavigationReference__.h"
#include "Resource_v1.h"

struct MemoryDomain_v1_Actions
{
    MemoryDomain_v1_OemActions oem;
};
struct MemoryDomain_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference__ mediaControllers;
};
struct MemoryDomain_v1_MemoryDomain
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    bool allowsMemoryChunkCreation;
    bool allowsBlockProvisioning;
    MemoryChunksCollection_v1_MemoryChunksCollection memoryChunks;
    MemoryDomain_v1_MemorySet interleavableMemorySets;
    bool allowsMirroring;
    bool allowsSparing;
    MemoryDomain_v1_Actions actions;
    MemoryDomain_v1_Links links;
};
struct MemoryDomain_v1_MemorySet
{
    NavigationReference__ memorySet;
};
struct MemoryDomain_v1_OemActions
{
};
#endif
