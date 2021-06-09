#ifndef VLANNETWORKINTERFACECOLLECTION_V1
#define VLANNETWORKINTERFACECOLLECTION_V1

#include "NavigationReference__.h"
#include "Resource_v1.h"

struct VLanNetworkInterfaceCollection_v1_VLanNetworkInterfaceCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference__ members;
};
#endif
