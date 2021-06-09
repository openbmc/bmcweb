#ifndef AGGREGATECOLLECTION_V1
#define AGGREGATECOLLECTION_V1

#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

struct AggregateCollectionV1AggregateCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReferenceRedfish members;
};
#endif
