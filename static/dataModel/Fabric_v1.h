#ifndef FABRIC_V1
#define FABRIC_V1

#include "AddressPoolCollection_v1.h"
#include "ConnectionCollection_v1.h"
#include "EndpointCollection_v1.h"
#include "EndpointGroupCollection_v1.h"
#include "Fabric_v1.h"
#include "Protocol_v1.h"
#include "Resource_v1.h"
#include "SwitchCollection_v1.h"
#include "ZoneCollection_v1.h"

struct Fabric_v1_Actions
{
    Fabric_v1_OemActions oem;
};
struct Fabric_v1_Fabric
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Protocol_v1_Protocol fabricType;
    Resource_v1_Resource status;
    int64_t maxZones;
    ZoneCollection_v1_ZoneCollection zones;
    EndpointCollection_v1_EndpointCollection endpoints;
    SwitchCollection_v1_SwitchCollection switches;
    Fabric_v1_Links links;
    Fabric_v1_Actions actions;
    AddressPoolCollection_v1_AddressPoolCollection addressPools;
    ConnectionCollection_v1_ConnectionCollection connections;
    EndpointGroupCollection_v1_EndpointGroupCollection endpointGroups;
};
struct Fabric_v1_Links
{
    Resource_v1_Resource oem;
};
struct Fabric_v1_OemActions
{
};
#endif
