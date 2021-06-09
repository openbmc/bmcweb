#ifndef CONNECTION_V1
#define CONNECTION_V1

#include "Connection_v1.h"
#include "NavigationReference__.h"
#include "Resource_v1.h"

enum class Connection_v1_AccessCapability
{
    Read,
    Write,
};
enum class Connection_v1_AccessState
{
    Optimized,
    NonOptimized,
    Standby,
    Unavailable,
    Transitioning,
};
enum class Connection_v1_ConnectionType
{
    Storage,
    Memory,
};
struct Connection_v1_Actions
{
    Connection_v1_OemActions oem;
};
struct Connection_v1_Connection
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Resource_v1_Resource status;
    Connection_v1_ConnectionType connectionType;
    Connection_v1_VolumeInfo volumeInfo;
    Connection_v1_Links links;
    Connection_v1_Actions actions;
};
struct Connection_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference__ initiatorEndpoints;
    NavigationReference__ targetEndpoints;
    NavigationReference__ initiatorEndpointGroups;
    NavigationReference__ targetEndpointGroups;
};
struct Connection_v1_OemActions
{};
struct Connection_v1_VolumeInfo
{
    Connection_v1_AccessCapability accessCapabilities;
    Connection_v1_AccessState accessState;
    NavigationReference__ volume;
};
#endif
