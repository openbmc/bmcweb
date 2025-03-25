// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
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
    Recovery,
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
    EfficiencyFavorPower,
    EfficiencyFavorPerformance,
};

enum class CompositionUseCase{
    Invalid,
    ResourceBlockCapable,
    ExpandableSystem,
};

enum class KMIPCachePolicy{
    Invalid,
    None,
    AfterFirstUse,
};

enum class DecommissionType{
    Invalid,
    All,
    UserData,
    ManagerConfig,
    BIOSConfig,
    NetworkConfig,
    StorageConfig,
    Logs,
    TPM,
};

enum class LastResetCauses{
    Invalid,
    PowerButtonPress,
    ManagementCommand,
    PowerRestorePolicy,
    RTCWakeup,
    WatchdogExpiration,
    OSSoftRestart,
    SystemCrash,
    ThermalEvent,
    PowerEvent,
    Unknown,
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
    {BootSource::Recovery, "Recovery"},
});

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

NLOHMANN_JSON_SERIALIZE_ENUM(IndicatorLED, {
    {IndicatorLED::Invalid, "Invalid"},
    {IndicatorLED::Unknown, "Unknown"},
    {IndicatorLED::Lit, "Lit"},
    {IndicatorLED::Blinking, "Blinking"},
    {IndicatorLED::Off, "Off"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(BootSourceOverrideEnabled, {
    {BootSourceOverrideEnabled::Invalid, "Invalid"},
    {BootSourceOverrideEnabled::Disabled, "Disabled"},
    {BootSourceOverrideEnabled::Once, "Once"},
    {BootSourceOverrideEnabled::Continuous, "Continuous"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(MemoryMirroring, {
    {MemoryMirroring::Invalid, "Invalid"},
    {MemoryMirroring::System, "System"},
    {MemoryMirroring::DIMM, "DIMM"},
    {MemoryMirroring::Hybrid, "Hybrid"},
    {MemoryMirroring::None, "None"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(BootSourceOverrideMode, {
    {BootSourceOverrideMode::Invalid, "Invalid"},
    {BootSourceOverrideMode::Legacy, "Legacy"},
    {BootSourceOverrideMode::UEFI, "UEFI"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(InterfaceType, {
    {InterfaceType::Invalid, "Invalid"},
    {InterfaceType::TPM1_2, "TPM1_2"},
    {InterfaceType::TPM2_0, "TPM2_0"},
    {InterfaceType::TCM1_0, "TCM1_0"},
});

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

NLOHMANN_JSON_SERIALIZE_ENUM(InterfaceTypeSelection, {
    {InterfaceTypeSelection::Invalid, "Invalid"},
    {InterfaceTypeSelection::None, "None"},
    {InterfaceTypeSelection::FirmwareUpdate, "FirmwareUpdate"},
    {InterfaceTypeSelection::BiosSetting, "BiosSetting"},
    {InterfaceTypeSelection::OemMethod, "OemMethod"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(WatchdogWarningActions, {
    {WatchdogWarningActions::Invalid, "Invalid"},
    {WatchdogWarningActions::None, "None"},
    {WatchdogWarningActions::DiagnosticInterrupt, "DiagnosticInterrupt"},
    {WatchdogWarningActions::SMI, "SMI"},
    {WatchdogWarningActions::MessagingInterrupt, "MessagingInterrupt"},
    {WatchdogWarningActions::SCI, "SCI"},
    {WatchdogWarningActions::OEM, "OEM"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(WatchdogTimeoutActions, {
    {WatchdogTimeoutActions::Invalid, "Invalid"},
    {WatchdogTimeoutActions::None, "None"},
    {WatchdogTimeoutActions::ResetSystem, "ResetSystem"},
    {WatchdogTimeoutActions::PowerCycle, "PowerCycle"},
    {WatchdogTimeoutActions::PowerDown, "PowerDown"},
    {WatchdogTimeoutActions::OEM, "OEM"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(PowerRestorePolicyTypes, {
    {PowerRestorePolicyTypes::Invalid, "Invalid"},
    {PowerRestorePolicyTypes::AlwaysOn, "AlwaysOn"},
    {PowerRestorePolicyTypes::AlwaysOff, "AlwaysOff"},
    {PowerRestorePolicyTypes::LastState, "LastState"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(BootOrderTypes, {
    {BootOrderTypes::Invalid, "Invalid"},
    {BootOrderTypes::BootOrder, "BootOrder"},
    {BootOrderTypes::AliasBootOrder, "AliasBootOrder"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(AutomaticRetryConfig, {
    {AutomaticRetryConfig::Invalid, "Invalid"},
    {AutomaticRetryConfig::Disabled, "Disabled"},
    {AutomaticRetryConfig::RetryAttempts, "RetryAttempts"},
    {AutomaticRetryConfig::RetryAlways, "RetryAlways"},
});

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

NLOHMANN_JSON_SERIALIZE_ENUM(GraphicalConnectTypesSupported, {
    {GraphicalConnectTypesSupported::Invalid, "Invalid"},
    {GraphicalConnectTypesSupported::KVMIP, "KVMIP"},
    {GraphicalConnectTypesSupported::OEM, "OEM"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(TrustedModuleRequiredToBoot, {
    {TrustedModuleRequiredToBoot::Invalid, "Invalid"},
    {TrustedModuleRequiredToBoot::Disabled, "Disabled"},
    {TrustedModuleRequiredToBoot::Required, "Required"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(StopBootOnFault, {
    {StopBootOnFault::Invalid, "Invalid"},
    {StopBootOnFault::Never, "Never"},
    {StopBootOnFault::AnyFault, "AnyFault"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(PowerMode, {
    {PowerMode::Invalid, "Invalid"},
    {PowerMode::MaximumPerformance, "MaximumPerformance"},
    {PowerMode::BalancedPerformance, "BalancedPerformance"},
    {PowerMode::PowerSaving, "PowerSaving"},
    {PowerMode::Static, "Static"},
    {PowerMode::OSControlled, "OSControlled"},
    {PowerMode::OEM, "OEM"},
    {PowerMode::EfficiencyFavorPower, "EfficiencyFavorPower"},
    {PowerMode::EfficiencyFavorPerformance, "EfficiencyFavorPerformance"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(CompositionUseCase, {
    {CompositionUseCase::Invalid, "Invalid"},
    {CompositionUseCase::ResourceBlockCapable, "ResourceBlockCapable"},
    {CompositionUseCase::ExpandableSystem, "ExpandableSystem"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(KMIPCachePolicy, {
    {KMIPCachePolicy::Invalid, "Invalid"},
    {KMIPCachePolicy::None, "None"},
    {KMIPCachePolicy::AfterFirstUse, "AfterFirstUse"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(DecommissionType, {
    {DecommissionType::Invalid, "Invalid"},
    {DecommissionType::All, "All"},
    {DecommissionType::UserData, "UserData"},
    {DecommissionType::ManagerConfig, "ManagerConfig"},
    {DecommissionType::BIOSConfig, "BIOSConfig"},
    {DecommissionType::NetworkConfig, "NetworkConfig"},
    {DecommissionType::StorageConfig, "StorageConfig"},
    {DecommissionType::Logs, "Logs"},
    {DecommissionType::TPM, "TPM"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(LastResetCauses, {
    {LastResetCauses::Invalid, "Invalid"},
    {LastResetCauses::PowerButtonPress, "PowerButtonPress"},
    {LastResetCauses::ManagementCommand, "ManagementCommand"},
    {LastResetCauses::PowerRestorePolicy, "PowerRestorePolicy"},
    {LastResetCauses::RTCWakeup, "RTCWakeup"},
    {LastResetCauses::WatchdogExpiration, "WatchdogExpiration"},
    {LastResetCauses::OSSoftRestart, "OSSoftRestart"},
    {LastResetCauses::SystemCrash, "SystemCrash"},
    {LastResetCauses::ThermalEvent, "ThermalEvent"},
    {LastResetCauses::PowerEvent, "PowerEvent"},
    {LastResetCauses::Unknown, "Unknown"},
});

}
// clang-format on
