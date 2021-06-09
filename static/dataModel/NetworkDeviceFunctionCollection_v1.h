#ifndef NETWORKDEVICEFUNCTIONCOLLECTION_V1
#define NETWORKDEVICEFUNCTIONCOLLECTION_V1

#include "NavigationReference__.h"
#include "Resource_v1.h"

struct NetworkDeviceFunctionCollection_v1_NetworkDeviceFunctionCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference__ members;
};
#endif
