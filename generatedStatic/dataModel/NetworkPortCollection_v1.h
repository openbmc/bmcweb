#ifndef NETWORKPORTCOLLECTION_V1
#define NETWORKPORTCOLLECTION_V1

#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

struct NetworkPortCollectionV1NetworkPortCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReferenceRedfish members;
};
#endif
