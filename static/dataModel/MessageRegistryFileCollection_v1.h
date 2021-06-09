#ifndef MESSAGEREGISTRYFILECOLLECTION_V1
#define MESSAGEREGISTRYFILECOLLECTION_V1

#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

struct MessageRegistryFileCollectionV1MessageRegistryFileCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReferenceRedfish members;
};
#endif
