#ifndef ENDPOINTCOLLECTION_V1
#define ENDPOINTCOLLECTION_V1

#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

struct EndpointCollectionV1EndpointCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReferenceRedfish members;
};
#endif
