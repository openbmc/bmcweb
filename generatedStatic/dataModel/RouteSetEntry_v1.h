#ifndef ROUTESETENTRY_V1
#define ROUTESETENTRY_V1

#include "Resource_v1.h"
#include "RouteSetEntry_v1.h"

struct RouteSetEntryV1OemActions
{};
struct RouteSetEntryV1Actions
{
    RouteSetEntryV1OemActions oem;
};
struct RouteSetEntryV1RouteSetEntry
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    bool valid;
    int64_t vCAction;
    int64_t hopCount;
    int64_t egressIdentifier;
    RouteSetEntryV1Actions actions;
};
#endif
