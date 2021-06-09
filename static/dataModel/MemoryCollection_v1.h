#ifndef MEMORYCOLLECTION_V1
#define MEMORYCOLLECTION_V1

#include "NavigationReference_.h"
#include "Resource_v1.h"

struct MemoryCollectionV1MemoryCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReference_ members;
};
#endif
