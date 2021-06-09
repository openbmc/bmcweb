#ifndef SWITCHCOLLECTION_V1
#define SWITCHCOLLECTION_V1

#include "NavigationReference__.h"
#include "Resource_v1.h"

struct SwitchCollection_v1_SwitchCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference__ members;
};
#endif
