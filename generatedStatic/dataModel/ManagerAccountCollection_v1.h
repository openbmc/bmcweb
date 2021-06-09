#ifndef MANAGERACCOUNTCOLLECTION_V1
#define MANAGERACCOUNTCOLLECTION_V1

#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

struct ManagerAccountCollectionV1ManagerAccountCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReferenceRedfish members;
};
#endif
