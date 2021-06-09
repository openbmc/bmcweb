#ifndef ETHERNETINTERFACECOLLECTION_V1
#define ETHERNETINTERFACECOLLECTION_V1

#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

struct EthernetInterfaceCollectionV1EthernetInterfaceCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReferenceRedfish members;
};
#endif
