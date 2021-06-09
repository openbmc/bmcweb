#ifndef NETWORKPORTCOLLECTION_V1
#define NETWORKPORTCOLLECTION_V1

#include "NavigationReference__.h"
#include "Resource_v1.h"

struct NetworkPortCollection_v1_NetworkPortCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference__ members;
};
#endif
