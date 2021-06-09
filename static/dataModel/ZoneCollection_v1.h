#ifndef ZONECOLLECTION_V1
#define ZONECOLLECTION_V1

#include "NavigationReference__.h"
#include "Resource_v1.h"

struct ZoneCollection_v1_ZoneCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference__ members;
};
#endif
