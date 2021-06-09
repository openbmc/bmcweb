#ifndef NETWORKDEVICEFUNCTIONCOLLECTION_V1
#define NETWORKDEVICEFUNCTIONCOLLECTION_V1

#include "NavigationReference_.h"
#include "Resource_v1.h"

struct NetworkDeviceFunctionCollectionV1NetworkDeviceFunctionCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReference_ members;
};
#endif
