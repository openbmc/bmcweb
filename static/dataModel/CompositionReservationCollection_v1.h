#ifndef COMPOSITIONRESERVATIONCOLLECTION_V1
#define COMPOSITIONRESERVATIONCOLLECTION_V1

#include "NavigationReference_.h"
#include "Resource_v1.h"

struct CompositionReservationCollectionV1CompositionReservationCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReference_ members;
};
#endif
