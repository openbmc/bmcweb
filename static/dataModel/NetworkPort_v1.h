#ifndef NETWORKPORT_V1
#define NETWORKPORT_V1

#include "NavigationReference_.h"
#include "NetworkPort_v1.h"
#include "Resource_v1.h"

enum class NetworkPortV1FlowControl
{
    None,
    TX,
    RX,
    TX_RX,
};
enum class NetworkPortV1LinkNetworkTechnology
{
    Ethernet,
    InfiniBand,
    FibreChannel,
};
enum class NetworkPortV1LinkStatus
{
    Down,
    Up,
    Starting,
    Training,
};
enum class NetworkPortV1PortConnectionType
{
    NotConnected,
    NPort,
    PointToPoint,
    PrivateLoop,
    PublicLoop,
    Generic,
    ExtenderFabric,
};
enum class NetworkPortV1SupportedEthernetCapabilities
{
    WakeOnLAN,
    EEE,
};
struct NetworkPortV1OemActions
{};
struct NetworkPortV1Actions
{
    NetworkPortV1OemActions oem;
};
struct NetworkPortV1NetDevFuncMaxBWAlloc
{
    NavigationReference_ networkDeviceFunction;
    int64_t maxBWAllocPercent;
};
struct NetworkPortV1NetDevFuncMinBWAlloc
{
    NavigationReference_ networkDeviceFunction;
    int64_t minBWAllocPercent;
};
struct NetworkPortV1SupportedLinkCapabilities
{
    NetworkPortV1LinkNetworkTechnology linkNetworkTechnology;
    int64_t linkSpeedMbps;
    int64_t capableLinkSpeedMbps;
    bool autoSpeedNegotiation;
};
struct NetworkPortV1NetworkPort
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ResourceV1Resource status;
    std::string physicalPortNumber;
    NetworkPortV1LinkStatus linkStatus;
    NetworkPortV1SupportedLinkCapabilities supportedLinkCapabilities;
    NetworkPortV1LinkNetworkTechnology activeLinkTechnology;
    NetworkPortV1SupportedEthernetCapabilities supportedEthernetCapabilities;
    NetworkPortV1NetDevFuncMinBWAlloc netDevFuncMinBWAlloc;
    NetworkPortV1NetDevFuncMaxBWAlloc netDevFuncMaxBWAlloc;
    std::string associatedNetworkAddresses;
    bool eEEEnabled;
    bool wakeOnLANEnabled;
    int64_t portMaximumMTU;
    NetworkPortV1FlowControl flowControlStatus;
    NetworkPortV1FlowControl flowControlConfiguration;
    bool signalDetected;
    NetworkPortV1Actions actions;
    NetworkPortV1PortConnectionType fCPortConnectionType;
    int64_t numberDiscoveredRemotePorts;
    int64_t maxFrameSize;
    std::string vendorId;
    std::string fCFabricName;
    int64_t currentLinkSpeedMbps;
};
#endif
