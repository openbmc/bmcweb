#ifndef MANAGERCOLLECTION_V1
#define MANAGERCOLLECTION_V1

#include "NavigationReference_.h"
#include "Resource_v1.h"

struct ManagerCollectionV1ManagerCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReference_ members;
};
#endif
