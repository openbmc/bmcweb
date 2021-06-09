#ifndef TASKCOLLECTION_V1
#define TASKCOLLECTION_V1

#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

struct TaskCollectionV1TaskCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReferenceRedfish members;
};
#endif
