#ifndef NVMEDOMAINCOLLECTION_V1
#define NVMEDOMAINCOLLECTION_V1

#include "NavigationReference.h"
#include "Resource_v1.h"

struct NVMeDomainCollection_v1_NVMeDomainCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference_ members;
};
#endif
