#ifndef POWERDOMAINCOLLECTION_V1
#define POWERDOMAINCOLLECTION_V1

#include "NavigationReference__.h"
#include "Resource_v1.h"

struct PowerDomainCollection_v1_PowerDomainCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference__ members;
};
#endif
