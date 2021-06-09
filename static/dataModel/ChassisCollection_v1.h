#ifndef CHASSISCOLLECTION_V1
#define CHASSISCOLLECTION_V1

#include "NavigationReference_.h"
#include "Resource_v1.h"

struct ChassisCollectionV1ChassisCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReference_ members;
};
#endif
