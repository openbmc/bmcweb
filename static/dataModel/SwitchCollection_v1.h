#ifndef SWITCHCOLLECTION_V1
#define SWITCHCOLLECTION_V1

#include "NavigationReference_.h"
#include "Resource_v1.h"

struct SwitchCollectionV1SwitchCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReference_ members;
};
#endif
