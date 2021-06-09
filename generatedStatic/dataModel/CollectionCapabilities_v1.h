#ifndef COLLECTIONCAPABILITIES_V1
#define COLLECTIONCAPABILITIES_V1

#include "CollectionCapabilities_v1.h"
#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

enum class CollectionCapabilitiesV1UseCase
{
    ComputerSystemComposition,
    ComputerSystemConstrainedComposition,
    VolumeCreation,
    ResourceBlockComposition,
    ResourceBlockConstrainedComposition,
};
struct CollectionCapabilitiesV1Links
{
    ResourceV1Resource oem;
    NavigationReferenceRedfish targetCollection;
    NavigationReferenceRedfish relatedItem;
};
struct CollectionCapabilitiesV1Capability
{
    NavigationReferenceRedfish capabilitiesObject;
    CollectionCapabilitiesV1UseCase useCase;
    CollectionCapabilitiesV1Links links;
};
struct CollectionCapabilitiesV1CollectionCapabilities
{
    CollectionCapabilitiesV1Capability capabilities;
    int64_t maxMembers;
};
#endif
