#ifndef ROUTEENTRYCOLLECTION_V1
#define ROUTEENTRYCOLLECTION_V1

#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

struct RouteEntryCollectionV1RouteEntryCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReferenceRedfish members;
};
#endif
