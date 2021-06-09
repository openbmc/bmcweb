#ifndef SIMPLESTORAGECOLLECTION_V1
#define SIMPLESTORAGECOLLECTION_V1

#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

struct SimpleStorageCollectionV1SimpleStorageCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReferenceRedfish members;
};
#endif
