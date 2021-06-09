#ifndef MEMORYCOLLECTION_V1
#define MEMORYCOLLECTION_V1

#include "NavigationReference__.h"
#include "Resource_v1.h"

struct MemoryCollection_v1_MemoryCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference__ members;
};
#endif
