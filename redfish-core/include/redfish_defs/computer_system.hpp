#pragma once
#include <nlohmann/json.hpp>

namespace computer_system
{
// clang-format off

enum class BootSourceOverrideEnabled{
    Invalid,
    Disabled,
    Once,
    Continuous,
};

NLOHMANN_JSON_SERIALIZE_ENUM(BootSourceOverrideEnabled, {
    {BootSourceOverrideEnabled::Invalid, "Invalid"},
    {BootSourceOverrideEnabled::Disabled, "Disabled"},
    {BootSourceOverrideEnabled::Once, "Once"},
    {BootSourceOverrideEnabled::Continuous, "Continuous"},
});

enum class IndicatorLED{
    Invalid,
    Unknown,
    Lit,
    Blinking,
    Off,
};

NLOHMANN_JSON_SERIALIZE_ENUM(IndicatorLED, {
    {IndicatorLED::Invalid, "Invalid"},
    {IndicatorLED::Unknown, "Unknown"},
    {IndicatorLED::Lit, "Lit"},
    {IndicatorLED::Blinking, "Blinking"},
    {IndicatorLED::Off, "Off"},
});

enum class PowerState{
    Invalid,
    On,
    Off,
    PoweringOn,
    PoweringOff,
};

NLOHMANN_JSON_SERIALIZE_ENUM(PowerState, {
    {PowerState::Invalid, "Invalid"},
    {PowerState::On, "On"},
    {PowerState::Off, "Off"},
    {PowerState::PoweringOn, "PoweringOn"},
    {PowerState::PoweringOff, "PoweringOff"},
});

enum class SystemType{
    Invalid,
    Physical,
    Virtual,
    OS,
    PhysicallyPartitioned,
    VirtuallyPartitioned,
    Composed,
    DPU,
};

NLOHMANN_JSON_SERIALIZE_ENUM(SystemType, {
    {SystemType::Invalid, "Invalid"},
    {SystemType::Physical, "Physical"},
    {SystemType::Virtual, "Virtual"},
    {SystemType::OS, "OS"},
    {SystemType::PhysicallyPartitioned, "PhysicallyPartitioned"},
    {SystemType::VirtuallyPartitioned, "VirtuallyPartitioned"},
    {SystemType::Composed, "Composed"},
    {SystemType::DPU, "DPU"},
});

enum class AutomaticRetryConfig{
    Invalid,
    Disabled,
    RetryAttempts,
    RetryAlways,
};

NLOHMANN_JSON_SERIALIZE_ENUM(AutomaticRetryConfig, {
    {AutomaticRetryConfig::Invalid, "Invalid"},
    {AutomaticRetryConfig::Disabled, "Disabled"},
    {AutomaticRetryConfig::RetryAttempts, "RetryAttempts"},
    {AutomaticRetryConfig::RetryAlways, "RetryAlways"},
});

enum class BootProgressTypes{
    Invalid,
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

NLOHMANN_JSON_SERIALIZE_ENUM(BootProgressTypes, {
    {BootProgressTypes::Invalid, "Invalid"},
    {BootProgressTypes::None, "None"},
    {BootProgressTypes::PrimaryProcessorInitializationStarted, "PrimaryProcessorInitializationStarted"},
    {BootProgressTypes::BusInitializationStarted, "BusInitializationStarted"},
    {BootProgressTypes::MemoryInitializationStarted, "MemoryInitializationStarted"},
    {BootProgressTypes::SecondaryProcessorInitializationStarted, "SecondaryProcessorInitializationStarted"},
    {BootProgressTypes::PCIResourceConfigStarted, "PCIResourceConfigStarted"},
    {BootProgressTypes::SystemHardwareInitializationComplete, "SystemHardwareInitializationComplete"},
    {BootProgressTypes::SetupEntered, "SetupEntered"},
    {BootProgressTypes::OSBootStarted, "OSBootStarted"},
    {BootProgressTypes::OSRunning, "OSRunning"},
    {BootProgressTypes::OEM, "OEM"},
});

enum class GraphicalConnectTypesSupported{
    Invalid,
    KVMIP,
    OEM,
};

NLOHMANN_JSON_SERIALIZE_ENUM(GraphicalConnectTypesSupported, {
    {GraphicalConnectTypesSupported::Invalid, "Invalid"},
    {GraphicalConnectTypesSupported::KVMIP, "KVMIP"},
    {GraphicalConnectTypesSupported::OEM, "OEM"},
});

enum class TrustedModuleRequiredToBoot{
    Invalid,
    Disabled,
    Required,
};

NLOHMANN_JSON_SERIALIZE_ENUM(TrustedModuleRequiredToBoot, {
    {TrustedModuleRequiredToBoot::Invalid, "Invalid"},
    {TrustedModuleRequiredToBoot::Disabled, "Disabled"},
    {TrustedModuleRequiredToBoot::Required, "Required"},
});

enum class PowerMode{
    Invalid,
    MaximumPerformance,
    BalancedPerformance,
    PowerSaving,
    Static,
    OSControlled,
    OEM,
};

NLOHMANN_JSON_SERIALIZE_ENUM(PowerMode, {
    {PowerMode::Invalid, "Invalid"},
    {PowerMode::MaximumPerformance, "MaximumPerformance"},
    {PowerMode::BalancedPerformance, "BalancedPerformance"},
    {PowerMode::PowerSaving, "PowerSaving"},
    {PowerMode::Static, "Static"},
    {PowerMode::OSControlled, "OSControlled"},
    {PowerMode::OEM, "OEM"},
});

enum class StopBootOnFault{
    Invalid,
    Never,
    AnyFault,
};

NLOHMANN_JSON_SERIALIZE_ENUM(StopBootOnFault, {
    {StopBootOnFault::Invalid, "Invalid"},
    {StopBootOnFault::Never, "Never"},
    {StopBootOnFault::AnyFault, "AnyFault"},
});

enum class BootSourceOverrideMode{
    Invalid,
    Legacy,
    UEFI,
};

NLOHMANN_JSON_SERIALIZE_ENUM(BootSourceOverrideMode, {
    {BootSourceOverrideMode::Invalid, "Invalid"},
    {BootSourceOverrideMode::Legacy, "Legacy"},
    {BootSourceOverrideMode::UEFI, "UEFI"},
});

enum class InterfaceType{
    Invalid,
    TPM1_2,
    TPM2_0,
    TCM1_0,
};

NLOHMANN_JSON_SERIALIZE_ENUM(InterfaceType, {
    {InterfaceType::Invalid, "Invalid"},
    {InterfaceType::TPM1_2, "TPM1_2"},
    {InterfaceType::TPM2_0, "TPM2_0"},
    {InterfaceType::TCM1_0, "TCM1_0"},
});

enum class MemoryMirroring{
    Invalid,
    System,
    DIMM,
    Hybrid,
    None,
};

NLOHMANN_JSON_SERIALIZE_ENUM(MemoryMirroring, {
    {MemoryMirroring::Invalid, "Invalid"},
    {MemoryMirroring::System, "System"},
    {MemoryMirroring::DIMM, "DIMM"},
    {MemoryMirroring::Hybrid, "Hybrid"},
    {MemoryMirroring::None, "None"},
});

enum class HostingRole{
    Invalid,
    ApplicationServer,
    StorageServer,
    Switch,
    Appliance,
    BareMetalServer,
    VirtualMachineServer,
    ContainerServer,
};

NLOHMANN_JSON_SERIALIZE_ENUM(HostingRole, {
    {HostingRole::Invalid, "Invalid"},
    {HostingRole::ApplicationServer, "ApplicationServer"},
    {HostingRole::StorageServer, "StorageServer"},
    {HostingRole::Switch, "Switch"},
    {HostingRole::Appliance, "Appliance"},
    {HostingRole::BareMetalServer, "BareMetalServer"},
    {HostingRole::VirtualMachineServer, "VirtualMachineServer"},
    {HostingRole::ContainerServer, "ContainerServer"},
});

enum class InterfaceTypeSelection{
    Invalid,
    None,
    FirmwareUpdate,
    BiosSetting,
    OemMethod,
};

NLOHMANN_JSON_SERIALIZE_ENUM(InterfaceTypeSelection, {
    {InterfaceTypeSelection::Invalid, "Invalid"},
    {InterfaceTypeSelection::None, "None"},
    {InterfaceTypeSelection::FirmwareUpdate, "FirmwareUpdate"},
    {InterfaceTypeSelection::BiosSetting, "BiosSetting"},
    {InterfaceTypeSelection::OemMethod, "OemMethod"},
});

enum class WatchdogTimeoutActions{
    Invalid,
    None,
    ResetSystem,
    PowerCycle,
    PowerDown,
    OEM,
};

NLOHMANN_JSON_SERIALIZE_ENUM(WatchdogTimeoutActions, {
    {WatchdogTimeoutActions::Invalid, "Invalid"},
    {WatchdogTimeoutActions::None, "None"},
    {WatchdogTimeoutActions::ResetSystem, "ResetSystem"},
    {WatchdogTimeoutActions::PowerCycle, "PowerCycle"},
    {WatchdogTimeoutActions::PowerDown, "PowerDown"},
    {WatchdogTimeoutActions::OEM, "OEM"},
});

enum class WatchdogWarningActions{
    Invalid,
    None,
    DiagnosticInterrupt,
    SMI,
    MessagingInterrupt,
    SCI,
    OEM,
};

NLOHMANN_JSON_SERIALIZE_ENUM(WatchdogWarningActions, {
    {WatchdogWarningActions::Invalid, "Invalid"},
    {WatchdogWarningActions::None, "None"},
    {WatchdogWarningActions::DiagnosticInterrupt, "DiagnosticInterrupt"},
    {WatchdogWarningActions::SMI, "SMI"},
    {WatchdogWarningActions::MessagingInterrupt, "MessagingInterrupt"},
    {WatchdogWarningActions::SCI, "SCI"},
    {WatchdogWarningActions::OEM, "OEM"},
});

enum class BootOrderTypes{
    Invalid,
    BootOrder,
    AliasBootOrder,
};

NLOHMANN_JSON_SERIALIZE_ENUM(BootOrderTypes, {
    {BootOrderTypes::Invalid, "Invalid"},
    {BootOrderTypes::BootOrder, "BootOrder"},
    {BootOrderTypes::AliasBootOrder, "AliasBootOrder"},
});

enum class PowerRestorePolicyTypes{
    Invalid,
    AlwaysOn,
    AlwaysOff,
    LastState,
};

NLOHMANN_JSON_SERIALIZE_ENUM(PowerRestorePolicyTypes, {
    {PowerRestorePolicyTypes::Invalid, "Invalid"},
    {PowerRestorePolicyTypes::AlwaysOn, "AlwaysOn"},
    {PowerRestorePolicyTypes::AlwaysOff, "AlwaysOff"},
    {PowerRestorePolicyTypes::LastState, "LastState"},
});

enum class BootSource{
    Invalid,
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

NLOHMANN_JSON_SERIALIZE_ENUM(BootSource, {
    {BootSource::Invalid, "Invalid"},
    {BootSource::None, "None"},
    {BootSource::Pxe, "Pxe"},
    {BootSource::Floppy, "Floppy"},
    {BootSource::Cd, "Cd"},
    {BootSource::Usb, "Usb"},
    {BootSource::Hdd, "Hdd"},
    {BootSource::BiosSetup, "BiosSetup"},
    {BootSource::Utilities, "Utilities"},
    {BootSource::Diags, "Diags"},
    {BootSource::UefiShell, "UefiShell"},
    {BootSource::UefiTarget, "UefiTarget"},
    {BootSource::SDCard, "SDCard"},
    {BootSource::UefiHttp, "UefiHttp"},
    {BootSource::RemoteDrive, "RemoteDrive"},
    {BootSource::UefiBootNext, "UefiBootNext"},
});

}
// clang-format on
