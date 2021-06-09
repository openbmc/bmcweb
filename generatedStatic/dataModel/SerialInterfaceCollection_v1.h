#ifndef SERIALINTERFACECOLLECTION_V1
#define SERIALINTERFACECOLLECTION_V1

#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

struct SerialInterfaceCollectionV1SerialInterfaceCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReferenceRedfish members;
};
#endif
