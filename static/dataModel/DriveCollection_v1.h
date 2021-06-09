#ifndef DRIVECOLLECTION_V1
#define DRIVECOLLECTION_V1

#include "NavigationReference_.h"
#include "Resource_v1.h"

struct DriveCollectionV1DriveCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReference_ members;
};
#endif
