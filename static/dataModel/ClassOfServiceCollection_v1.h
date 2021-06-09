#ifndef CLASSOFSERVICECOLLECTION_V1
#define CLASSOFSERVICECOLLECTION_V1

#include "NavigationReference__.h"
#include "Resource_v1.h"

struct ClassOfServiceCollection_v1_ClassOfServiceCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference__ members;
};
#endif
