#ifndef PORT_V1
#define PORT_V1

#include "NavigationReference__.h"
#include "Port_v1.h"
#include "Protocol_v1.h"
#include "Resource_v1.h"

enum class Port_v1_FlowControl {
    None,
    TX,
    RX,
    TX_RX,
};
enum class Port_v1_LinkNetworkTechnology {
    Ethernet,
    InfiniBand,
    FibreChannel,
    GenZ,
};
enum class Port_v1_LinkState {
    Enabled,
    Disabled,
};
enum class Port_v1_LinkStatus {
    LinkUp,
    Starting,
    Training,
    LinkDown,
    NoLink,
};
enum class Port_v1_PortConnectionType {
    NotConnected,
    NPort,
    PointToPoint,
    PrivateLoop,
    PublicLoop,
    Generic,
    ExtenderFabric,
};
enum class Port_v1_PortMedium {
    Electrical,
    Optical,
};
enum class Port_v1_PortType {
    UpstreamPort,
    DownstreamPort,
    InterswitchPort,
    ManagementPort,
    BidirectionalPort,
    UnconfiguredPort,
};
enum class Port_v1_SupportedEthernetCapabilities {
    WakeOnLAN,
    EEE,
};
struct Port_v1_Actions
{
    Port_v1_OemActions oem;
};
struct Port_v1_ConfiguredNetworkLink
{
    double configuredLinkSpeedGbps;
    int64_t configuredWidth;
};
struct Port_v1_EthernetProperties
{
    Port_v1_SupportedEthernetCapabilities supportedEthernetCapabilities;
    Port_v1_FlowControl flowControlStatus;
    Port_v1_FlowControl flowControlConfiguration;
};
struct Port_v1_FibreChannelProperties
{
    Port_v1_PortConnectionType portConnectionType;
    int64_t numberDiscoveredRemotePorts;
    std::string fabricName;
};
struct Port_v1_GenZ
{
    NavigationReference__ LPRT;
    NavigationReference__ MPRT;
    NavigationReference__ VCAT;
};
struct Port_v1_LinkConfiguration
{
    double capableLinkSpeedGbps;
    Port_v1_ConfiguredNetworkLink configuredNetworkLinks;
    bool autoSpeedNegotiationCapable;
    bool autoSpeedNegotiationEnabled;
};
struct Port_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference__ associatedEndpoints;
    NavigationReference__ connectedSwitches;
    NavigationReference__ connectedSwitchPorts;
    NavigationReference__ connectedPorts;
};
struct Port_v1_OemActions
{
};
struct Port_v1_Port
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Resource_v1_Resource status;
    std::string portId;
    Protocol_v1_Protocol portProtocol;
    Port_v1_PortType portType;
    double currentSpeedGbps;
    double maxSpeedGbps;
    int64_t width;
    Port_v1_Links links;
    Port_v1_Actions actions;
    Resource_v1_Resource location;
    Port_v1_PortMedium portMedium;
    Port_v1_LinkNetworkTechnology linkNetworkTechnology;
    bool interfaceEnabled;
    bool signalDetected;
    int64_t linkTransitionIndicator;
    int64_t activeWidth;
    Port_v1_LinkState linkState;
    Port_v1_LinkStatus linkStatus;
    Port_v1_GenZ genZ;
    NavigationReference__ metrics;
    bool locationIndicatorActive;
    int64_t maxFrameSize;
    Port_v1_LinkConfiguration linkConfiguration;
    Port_v1_FibreChannelProperties fibreChannel;
    Port_v1_EthernetProperties ethernet;
};
#endif
