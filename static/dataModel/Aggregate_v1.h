#ifndef AGGREGATE_V1
#define AGGREGATE_V1

#include "Aggregate_v1.h"
#include "NavigationReference__.h"
#include "Resource_v1.h"

struct Aggregate_v1_Actions
{
    Aggregate_v1_OemActions oem;
};
struct Aggregate_v1_Aggregate
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    NavigationReference__ elements;
    int64_t elementsCount;
    Aggregate_v1_Actions actions;
};
struct Aggregate_v1_OemActions
{
};
#endif
