#ifndef ROUTEENTRYCOLLECTION_V1
#define ROUTEENTRYCOLLECTION_V1

#include "NavigationReference__.h"
#include "Resource_v1.h"

struct RouteEntryCollection_v1_RouteEntryCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference__ members;
};
#endif
