#ifndef ADDRESSPOOL_V1
#define ADDRESSPOOL_V1

#include "AddressPool_v1.h"
#include "NavigationReference__.h"
#include "Resource_v1.h"

struct AddressPool_v1_Actions
{
    AddressPool_v1_OemActions oem;
};
struct AddressPool_v1_AddressPool
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Resource_v1_Resource status;
    AddressPool_v1_GenZ genZ;
    AddressPool_v1_Links links;
    AddressPool_v1_Actions actions;
    AddressPool_v1_Ethernet ethernet;
};
struct AddressPool_v1_ASNumberRange
{
    int64_t lower;
    int64_t upper;
};
struct AddressPool_v1_BFDSingleHopOnly
{
    int64_t localMultiplier;
    int64_t desiredMinTxIntervalMilliseconds;
    int64_t requiredMinRxIntervalMilliseconds;
    bool demandModeEnabled;
    std::string keyChain;
    bool meticulousModeEnabled;
    int64_t sourcePort;
};
struct AddressPool_v1_BGPEvpn
{
    AddressPool_v1_VLANIdentifierAddressRange vLANIdentifierAddressRange;
    AddressPool_v1_ESINumberRange eSINumberRange;
    AddressPool_v1_EVINumberRange eVINumberRange;
    AddressPool_v1_RouteDistinguisherRange routeDistinguisherRange;
    AddressPool_v1_RouteTargetRange routeTargetRange;
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
struct AddressPool_v1_BGPNeighbor
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
    AddressPool_v1_MaxPrefix maxPrefix;
};
struct AddressPool_v1_BGPRoute
{
    bool flapDampingEnabled;
    bool externalCompareRouterIdEnabled;
    bool advertiseInactiveRoutesEnabled;
    bool sendDefaultRouteEnabled;
    int64_t distanceExternal;
    int64_t distanceInternal;
    int64_t distanceLocal;
};
struct AddressPool_v1_CommonBGPProperties
{
    AddressPool_v1_ASNumberRange aSNumberRange;
    AddressPool_v1_BGPNeighbor bGPNeighbor;
    AddressPool_v1_GracefulRestart gracefulRestart;
    AddressPool_v1_MultiplePaths multiplePaths;
    AddressPool_v1_BGPRoute bGPRoute;
    bool sendCommunityEnabled;
};
struct AddressPool_v1_DHCP
{
    bool dHCPRelayEnabled;
    int64_t dHCPInterfaceMTUBytes;
    std::string dHCPServer;
};
struct AddressPool_v1_EBGP
{
    AddressPool_v1_ASNumberRange aSNumberRange;
    AddressPool_v1_BGPNeighbor bGPNeighbor;
    AddressPool_v1_GracefulRestart gracefulRestart;
    AddressPool_v1_MultiplePaths multiplePaths;
    AddressPool_v1_BGPRoute bGPRoute;
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
struct AddressPool_v1_ESINumberRange
{
    int64_t lower;
    int64_t upper;
};
struct AddressPool_v1_Ethernet
{
    AddressPool_v1_IPv4 iPv4;
    AddressPool_v1_BGPEvpn bGPEvpn;
    AddressPool_v1_EBGP EBGP;
    AddressPool_v1_CommonBGPProperties multiProtocolIBGP;
    AddressPool_v1_EBGP multiProtocolEBGP;
    AddressPool_v1_BFDSingleHopOnly bFDSingleHopOnly;
};
struct AddressPool_v1_EVINumberRange
{
    int64_t lower;
    int64_t upper;
};
struct AddressPool_v1_GenZ
{
    int64_t minCID;
    int64_t maxCID;
    int64_t minSID;
    int64_t maxSID;
    std::string accessKey;
};
struct AddressPool_v1_GracefulRestart
{
    bool gracefulRestartEnabled;
    int64_t timeSeconds;
    int64_t staleRoutesTimeSeconds;
    bool helperModeEnabled;
};
struct AddressPool_v1_IPv4
{
    AddressPool_v1_VLANIdentifierAddressRange vLANIdentifierAddressRange;
    AddressPool_v1_IPv4AddressRange hostAddressRange;
    AddressPool_v1_IPv4AddressRange loopbackAddressRange;
    AddressPool_v1_IPv4AddressRange fabricLinkAddressRange;
    AddressPool_v1_IPv4AddressRange managementAddressRange;
    AddressPool_v1_IPv4AddressRange iBGPAddressRange;
    AddressPool_v1_IPv4AddressRange eBGPAddressRange;
    std::string dNSServer;
    std::string nTPServer;
    AddressPool_v1_DHCP DHCP;
    int64_t nativeVLAN;
    std::string dNSDomainName;
    bool distributeIntoUnderlayEnabled;
    std::string nTPTimezone;
    int64_t nTPOffsetHoursMinutes;
    std::string gatewayIPAddress;
    std::string anycastGatewayIPAddress;
    std::string anycastGatewayMACAddress;
};
struct AddressPool_v1_IPv4AddressRange
{
    std::string lower;
    std::string upper;
};
struct AddressPool_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference__ endpoints;
    NavigationReference__ zones;
};
struct AddressPool_v1_MaxPrefix
{
    int64_t maxPrefixNumber;
    bool thresholdWarningOnlyEnabled;
    double shutdownThresholdPercentage;
    int64_t restartTimerSeconds;
};
struct AddressPool_v1_MultiplePaths
{
    bool useMultiplePathsEnabled;
    int64_t maximumPaths;
};
struct AddressPool_v1_OemActions
{};
struct AddressPool_v1_RouteDistinguisherRange
{
    int64_t lower;
    int64_t upper;
};
struct AddressPool_v1_RouteTargetRange
{
    int64_t lower;
    int64_t upper;
};
struct AddressPool_v1_VLANIdentifierAddressRange
{
    int64_t lower;
    int64_t upper;
};
#endif
