#ifndef ZONE_V1
#define ZONE_V1

#include "NavigationReference_.h"
#include "Resource_v1.h"
#include "Zone_v1.h"

enum class ZoneV1ExternalAccessibility
{
    GloballyAccessible,
    NonZonedAccessible,
    ZoneOnly,
    NoInternalRouting,
};
enum class ZoneV1ZoneType
{
    Default,
    ZoneOfEndpoints,
    ZoneOfZones,
    ZoneOfResourceBlocks,
};
struct ZoneV1OemActions
{};
struct ZoneV1Actions
{
    ZoneV1OemActions oem;
};
struct ZoneV1Links
{
    ResourceV1Resource oem;
    NavigationReference_ endpoints;
    NavigationReference_ involvedSwitches;
    NavigationReference_ resourceBlocks;
    NavigationReference_ addressPools;
    NavigationReference_ containedByZones;
    NavigationReference_ containsZones;
};
struct ZoneV1Zone
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ResourceV1Resource status;
    ZoneV1Links links;
    ZoneV1Actions actions;
    ResourceV1Resource identifiers;
    ZoneV1ExternalAccessibility externalAccessibility;
    ZoneV1ZoneType zoneType;
    bool defaultRoutingEnabled;
};
#endif
