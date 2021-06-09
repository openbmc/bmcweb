#ifndef RESOURCEBLOCKCOLLECTION_V1
#define RESOURCEBLOCKCOLLECTION_V1

#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

struct ResourceBlockCollectionV1ResourceBlockCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReferenceRedfish members;
};
#endif
