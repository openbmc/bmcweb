#ifndef MANAGERACCOUNTCOLLECTION_V1
#define MANAGERACCOUNTCOLLECTION_V1

#include "NavigationReference__.h"
#include "Resource_v1.h"

struct ManagerAccountCollection_v1_ManagerAccountCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference__ members;
};
#endif
