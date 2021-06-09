#ifndef MANAGER_V1
#define MANAGER_V1

#include "AccountService_v1.h"
#include "EthernetInterfaceCollection_v1.h"
#include "LogServiceCollection_v1.h"
#include "ManagerNetworkProtocol_v1.h"
#include "Manager_v1.h"
#include "NavigationReference__.h"
#include "Redundancy_v1.h"
#include "Resource_v1.h"
#include "SerialInterfaceCollection_v1.h"
#include "VirtualMediaCollection_v1.h"

#include <chrono>

enum class Manager_v1_CommandConnectTypesSupported
{
    SSH,
    Telnet,
    IPMI,
    Oem,
};
enum class Manager_v1_GraphicalConnectTypesSupported
{
    KVMIP,
    Oem,
};
enum class Manager_v1_ManagerType
{
    ManagementController,
    EnclosureManager,
    BMC,
    RackManager,
    AuxiliaryController,
    Service,
};
enum class Manager_v1_ResetToDefaultsType
{
    ResetAll,
    PreserveNetworkAndUsers,
    PreserveNetwork,
};
enum class Manager_v1_SerialConnectTypesSupported
{
    SSH,
    Telnet,
    IPMI,
    Oem,
};
struct Manager_v1_Actions
{
    Manager_v1_OemActions oem;
};
struct Manager_v1_CommandShell
{
    bool serviceEnabled;
    int64_t maxConcurrentSessions;
    Manager_v1_CommandConnectTypesSupported connectTypesSupported;
};
struct Manager_v1_GraphicalConsole
{
    bool serviceEnabled;
    int64_t maxConcurrentSessions;
    Manager_v1_GraphicalConnectTypesSupported connectTypesSupported;
};
struct Manager_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference__ managerForServers;
    NavigationReference__ managerForChassis;
    NavigationReference__ managerInChassis;
    NavigationReference__ activeSoftwareImage;
    NavigationReference__ softwareImages;
    NavigationReference__ managedBy;
    NavigationReference__ managerForManagers;
};
struct Manager_v1_Manager
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Manager_v1_ManagerType managerType;
    Manager_v1_Links links;
    EthernetInterfaceCollection_v1_EthernetInterfaceCollection
        ethernetInterfaces;
    SerialInterfaceCollection_v1_SerialInterfaceCollection serialInterfaces;
    ManagerNetworkProtocol_v1_ManagerNetworkProtocol networkProtocol;
    LogServiceCollection_v1_LogServiceCollection logServices;
    VirtualMediaCollection_v1_VirtualMediaCollection virtualMedia;
    string serviceEntryPointUUID;
    string UUID;
    std::string model;
    std::chrono::time_point dateTime;
    std::string dateTimeLocalOffset;
    std::string firmwareVersion;
    Manager_v1_SerialConsole serialConsole;
    Manager_v1_CommandShell commandShell;
    Manager_v1_GraphicalConsole graphicalConsole;
    Manager_v1_Actions actions;
    Resource_v1_Resource status;
    Redundancy_v1_Redundancy redundancy;
    Resource_v1_Resource powerState;
    NavigationReference__ hostInterfaces;
    bool autoDSTEnabled;
    std::string remoteRedfishServiceUri;
    AccountService_v1_AccountService remoteAccountService;
    std::string manufacturer;
    std::string serialNumber;
    std::string partNumber;
    std::chrono::time_point lastResetTime;
    std::string timeZoneName;
};
struct Manager_v1_ManagerService
{
    bool serviceEnabled;
    int64_t maxConcurrentSessions;
};
struct Manager_v1_OemActions
{};
struct Manager_v1_SerialConsole
{
    bool serviceEnabled;
    int64_t maxConcurrentSessions;
    Manager_v1_SerialConnectTypesSupported connectTypesSupported;
};
#endif
