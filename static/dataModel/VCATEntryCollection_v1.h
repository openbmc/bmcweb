#ifndef VCATENTRYCOLLECTION_V1
#define VCATENTRYCOLLECTION_V1

#include "NavigationReference_.h"
#include "Resource_v1.h"

struct VCATEntryCollectionV1VCATEntryCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReference_ members;
};
#endif
