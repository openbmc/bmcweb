#ifndef DRIVECOLLECTION_V1
#define DRIVECOLLECTION_V1

#include "NavigationReference__.h"
#include "Resource_v1.h"

struct DriveCollection_v1_DriveCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference__ members;
};
#endif
