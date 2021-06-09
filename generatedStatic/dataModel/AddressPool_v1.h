#ifndef ADDRESSPOOL_V1
#define ADDRESSPOOL_V1

#include "AddressPool_v1.h"
#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

struct AddressPoolV1OemActions
{};
struct AddressPoolV1Actions
{
    AddressPoolV1OemActions oem;
};
struct AddressPoolV1GenZ
{
    int64_t minCID;
    int64_t maxCID;
    int64_t minSID;
    int64_t maxSID;
    std::string accessKey;
};
struct AddressPoolV1Links
{
    ResourceV1Resource oem;
    NavigationReferenceRedfish endpoints;
    NavigationReferenceRedfish zones;
};
struct AddressPoolV1VLANIdentifierAddressRange
{
    int64_t lower;
    int64_t upper;
};
struct AddressPoolV1IPv4AddressRange
{
    std::string lower;
    std::string upper;
};
struct AddressPoolV1DHCP
{
    bool dHCPRelayEnabled;
    int64_t dHCPInterfaceMTUBytes;
    std::string dHCPServer;
};
struct AddressPoolV1IPv4
{
    AddressPoolV1VLANIdentifierAddressRange vLANIdentifierAddressRange;
    AddressPoolV1IPv4AddressRange hostAddressRange;
    AddressPoolV1IPv4AddressRange loopbackAddressRange;
    AddressPoolV1IPv4AddressRange fabricLinkAddressRange;
    AddressPoolV1IPv4AddressRange managementAddressRange;
    AddressPoolV1IPv4AddressRange iBGPAddressRange;
    AddressPoolV1IPv4AddressRange eBGPAddressRange;
    std::string dNSServer;
    std::string nTPServer;
    AddressPoolV1DHCP DHCP;
    int64_t nativeVLAN;
    std::string dNSDomainName;
    bool distributeIntoUnderlayEnabled;
    std::string nTPTimezone;
    int64_t nTPOffsetHoursMinutes;
    std::string gatewayIPAddress;
    std::string anycastGatewayIPAddress;
    std::string anycastGatewayMACAddress;
};
struct AddressPoolV1ESINumberRange
{
    int64_t lower;
    int64_t upper;
};
struct AddressPoolV1EVINumberRange
{
    int64_t lower;
    int64_t upper;
};
struct AddressPoolV1RouteDistinguisherRange
{
    int64_t lower;
    int64_t upper;
};
struct AddressPoolV1RouteTargetRange
{
    int64_t lower;
    int64_t upper;
};
struct AddressPoolV1BGPEvpn
{
    AddressPoolV1VLANIdentifierAddressRange vLANIdentifierAddressRange;
    AddressPoolV1ESINumberRange eSINumberRange;
    AddressPoolV1EVINumberRange eVINumberRange;
    AddressPoolV1RouteDistinguisherRange routeDistinguisherRange;
    AddressPoolV1RouteTargetRange routeTargetRange;
    std::string gatewayIPAddress;
    std::string anycastGatewayIPAddress;
    std::string anycastGatewayMACAddress;
    bool aRPProxyEnabled;
    bool aRPSupressionEnabled;
    bool nDPSupressionEnabled;
    bool nDPProxyEnabled;
    bool underlayMulticastEnabled;
    bool unknownUnicastSuppressionEnabled;
};
struct AddressPoolV1ASNumberRange
{
    int64_t lower;
    int64_t upper;
};
struct AddressPoolV1MaxPrefix
{
    int64_t maxPrefixNumber;
    bool thresholdWarningOnlyEnabled;
    double shutdownThresholdPercentage;
    int64_t restartTimerSeconds;
};
struct AddressPoolV1BGPNeighbor
{
    std::string address;
    bool allowOwnASEnabled;
    int64_t connectRetrySeconds;
    int64_t holdTimeSeconds;
    int64_t keepaliveIntervalSeconds;
    int64_t minimumAdvertisementIntervalSeconds;
    int64_t tCPMaxSegmentSizeBytes;
    bool pathMTUDiscoveryEnabled;
    bool passiveModeEnabled;
    bool treatAsWithdrawEnabled;
    bool replacePeerASEnabled;
    int64_t peerAS;
    int64_t localAS;
    bool logStateChangesEnabled;
    AddressPoolV1MaxPrefix maxPrefix;
};
struct AddressPoolV1GracefulRestart
{
    bool gracefulRestartEnabled;
    int64_t timeSeconds;
    int64_t staleRoutesTimeSeconds;
    bool helperModeEnabled;
};
struct AddressPoolV1MultiplePaths
{
    bool useMultiplePathsEnabled;
    int64_t maximumPaths;
};
struct AddressPoolV1BGPRoute
{
    bool flapDampingEnabled;
    bool externalCompareRouterIdEnabled;
    bool advertiseInactiveRoutesEnabled;
    bool sendDefaultRouteEnabled;
    int64_t distanceExternal;
    int64_t distanceInternal;
    int64_t distanceLocal;
};
struct AddressPoolV1EBGP
{
    AddressPoolV1ASNumberRange aSNumberRange;
    AddressPoolV1BGPNeighbor bGPNeighbor;
    AddressPoolV1GracefulRestart gracefulRestart;
    AddressPoolV1MultiplePaths multiplePaths;
    AddressPoolV1BGPRoute bGPRoute;
    bool sendCommunityEnabled;
    bool alwaysCompareMEDEnabled;
    int64_t MED;
    int64_t bGPWeight;
    int64_t bGPLocalPreference;
    bool allowDuplicateASEnabled;
    bool allowOverrideASEnabled;
    bool multihopEnabled;
    int64_t multihopTTL;
};
struct AddressPoolV1CommonBGPProperties
{
    AddressPoolV1ASNumberRange aSNumberRange;
    AddressPoolV1BGPNeighbor bGPNeighbor;
    AddressPoolV1GracefulRestart gracefulRestart;
    AddressPoolV1MultiplePaths multiplePaths;
    AddressPoolV1BGPRoute bGPRoute;
    bool sendCommunityEnabled;
};
struct AddressPoolV1BFDSingleHopOnly
{
    int64_t localMultiplier;
    int64_t desiredMinTxIntervalMilliseconds;
    int64_t requiredMinRxIntervalMilliseconds;
    bool demandModeEnabled;
    std::string keyChain;
    bool meticulousModeEnabled;
    int64_t sourcePort;
};
struct AddressPoolV1Ethernet
{
    AddressPoolV1IPv4 iPv4;
    AddressPoolV1BGPEvpn bGPEvpn;
    AddressPoolV1EBGP EBGP;
    AddressPoolV1CommonBGPProperties multiProtocolIBGP;
    AddressPoolV1EBGP multiProtocolEBGP;
    AddressPoolV1BFDSingleHopOnly bFDSingleHopOnly;
};
struct AddressPoolV1AddressPool
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ResourceV1Resource status;
    AddressPoolV1GenZ genZ;
    AddressPoolV1Links links;
    AddressPoolV1Actions actions;
    AddressPoolV1Ethernet ethernet;
};
#endif
