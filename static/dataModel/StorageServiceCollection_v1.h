#ifndef STORAGESERVICECOLLECTION_V1
#define STORAGESERVICECOLLECTION_V1

#include "NavigationReference_.h"
#include "Resource_v1.h"

struct StorageServiceCollectionV1StorageServiceCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReference_ members;
};
#endif
