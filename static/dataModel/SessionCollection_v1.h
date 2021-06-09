#ifndef SESSIONCOLLECTION_V1
#define SESSIONCOLLECTION_V1

#include "NavigationReference__.h"
#include "Resource_v1.h"

struct SessionCollection_v1_SessionCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference__ members;
};
#endif
