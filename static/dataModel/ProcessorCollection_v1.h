#ifndef PROCESSORCOLLECTION_V1
#define PROCESSORCOLLECTION_V1

#include "NavigationReference_.h"
#include "Resource_v1.h"

struct ProcessorCollectionV1ProcessorCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReference_ members;
};
#endif
