#ifndef OUTLETCOLLECTION_V1
#define OUTLETCOLLECTION_V1

#include "NavigationReference_.h"
#include "Resource_v1.h"

struct OutletCollectionV1OutletCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReference_ members;
};
#endif
