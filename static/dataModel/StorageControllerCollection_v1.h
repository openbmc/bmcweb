#ifndef STORAGECONTROLLERCOLLECTION_V1
#define STORAGECONTROLLERCOLLECTION_V1

#include "NavigationReference__.h"
#include "Resource_v1.h"

struct StorageControllerCollection_v1_StorageControllerCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference__ members;
};
#endif
