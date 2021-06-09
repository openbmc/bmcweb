#ifndef COMPUTERSYSTEM_V1
#define COMPUTERSYSTEM_V1

#include <chrono>
#include "Bios_v1.h"
#include "BootOptionCollection_v1.h"
#include "CertificateCollection_v1.h"
#include "ComputerSystem_v1.h"
#include "EthernetInterfaceCollection_v1.h"
#include "FabricAdapterCollection_v1.h"
#include "HostedStorageServices_v1.h"
#include "LogServiceCollection_v1.h"
#include "MemoryCollection_v1.h"
#include "MemoryDomainCollection_v1.h"
#include "MemoryMetrics_v1.h"
#include "NavigationReference__.h"
#include "NetworkInterfaceCollection_v1.h"
#include "ProcessorCollection_v1.h"
#include "ProcessorMetrics_v1.h"
#include "Redundancy_v1.h"
#include "Resource_v1.h"
#include "SecureBoot_v1.h"
#include "SimpleStorageCollection_v1.h"
#include "StorageCollection_v1.h"
#include "VirtualMediaCollection_v1.h"

enum class ComputerSystem_v1_AutomaticRetryConfig {
    Disabled,
    RetryAttempts,
    RetryAlways,
};
enum class ComputerSystem_v1_BootOrderTypes {
    BootOrder,
    AliasBootOrder,
};
enum class ComputerSystem_v1_BootProgressTypes {
    None,
    PrimaryProcessorInitializationStarted,
    BusInitializationStarted,
    MemoryInitializationStarted,
    SecondaryProcessorInitializationStarted,
    PCIResourceConfigStarted,
    SystemHardwareInitializationComplete,
    OSBootStarted,
    OSRunning,
    OEM,
};
enum class ComputerSystem_v1_BootSource {
    None,
    Pxe,
    Floppy,
    Cd,
    Usb,
    Hdd,
    BiosSetup,
    Utilities,
    Diags,
    UefiShell,
    UefiTarget,
    SDCard,
    UefiHttp,
    RemoteDrive,
    UefiBootNext,
};
enum class ComputerSystem_v1_BootSourceOverrideEnabled {
    Disabled,
    Once,
    Continuous,
};
enum class ComputerSystem_v1_BootSourceOverrideMode {
    Legacy,
    UEFI,
};
enum class ComputerSystem_v1_GraphicalConnectTypesSupported {
    KVMIP,
    OEM,
};
enum class ComputerSystem_v1_HostingRole {
    ApplicationServer,
    StorageServer,
    Switch,
    Appliance,
    BareMetalServer,
    VirtualMachineServer,
    ContainerServer,
};
enum class ComputerSystem_v1_IndicatorLED {
    Unknown,
    Lit,
    Blinking,
    Off,
};
enum class ComputerSystem_v1_InterfaceType {
    TPM1_2,
    TPM2_0,
    TCM1_0,
};
enum class ComputerSystem_v1_InterfaceTypeSelection {
    None,
    FirmwareUpdate,
    BiosSetting,
    OemMethod,
};
enum class ComputerSystem_v1_MemoryMirroring {
    System,
    DIMM,
    Hybrid,
    None,
};
enum class ComputerSystem_v1_PowerRestorePolicyTypes {
    AlwaysOn,
    AlwaysOff,
    LastState,
};
enum class ComputerSystem_v1_PowerState {
    On,
    Off,
    PoweringOn,
    PoweringOff,
};
enum class ComputerSystem_v1_SystemType {
    Physical,
    Virtual,
    OS,
    PhysicallyPartitioned,
    VirtuallyPartitioned,
    Composed,
};
enum class ComputerSystem_v1_WatchdogTimeoutActions {
    None,
    ResetSystem,
    PowerCycle,
    PowerDown,
    OEM,
};
enum class ComputerSystem_v1_WatchdogWarningActions {
    None,
    DiagnosticInterrupt,
    SMI,
    MessagingInterrupt,
    SCI,
    OEM,
};
struct ComputerSystem_v1_Actions
{
    ComputerSystem_v1_OemActions oem;
};
struct ComputerSystem_v1_Boot
{
    ComputerSystem_v1_BootSource bootSourceOverrideTarget;
    ComputerSystem_v1_BootSourceOverrideEnabled bootSourceOverrideEnabled;
    std::string uefiTargetBootSourceOverride;
    ComputerSystem_v1_BootSourceOverrideMode bootSourceOverrideMode;
    BootOptionCollection_v1_BootOptionCollection bootOptions;
    std::string bootNext;
    std::string bootOrder;
    ComputerSystem_v1_BootSource aliasBootOrder;
    ComputerSystem_v1_BootOrderTypes bootOrderPropertySelection;
    CertificateCollection_v1_CertificateCollection certificates;
    std::string httpBootUri;
    ComputerSystem_v1_AutomaticRetryConfig automaticRetryConfig;
    int64_t automaticRetryAttempts;
    int64_t remainingAutomaticRetryAttempts;
};
struct ComputerSystem_v1_BootProgress
{
    ComputerSystem_v1_BootProgressTypes lastState;
    std::chrono::time_point lastStateTime;
    std::string oemLastState;
    Resource_v1_Resource oem;
};
struct ComputerSystem_v1_ComputerSystem
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ComputerSystem_v1_SystemType systemType;
    ComputerSystem_v1_Links links;
    std::string assetTag;
    std::string manufacturer;
    std::string model;
    std::string SKU;
    std::string serialNumber;
    std::string partNumber;
    string UUID;
    std::string hostName;
    ComputerSystem_v1_IndicatorLED indicatorLED;
    ComputerSystem_v1_PowerState powerState;
    ComputerSystem_v1_Boot boot;
    std::string biosVersion;
    ComputerSystem_v1_ProcessorSummary processorSummary;
    ComputerSystem_v1_MemorySummary memorySummary;
    ComputerSystem_v1_Actions actions;
    ProcessorCollection_v1_ProcessorCollection processors;
    EthernetInterfaceCollection_v1_EthernetInterfaceCollection ethernetInterfaces;
    SimpleStorageCollection_v1_SimpleStorageCollection simpleStorage;
    LogServiceCollection_v1_LogServiceCollection logServices;
    Resource_v1_Resource status;
    ComputerSystem_v1_TrustedModules trustedModules;
    SecureBoot_v1_SecureBoot secureBoot;
    Bios_v1_Bios bios;
    MemoryCollection_v1_MemoryCollection memory;
    StorageCollection_v1_StorageCollection storage;
    ComputerSystem_v1_HostingRole hostingRoles;
    NavigationReference__ pCIeDevices;
    NavigationReference__ pCIeFunctions;
    ComputerSystem_v1_HostedServices hostedServices;
    MemoryDomainCollection_v1_MemoryDomainCollection memoryDomains;
    NetworkInterfaceCollection_v1_NetworkInterfaceCollection networkInterfaces;
    Redundancy_v1_Redundancy redundancy;
    ComputerSystem_v1_WatchdogTimer hostWatchdogTimer;
    std::string subModel;
    ComputerSystem_v1_PowerRestorePolicyTypes powerRestorePolicy;
    FabricAdapterCollection_v1_FabricAdapterCollection fabricAdapters;
    std::chrono::time_point lastResetTime;
    bool locationIndicatorActive;
    ComputerSystem_v1_BootProgress bootProgress;
    double powerOnDelaySeconds;
    double powerOffDelaySeconds;
    double powerCycleDelaySeconds;
    ComputerSystem_v1_HostSerialConsole serialConsole;
    ComputerSystem_v1_HostGraphicalConsole graphicalConsole;
    ComputerSystem_v1_VirtualMediaConfig virtualMediaConfig;
    VirtualMediaCollection_v1_VirtualMediaCollection virtualMedia;
};
struct ComputerSystem_v1_HostedServices
{
    HostedStorageServices_v1_HostedStorageServices storageServices;
    Resource_v1_Resource oem;
};
struct ComputerSystem_v1_HostGraphicalConsole
{
    bool serviceEnabled;
    int64_t port;
    ComputerSystem_v1_GraphicalConnectTypesSupported connectTypesSupported;
    int64_t maxConcurrentSessions;
};
struct ComputerSystem_v1_HostSerialConsole
{
    int64_t maxConcurrentSessions;
    ComputerSystem_v1_SerialConsoleProtocol SSH;
    ComputerSystem_v1_SerialConsoleProtocol telnet;
    ComputerSystem_v1_SerialConsoleProtocol IPMI;
};
struct ComputerSystem_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference__ chassis;
    NavigationReference__ managedBy;
    NavigationReference__ poweredBy;
    NavigationReference__ cooledBy;
    NavigationReference__ endpoints;
    NavigationReference__ resourceBlocks;
    NavigationReference__ consumingComputerSystems;
    NavigationReference__ supplyingComputerSystems;
};
struct ComputerSystem_v1_MemorySummary
{
    double totalSystemMemoryGiB;
    Resource_v1_Resource status;
    ComputerSystem_v1_MemoryMirroring memoryMirroring;
    double totalSystemPersistentMemoryGiB;
    MemoryMetrics_v1_MemoryMetrics metrics;
};
struct ComputerSystem_v1_OemActions
{
};
struct ComputerSystem_v1_ProcessorSummary
{
    int64_t count;
    std::string model;
    Resource_v1_Resource status;
    int64_t logicalProcessorCount;
    ProcessorMetrics_v1_ProcessorMetrics metrics;
};
struct ComputerSystem_v1_SerialConsoleProtocol
{
    bool serviceEnabled;
    int64_t port;
    bool sharedWithManagerCLI;
    std::string consoleEntryCommand;
    std::string hotKeySequenceDisplay;
};
struct ComputerSystem_v1_TrustedModules
{
    std::string firmwareVersion;
    ComputerSystem_v1_InterfaceType interfaceType;
    Resource_v1_Resource status;
    Resource_v1_Resource oem;
    std::string firmwareVersion2;
    ComputerSystem_v1_InterfaceTypeSelection interfaceTypeSelection;
};
struct ComputerSystem_v1_VirtualMediaConfig
{
    bool serviceEnabled;
    int64_t port;
};
struct ComputerSystem_v1_WatchdogTimer
{
    bool functionEnabled;
    ComputerSystem_v1_WatchdogWarningActions warningAction;
    ComputerSystem_v1_WatchdogTimeoutActions timeoutAction;
    Resource_v1_Resource status;
    Resource_v1_Resource oem;
};
#endif
