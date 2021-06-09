#ifndef FABRICCOLLECTION_V1
#define FABRICCOLLECTION_V1

#include "NavigationReference_.h"
#include "Resource_v1.h"

struct FabricCollectionV1FabricCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReference_ members;
};
#endif
