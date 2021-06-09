#ifndef ENDPOINTGROUP_V1
#define ENDPOINTGROUP_V1

#include "EndpointGroup_v1.h"
#include "NavigationReference__.h"
#include "Resource_v1.h"

enum class EndpointGroup_v1_AccessState
{
    Optimized,
    NonOptimized,
    Standby,
    Unavailable,
    Transitioning,
};
enum class EndpointGroup_v1_GroupType
{
    Client,
    Server,
    Initiator,
    Target,
};
struct EndpointGroup_v1_Actions
{
    EndpointGroup_v1_OemActions oem;
};
struct EndpointGroup_v1_EndpointGroup
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Resource_v1_Resource identifier;
    EndpointGroup_v1_GroupType groupType;
    EndpointGroup_v1_AccessState accessState;
    int64_t targetEndpointGroupIdentifier;
    bool preferred;
    NavigationReference__ endpoints;
    EndpointGroup_v1_Links links;
    EndpointGroup_v1_Actions actions;
};
struct EndpointGroup_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference__ endpoints;
    NavigationReference__ connections;
};
struct EndpointGroup_v1_OemActions
{};
#endif
