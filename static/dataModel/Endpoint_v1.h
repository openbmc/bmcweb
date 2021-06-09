#ifndef ENDPOINT_V1
#define ENDPOINT_V1

#include "Endpoint_v1.h"
#include "IPAddresses_v1.h"
#include "NavigationReference__.h"
#include "Protocol_v1.h"
#include "Redundancy_v1.h"
#include "Resource_v1.h"

enum class Endpoint_v1_EntityRole
{
    Initiator,
    Target,
    Both,
};
enum class Endpoint_v1_EntityType
{
    StorageInitiator,
    RootComplex,
    NetworkController,
    Drive,
    StorageExpander,
    DisplayController,
    Bridge,
    Processor,
    Volume,
    AccelerationFunction,
    MediaController,
    MemoryChunk,
    Switch,
    FabricBridge,
    Manager,
};
struct Endpoint_v1_Actions
{
    Endpoint_v1_OemActions oem;
};
struct Endpoint_v1_ConnectedEntity
{
    Endpoint_v1_EntityType entityType;
    Endpoint_v1_EntityRole entityRole;
    Endpoint_v1_PciId entityPciId;
    int64_t pciFunctionNumber;
    std::string pciClassCode;
    Resource_v1_Resource identifiers;
    NavigationReference__ entityLink;
    Resource_v1_Resource oem;
    Endpoint_v1_GenZ genZ;
};
struct Endpoint_v1_Endpoint
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Resource_v1_Resource status;
    Protocol_v1_Protocol endpointProtocol;
    Endpoint_v1_ConnectedEntity connectedEntities;
    Resource_v1_Resource identifiers;
    Endpoint_v1_PciId pciId;
    Redundancy_v1_Redundancy redundancy;
    int64_t hostReservationMemoryBytes;
    Endpoint_v1_Links links;
    Endpoint_v1_Actions actions;
    Endpoint_v1_IPTransportDetails iPTransportDetails;
};
struct Endpoint_v1_GCID
{
    std::string CID;
    std::string SID;
};
struct Endpoint_v1_GenZ
{
    Endpoint_v1_GCID GCID;
    std::string accessKey;
    std::string regionKey;
};
struct Endpoint_v1_IPTransportDetails
{
    Protocol_v1_Protocol transportProtocol;
    IPAddresses_v1_IPAddresses iPv4Address;
    IPAddresses_v1_IPAddresses iPv6Address;
    double port;
};
struct Endpoint_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference__ mutuallyExclusiveEndpoints;
    NavigationReference__ ports;
    NavigationReference__ networkDeviceFunction;
    NavigationReference__ connectedPorts;
    NavigationReference__ addressPools;
    NavigationReference__ connections;
};
struct Endpoint_v1_OemActions
{};
struct Endpoint_v1_PciId
{
    std::string deviceId;
    std::string vendorId;
    std::string subsystemId;
    std::string subsystemVendorId;
    int64_t functionNumber;
    std::string classCode;
};
#endif
