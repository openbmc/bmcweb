#ifndef TRIGGERSCOLLECTION_V1
#define TRIGGERSCOLLECTION_V1

#include "NavigationReference__.h"
#include "Resource_v1.h"

struct TriggersCollection_v1_TriggersCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference__ members;
};
#endif
