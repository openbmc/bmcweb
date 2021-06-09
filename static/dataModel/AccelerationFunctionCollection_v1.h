#ifndef ACCELERATIONFUNCTIONCOLLECTION_V1
#define ACCELERATIONFUNCTIONCOLLECTION_V1

#include "NavigationReference__.h"
#include "Resource_v1.h"

struct AccelerationFunctionCollection_v1_AccelerationFunctionCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference__ members;
};
#endif
