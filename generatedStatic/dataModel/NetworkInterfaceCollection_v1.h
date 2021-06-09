#ifndef NETWORKINTERFACECOLLECTION_V1
#define NETWORKINTERFACECOLLECTION_V1

#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

struct NetworkInterfaceCollectionV1NetworkInterfaceCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReferenceRedfish members;
};
#endif
