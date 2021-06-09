#ifndef OUTLETGROUPCOLLECTION_V1
#define OUTLETGROUPCOLLECTION_V1

#include "NavigationReference_.h"
#include "Resource_v1.h"

struct OutletGroupCollectionV1OutletGroupCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReference_ members;
};
#endif
