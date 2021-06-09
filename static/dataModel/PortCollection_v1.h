#ifndef PORTCOLLECTION_V1
#define PORTCOLLECTION_V1

#include "NavigationReference__.h"
#include "Resource_v1.h"

struct PortCollection_v1_PortCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference__ members;
};
#endif
