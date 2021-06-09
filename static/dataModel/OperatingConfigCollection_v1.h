#ifndef OPERATINGCONFIGCOLLECTION_V1
#define OPERATINGCONFIGCOLLECTION_V1

#include "NavigationReference__.h"
#include "Resource_v1.h"

struct OperatingConfigCollection_v1_OperatingConfigCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference__ members;
};
#endif
