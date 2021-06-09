#ifndef BOOTOPTIONCOLLECTION_V1
#define BOOTOPTIONCOLLECTION_V1

#include "NavigationReference__.h"
#include "Resource_v1.h"

struct BootOptionCollection_v1_BootOptionCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference__ members;
};
#endif
