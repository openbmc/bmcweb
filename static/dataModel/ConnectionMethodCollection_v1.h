#ifndef CONNECTIONMETHODCOLLECTION_V1
#define CONNECTIONMETHODCOLLECTION_V1

#include "NavigationReference_.h"
#include "Resource_v1.h"

struct ConnectionMethodCollectionV1ConnectionMethodCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReference_ members;
};
#endif
