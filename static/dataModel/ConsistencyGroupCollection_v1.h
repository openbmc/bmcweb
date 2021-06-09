#ifndef CONSISTENCYGROUPCOLLECTION_V1
#define CONSISTENCYGROUPCOLLECTION_V1

#include "NavigationReference_.h"
#include "Resource_v1.h"

struct ConsistencyGroupCollectionV1ConsistencyGroupCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReference_ members;
};
#endif
