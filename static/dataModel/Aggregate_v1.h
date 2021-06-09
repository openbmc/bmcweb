#ifndef AGGREGATE_V1
#define AGGREGATE_V1

#include "Aggregate_v1.h"
#include "NavigationReference_.h"
#include "Resource_v1.h"

struct AggregateV1OemActions
{};
struct AggregateV1Actions
{
    AggregateV1OemActions oem;
};
struct AggregateV1Aggregate
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    NavigationReference_ elements;
    int64_t elementsCount;
    AggregateV1Actions actions;
};
#endif
