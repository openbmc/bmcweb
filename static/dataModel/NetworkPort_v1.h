#ifndef NETWORKPORT_V1
#define NETWORKPORT_V1

#include "NavigationReference__.h"
#include "NetworkPort_v1.h"
#include "Resource_v1.h"

enum class NetworkPort_v1_FlowControl {
    None,
    TX,
    RX,
    TX_RX,
};
enum class NetworkPort_v1_LinkNetworkTechnology {
    Ethernet,
    InfiniBand,
    FibreChannel,
};
enum class NetworkPort_v1_LinkStatus {
    Down,
    Up,
    Starting,
    Training,
};
enum class NetworkPort_v1_PortConnectionType {
    NotConnected,
    NPort,
    PointToPoint,
    PrivateLoop,
    PublicLoop,
    Generic,
    ExtenderFabric,
};
enum class NetworkPort_v1_SupportedEthernetCapabilities {
    WakeOnLAN,
    EEE,
};
struct NetworkPort_v1_Actions
{
    NetworkPort_v1_OemActions oem;
};
struct NetworkPort_v1_NetDevFuncMaxBWAlloc
{
    NavigationReference__ networkDeviceFunction;
    int64_t maxBWAllocPercent;
};
struct NetworkPort_v1_NetDevFuncMinBWAlloc
{
    NavigationReference__ networkDeviceFunction;
    int64_t minBWAllocPercent;
};
struct NetworkPort_v1_NetworkPort
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Resource_v1_Resource status;
    std::string physicalPortNumber;
    NetworkPort_v1_LinkStatus linkStatus;
    NetworkPort_v1_SupportedLinkCapabilities supportedLinkCapabilities;
    NetworkPort_v1_LinkNetworkTechnology activeLinkTechnology;
    NetworkPort_v1_SupportedEthernetCapabilities supportedEthernetCapabilities;
    NetworkPort_v1_NetDevFuncMinBWAlloc netDevFuncMinBWAlloc;
    NetworkPort_v1_NetDevFuncMaxBWAlloc netDevFuncMaxBWAlloc;
    std::string associatedNetworkAddresses;
    bool eEEEnabled;
    bool wakeOnLANEnabled;
    int64_t portMaximumMTU;
    NetworkPort_v1_FlowControl flowControlStatus;
    NetworkPort_v1_FlowControl flowControlConfiguration;
    bool signalDetected;
    NetworkPort_v1_Actions actions;
    NetworkPort_v1_PortConnectionType fCPortConnectionType;
    int64_t numberDiscoveredRemotePorts;
    int64_t maxFrameSize;
    std::string vendorId;
    std::string fCFabricName;
    int64_t currentLinkSpeedMbps;
};
struct NetworkPort_v1_OemActions
{
};
struct NetworkPort_v1_SupportedLinkCapabilities
{
    NetworkPort_v1_LinkNetworkTechnology linkNetworkTechnology;
    int64_t linkSpeedMbps;
    int64_t capableLinkSpeedMbps;
    bool autoSpeedNegotiation;
};
#endif
