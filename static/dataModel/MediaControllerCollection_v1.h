#ifndef MEDIACONTROLLERCOLLECTION_V1
#define MEDIACONTROLLERCOLLECTION_V1

#include "NavigationReference__.h"
#include "Resource_v1.h"

struct MediaControllerCollection_v1_MediaControllerCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference__ members;
};
#endif
