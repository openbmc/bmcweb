#ifndef MEDIACONTROLLERCOLLECTION_V1
#define MEDIACONTROLLERCOLLECTION_V1

#include "NavigationReference_.h"
#include "Resource_v1.h"

struct MediaControllerCollectionV1MediaControllerCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReference_ members;
};
#endif
