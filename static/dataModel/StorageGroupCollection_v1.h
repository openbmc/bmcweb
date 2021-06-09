#ifndef STORAGEGROUPCOLLECTION_V1
#define STORAGEGROUPCOLLECTION_V1

#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

struct StorageGroupCollectionV1StorageGroupCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReferenceRedfish members;
};
#endif
