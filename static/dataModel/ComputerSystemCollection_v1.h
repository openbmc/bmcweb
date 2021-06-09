#ifndef COMPUTERSYSTEMCOLLECTION_V1
#define COMPUTERSYSTEMCOLLECTION_V1

#include "NavigationReference__.h"
#include "Resource_v1.h"

struct ComputerSystemCollection_v1_ComputerSystemCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference__ members;
};
#endif
