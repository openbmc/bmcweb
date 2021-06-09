#ifndef MEMORYDOMAIN_V1
#define MEMORYDOMAIN_V1

#include "MemoryChunksCollection_v1.h"
#include "MemoryDomain_v1.h"
#include "NavigationReference_.h"
#include "Resource_v1.h"

struct MemoryDomainV1OemActions
{};
struct MemoryDomainV1Actions
{
    MemoryDomainV1OemActions oem;
};
struct MemoryDomainV1Links
{
    ResourceV1Resource oem;
    NavigationReference_ mediaControllers;
};
struct MemoryDomainV1MemorySet
{
    NavigationReference_ memorySet;
};
struct MemoryDomainV1MemoryDomain
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    bool allowsMemoryChunkCreation;
    bool allowsBlockProvisioning;
    MemoryChunksCollectionV1MemoryChunksCollection memoryChunks;
    MemoryDomainV1MemorySet interleavableMemorySets;
    bool allowsMirroring;
    bool allowsSparing;
    MemoryDomainV1Actions actions;
    MemoryDomainV1Links links;
};
#endif
