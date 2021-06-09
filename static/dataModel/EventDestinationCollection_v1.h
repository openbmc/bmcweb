#ifndef EVENTDESTINATIONCOLLECTION_V1
#define EVENTDESTINATIONCOLLECTION_V1

#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

struct EventDestinationCollectionV1EventDestinationCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReferenceRedfish members;
};
#endif
