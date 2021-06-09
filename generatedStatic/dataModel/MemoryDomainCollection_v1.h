#ifndef MEMORYDOMAINCOLLECTION_V1
#define MEMORYDOMAINCOLLECTION_V1

#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

struct MemoryDomainCollectionV1MemoryDomainCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReferenceRedfish members;
};
#endif
