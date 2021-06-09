#ifndef ZONECOLLECTION_V1
#define ZONECOLLECTION_V1

#include "NavigationReference_.h"
#include "Resource_v1.h"

struct ZoneCollectionV1ZoneCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReference_ members;
};
#endif
