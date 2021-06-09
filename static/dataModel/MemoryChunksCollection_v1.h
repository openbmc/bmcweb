#ifndef MEMORYCHUNKSCOLLECTION_V1
#define MEMORYCHUNKSCOLLECTION_V1

#include "NavigationReference__.h"
#include "Resource_v1.h"

struct MemoryChunksCollection_v1_MemoryChunksCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference__ members;
};
#endif
