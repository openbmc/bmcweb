#ifndef TASKCOLLECTION_V1
#define TASKCOLLECTION_V1

#include "NavigationReference__.h"
#include "Resource_v1.h"

struct TaskCollection_v1_TaskCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference__ members;
};
#endif
