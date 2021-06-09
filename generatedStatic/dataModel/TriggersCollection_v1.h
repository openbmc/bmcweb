#ifndef TRIGGERSCOLLECTION_V1
#define TRIGGERSCOLLECTION_V1

#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

struct TriggersCollectionV1TriggersCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReferenceRedfish members;
};
#endif
