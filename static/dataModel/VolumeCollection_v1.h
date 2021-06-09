#ifndef VOLUMECOLLECTION_V1
#define VOLUMECOLLECTION_V1

#include "NavigationReference__.h"
#include "Resource_v1.h"

struct VolumeCollection_v1_VolumeCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference__ members;
};
#endif
