#ifndef PORT_V1
#define PORT_V1

#include "NavigationReference_.h"
#include "Port_v1.h"
#include "Protocol_v1.h"
#include "Resource_v1.h"

enum class PortV1FiberConnectionType
{
    SingleMode,
    MultiMode,
};
enum class PortV1FlowControl
{
    None,
    TX,
    RX,
    TX_RX,
};
enum class PortV1IEEE802IdSubtype
{
    ChassisComp,
    IfAlias,
    PortComp,
    MacAddr,
    NetworkAddr,
    IfName,
    AgentId,
    LocalAssign,
    NotTransmitted,
};
enum class PortV1LinkNetworkTechnology
{
    Ethernet,
    InfiniBand,
    FibreChannel,
    GenZ,
};
enum class PortV1LinkState
{
    Enabled,
    Disabled,
};
enum class PortV1LinkStatus
{
    LinkUp,
    Starting,
    Training,
    LinkDown,
    NoLink,
};
enum class PortV1MediumType
{
    Copper,
    FiberOptic,
};
enum class PortV1PortConnectionType
{
    NotConnected,
    NPort,
    PointToPoint,
    PrivateLoop,
    PublicLoop,
    Generic,
    ExtenderFabric,
};
enum class PortV1PortMedium
{
    Electrical,
    Optical,
};
enum class PortV1PortType
{
    UpstreamPort,
    DownstreamPort,
    InterswitchPort,
    ManagementPort,
    BidirectionalPort,
    UnconfiguredPort,
};
enum class PortV1SFPType
{
    SFP,
    SFPPlus,
    SFP28,
    cSFP,
    SFPDD,
    QSFP,
    QSFPPlus,
    QSFP14,
    QSFP28,
    QSFP56,
    MiniSASHD,
};
enum class PortV1SupportedEthernetCapabilities
{
    WakeOnLAN,
    EEE,
};
struct PortV1OemActions
{};
struct PortV1Actions
{
    PortV1OemActions oem;
};
struct PortV1ConfiguredNetworkLink
{
    double configuredLinkSpeedGbps;
    int64_t configuredWidth;
};
struct PortV1LLDPTransmit
{
    std::string chassisId;
    PortV1IEEE802IdSubtype chassisIdSubtype;
    int64_t managementVlanId;
    std::string managementAddressIPv4;
    std::string managementAddressIPv6;
    std::string managementAddressMAC;
    std::string portId;
    PortV1IEEE802IdSubtype portIdSubtype;
};
struct PortV1LLDPReceive
{
    std::string chassisId;
    PortV1IEEE802IdSubtype chassisIdSubtype;
    int64_t managementVlanId;
    std::string managementAddressIPv4;
    std::string managementAddressIPv6;
    std::string managementAddressMAC;
    std::string portId;
    PortV1IEEE802IdSubtype portIdSubtype;
};
struct PortV1EthernetProperties
{
    PortV1SupportedEthernetCapabilities supportedEthernetCapabilities;
    PortV1FlowControl flowControlStatus;
    PortV1FlowControl flowControlConfiguration;
    std::string associatedMACAddresses;
    bool lLDPEnabled;
    PortV1LLDPTransmit lLDPTransmit;
    PortV1LLDPReceive lLDPReceive;
};
struct PortV1FibreChannelProperties
{
    PortV1PortConnectionType portConnectionType;
    int64_t numberDiscoveredRemotePorts;
    std::string fabricName;
    std::string associatedWorldWideNames;
};
struct PortV1FunctionMaxBandwidth
{
    NavigationReference_ networkDeviceFunction;
    int64_t allocationPercent;
};
struct PortV1FunctionMinBandwidth
{
    NavigationReference_ networkDeviceFunction;
    int64_t allocationPercent;
};
struct PortV1GenZ
{
    NavigationReference_ LPRT;
    NavigationReference_ MPRT;
    NavigationReference_ VCAT;
};
struct PortV1LinkConfiguration
{
    double capableLinkSpeedGbps;
    PortV1ConfiguredNetworkLink configuredNetworkLinks;
    bool autoSpeedNegotiationCapable;
    bool autoSpeedNegotiationEnabled;
};
struct PortV1Links
{
    ResourceV1Resource oem;
    NavigationReference_ associatedEndpoints;
    NavigationReference_ connectedSwitches;
    NavigationReference_ connectedSwitchPorts;
    NavigationReference_ connectedPorts;
};
struct PortV1SFP
{
    ResourceV1Resource status;
    PortV1SFPType supportedSFPTypes;
    PortV1SFPType type;
    std::string manufacturer;
    std::string serialNumber;
    std::string partNumber;
    PortV1MediumType mediumType;
    PortV1FiberConnectionType fiberConnectionType;
};
struct PortV1Port
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ResourceV1Resource status;
    std::string portId;
    ProtocolV1Protocol portProtocol;
    PortV1PortType portType;
    double currentSpeedGbps;
    double maxSpeedGbps;
    int64_t width;
    PortV1Links links;
    PortV1Actions actions;
    ResourceV1Resource location;
    PortV1PortMedium portMedium;
    PortV1LinkNetworkTechnology linkNetworkTechnology;
    bool interfaceEnabled;
    bool signalDetected;
    int64_t linkTransitionIndicator;
    int64_t activeWidth;
    PortV1LinkState linkState;
    PortV1LinkStatus linkStatus;
    PortV1GenZ genZ;
    NavigationReference_ metrics;
    bool locationIndicatorActive;
    int64_t maxFrameSize;
    PortV1LinkConfiguration linkConfiguration;
    PortV1FibreChannelProperties fibreChannel;
    PortV1EthernetProperties ethernet;
    PortV1FunctionMinBandwidth functionMinBandwidth;
    PortV1FunctionMaxBandwidth functionMaxBandwidth;
    PortV1SFP SFP;
    NavigationReference_ environmentMetrics;
    std::string currentProtocolVersion;
    std::string capableProtocolVersions;
    bool enabled;
};
#endif
