#ifndef OUTLETGROUPCOLLECTION_V1
#define OUTLETGROUPCOLLECTION_V1

#include "NavigationReference__.h"
#include "Resource_v1.h"

struct OutletGroupCollection_v1_OutletGroupCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference__ members;
};
#endif
