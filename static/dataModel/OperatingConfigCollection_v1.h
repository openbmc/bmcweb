#ifndef OPERATINGCONFIGCOLLECTION_V1
#define OPERATINGCONFIGCOLLECTION_V1

#include "NavigationReference_.h"
#include "Resource_v1.h"

struct OperatingConfigCollectionV1OperatingConfigCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReference_ members;
};
#endif
