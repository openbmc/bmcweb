#ifndef COMPOSITIONSERVICE_V1
#define COMPOSITIONSERVICE_V1

#include "CompositionService_v1.h"
#include "Resource_v1.h"
#include "ResourceBlockCollection_v1.h"
#include "ZoneCollection_v1.h"

struct CompositionService_v1_Actions
{
    CompositionService_v1_OemActions oem;
};
struct CompositionService_v1_CompositionService
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Resource_v1_Resource status;
    bool serviceEnabled;
    CompositionService_v1_Actions actions;
    ResourceBlockCollection_v1_ResourceBlockCollection resourceBlocks;
    ZoneCollection_v1_ZoneCollection resourceZones;
    bool allowOverprovisioning;
    bool allowZoneAffinity;
};
struct CompositionService_v1_OemActions
{
};
#endif
