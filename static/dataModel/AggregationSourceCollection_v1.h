#ifndef AGGREGATIONSOURCECOLLECTION_V1
#define AGGREGATIONSOURCECOLLECTION_V1

#include "NavigationReference_.h"
#include "Resource_v1.h"

struct AggregationSourceCollectionV1AggregationSourceCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReference_ members;
};
#endif
