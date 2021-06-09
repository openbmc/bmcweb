#ifndef STORAGESYSTEMCOLLECTION_V1
#define STORAGESYSTEMCOLLECTION_V1

#include "NavigationReference_.h"
#include "Resource_v1.h"

struct StorageSystemCollectionV1StorageSystemCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReference_ members;
};
#endif
