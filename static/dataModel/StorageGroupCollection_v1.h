#ifndef STORAGEGROUPCOLLECTION_V1
#define STORAGEGROUPCOLLECTION_V1

#include "NavigationReference__.h"
#include "Resource_v1.h"

struct StorageGroupCollection_v1_StorageGroupCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference__ members;
};
#endif
