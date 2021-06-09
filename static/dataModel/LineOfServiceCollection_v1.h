#ifndef LINEOFSERVICECOLLECTION_V1
#define LINEOFSERVICECOLLECTION_V1

#include "NavigationReference_.h"
#include "Resource_v1.h"

struct LineOfServiceCollectionV1LineOfServiceCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReference_ members;
};
#endif
