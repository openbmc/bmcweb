#ifndef CONNECTION_V1
#define CONNECTION_V1

#include "Connection_v1.h"
#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

enum class ConnectionV1AccessCapability
{
    Read,
    Write,
};
enum class ConnectionV1AccessState
{
    Optimized,
    NonOptimized,
    Standby,
    Unavailable,
    Transitioning,
};
enum class ConnectionV1ConnectionType
{
    Storage,
    Memory,
};
struct ConnectionV1OemActions
{};
struct ConnectionV1Actions
{
    ConnectionV1OemActions oem;
};
struct ConnectionV1VolumeInfo
{
    ConnectionV1AccessCapability accessCapabilities;
    ConnectionV1AccessState accessState;
    NavigationReferenceRedfish volume;
};
struct ConnectionV1Links
{
    ResourceV1Resource oem;
    NavigationReferenceRedfish initiatorEndpoints;
    NavigationReferenceRedfish targetEndpoints;
    NavigationReferenceRedfish initiatorEndpointGroups;
    NavigationReferenceRedfish targetEndpointGroups;
};
struct ConnectionV1MemoryChunkInfo
{
    ConnectionV1AccessCapability accessCapabilities;
    ConnectionV1AccessState accessState;
    NavigationReferenceRedfish memoryChunk;
};
struct ConnectionV1GenZConnectionKey
{
    bool rKeyDomainCheckingEnabled;
    std::string accessKey;
    std::string rKeyReadOnlyKey;
    std::string rKeyReadWriteKey;
};
struct ConnectionV1ConnectionKey
{
    ConnectionV1GenZConnectionKey genZ;
};
struct ConnectionV1Connection
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ResourceV1Resource status;
    ConnectionV1ConnectionType connectionType;
    ConnectionV1VolumeInfo volumeInfo;
    ConnectionV1Links links;
    ConnectionV1Actions actions;
    ConnectionV1MemoryChunkInfo memoryChunkInfo;
    ConnectionV1ConnectionKey connectionKeys;
};
#endif
