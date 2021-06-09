#ifndef COLLECTIONCAPABILITIES_V1
#define COLLECTIONCAPABILITIES_V1

#include "CollectionCapabilities_v1.h"
#include "NavigationReference__.h"
#include "Resource_v1.h"

enum class CollectionCapabilities_v1_UseCase {
    ComputerSystemComposition,
    ComputerSystemConstrainedComposition,
    VolumeCreation,
};
struct CollectionCapabilities_v1_Capability
{
    NavigationReference__ capabilitiesObject;
    CollectionCapabilities_v1_UseCase useCase;
    CollectionCapabilities_v1_Links links;
};
struct CollectionCapabilities_v1_CollectionCapabilities
{
    CollectionCapabilities_v1_Capability capabilities;
    int64_t maxMembers;
};
struct CollectionCapabilities_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference__ targetCollection;
    NavigationReference__ relatedItem;
};
#endif
