#ifndef PCIEFUNCTIONCOLLECTION_V1
#define PCIEFUNCTIONCOLLECTION_V1

#include "NavigationReference__.h"
#include "Resource_v1.h"

struct PCIeFunctionCollection_v1_PCIeFunctionCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference__ members;
};
#endif
