#ifndef FANCOLLECTION_V1
#define FANCOLLECTION_V1

#include "NavigationReference_.h"
#include "Resource_v1.h"

struct FanCollectionV1FanCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReference_ members;
};
#endif
