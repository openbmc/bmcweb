#ifndef AGGREGATIONSOURCECOLLECTION_V1
#define AGGREGATIONSOURCECOLLECTION_V1

#include "NavigationReference__.h"
#include "Resource_v1.h"

struct AggregationSourceCollection_v1_AggregationSourceCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference__ members;
};
#endif
