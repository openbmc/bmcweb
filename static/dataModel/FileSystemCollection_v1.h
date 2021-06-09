#ifndef FILESYSTEMCOLLECTION_V1
#define FILESYSTEMCOLLECTION_V1

#include "NavigationReference_.h"
#include "Resource_v1.h"

struct FileSystemCollectionV1FileSystemCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReference_ members;
};
#endif
