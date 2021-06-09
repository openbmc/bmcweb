#ifndef LINEOFSERVICECOLLECTION_V1
#define LINEOFSERVICECOLLECTION_V1

#include "NavigationReference__.h"
#include "Resource_v1.h"

struct LineOfServiceCollection_v1_LineOfServiceCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference__ members;
};
#endif
