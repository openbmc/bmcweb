#ifndef COMPUTERSYSTEMCOLLECTION_V1
#define COMPUTERSYSTEMCOLLECTION_V1

#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

struct ComputerSystemCollectionV1ComputerSystemCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReferenceRedfish members;
};
#endif
