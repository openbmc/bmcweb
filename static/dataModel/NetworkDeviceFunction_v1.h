#ifndef NETWORKDEVICEFUNCTION_V1
#define NETWORKDEVICEFUNCTION_V1

#include "NavigationReference__.h"
#include "NetworkDeviceFunction_v1.h"
#include "Resource_v1.h"
#include "VLanNetworkInterfaceCollection_v1.h"
#include "VLanNetworkInterface_v1.h"

enum class NetworkDeviceFunction_v1_AuthenticationMethod
{
    None,
    CHAP,
    MutualCHAP,
};
enum class NetworkDeviceFunction_v1_BootMode
{
    Disabled,
    PXE,
    iSCSI,
    FibreChannel,
    FibreChannelOverEthernet,
};
enum class NetworkDeviceFunction_v1_IPAddressType
{
    IPv4,
    IPv6,
};
enum class NetworkDeviceFunction_v1_NetworkDeviceTechnology
{
    Disabled,
    Ethernet,
    FibreChannel,
    iSCSI,
    FibreChannelOverEthernet,
    InfiniBand,
};
enum class NetworkDeviceFunction_v1_WWNSource
{
    ConfiguredLocally,
    ProvidedByFabric,
};
struct NetworkDeviceFunction_v1_Actions
{
    NetworkDeviceFunction_v1_OemActions oem;
};
struct NetworkDeviceFunction_v1_BootTargets
{
    std::string WWPN;
    std::string LUNID;
    int64_t bootPriority;
};
struct NetworkDeviceFunction_v1_Ethernet
{
    std::string permanentMACAddress;
    std::string mACAddress;
    int64_t mTUSize;
    VLanNetworkInterface_v1_VLanNetworkInterface VLAN;
    VLanNetworkInterfaceCollection_v1_VLanNetworkInterfaceCollection vLANs;
    int64_t mTUSizeMaximum;
};
struct NetworkDeviceFunction_v1_FibreChannel
{
    std::string permanentWWPN;
    std::string permanentWWNN;
    std::string WWPN;
    std::string WWNN;
    NetworkDeviceFunction_v1_WWNSource wWNSource;
    int64_t fCoELocalVLANId;
    bool allowFIPVLANDiscovery;
    int64_t fCoEActiveVLANId;
    NetworkDeviceFunction_v1_BootTargets bootTargets;
    std::string fibreChannelId;
};
struct NetworkDeviceFunction_v1_InfiniBand
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
struct NetworkDeviceFunction_v1_iSCSIBoot
{
    NetworkDeviceFunction_v1_IPAddressType iPAddressType;
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
    NetworkDeviceFunction_v1_AuthenticationMethod authenticationMethod;
    std::string cHAPUsername;
    std::string cHAPSecret;
    std::string mutualCHAPUsername;
    std::string mutualCHAPSecret;
};
struct NetworkDeviceFunction_v1_Links
{
    NavigationReference__ pCIeFunction;
    NavigationReference__ endpoints;
    NavigationReference__ physicalPortAssignment;
    NavigationReference__ physicalNetworkPortAssignment;
};
struct NetworkDeviceFunction_v1_NetworkDeviceFunction
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Resource_v1_Resource status;
    NetworkDeviceFunction_v1_NetworkDeviceTechnology netDevFuncType;
    bool deviceEnabled;
    NetworkDeviceFunction_v1_NetworkDeviceTechnology netDevFuncCapabilities;
    NetworkDeviceFunction_v1_Ethernet ethernet;
    NetworkDeviceFunction_v1_iSCSIBoot iSCSIBoot;
    NetworkDeviceFunction_v1_FibreChannel fibreChannel;
    NavigationReference__ assignablePhysicalPorts;
    NavigationReference__ physicalPortAssignment;
    NetworkDeviceFunction_v1_BootMode bootMode;
    bool virtualFunctionsEnabled;
    int64_t maxVirtualFunctions;
    NetworkDeviceFunction_v1_Links links;
    NetworkDeviceFunction_v1_Actions actions;
    NavigationReference__ assignablePhysicalNetworkPorts;
    NavigationReference__ physicalNetworkPortAssignment;
    NetworkDeviceFunction_v1_InfiniBand infiniBand;
};
struct NetworkDeviceFunction_v1_OemActions
{};
#endif
