#ifndef FABRICCOLLECTION_V1
#define FABRICCOLLECTION_V1

#include "NavigationReference__.h"
#include "Resource_v1.h"

struct FabricCollection_v1_FabricCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference__ members;
};
#endif
