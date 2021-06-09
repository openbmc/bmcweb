#ifndef FILESHARECOLLECTION_V1
#define FILESHARECOLLECTION_V1

#include "NavigationReference_.h"
#include "Resource_v1.h"

struct FileShareCollectionV1FileShareCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReference_ members;
};
#endif
