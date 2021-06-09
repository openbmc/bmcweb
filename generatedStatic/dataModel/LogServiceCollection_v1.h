#ifndef LOGSERVICECOLLECTION_V1
#define LOGSERVICECOLLECTION_V1

#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

struct LogServiceCollectionV1LogServiceCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReferenceRedfish members;
};
#endif
