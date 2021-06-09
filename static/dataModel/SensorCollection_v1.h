#ifndef SENSORCOLLECTION_V1
#define SENSORCOLLECTION_V1

#include "NavigationReference__.h"
#include "Resource_v1.h"

struct SensorCollection_v1_SensorCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference__ members;
};
#endif
