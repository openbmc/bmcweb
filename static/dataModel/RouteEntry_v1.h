#ifndef ROUTEENTRY_V1
#define ROUTEENTRY_V1

#include "Resource_v1.h"
#include "RouteEntry_v1.h"
#include "RouteSetEntryCollection_v1.h"

struct RouteEntry_v1_Actions
{
    RouteEntry_v1_OemActions oem;
};
struct RouteEntry_v1_OemActions
{
};
struct RouteEntry_v1_RouteEntry
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    RouteSetEntryCollection_v1_RouteSetEntryCollection routeSet;
    std::string rawEntryHex;
    int64_t minimumHopCount;
    RouteEntry_v1_Actions actions;
};
#endif
