#ifndef NETWORKADAPTERCOLLECTION_V1
#define NETWORKADAPTERCOLLECTION_V1

#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

struct NetworkAdapterCollectionV1NetworkAdapterCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReferenceRedfish members;
};
#endif
