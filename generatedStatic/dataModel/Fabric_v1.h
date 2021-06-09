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

struct FabricV1OemActions
{};
struct FabricV1Actions
{
    FabricV1OemActions oem;
};
struct FabricV1Links
{
    ResourceV1Resource oem;
};
struct FabricV1Fabric
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ProtocolV1Protocol fabricType;
    ResourceV1Resource status;
    int64_t maxZones;
    ZoneCollectionV1ZoneCollection zones;
    EndpointCollectionV1EndpointCollection endpoints;
    SwitchCollectionV1SwitchCollection switches;
    FabricV1Links links;
    FabricV1Actions actions;
    AddressPoolCollectionV1AddressPoolCollection addressPools;
    ConnectionCollectionV1ConnectionCollection connections;
    EndpointGroupCollectionV1EndpointGroupCollection endpointGroups;
};
#endif
