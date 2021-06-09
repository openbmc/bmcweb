#ifndef MANAGER_V1
#define MANAGER_V1

#include "AccountService_v1.h"
#include "EthernetInterfaceCollection_v1.h"
#include "LogServiceCollection_v1.h"
#include "ManagerNetworkProtocol_v1.h"
#include "Manager_v1.h"
#include "NavigationReference_.h"
#include "Redundancy_v1.h"
#include "Resource_v1.h"
#include "SerialInterfaceCollection_v1.h"
#include "VirtualMediaCollection_v1.h"

#include <chrono>

enum class ManagerV1CommandConnectTypesSupported
{
    SSH,
    Telnet,
    IPMI,
    Oem,
};
enum class ManagerV1GraphicalConnectTypesSupported
{
    KVMIP,
    Oem,
};
enum class ManagerV1ManagerType
{
    ManagementController,
    EnclosureManager,
    BMC,
    RackManager,
    AuxiliaryController,
    Service,
};
enum class ManagerV1ResetToDefaultsType
{
    ResetAll,
    PreserveNetworkAndUsers,
    PreserveNetwork,
};
enum class ManagerV1SerialConnectTypesSupported
{
    SSH,
    Telnet,
    IPMI,
    Oem,
};
struct ManagerV1OemActions
{};
struct ManagerV1Actions
{
    ManagerV1OemActions oem;
};
struct ManagerV1CommandShell
{
    bool serviceEnabled;
    int64_t maxConcurrentSessions;
    ManagerV1CommandConnectTypesSupported connectTypesSupported;
};
struct ManagerV1GraphicalConsole
{
    bool serviceEnabled;
    int64_t maxConcurrentSessions;
    ManagerV1GraphicalConnectTypesSupported connectTypesSupported;
};
struct ManagerV1Links
{
    ResourceV1Resource oem;
    NavigationReference_ managerForServers;
    NavigationReference_ managerForChassis;
    NavigationReference_ managerInChassis;
    NavigationReference_ managerForSwitches;
    NavigationReference_ activeSoftwareImage;
    NavigationReference_ softwareImages;
    NavigationReference_ managedBy;
    NavigationReference_ managerForManagers;
};
struct ManagerV1SerialConsole
{
    bool serviceEnabled;
    int64_t maxConcurrentSessions;
    ManagerV1SerialConnectTypesSupported connectTypesSupported;
};
struct ManagerV1Manager
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ManagerV1ManagerType managerType;
    ManagerV1Links links;
    EthernetInterfaceCollectionV1EthernetInterfaceCollection ethernetInterfaces;
    SerialInterfaceCollectionV1SerialInterfaceCollection serialInterfaces;
    ManagerNetworkProtocolV1ManagerNetworkProtocol networkProtocol;
    LogServiceCollectionV1LogServiceCollection logServices;
    VirtualMediaCollectionV1VirtualMediaCollection virtualMedia;
    std::string serviceEntryPointUUID;
    std::string UUID;
    std::string model;
    std::chrono::time_point<std::chrono::system_clock> dateTime;
    std::string dateTimeLocalOffset;
    std::string firmwareVersion;
    ManagerV1SerialConsole serialConsole;
    ManagerV1CommandShell commandShell;
    ManagerV1GraphicalConsole graphicalConsole;
    ManagerV1Actions actions;
    ResourceV1Resource status;
    RedundancyV1Redundancy redundancy;
    ResourceV1Resource powerState;
    NavigationReference_ hostInterfaces;
    bool autoDSTEnabled;
    std::string remoteRedfishServiceUri;
    AccountServiceV1AccountService remoteAccountService;
    std::string manufacturer;
    std::string serialNumber;
    std::string partNumber;
    std::chrono::time_point<std::chrono::system_clock> lastResetTime;
    std::string timeZoneName;
    ResourceV1Resource location;
    bool locationIndicatorActive;
    std::string sparePartNumber;
    NavigationReference_ uSBPorts;
};
struct ManagerV1ManagerService
{
    bool serviceEnabled;
    int64_t maxConcurrentSessions;
};
#endif
