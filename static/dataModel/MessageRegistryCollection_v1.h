#ifndef MESSAGEREGISTRYCOLLECTION_V1
#define MESSAGEREGISTRYCOLLECTION_V1

#include "NavigationReference_.h"
#include "Resource_v1.h"

struct MessageRegistryCollectionV1MessageRegistryCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReference_ members;
};
#endif
