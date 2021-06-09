#ifndef MESSAGEREGISTRYFILECOLLECTION_V1
#define MESSAGEREGISTRYFILECOLLECTION_V1

#include "NavigationReference__.h"
#include "Resource_v1.h"

struct MessageRegistryFileCollection_v1_MessageRegistryFileCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference__ members;
};
#endif
