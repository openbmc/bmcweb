#ifndef GRAPHICSCONTROLLERCOLLECTION_V1
#define GRAPHICSCONTROLLERCOLLECTION_V1

#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

struct GraphicsControllerCollectionV1GraphicsControllerCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReferenceRedfish members;
};
#endif
