#ifndef VCATENTRYCOLLECTION_V1
#define VCATENTRYCOLLECTION_V1

#include "NavigationReference__.h"
#include "Resource_v1.h"

struct VCATEntryCollection_v1_VCATEntryCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference__ members;
};
#endif
