#ifndef ENDPOINTCOLLECTION_V1
#define ENDPOINTCOLLECTION_V1

#include "NavigationReference__.h"
#include "Resource_v1.h"

struct EndpointCollection_v1_EndpointCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference__ members;
};
#endif
