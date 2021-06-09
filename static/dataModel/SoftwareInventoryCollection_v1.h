#ifndef SOFTWAREINVENTORYCOLLECTION_V1
#define SOFTWAREINVENTORYCOLLECTION_V1

#include "NavigationReference_.h"
#include "Resource_v1.h"

struct SoftwareInventoryCollectionV1SoftwareInventoryCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReference_ members;
};
#endif
