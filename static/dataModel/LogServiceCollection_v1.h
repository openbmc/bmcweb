#ifndef LOGSERVICECOLLECTION_V1
#define LOGSERVICECOLLECTION_V1

#include "NavigationReference__.h"
#include "Resource_v1.h"

struct LogServiceCollection_v1_LogServiceCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference__ members;
};
#endif
