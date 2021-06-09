#ifndef STORAGESYSTEMCOLLECTION_V1
#define STORAGESYSTEMCOLLECTION_V1

#include "NavigationReference__.h"
#include "Resource_v1.h"

struct StorageSystemCollection_v1_StorageSystemCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference__ members;
};
#endif
