#ifndef VLANNETWORKINTERFACECOLLECTION_V1
#define VLANNETWORKINTERFACECOLLECTION_V1

#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

struct VLanNetworkInterfaceCollectionV1VLanNetworkInterfaceCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReferenceRedfish members;
};
#endif
