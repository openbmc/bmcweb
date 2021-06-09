#ifndef RESOURCEBLOCKCOLLECTION_V1
#define RESOURCEBLOCKCOLLECTION_V1

#include "NavigationReference__.h"
#include "Resource_v1.h"

struct ResourceBlockCollection_v1_ResourceBlockCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference__ members;
};
#endif
