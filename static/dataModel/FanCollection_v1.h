#ifndef FANCOLLECTION_V1
#define FANCOLLECTION_V1

#include "NavigationReference.h"
#include "Resource_v1.h"

struct FanCollection_v1_FanCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference_ members;
};
#endif
