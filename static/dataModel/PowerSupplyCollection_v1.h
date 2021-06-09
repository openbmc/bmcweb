#ifndef POWERSUPPLYCOLLECTION_V1
#define POWERSUPPLYCOLLECTION_V1

#include "NavigationReference.h"
#include "Resource_v1.h"

struct PowerSupplyCollection_v1_PowerSupplyCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference_ members;
};
#endif
