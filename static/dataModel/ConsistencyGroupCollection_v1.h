#ifndef CONSISTENCYGROUPCOLLECTION_V1
#define CONSISTENCYGROUPCOLLECTION_V1

#include "NavigationReference__.h"
#include "Resource_v1.h"

struct ConsistencyGroupCollection_v1_ConsistencyGroupCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference__ members;
};
#endif
