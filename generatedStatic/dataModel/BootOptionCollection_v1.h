#ifndef BOOTOPTIONCOLLECTION_V1
#define BOOTOPTIONCOLLECTION_V1

#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

struct BootOptionCollectionV1BootOptionCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReferenceRedfish members;
};
#endif
