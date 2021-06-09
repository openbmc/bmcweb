#ifndef NETWORKDEVICEFUNCTION_V1
#define NETWORKDEVICEFUNCTION_V1

#include "NavigationReferenceRedfish.h"
#include "NetworkDeviceFunction_v1.h"
#include "Resource_v1.h"
#include "VLanNetworkInterfaceCollection_v1.h"
#include "VLanNetworkInterface_v1.h"

enum class NetworkDeviceFunctionV1AuthenticationMethod
{
    None,
    CHAP,
    MutualCHAP,
};
enum class NetworkDeviceFunctionV1BootMode
{
    Disabled,
    PXE,
    iSCSI,
    FibreChannel,
    FibreChannelOverEthernet,
};
enum class NetworkDeviceFunctionV1IPAddressType
{
    IPv4,
    IPv6,
};
enum class NetworkDeviceFunctionV1NetworkDeviceTechnology
{
    Disabled,
    Ethernet,
    FibreChannel,
    iSCSI,
    FibreChannelOverEthernet,
    InfiniBand,
};
enum class NetworkDeviceFunctionV1WWNSource
{
    ConfiguredLocally,
    ProvidedByFabric,
};
struct NetworkDeviceFunctionV1OemActions
{};
struct NetworkDeviceFunctionV1Actions
{
    NetworkDeviceFunctionV1OemActions oem;
};
struct NetworkDeviceFunctionV1BootTargets
{
    std::string WWPN;
    std::string LUNID;
    int64_t bootPriority;
};
struct NetworkDeviceFunctionV1Ethernet
{
    std::string permanentMACAddress;
    std::string mACAddress;
    int64_t mTUSize;
    VLanNetworkInterfaceV1VLanNetworkInterface VLAN;
    VLanNetworkInterfaceCollectionV1VLanNetworkInterfaceCollection vLANs;
    int64_t mTUSizeMaximum;
};
struct NetworkDeviceFunctionV1FibreChannel
{
    std::string permanentWWPN;
    std::string permanentWWNN;
    std::string WWPN;
    std::string WWNN;
    NetworkDeviceFunctionV1WWNSource wWNSource;
    int64_t fCoELocalVLANId;
    bool allowFIPVLANDiscovery;
    int64_t fCoEActiveVLANId;
    NetworkDeviceFunctionV1BootTargets bootTargets;
    std::string fibreChannelId;
};
struct NetworkDeviceFunctionV1InfiniBand
{
    std::string permanentPortGUID;
    std::string permanentNodeGUID;
    std::string permanentSystemGUID;
    std::string portGUID;
    std::string nodeGUID;
    std::string systemGUID;
    int64_t supportedMTUSizes;
    int64_t mTUSize;
};
struct NetworkDeviceFunctionV1ISCSIBoot
{
    NetworkDeviceFunctionV1IPAddressType iPAddressType;
    std::string initiatorIPAddress;
    std::string initiatorName;
    std::string initiatorDefaultGateway;
    std::string initiatorNetmask;
    bool targetInfoViaDHCP;
    std::string primaryTargetName;
    std::string primaryTargetIPAddress;
    int64_t primaryTargetTCPPort;
    int64_t primaryLUN;
    bool primaryVLANEnable;
    int64_t primaryVLANId;
    std::string primaryDNS;
    std::string secondaryTargetName;
    std::string secondaryTargetIPAddress;
    int64_t secondaryTargetTCPPort;
    int64_t secondaryLUN;
    bool secondaryVLANEnable;
    int64_t secondaryVLANId;
    std::string secondaryDNS;
    bool iPMaskDNSViaDHCP;
    bool routerAdvertisementEnabled;
    NetworkDeviceFunctionV1AuthenticationMethod authenticationMethod;
    std::string cHAPUsername;
    std::string cHAPSecret;
    std::string mutualCHAPUsername;
    std::string mutualCHAPSecret;
};
struct NetworkDeviceFunctionV1Links
{
    ResourceV1Resource oem;
    NavigationReferenceRedfish pCIeFunction;
    NavigationReferenceRedfish endpoints;
    NavigationReferenceRedfish physicalPortAssignment;
    NavigationReferenceRedfish physicalNetworkPortAssignment;
};
struct NetworkDeviceFunctionV1NetworkDeviceFunction
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ResourceV1Resource status;
    NetworkDeviceFunctionV1NetworkDeviceTechnology netDevFuncType;
    bool deviceEnabled;
    NetworkDeviceFunctionV1NetworkDeviceTechnology netDevFuncCapabilities;
    NetworkDeviceFunctionV1Ethernet ethernet;
    NetworkDeviceFunctionV1ISCSIBoot iSCSIBoot;
    NetworkDeviceFunctionV1FibreChannel fibreChannel;
    NavigationReferenceRedfish assignablePhysicalPorts;
    NavigationReferenceRedfish physicalPortAssignment;
    NetworkDeviceFunctionV1BootMode bootMode;
    bool virtualFunctionsEnabled;
    int64_t maxVirtualFunctions;
    NetworkDeviceFunctionV1Links links;
    NetworkDeviceFunctionV1Actions actions;
    NavigationReferenceRedfish assignablePhysicalNetworkPorts;
    NavigationReferenceRedfish physicalNetworkPortAssignment;
    NetworkDeviceFunctionV1InfiniBand infiniBand;
    NavigationReferenceRedfish metrics;
};
#endif
