#ifndef SIMPLESTORAGECOLLECTION_V1
#define SIMPLESTORAGECOLLECTION_V1

#include "NavigationReference__.h"
#include "Resource_v1.h"

struct SimpleStorageCollection_v1_SimpleStorageCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference__ members;
};
#endif
