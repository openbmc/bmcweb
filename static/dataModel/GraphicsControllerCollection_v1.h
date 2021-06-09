#ifndef GRAPHICSCONTROLLERCOLLECTION_V1
#define GRAPHICSCONTROLLERCOLLECTION_V1

#include "NavigationReference.h"
#include "Resource_v1.h"

struct GraphicsControllerCollection_v1_GraphicsControllerCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference_ members;
};
#endif
