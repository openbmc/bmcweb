#ifndef CONNECTIONCOLLECTION_V1
#define CONNECTIONCOLLECTION_V1

#include "NavigationReference__.h"
#include "Resource_v1.h"

struct ConnectionCollection_v1_ConnectionCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference__ members;
};
#endif
