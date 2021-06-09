#ifndef ROUTESETENTRY_V1
#define ROUTESETENTRY_V1

#include "Resource_v1.h"
#include "RouteSetEntry_v1.h"

struct RouteSetEntry_v1_Actions
{
    RouteSetEntry_v1_OemActions oem;
};
struct RouteSetEntry_v1_OemActions
{
};
struct RouteSetEntry_v1_RouteSetEntry
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    bool valid;
    int64_t vCAction;
    int64_t hopCount;
    int64_t egressIdentifier;
    RouteSetEntry_v1_Actions actions;
};
#endif
