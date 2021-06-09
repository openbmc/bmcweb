#ifndef HOSTINTERFACECOLLECTION_V1
#define HOSTINTERFACECOLLECTION_V1

#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

struct HostInterfaceCollectionV1HostInterfaceCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReferenceRedfish members;
};
#endif
