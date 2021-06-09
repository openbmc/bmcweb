#ifndef PORTCOLLECTION_V1
#define PORTCOLLECTION_V1

#include "NavigationReference_.h"
#include "Resource_v1.h"

struct PortCollectionV1PortCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReference_ members;
};
#endif
