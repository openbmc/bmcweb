#ifndef ENDPOINTGROUPCOLLECTION_V1
#define ENDPOINTGROUPCOLLECTION_V1

#include "NavigationReference__.h"
#include "Resource_v1.h"

struct EndpointGroupCollection_v1_EndpointGroupCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference__ members;
};
#endif
