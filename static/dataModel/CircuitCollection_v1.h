#ifndef CIRCUITCOLLECTION_V1
#define CIRCUITCOLLECTION_V1

#include "NavigationReference_.h"
#include "Resource_v1.h"

struct CircuitCollectionV1CircuitCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReference_ members;
};
#endif
