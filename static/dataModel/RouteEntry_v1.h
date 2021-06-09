#ifndef ROUTEENTRY_V1
#define ROUTEENTRY_V1

#include "Resource_v1.h"
#include "RouteEntry_v1.h"
#include "RouteSetEntryCollection_v1.h"

struct RouteEntryV1OemActions
{};
struct RouteEntryV1Actions
{
    RouteEntryV1OemActions oem;
};
struct RouteEntryV1RouteEntry
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    RouteSetEntryCollectionV1RouteSetEntryCollection routeSet;
    std::string rawEntryHex;
    int64_t minimumHopCount;
    RouteEntryV1Actions actions;
};
#endif
