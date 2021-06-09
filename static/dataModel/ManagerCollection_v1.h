#ifndef MANAGERCOLLECTION_V1
#define MANAGERCOLLECTION_V1

#include "NavigationReference__.h"
#include "Resource_v1.h"

struct ManagerCollection_v1_ManagerCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference__ members;
};
#endif
