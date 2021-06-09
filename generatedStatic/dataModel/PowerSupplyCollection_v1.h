#ifndef POWERSUPPLYCOLLECTION_V1
#define POWERSUPPLYCOLLECTION_V1

#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

struct PowerSupplyCollectionV1PowerSupplyCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReferenceRedfish members;
};
#endif
