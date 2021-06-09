#ifndef COMPUTERSYSTEM_V1
#define COMPUTERSYSTEM_V1

#include "Bios_v1.h"
#include "BootOptionCollection_v1.h"
#include "CertificateCollection_v1.h"
#include "ComputerSystem_v1.h"
#include "EthernetInterfaceCollection_v1.h"
#include "FabricAdapterCollection_v1.h"
#include "GraphicsControllerCollection_v1.h"
#include "HostedStorageServices_v1.h"
#include "LogServiceCollection_v1.h"
#include "MemoryCollection_v1.h"
#include "MemoryDomainCollection_v1.h"
#include "MemoryMetrics_v1.h"
#include "NavigationReference_.h"
#include "NetworkInterfaceCollection_v1.h"
#include "ProcessorCollection_v1.h"
#include "ProcessorMetrics_v1.h"
#include "Redundancy_v1.h"
#include "Resource_v1.h"
#include "SecureBoot_v1.h"
#include "SimpleStorageCollection_v1.h"
#include "SoftwareInventory_v1.h"
#include "StorageCollection_v1.h"
#include "USBControllerCollection_v1.h"
#include "VirtualMediaCollection_v1.h"

#include <chrono>

enum class ComputerSystemV1AutomaticRetryConfig
{
    Disabled,
    RetryAttempts,
    RetryAlways,
};
enum class ComputerSystemV1BootOrderTypes
{
    BootOrder,
    AliasBootOrder,
};
enum class ComputerSystemV1BootProgressTypes
{
    None,
    PrimaryProcessorInitializationStarted,
    BusInitializationStarted,
    MemoryInitializationStarted,
    SecondaryProcessorInitializationStarted,
    PCIResourceConfigStarted,
    SystemHardwareInitializationComplete,
    SetupEntered,
    OSBootStarted,
    OSRunning,
    OEM,
};
enum class ComputerSystemV1BootSource
{
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
enum class ComputerSystemV1BootSourceOverrideEnabled
{
    Disabled,
    Once,
    Continuous,
};
enum class ComputerSystemV1BootSourceOverrideMode
{
    Legacy,
    UEFI,
};
enum class ComputerSystemV1GraphicalConnectTypesSupported
{
    KVMIP,
    OEM,
};
enum class ComputerSystemV1HostingRole
{
    ApplicationServer,
    StorageServer,
    Switch,
    Appliance,
    BareMetalServer,
    VirtualMachineServer,
    ContainerServer,
};
enum class ComputerSystemV1IndicatorLED
{
    Unknown,
    Lit,
    Blinking,
    Off,
};
enum class ComputerSystemV1InterfaceType
{
    TPM1_2,
    TPM2_0,
    TCM1_0,
};
enum class ComputerSystemV1InterfaceTypeSelection
{
    None,
    FirmwareUpdate,
    BiosSetting,
    OemMethod,
};
enum class ComputerSystemV1MemoryMirroring
{
    System,
    DIMM,
    Hybrid,
    None,
};
enum class ComputerSystemV1PowerMode
{
    MaximumPerformance,
    BalancedPerformance,
    PowerSaving,
    Static,
    OSControlled,
    OEM,
};
enum class ComputerSystemV1PowerRestorePolicyTypes
{
    AlwaysOn,
    AlwaysOff,
    LastState,
};
enum class ComputerSystemV1PowerState
{
    On,
    Off,
    PoweringOn,
    PoweringOff,
};
enum class ComputerSystemV1StopBootOnFault
{
    Never,
    AnyFault,
};
enum class ComputerSystemV1SystemType
{
    Physical,
    Virtual,
    OS,
    PhysicallyPartitioned,
    VirtuallyPartitioned,
    Composed,
};
enum class ComputerSystemV1TrustedModuleRequiredToBoot
{
    Disabled,
    Required,
};
enum class ComputerSystemV1WatchdogTimeoutActions
{
    None,
    ResetSystem,
    PowerCycle,
    PowerDown,
    OEM,
};
enum class ComputerSystemV1WatchdogWarningActions
{
    None,
    DiagnosticInterrupt,
    SMI,
    MessagingInterrupt,
    SCI,
    OEM,
};
struct ComputerSystemV1OemActions
{};
struct ComputerSystemV1Actions
{
    ComputerSystemV1OemActions oem;
};
struct ComputerSystemV1Boot
{
    ComputerSystemV1BootSource bootSourceOverrideTarget;
    ComputerSystemV1BootSourceOverrideEnabled bootSourceOverrideEnabled;
    std::string uefiTargetBootSourceOverride;
    ComputerSystemV1BootSourceOverrideMode bootSourceOverrideMode;
    BootOptionCollectionV1BootOptionCollection bootOptions;
    std::string bootNext;
    std::string bootOrder;
    ComputerSystemV1BootSource aliasBootOrder;
    ComputerSystemV1BootOrderTypes bootOrderPropertySelection;
    CertificateCollectionV1CertificateCollection certificates;
    std::string httpBootUri;
    ComputerSystemV1AutomaticRetryConfig automaticRetryConfig;
    int64_t automaticRetryAttempts;
    int64_t remainingAutomaticRetryAttempts;
    ComputerSystemV1TrustedModuleRequiredToBoot trustedModuleRequiredToBoot;
    ComputerSystemV1StopBootOnFault stopBootOnFault;
};
struct ComputerSystemV1BootProgress
{
    ComputerSystemV1BootProgressTypes lastState;
    std::chrono::time_point<std::chrono::system_clock> lastStateTime;
    std::string oemLastState;
    ResourceV1Resource oem;
};
struct ComputerSystemV1Links
{
    ResourceV1Resource oem;
    NavigationReference_ chassis;
    NavigationReference_ managedBy;
    NavigationReference_ poweredBy;
    NavigationReference_ cooledBy;
    NavigationReference_ endpoints;
    NavigationReference_ resourceBlocks;
    NavigationReference_ consumingComputerSystems;
    NavigationReference_ supplyingComputerSystems;
};
struct ComputerSystemV1ProcessorSummary
{
    int64_t count;
    std::string model;
    ResourceV1Resource status;
    int64_t logicalProcessorCount;
    ProcessorMetricsV1ProcessorMetrics metrics;
    int64_t coreCount;
    bool threadingEnabled;
};
struct ComputerSystemV1MemorySummary
{
    double totalSystemMemoryGiB;
    ResourceV1Resource status;
    ComputerSystemV1MemoryMirroring memoryMirroring;
    double totalSystemPersistentMemoryGiB;
    MemoryMetricsV1MemoryMetrics metrics;
};
struct ComputerSystemV1TrustedModules
{
    std::string firmwareVersion;
    ComputerSystemV1InterfaceType interfaceType;
    ResourceV1Resource status;
    ResourceV1Resource oem;
    std::string firmwareVersion2;
    ComputerSystemV1InterfaceTypeSelection interfaceTypeSelection;
};
struct ComputerSystemV1HostedServices
{
    HostedStorageServicesV1HostedStorageServices storageServices;
    ResourceV1Resource oem;
};
struct ComputerSystemV1WatchdogTimer
{
    bool functionEnabled;
    ComputerSystemV1WatchdogWarningActions warningAction;
    ComputerSystemV1WatchdogTimeoutActions timeoutAction;
    ResourceV1Resource status;
    ResourceV1Resource oem;
};
struct ComputerSystemV1SerialConsoleProtocol
{
    bool serviceEnabled;
    int64_t port;
    bool sharedWithManagerCLI;
    std::string consoleEntryCommand;
    std::string hotKeySequenceDisplay;
};
struct ComputerSystemV1HostSerialConsole
{
    int64_t maxConcurrentSessions;
    ComputerSystemV1SerialConsoleProtocol SSH;
    ComputerSystemV1SerialConsoleProtocol telnet;
    ComputerSystemV1SerialConsoleProtocol IPMI;
};
struct ComputerSystemV1HostGraphicalConsole
{
    bool serviceEnabled;
    int64_t port;
    ComputerSystemV1GraphicalConnectTypesSupported connectTypesSupported;
    int64_t maxConcurrentSessions;
};
struct ComputerSystemV1VirtualMediaConfig
{
    bool serviceEnabled;
    int64_t port;
};
struct ComputerSystemV1ComputerSystem
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ComputerSystemV1SystemType systemType;
    ComputerSystemV1Links links;
    std::string assetTag;
    std::string manufacturer;
    std::string model;
    std::string SKU;
    std::string serialNumber;
    std::string partNumber;
    std::string UUID;
    std::string hostName;
    ComputerSystemV1IndicatorLED indicatorLED;
    ComputerSystemV1PowerState powerState;
    ComputerSystemV1Boot boot;
    std::string biosVersion;
    ComputerSystemV1ProcessorSummary processorSummary;
    ComputerSystemV1MemorySummary memorySummary;
    ComputerSystemV1Actions actions;
    ProcessorCollectionV1ProcessorCollection processors;
    EthernetInterfaceCollectionV1EthernetInterfaceCollection ethernetInterfaces;
    SimpleStorageCollectionV1SimpleStorageCollection simpleStorage;
    LogServiceCollectionV1LogServiceCollection logServices;
    ResourceV1Resource status;
    ComputerSystemV1TrustedModules trustedModules;
    SecureBootV1SecureBoot secureBoot;
    BiosV1Bios bios;
    MemoryCollectionV1MemoryCollection memory;
    StorageCollectionV1StorageCollection storage;
    ComputerSystemV1HostingRole hostingRoles;
    NavigationReference_ pCIeDevices;
    NavigationReference_ pCIeFunctions;
    ComputerSystemV1HostedServices hostedServices;
    MemoryDomainCollectionV1MemoryDomainCollection memoryDomains;
    NetworkInterfaceCollectionV1NetworkInterfaceCollection networkInterfaces;
    RedundancyV1Redundancy redundancy;
    ComputerSystemV1WatchdogTimer hostWatchdogTimer;
    std::string subModel;
    ComputerSystemV1PowerRestorePolicyTypes powerRestorePolicy;
    FabricAdapterCollectionV1FabricAdapterCollection fabricAdapters;
    std::chrono::time_point<std::chrono::system_clock> lastResetTime;
    bool locationIndicatorActive;
    ComputerSystemV1BootProgress bootProgress;
    double powerOnDelaySeconds;
    double powerOffDelaySeconds;
    double powerCycleDelaySeconds;
    ComputerSystemV1HostSerialConsole serialConsole;
    ComputerSystemV1HostGraphicalConsole graphicalConsole;
    ComputerSystemV1VirtualMediaConfig virtualMediaConfig;
    VirtualMediaCollectionV1VirtualMediaCollection virtualMedia;
    CertificateCollectionV1CertificateCollection certificates;
    SoftwareInventoryV1SoftwareInventory measurements;
    GraphicsControllerCollectionV1GraphicsControllerCollection
        graphicsControllers;
    USBControllerCollectionV1USBControllerCollection uSBControllers;
    ComputerSystemV1PowerMode powerMode;
};
#endif
