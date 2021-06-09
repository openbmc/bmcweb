#ifndef PCIEFUNCTIONCOLLECTION_V1
#define PCIEFUNCTIONCOLLECTION_V1

#include "NavigationReference_.h"
#include "Resource_v1.h"

struct PCIeFunctionCollectionV1PCIeFunctionCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReference_ members;
};
#endif
