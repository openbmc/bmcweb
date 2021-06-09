#ifndef ACCELERATIONFUNCTIONCOLLECTION_V1
#define ACCELERATIONFUNCTIONCOLLECTION_V1

#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

struct AccelerationFunctionCollectionV1AccelerationFunctionCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReferenceRedfish members;
};
#endif
