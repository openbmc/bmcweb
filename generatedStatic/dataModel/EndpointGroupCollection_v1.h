#ifndef ENDPOINTGROUPCOLLECTION_V1
#define ENDPOINTGROUPCOLLECTION_V1

#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

struct EndpointGroupCollectionV1EndpointGroupCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReferenceRedfish members;
};
#endif
