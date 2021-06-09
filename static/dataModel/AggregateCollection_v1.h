#ifndef AGGREGATECOLLECTION_V1
#define AGGREGATECOLLECTION_V1

#include "NavigationReference__.h"
#include "Resource_v1.h"

struct AggregateCollection_v1_AggregateCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference__ members;
};
#endif
