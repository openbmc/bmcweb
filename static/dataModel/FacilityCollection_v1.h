#ifndef FACILITYCOLLECTION_V1
#define FACILITYCOLLECTION_V1

#include "NavigationReference_.h"
#include "Resource_v1.h"

struct FacilityCollectionV1FacilityCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReference_ members;
};
#endif
