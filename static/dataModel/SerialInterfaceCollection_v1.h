#ifndef SERIALINTERFACECOLLECTION_V1
#define SERIALINTERFACECOLLECTION_V1

#include "NavigationReference__.h"
#include "Resource_v1.h"

struct SerialInterfaceCollection_v1_SerialInterfaceCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference__ members;
};
#endif
