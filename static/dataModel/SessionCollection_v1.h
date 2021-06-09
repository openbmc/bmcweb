#ifndef SESSIONCOLLECTION_V1
#define SESSIONCOLLECTION_V1

#include "NavigationReference_.h"
#include "Resource_v1.h"

struct SessionCollectionV1SessionCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReference_ members;
};
#endif
