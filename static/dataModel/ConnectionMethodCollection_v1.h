#ifndef CONNECTIONMETHODCOLLECTION_V1
#define CONNECTIONMETHODCOLLECTION_V1

#include "NavigationReference__.h"
#include "Resource_v1.h"

struct ConnectionMethodCollection_v1_ConnectionMethodCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference__ members;
};
#endif
