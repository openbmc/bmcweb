#ifndef ZONE_V1
#define ZONE_V1

#include "NavigationReference__.h"
#include "Resource_v1.h"
#include "Zone_v1.h"

enum class Zone_v1_ExternalAccessibility
{
    GloballyAccessible,
    NonZonedAccessible,
    ZoneOnly,
    NoInternalRouting,
};
enum class Zone_v1_ZoneType
{
    Default,
    ZoneOfEndpoints,
    ZoneOfZones,
};
struct Zone_v1_Actions
{
    Zone_v1_OemActions oem;
};
struct Zone_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference__ endpoints;
    NavigationReference__ involvedSwitches;
    NavigationReference__ resourceBlocks;
    NavigationReference__ addressPools;
    NavigationReference__ containedByZones;
    NavigationReference__ containsZones;
};
struct Zone_v1_OemActions
{};
struct Zone_v1_Zone
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Resource_v1_Resource status;
    Zone_v1_Links links;
    Zone_v1_Actions actions;
    Resource_v1_Resource identifiers;
    Zone_v1_ExternalAccessibility externalAccessibility;
    Zone_v1_ZoneType zoneType;
    bool defaultRoutingEnabled;
};
#endif
