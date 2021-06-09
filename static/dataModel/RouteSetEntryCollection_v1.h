#ifndef ROUTESETENTRYCOLLECTION_V1
#define ROUTESETENTRYCOLLECTION_V1

#include "NavigationReference_.h"
#include "Resource_v1.h"

struct RouteSetEntryCollectionV1RouteSetEntryCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReference_ members;
};
#endif
