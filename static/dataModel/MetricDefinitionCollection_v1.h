#ifndef METRICDEFINITIONCOLLECTION_V1
#define METRICDEFINITIONCOLLECTION_V1

#include "NavigationReference__.h"
#include "Resource_v1.h"

struct MetricDefinitionCollection_v1_MetricDefinitionCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference__ members;
};
#endif
