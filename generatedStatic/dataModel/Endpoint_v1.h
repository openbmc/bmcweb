#ifndef ENDPOINT_V1
#define ENDPOINT_V1

#include "Endpoint_v1.h"
#include "IPAddresses_v1.h"
#include "NavigationReferenceRedfish.h"
#include "Protocol_v1.h"
#include "Redundancy_v1.h"
#include "Resource_v1.h"

enum class EndpointV1EntityRole
{
    Initiator,
    Target,
    Both,
};
enum class EndpointV1EntityType
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
    StorageSubsystem,
};
struct EndpointV1OemActions
{};
struct EndpointV1Actions
{
    EndpointV1OemActions oem;
};
struct EndpointV1PciId
{
    std::string deviceId;
    std::string vendorId;
    std::string subsystemId;
    std::string subsystemVendorId;
    int64_t functionNumber;
    std::string classCode;
};
struct EndpointV1GCID
{
    std::string CID;
    std::string SID;
};
struct EndpointV1GenZ
{
    EndpointV1GCID GCID;
    std::string accessKey;
    std::string regionKey;
};
struct EndpointV1ConnectedEntity
{
    EndpointV1EntityType entityType;
    EndpointV1EntityRole entityRole;
    EndpointV1PciId entityPciId;
    int64_t pciFunctionNumber;
    std::string pciClassCode;
    ResourceV1Resource identifiers;
    NavigationReferenceRedfish entityLink;
    ResourceV1Resource oem;
    EndpointV1GenZ genZ;
};
struct EndpointV1Links
{
    ResourceV1Resource oem;
    NavigationReferenceRedfish mutuallyExclusiveEndpoints;
    NavigationReferenceRedfish ports;
    NavigationReferenceRedfish networkDeviceFunction;
    NavigationReferenceRedfish connectedPorts;
    NavigationReferenceRedfish addressPools;
    NavigationReferenceRedfish connections;
    NavigationReferenceRedfish zones;
};
struct EndpointV1IPTransportDetails
{
    ProtocolV1Protocol transportProtocol;
    IPAddressesV1IPAddresses iPv4Address;
    IPAddressesV1IPAddresses iPv6Address;
    double port;
};
struct EndpointV1Endpoint
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ResourceV1Resource status;
    ProtocolV1Protocol endpointProtocol;
    EndpointV1ConnectedEntity connectedEntities;
    ResourceV1Resource identifiers;
    EndpointV1PciId pciId;
    RedundancyV1Redundancy redundancy;
    int64_t hostReservationMemoryBytes;
    EndpointV1Links links;
    EndpointV1Actions actions;
    EndpointV1IPTransportDetails iPTransportDetails;
};
#endif
