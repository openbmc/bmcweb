#ifndef SPARERESOURCESET_V1
#define SPARERESOURCESET_V1

#include "NavigationReference_.h"
#include "Resource_v1.h"
#include "SpareResourceSet_v1.h"

#include <chrono>

struct SpareResourceSetV1OemActions
{};
struct SpareResourceSetV1Actions
{
    SpareResourceSetV1OemActions oem;
};
struct SpareResourceSetV1Links
{
    ResourceV1Resource oem;
    NavigationReference_ onHandSpares;
    NavigationReference_ replacementSpareSets;
};
struct SpareResourceSetV1SpareResourceSet
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    std::string resourceType;
    std::chrono::milliseconds timeToProvision;
    std::chrono::milliseconds timeToReplenish;
    ResourceV1Resource onHandLocation;
    bool onLine;
    SpareResourceSetV1Links links;
    SpareResourceSetV1Actions actions;
};
#endif
