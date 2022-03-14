#pragma once
#include <nlohmann/json.hpp>

namespace computer_system
{
// clang-format off

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

enum class IndicatorLED{
    Invalid,
    Unknown,
    Lit,
    Blinking,
    Off,
};

enum class PowerState{
    Invalid,
    On,
    Off,
    PoweringOn,
    PoweringOff,
};

enum class BootSourceOverrideEnabled{
    Invalid,
    Disabled,
    Once,
    Continuous,
};

enum class MemoryMirroring{
    Invalid,
    System,
    DIMM,
    Hybrid,
    None,
};

enum class BootSourceOverrideMode{
    Invalid,
    Legacy,
    UEFI,
};

enum class InterfaceType{
    Invalid,
    TPM1_2,
    TPM2_0,
    TCM1_0,
};

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

enum class InterfaceTypeSelection{
    Invalid,
    None,
    FirmwareUpdate,
    BiosSetting,
    OemMethod,
};

enum class WatchdogWarningActions{
    Invalid,
    None,
    DiagnosticInterrupt,
    SMI,
    MessagingInterrupt,
    SCI,
    OEM,
};

enum class WatchdogTimeoutActions{
    Invalid,
    None,
    ResetSystem,
    PowerCycle,
    PowerDown,
    OEM,
};

enum class PowerRestorePolicyTypes{
    Invalid,
    AlwaysOn,
    AlwaysOff,
    LastState,
};

enum class BootOrderTypes{
    Invalid,
    BootOrder,
    AliasBootOrder,
};

enum class AutomaticRetryConfig{
    Invalid,
    Disabled,
    RetryAttempts,
    RetryAlways,
};

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

enum class GraphicalConnectTypesSupported{
    Invalid,
    KVMIP,
    OEM,
};

enum class TrustedModuleRequiredToBoot{
    Invalid,
    Disabled,
    Required,
};

enum class StopBootOnFault{
    Invalid,
    Never,
    AnyFault,
};

enum class PowerMode{
    Invalid,
    MaximumPerformance,
    BalancedPerformance,
    PowerSaving,
    Static,
    OSControlled,
    OEM,
};

NLOHMANN_JSON_SERIALIZE_ENUM(BootSource, { //NOLINT
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

NLOHMANN_JSON_SERIALIZE_ENUM(SystemType, { //NOLINT
    {SystemType::Invalid, "Invalid"},
    {SystemType::Physical, "Physical"},
    {SystemType::Virtual, "Virtual"},
    {SystemType::OS, "OS"},
    {SystemType::PhysicallyPartitioned, "PhysicallyPartitioned"},
    {SystemType::VirtuallyPartitioned, "VirtuallyPartitioned"},
    {SystemType::Composed, "Composed"},
    {SystemType::DPU, "DPU"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(IndicatorLED, { //NOLINT
    {IndicatorLED::Invalid, "Invalid"},
    {IndicatorLED::Unknown, "Unknown"},
    {IndicatorLED::Lit, "Lit"},
    {IndicatorLED::Blinking, "Blinking"},
    {IndicatorLED::Off, "Off"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(PowerState, { //NOLINT
    {PowerState::Invalid, "Invalid"},
    {PowerState::On, "On"},
    {PowerState::Off, "Off"},
    {PowerState::PoweringOn, "PoweringOn"},
    {PowerState::PoweringOff, "PoweringOff"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(BootSourceOverrideEnabled, { //NOLINT
    {BootSourceOverrideEnabled::Invalid, "Invalid"},
    {BootSourceOverrideEnabled::Disabled, "Disabled"},
    {BootSourceOverrideEnabled::Once, "Once"},
    {BootSourceOverrideEnabled::Continuous, "Continuous"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(MemoryMirroring, { //NOLINT
    {MemoryMirroring::Invalid, "Invalid"},
    {MemoryMirroring::System, "System"},
    {MemoryMirroring::DIMM, "DIMM"},
    {MemoryMirroring::Hybrid, "Hybrid"},
    {MemoryMirroring::None, "None"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(BootSourceOverrideMode, { //NOLINT
    {BootSourceOverrideMode::Invalid, "Invalid"},
    {BootSourceOverrideMode::Legacy, "Legacy"},
    {BootSourceOverrideMode::UEFI, "UEFI"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(InterfaceType, { //NOLINT
    {InterfaceType::Invalid, "Invalid"},
    {InterfaceType::TPM1_2, "TPM1_2"},
    {InterfaceType::TPM2_0, "TPM2_0"},
    {InterfaceType::TCM1_0, "TCM1_0"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(HostingRole, { //NOLINT
    {HostingRole::Invalid, "Invalid"},
    {HostingRole::ApplicationServer, "ApplicationServer"},
    {HostingRole::StorageServer, "StorageServer"},
    {HostingRole::Switch, "Switch"},
    {HostingRole::Appliance, "Appliance"},
    {HostingRole::BareMetalServer, "BareMetalServer"},
    {HostingRole::VirtualMachineServer, "VirtualMachineServer"},
    {HostingRole::ContainerServer, "ContainerServer"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(InterfaceTypeSelection, { //NOLINT
    {InterfaceTypeSelection::Invalid, "Invalid"},
    {InterfaceTypeSelection::None, "None"},
    {InterfaceTypeSelection::FirmwareUpdate, "FirmwareUpdate"},
    {InterfaceTypeSelection::BiosSetting, "BiosSetting"},
    {InterfaceTypeSelection::OemMethod, "OemMethod"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(WatchdogWarningActions, { //NOLINT
    {WatchdogWarningActions::Invalid, "Invalid"},
    {WatchdogWarningActions::None, "None"},
    {WatchdogWarningActions::DiagnosticInterrupt, "DiagnosticInterrupt"},
    {WatchdogWarningActions::SMI, "SMI"},
    {WatchdogWarningActions::MessagingInterrupt, "MessagingInterrupt"},
    {WatchdogWarningActions::SCI, "SCI"},
    {WatchdogWarningActions::OEM, "OEM"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(WatchdogTimeoutActions, { //NOLINT
    {WatchdogTimeoutActions::Invalid, "Invalid"},
    {WatchdogTimeoutActions::None, "None"},
    {WatchdogTimeoutActions::ResetSystem, "ResetSystem"},
    {WatchdogTimeoutActions::PowerCycle, "PowerCycle"},
    {WatchdogTimeoutActions::PowerDown, "PowerDown"},
    {WatchdogTimeoutActions::OEM, "OEM"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(PowerRestorePolicyTypes, { //NOLINT
    {PowerRestorePolicyTypes::Invalid, "Invalid"},
    {PowerRestorePolicyTypes::AlwaysOn, "AlwaysOn"},
    {PowerRestorePolicyTypes::AlwaysOff, "AlwaysOff"},
    {PowerRestorePolicyTypes::LastState, "LastState"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(BootOrderTypes, { //NOLINT
    {BootOrderTypes::Invalid, "Invalid"},
    {BootOrderTypes::BootOrder, "BootOrder"},
    {BootOrderTypes::AliasBootOrder, "AliasBootOrder"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(AutomaticRetryConfig, { //NOLINT
    {AutomaticRetryConfig::Invalid, "Invalid"},
    {AutomaticRetryConfig::Disabled, "Disabled"},
    {AutomaticRetryConfig::RetryAttempts, "RetryAttempts"},
    {AutomaticRetryConfig::RetryAlways, "RetryAlways"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(BootProgressTypes, { //NOLINT
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

NLOHMANN_JSON_SERIALIZE_ENUM(GraphicalConnectTypesSupported, { //NOLINT
    {GraphicalConnectTypesSupported::Invalid, "Invalid"},
    {GraphicalConnectTypesSupported::KVMIP, "KVMIP"},
    {GraphicalConnectTypesSupported::OEM, "OEM"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(TrustedModuleRequiredToBoot, { //NOLINT
    {TrustedModuleRequiredToBoot::Invalid, "Invalid"},
    {TrustedModuleRequiredToBoot::Disabled, "Disabled"},
    {TrustedModuleRequiredToBoot::Required, "Required"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(StopBootOnFault, { //NOLINT
    {StopBootOnFault::Invalid, "Invalid"},
    {StopBootOnFault::Never, "Never"},
    {StopBootOnFault::AnyFault, "AnyFault"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(PowerMode, { //NOLINT
    {PowerMode::Invalid, "Invalid"},
    {PowerMode::MaximumPerformance, "MaximumPerformance"},
    {PowerMode::BalancedPerformance, "BalancedPerformance"},
    {PowerMode::PowerSaving, "PowerSaving"},
    {PowerMode::Static, "Static"},
    {PowerMode::OSControlled, "OSControlled"},
    {PowerMode::OEM, "OEM"},
});

}
// clang-format on
