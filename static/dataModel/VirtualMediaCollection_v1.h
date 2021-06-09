#ifndef VIRTUALMEDIACOLLECTION_V1
#define VIRTUALMEDIACOLLECTION_V1

#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

struct VirtualMediaCollectionV1VirtualMediaCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReferenceRedfish members;
};
#endif
