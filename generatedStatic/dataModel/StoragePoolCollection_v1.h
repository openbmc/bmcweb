#ifndef STORAGEPOOLCOLLECTION_V1
#define STORAGEPOOLCOLLECTION_V1

#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

struct StoragePoolCollectionV1StoragePoolCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReferenceRedfish members;
};
#endif
