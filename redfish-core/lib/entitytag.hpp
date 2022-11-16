#pragma once

#include <registries/privilege_registry.hpp>

namespace redfish
{

using EntityTag = redfish::privileges::EntityTag;

inline void setEntityTagsInRegistry()
{
    // account_service.hpp
    redfish::privileges::entityTagMap["/redfish/v1/AccountService/"] =
        EntityTag::tagAccountService;
    redfish::privileges::entityTagMap["/redfish/v1/AccountService/Accounts/"] =
        EntityTag::tagManagerAccountCollection;
    redfish::privileges::entityTagMap
        ["/redfish/v1/AccountService/Accounts/<str>/"] =
            EntityTag::tagManagerAccount;

    // bios.hpp
    redfish::privileges::entityTagMap["/redfish/v1/Systems/<str>/Bios/"] =
        EntityTag::tagBios;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Systems/<str>/Bios/Actions/Bios.ResetBios/"] =
            EntityTag::tagBios;

    // cable.hpp
    redfish::privileges::entityTagMap["/redfish/v1/Cables/<str>/"] =
        EntityTag::tagCable;
    redfish::privileges::entityTagMap["/redfish/v1/Cables/"] =
        EntityTag::tagCableCollection;

    // certificate_service.hpp
    redfish::privileges::entityTagMap["/redfish/v1/CertificateService/"] =
        EntityTag::tagCertificateService;
    redfish::privileges::entityTagMap
        ["/redfish/v1/CertificateService/CertificateLocations/"] =
            EntityTag::tagCertificateLocations;
    redfish::privileges::entityTagMap
        ["/redfish/v1/CertificateService/Actions/CertificateService.ReplaceCertificate/"] =
            EntityTag::tagCertificateService;
    redfish::privileges::entityTagMap
        ["/redfish/v1/CertificateService/Actions/CertificateService.GenerateCSR/"] =
            EntityTag::tagCertificateService;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Managers/bmc/NetworkProtocol/HTTPS/Certificates/"] =
            EntityTag::tagCertificateCollection;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Managers/bmc/NetworkProtocol/HTTPS/Certificates/<str>/"] =
            EntityTag::tagCertificate;
    redfish::privileges::entityTagMap
        ["/redfish/v1/AccountService/LDAP/Certificates/"] =
            EntityTag::tagCertificateCollection;
    redfish::privileges::entityTagMap
        ["/redfish/v1/AccountService/LDAP/Certificates/<str>/"] =
            EntityTag::tagCertificate;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Managers/bmc/Truststore/Certificates/"] =
            EntityTag::tagCertificateCollection;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Managers/bmc/Truststore/Certificates/<str>/"] =
            EntityTag::tagCertificate;

    // chassis.hpp
    redfish::privileges::entityTagMap["/redfish/v1/Chassis/"] =
        EntityTag::tagChassisCollection;
    redfish::privileges::entityTagMap["/redfish/v1/Chassis/<str>/"] =
        EntityTag::tagChassis;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Chassis/<str>/Actions/Chassis.Reset/"] =
            EntityTag::tagChassis;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Chassis/<str>/ResetActionInfo/"] =
            EntityTag::tagActionInfo;

    // environment_metrics.hpp
    redfish::privileges::entityTagMap
        ["/redfish/v1/Chassis/<str>/EnvironmentMetrics/"] =
            EntityTag::tagEnvironmentMetrics;

    // ethernet.hpp
    redfish::privileges::entityTagMap
        ["/redfish/v1/Managers/bmc/EthernetInterfaces/"] =
            EntityTag::tagEthernetInterfaceCollection;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Managers/bmc/EthernetInterfaces/<str>/"] =
            EntityTag::tagEthernetInterface;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Managers/bmc/EthernetInterfaces/<str>/VLANs/<str>/"] =
            EntityTag::tagVLanNetworkInterface;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Managers/bmc/EthernetInterfaces/<str>/VLANs/"] =
            EntityTag::tagVLanNetworkInterfaceCollection;

    // event_service.hpp
    redfish::privileges::entityTagMap["/redfish/v1/EventService/"] =
        EntityTag::tagEventService;
    redfish::privileges::entityTagMap
        ["/redfish/v1/EventService/Actions/EventService.SubmitTestEvent/"] =
            EntityTag::tagEventService;
    redfish::privileges::entityTagMap
        ["/redfish/v1/EventService/Subscriptions/"] =
            EntityTag::tagEventDestinationCollection;
    redfish::privileges::entityTagMap
        ["/redfish/v1/EventService/Subscriptions/<str>/"] =
            EntityTag::tagEventDestination;

    // hypervisor_system.hpp
    redfish::privileges::entityTagMap["/redfish/v1/Systems/hypervisor/"] =
        EntityTag::tagComputerSystem;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Systems/hypervisor/EthernetInterfaces/"] =
            EntityTag::tagEthernetInterfaceCollection;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Systems/hypervisor/EthernetInterfaces/<str>/"] =
            EntityTag::tagEthernetInterface;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Systems/hypervisor/ResetActionInfo/"] =
            EntityTag::tagActionInfo;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Systems/hypervisor/Actions/ComputerSystem.Reset/"] =
            EntityTag::tagComputerSystem;

    // log_services.hpp
    redfish::privileges::entityTagMap
        ["/redfish/v1/Systems/<str>/LogServices/"] =
            EntityTag::tagLogServiceCollection;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Systems/<str>/LogServices/EventLog/"] =
            EntityTag::tagLogService;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Systems/<str>/LogServices/EventLog/Entries/"] =
            EntityTag::tagLogEntryCollection;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Systems/<str>/LogServices/EventLog/Entries/<str>/"] =
            EntityTag::tagLogEntry;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Systems/<str>/LogServices/EventLog/Entries/<str>/attachment"] =
            EntityTag::tagLogEntry;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Systems/<str>/LogServices/HostLogger/"] =
            EntityTag::tagLogService;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Systems/<str>/LogServices/HostLogger/Entries/"] =
            EntityTag::tagLogEntryCollection;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Systems/<str>/LogServices/HostLogger/Entries/<str>/"] =
            EntityTag::tagLogEntry;
    redfish::privileges::entityTagMap["/redfish/v1/Managers/bmc/LogServices/"] =
        EntityTag::tagLogServiceCollection;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Managers/bmc/LogServices/Journal/"] =
            EntityTag::tagLogService;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Managers/bmc/LogServices/Journal/Entries/"] =
            EntityTag::tagLogEntryCollection;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Managers/bmc/LogServices/Journal/Entries/<str>/"] =
            EntityTag::tagLogEntry;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Systems/<str>/LogServices/HostLogger/"] =
            EntityTag::tagLogService;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Managers/bmc/LogServices/Dump/"] =
            EntityTag::tagLogService;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Managers/bmc/LogServices/Dump/Entries/"] =
            EntityTag::tagLogEntryCollection;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Managers/bmc/LogServices/Dump/Entries/<str>/"] =
            EntityTag::tagLogEntry;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Managers/bmc/LogServices/Dump/Actions/LogService.CollectDiagnosticData/"] =
            EntityTag::tagLogService;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Managers/bmc/LogServices/Dump/Actions/LogService.ClearLog/"] =
            EntityTag::tagLogService;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Managers/bmc/LogServices/FaultLog/"] =
            EntityTag::tagLogService;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Managers/bmc/LogServices/FaultLog/Entries/"] =
            EntityTag::tagLogEntryCollection;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Managers/bmc/LogServices/FaultLog/Entries/<str>/"] =
            EntityTag::tagLogEntry;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Managers/bmc/LogServices/FaultLog/Actions/LogService.ClearLog/"] =
            EntityTag::tagLogService;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Systems/<str>/LogServices/Dump/"] =
            EntityTag::tagLogService;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Systems/<str>/LogServices/Dump/Entries/"] =
            EntityTag::tagLogEntryCollection;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Systems/<str>/LogServices/Dump/Entries/<str>/"] =
            EntityTag::tagLogEntry;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Systems/<str>/LogServices/Dump/Actions/LogService.CollectDiagnosticData/"] =
            EntityTag::tagLogService;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Systems/<str>/LogServices/Dump/Actions/LogService.ClearLog/"] =
            EntityTag::tagLogService;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Systems/<str>/LogServices/Crashdump/"] =
            EntityTag::tagLogService;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Systems/<str>/LogServices/Crashdump/Actions/LogService.ClearLog/"] =
            EntityTag::tagLogService;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Systems/<str>/LogServices/Crashdump/Entries/"] =
            EntityTag::tagLogEntryCollection;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Systems/<str>/LogServices/Crashdump/Entries/<str>/"] =
            EntityTag::tagLogEntry;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Systems/<str>/LogServices/Crashdump/Entries/<str>/<str>/"] =
            EntityTag::tagLogEntry;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Systems/<str>/LogServices/Crashdump/Actions/LogService.CollectDiagnosticData/"] =
            EntityTag::tagLogService;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Systems/<str>/LogServices/EventLog/Actions/LogService.ClearLog/"] =
            EntityTag::tagLogService;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Systems/<str>/LogServices/PostCodes/"] =
            EntityTag::tagLogService;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Systems/<str>/LogServices/PostCodes/Actions/LogService.ClearLog/"] =
            EntityTag::tagLogService;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Systems/<str>/LogServices/PostCodes/Entries/"] =
            EntityTag::tagLogEntryCollection;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Systems/<str>/LogServices/PostCodes/Entries/<str>/attachment/"] =
            EntityTag::tagLogEntry;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Systems/<str>/LogServices/PostCodes/Entries/<str>/"] =
            EntityTag::tagLogEntry;

    // manager_diagnostic_data.hpp
    redfish::privileges::entityTagMap
        ["/redfish/v1/Managers/bmc/ManagerDiagnosticData"] =
            EntityTag::tagManagerDiagnosticData;

    // managers.hpp
    redfish::privileges::entityTagMap
        ["/redfish/v1/Managers/bmc/Actions/Manager.Reset/"] =
            EntityTag::tagManager;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Managers/bmc/Actions/Manager.ResetToDefaults/"] =
            EntityTag::tagManager;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Managers/bmc/ResetActionInfo/"] =
            EntityTag::tagActionInfo;
    redfish::privileges::entityTagMap["/redfish/v1/Managers/bmc/"] =
        EntityTag::tagManager;
    redfish::privileges::entityTagMap["/redfish/v1/Managers/"] =
        EntityTag::tagManagerCollection;

    // memory.hpp
    redfish::privileges::entityTagMap["/redfish/v1/Systems/<str>/Memory/"] =
        EntityTag::tagMemoryCollection;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Systems/<str>/Memory/<str>/"] = EntityTag::tagMemory;

    // message_registries.hpp
    redfish::privileges::entityTagMap["/redfish/v1/Registries/"] =
        EntityTag::tagMessageRegistryFileCollection;
    redfish::privileges::entityTagMap["/redfish/v1/Registries/<str>/"] =
        EntityTag::tagMessageRegistryFile;
    redfish::privileges::entityTagMap["/redfish/v1/Registries/<str>/<str>/"] =
        EntityTag::tagMessageRegistryFile;

    // metric_report_definition.hpp
    redfish::privileges::entityTagMap
        ["/redfish/v1/TelemetryService/MetricReportDefinitions/"] =
            EntityTag::tagMetricReportDefinitionCollection;
    redfish::privileges::entityTagMap
        ["/redfish/v1/TelemetryService/MetricReportDefinitions/<str>/"] =
            EntityTag::tagMetricReportDefinition;

    // metric_report.hpp
    redfish::privileges::entityTagMap
        ["/redfish/v1/TelemetryService/MetricReports/"] =
            EntityTag::tagMetricReportCollection;
    redfish::privileges::entityTagMap
        ["/redfish/v1/TelemetryService/MetricReports/<str>/"] =
            EntityTag::tagMetricReport;

    // network_protocol.hpp
    redfish::privileges::entityTagMap
        ["/redfish/v1/Managers/bmc/NetworkProtocol/"] =
            EntityTag::tagManagerNetworkProtocol;

    // pcie_slots.hpp
    redfish::privileges::entityTagMap["/redfish/v1/Chassis/<str>/PCIeSlots/"] =
        EntityTag::tagPCIeSlots;

    // pcie.hpp
    redfish::privileges::entityTagMap
        ["/redfish/v1/Systems/<str>/PCIeDevices/"] =
            EntityTag::tagPCIeDeviceCollection;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Systems/<str>/PCIeDevices/<str>/"] =
            EntityTag::tagPCIeDevice;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Systems/system/PCIeDevices/<str>/PCIeFunctions/"] =
            EntityTag::tagPCIeFunctionCollection;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Systems/system/PCIeDevices/<str>/PCIeFunctions/<str>/"] =
            EntityTag::tagPCIeFunction;

    // power_subsystem.hpp
    redfish::privileges::entityTagMap
        ["/redfish/v1/Chassis/<str>/PowerSubsystem/"] =
            EntityTag::tagPowerSubsystem;

    // power.hpp
    redfish::privileges::entityTagMap["/redfish/v1/Chassis/<str>/Power/"] =
        EntityTag::tagPower;

    // processor.hpp
    redfish::privileges::entityTagMap
        ["/redfish/v1/Systems/system/Processors/<str>/OperatingConfigs/"] =
            EntityTag::tagOperatingConfigCollection;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Systems/system/Processors/<str>/OperatingConfigs/<str>/"] =
            EntityTag::tagOperatingConfig;
    redfish::privileges::entityTagMap["/redfish/v1/Systems/<str>/Processors/"] =
        EntityTag::tagProcessorCollection;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Systems/<str>/Processors/<str>/"] =
            EntityTag::tagProcessor;

    // redfish_sessions.hpp
    redfish::privileges::entityTagMap
        ["/redfish/v1/SessionService/Sessions/<str>/"] = EntityTag::tagSession;
    redfish::privileges::entityTagMap["/redfish/v1/SessionService/Sessions/"] =
        EntityTag::tagSessionCollection;
    redfish::privileges::entityTagMap
        ["/redfish/v1/SessionService/Sessions/Members/"] =
            EntityTag::tagSessionCollection;
    redfish::privileges::entityTagMap["/redfish/v1/SessionService/"] =
        EntityTag::tagSessionService;

    // redfish_v1.hpp
    redfish::privileges::entityTagMap["/redfish/v1/JsonSchemas/"] =
        EntityTag::tagJsonSchemaFileCollection;
    redfish::privileges::entityTagMap["/redfish/v1/JsonSchemas/<str>/"] =
        EntityTag::tagJsonSchemaFile;

    // roles.hpp
    redfish::privileges::entityTagMap["/redfish/v1/AccountService/Roles/"] =
        EntityTag::tagRoleCollection;
    redfish::privileges::entityTagMap
        ["/redfish/v1/AccountService/Roles/<str>/"] = EntityTag::tagRole;

    // sensors.hpp
    redfish::privileges::entityTagMap["/redfish/v1/Chassis/<str>/Sensors/"] =
        EntityTag::tagSensorCollection;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Chassis/<str>/Sensors/<str>/"] = EntityTag::tagSensor;

    // service_root.hpp
    redfish::privileges::entityTagMap["/redfish/v1/"] =
        EntityTag::tagServiceRoot;

    // storage.hpp
    redfish::privileges::entityTagMap["/redfish/v1/Systems/<str>/Storage/"] =
        EntityTag::tagStorageCollection;
    redfish::privileges::entityTagMap["/redfish/v1/Systems/system/Storage/1/"] =
        EntityTag::tagStorage;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Systems/<str>/Storage/1/Drives/"] =
            EntityTag::tagDriveCollection;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Systems/<str>/Storage/1/Drives/<str>/"] =
            EntityTag::tagDrive;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Chassis/<str>/Drives/<str>/"] = EntityTag::tagChassis;

    // systems.hpp
    redfish::privileges::entityTagMap["/redfish/v1/Systems/"] =
        EntityTag::tagComputerSystemCollection;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Systems/system/Actions/ComputerSystem.Reset/"] =
            EntityTag::tagComputerSystem;
    redfish::privileges::entityTagMap["/redfish/v1/Systems/system/"] =
        EntityTag::tagComputerSystem;
    redfish::privileges::entityTagMap["/redfish/v1/Systems/<str>/"] =
        EntityTag::tagComputerSystem;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Systems/system/ResetActionInfo/"] =
            EntityTag::tagActionInfo;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Systems/<str>/ResetActionInfo/"] =
            EntityTag::tagActionInfo;

    // task.hpp
    redfish::privileges::entityTagMap
        ["/redfish/v1/TaskService/Tasks/<str>/Monitor/"] = EntityTag::tagTask;
    redfish::privileges::entityTagMap["/redfish/v1/TaskService/Tasks/"] =
        EntityTag::tagTaskCollection;
    redfish::privileges::entityTagMap["/redfish/v1/TaskService/Tasks/<str>/"] =
        EntityTag::tagTask;
    redfish::privileges::entityTagMap["/redfish/v1/TaskService/"] =
        EntityTag::tagTaskService;

    //  telemetry_service.hpp
    redfish::privileges::entityTagMap["/redfish/v1/TelemetryService/"] =
        EntityTag::tagTelemetryService;

    /// thermal_subsystem.hpp
    redfish::privileges::entityTagMap
        ["/redfish/v1/Chassis/<str>/ThermalSubsystem/"] =
            EntityTag::tagThermalSubsystem;

    // thermal.hpp
    redfish::privileges::entityTagMap["/redfish/v1/Chassis/<str>/Thermal/"] =
        EntityTag::tagThermal;

    // trigger.hpp
    redfish::privileges::entityTagMap
        ["/redfish/v1/TelemetryService/Triggers/"] =
            EntityTag::tagTriggersCollection;
    redfish::privileges::entityTagMap
        ["/redfish/v1/TelemetryService/Triggers/<str>/"] =
            EntityTag::tagTriggers;

    // update_service.hpp
    redfish::privileges::entityTagMap
        ["/redfish/v1/UpdateService/Actions/UpdateService.SimpleUpdate/"] =
            EntityTag::tagUpdateService;
    redfish::privileges::entityTagMap["/redfish/v1/UpdateService/"] =
        EntityTag::tagUpdateService;
    redfish::privileges::entityTagMap["/redfish/v1/UpdateService/update/"] =
        EntityTag::tagUpdateService;
    redfish::privileges::entityTagMap
        ["/redfish/v1/UpdateService/FirmwareInventory/"] =
            EntityTag::tagSoftwareInventoryCollection;
    redfish::privileges::entityTagMap
        ["/redfish/v1/UpdateService/FirmwareInventory/<str>/"] =
            EntityTag::tagSoftwareInventory;

    // virtual_media.hpp
    redfish::privileges::entityTagMap
        ["/redfish/v1/Managers/<str>/VirtualMedia/<str>/Actions/VirtualMedia.InsertMedia"] =
            EntityTag::tagVirtualMedia;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Managers/<str>/VirtualMedia/<str>/Actions/VirtualMedia.EjectMedia"] =
            EntityTag::tagVirtualMedia;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Managers/<str>/VirtualMedia/"] =
            EntityTag::tagVirtualMediaCollection;
    redfish::privileges::entityTagMap
        ["/redfish/v1/Managers/<str>/VirtualMedia/<str>/"] =
            EntityTag::tagVirtualMedia;
}
} // namespace redfish