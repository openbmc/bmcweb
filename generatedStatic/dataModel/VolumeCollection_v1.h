#ifndef VOLUMECOLLECTION_V1
#define VOLUMECOLLECTION_V1

#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

struct VolumeCollectionV1VolumeCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReferenceRedfish members;
};
#endif
