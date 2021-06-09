#ifndef CLASSOFSERVICECOLLECTION_V1
#define CLASSOFSERVICECOLLECTION_V1

#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

struct ClassOfServiceCollectionV1ClassOfServiceCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReferenceRedfish members;
};
#endif
