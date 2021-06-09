#ifndef ENDPOINTGROUP_V1
#define ENDPOINTGROUP_V1

#include "EndpointGroup_v1.h"
#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

enum class EndpointGroupV1AccessState
{
    Optimized,
    NonOptimized,
    Standby,
    Unavailable,
    Transitioning,
};
enum class EndpointGroupV1GroupType
{
    Client,
    Server,
    Initiator,
    Target,
};
struct EndpointGroupV1OemActions
{};
struct EndpointGroupV1Actions
{
    EndpointGroupV1OemActions oem;
};
struct EndpointGroupV1Links
{
    ResourceV1Resource oem;
    NavigationReferenceRedfish endpoints;
    NavigationReferenceRedfish connections;
};
struct EndpointGroupV1EndpointGroup
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ResourceV1Resource identifier;
    EndpointGroupV1GroupType groupType;
    EndpointGroupV1AccessState accessState;
    int64_t targetEndpointGroupIdentifier;
    bool preferred;
    NavigationReferenceRedfish endpoints;
    EndpointGroupV1Links links;
    EndpointGroupV1Actions actions;
};
#endif
