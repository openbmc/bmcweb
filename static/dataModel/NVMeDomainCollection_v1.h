#ifndef NVMEDOMAINCOLLECTION_V1
#define NVMEDOMAINCOLLECTION_V1

#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

struct NVMeDomainCollectionV1NVMeDomainCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReferenceRedfish members;
};
#endif
