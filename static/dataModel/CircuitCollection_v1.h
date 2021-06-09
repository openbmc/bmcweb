#ifndef CIRCUITCOLLECTION_V1
#define CIRCUITCOLLECTION_V1

#include "NavigationReference__.h"
#include "Resource_v1.h"

struct CircuitCollection_v1_CircuitCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference__ members;
};
#endif
