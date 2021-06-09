#ifndef STORAGECOLLECTION_V1
#define STORAGECOLLECTION_V1

#include "NavigationReference_.h"
#include "Resource_v1.h"

struct StorageCollectionV1StorageCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReference_ members;
};
#endif
