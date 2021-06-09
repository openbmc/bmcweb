#ifndef SENSORCOLLECTION_V1
#define SENSORCOLLECTION_V1

#include "NavigationReference_.h"
#include "Resource_v1.h"

struct SensorCollectionV1SensorCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReference_ members;
};
#endif
