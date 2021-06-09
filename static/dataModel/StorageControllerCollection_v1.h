#ifndef STORAGECONTROLLERCOLLECTION_V1
#define STORAGECONTROLLERCOLLECTION_V1

#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

struct StorageControllerCollectionV1StorageControllerCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReferenceRedfish members;
};
#endif
