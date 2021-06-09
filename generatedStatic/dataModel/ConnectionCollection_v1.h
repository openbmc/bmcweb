#ifndef CONNECTIONCOLLECTION_V1
#define CONNECTIONCOLLECTION_V1

#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

struct ConnectionCollectionV1ConnectionCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReferenceRedfish members;
};
#endif
