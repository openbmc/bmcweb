#ifndef POWERDISTRIBUTIONCOLLECTION_V1
#define POWERDISTRIBUTIONCOLLECTION_V1

#include "NavigationReference_.h"
#include "Resource_v1.h"

struct PowerDistributionCollectionV1PowerDistributionCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReference_ members;
};
#endif
