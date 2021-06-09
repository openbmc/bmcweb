#ifndef MEMORYCHUNKSCOLLECTION_V1
#define MEMORYCHUNKSCOLLECTION_V1

#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

struct MemoryChunksCollectionV1MemoryChunksCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReferenceRedfish members;
};
#endif
