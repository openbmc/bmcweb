#ifndef SPARERESOURCESET_V1
#define SPARERESOURCESET_V1

#include <chrono>
#include "NavigationReference__.h"
#include "Resource_v1.h"
#include "SpareResourceSet_v1.h"

struct SpareResourceSet_v1_Actions
{
    SpareResourceSet_v1_OemActions oem;
};
struct SpareResourceSet_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference__ onHandSpares;
    NavigationReference__ replacementSpareSets;
};
struct SpareResourceSet_v1_OemActions
{
};
struct SpareResourceSet_v1_SpareResourceSet
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    std::string resourceType;
    std::chrono::milliseconds timeToProvision;
    std::chrono::milliseconds timeToReplenish;
    Resource_v1_Resource onHandLocation;
    bool onLine;
    SpareResourceSet_v1_Links links;
    SpareResourceSet_v1_Actions actions;
};
#endif
