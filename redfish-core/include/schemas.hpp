#pragma once
/****************************************************************
 *                 READ THIS WARNING FIRST
 * This is an auto-generated header which contains definitions
 * for Redfish DMTF defined schemas.
 * DO NOT modify this registry outside of running the
 * update_schemas.py script.  The definitions contained within
 * this file are owned by DMTF.  Any modifications to these files
 * should be first pushed to the relevant registry in the DMTF
 * github organization.
 ***************************************************************/
// clang-format off
#include <array>
#include <schema_common.hpp>

namespace redfish::schemas
{
    constexpr SchemaVersion accountService{
        "AccountService",
        "v1_11_1"
    };

    constexpr SchemaVersion actionInfo{
        "ActionInfo",
        "v1_3_0"
    };

    constexpr SchemaVersion assembly{
        "Assembly",
        "v1_4_0"
    };

    constexpr SchemaVersion attributeRegistry{
        "AttributeRegistry",
        "v1_3_6"
    };

    constexpr SchemaVersion bios{
        "Bios",
        "v1_2_0"
    };

    constexpr SchemaVersion cable{
        "Cable",
        "v1_2_0"
    };

    constexpr SchemaVersion cableCollection{
        "CableCollection",
        ""
    };

    constexpr SchemaVersion certificate{
        "Certificate",
        "v1_6_0"
    };

    constexpr SchemaVersion certificateCollection{
        "CertificateCollection",
        ""
    };

    constexpr SchemaVersion certificateLocations{
        "CertificateLocations",
        "v1_0_2"
    };

    constexpr SchemaVersion certificateService{
        "CertificateService",
        "v1_0_4"
    };

    constexpr SchemaVersion chassis{
        "Chassis",
        "v1_21_0"
    };

    constexpr SchemaVersion chassisCollection{
        "ChassisCollection",
        ""
    };

    constexpr SchemaVersion computerSystem{
        "ComputerSystem",
        "v1_19_0"
    };

    constexpr SchemaVersion computerSystemCollection{
        "ComputerSystemCollection",
        ""
    };

    constexpr SchemaVersion drive{
        "Drive",
        "v1_15_0"
    };

    constexpr SchemaVersion driveCollection{
        "DriveCollection",
        ""
    };

    constexpr SchemaVersion environmentMetrics{
        "EnvironmentMetrics",
        "v1_3_0"
    };

    constexpr SchemaVersion ethernetInterface{
        "EthernetInterface",
        "v1_9_0"
    };

    constexpr SchemaVersion ethernetInterfaceCollection{
        "EthernetInterfaceCollection",
        ""
    };

    constexpr SchemaVersion event{
        "Event",
        "v1_7_1"
    };

    constexpr SchemaVersion eventDestination{
        "EventDestination",
        "v1_12_0"
    };

    constexpr SchemaVersion eventDestinationCollection{
        "EventDestinationCollection",
        ""
    };

    constexpr SchemaVersion eventService{
        "EventService",
        "v1_8_0"
    };

    constexpr SchemaVersion fan{
        "Fan",
        "v1_3_0"
    };

    constexpr SchemaVersion fanCollection{
        "FanCollection",
        ""
    };

    constexpr SchemaVersion iPAddresses{
        "IPAddresses",
        "v1_1_3"
    };

    constexpr SchemaVersion jsonSchemaFile{
        "JsonSchemaFile",
        "v1_1_4"
    };

    constexpr SchemaVersion jsonSchemaFileCollection{
        "JsonSchemaFileCollection",
        ""
    };

    constexpr SchemaVersion logEntry{
        "LogEntry",
        "v1_13_0"
    };

    constexpr SchemaVersion logEntryCollection{
        "LogEntryCollection",
        ""
    };

    constexpr SchemaVersion logService{
        "LogService",
        "v1_3_1"
    };

    constexpr SchemaVersion logServiceCollection{
        "LogServiceCollection",
        ""
    };

    constexpr SchemaVersion manager{
        "Manager",
        "v1_16_0"
    };

    constexpr SchemaVersion managerAccount{
        "ManagerAccount",
        "v1_9_0"
    };

    constexpr SchemaVersion managerAccountCollection{
        "ManagerAccountCollection",
        ""
    };

    constexpr SchemaVersion managerCollection{
        "ManagerCollection",
        ""
    };

    constexpr SchemaVersion managerDiagnosticData{
        "ManagerDiagnosticData",
        "v1_1_0"
    };

    constexpr SchemaVersion managerNetworkProtocol{
        "ManagerNetworkProtocol",
        "v1_9_0"
    };

    constexpr SchemaVersion memory{
        "Memory",
        "v1_16_0"
    };

    constexpr SchemaVersion memoryCollection{
        "MemoryCollection",
        ""
    };

    constexpr SchemaVersion message{
        "Message",
        "v1_1_2"
    };

    constexpr SchemaVersion messageRegistry{
        "MessageRegistry",
        "v1_5_0"
    };

    constexpr SchemaVersion messageRegistryCollection{
        "MessageRegistryCollection",
        ""
    };

    constexpr SchemaVersion messageRegistryFile{
        "MessageRegistryFile",
        "v1_1_3"
    };

    constexpr SchemaVersion messageRegistryFileCollection{
        "MessageRegistryFileCollection",
        ""
    };

    constexpr SchemaVersion metricDefinition{
        "MetricDefinition",
        "v1_3_1"
    };

    constexpr SchemaVersion metricDefinitionCollection{
        "MetricDefinitionCollection",
        ""
    };

    constexpr SchemaVersion metricReport{
        "MetricReport",
        "v1_5_0"
    };

    constexpr SchemaVersion metricReportCollection{
        "MetricReportCollection",
        ""
    };

    constexpr SchemaVersion metricReportDefinition{
        "MetricReportDefinition",
        "v1_4_2"
    };

    constexpr SchemaVersion metricReportDefinitionCollection{
        "MetricReportDefinitionCollection",
        ""
    };

    constexpr SchemaVersion operatingConfig{
        "OperatingConfig",
        "v1_0_2"
    };

    constexpr SchemaVersion operatingConfigCollection{
        "OperatingConfigCollection",
        ""
    };

    constexpr SchemaVersion pCIeDevice{
        "PCIeDevice",
        "v1_10_0"
    };

    constexpr SchemaVersion pCIeDeviceCollection{
        "PCIeDeviceCollection",
        ""
    };

    constexpr SchemaVersion pCIeFunction{
        "PCIeFunction",
        "v1_4_0"
    };

    constexpr SchemaVersion pCIeFunctionCollection{
        "PCIeFunctionCollection",
        ""
    };

    constexpr SchemaVersion pCIeSlots{
        "PCIeSlots",
        "v1_5_0"
    };

    constexpr SchemaVersion physicalContext{
        "PhysicalContext",
        ""
    };

    constexpr SchemaVersion power{
        "Power",
        "v1_7_1"
    };

    constexpr SchemaVersion powerSubsystem{
        "PowerSubsystem",
        "v1_1_0"
    };

    constexpr SchemaVersion powerSupply{
        "PowerSupply",
        "v1_5_0"
    };

    constexpr SchemaVersion powerSupplyCollection{
        "PowerSupplyCollection",
        ""
    };

    constexpr SchemaVersion privileges{
        "Privileges",
        "v1_0_5"
    };

    constexpr SchemaVersion processor{
        "Processor",
        "v1_16_0"
    };

    constexpr SchemaVersion processorCollection{
        "ProcessorCollection",
        ""
    };

    constexpr SchemaVersion redfishError{
        "RedfishError",
        "v1_0_1"
    };

    constexpr SchemaVersion redfishExtensions{
        "RedfishExtensions",
        "v1_0_0"
    };

    constexpr SchemaVersion redundancy{
        "Redundancy",
        "v1_4_1"
    };

    constexpr SchemaVersion resource{
        "Resource",
        "v1_14_1"
    };

    constexpr SchemaVersion role{
        "Role",
        "v1_3_1"
    };

    constexpr SchemaVersion roleCollection{
        "RoleCollection",
        ""
    };

    constexpr SchemaVersion sensor{
        "Sensor",
        "v1_6_0"
    };

    constexpr SchemaVersion sensorCollection{
        "SensorCollection",
        ""
    };

    constexpr SchemaVersion serviceRoot{
        "ServiceRoot",
        "v1_14_0"
    };

    constexpr SchemaVersion session{
        "Session",
        "v1_5_0"
    };

    constexpr SchemaVersion sessionCollection{
        "SessionCollection",
        ""
    };

    constexpr SchemaVersion sessionService{
        "SessionService",
        "v1_1_8"
    };

    constexpr SchemaVersion settings{
        "Settings",
        "v1_3_5"
    };

    constexpr SchemaVersion softwareInventory{
        "SoftwareInventory",
        "v1_8_0"
    };

    constexpr SchemaVersion softwareInventoryCollection{
        "SoftwareInventoryCollection",
        ""
    };

    constexpr SchemaVersion storage{
        "Storage",
        "v1_13_0"
    };

    constexpr SchemaVersion storageCollection{
        "StorageCollection",
        ""
    };

    constexpr SchemaVersion storageController{
        "StorageController",
        "v1_6_0"
    };

    constexpr SchemaVersion storageControllerCollection{
        "StorageControllerCollection",
        ""
    };

    constexpr SchemaVersion task{
        "Task",
        "v1_6_1"
    };

    constexpr SchemaVersion taskCollection{
        "TaskCollection",
        ""
    };

    constexpr SchemaVersion taskService{
        "TaskService",
        "v1_2_0"
    };

    constexpr SchemaVersion telemetryService{
        "TelemetryService",
        "v1_3_1"
    };

    constexpr SchemaVersion thermal{
        "Thermal",
        "v1_7_1"
    };

    constexpr SchemaVersion thermalMetrics{
        "ThermalMetrics",
        "v1_0_1"
    };

    constexpr SchemaVersion thermalSubsystem{
        "ThermalSubsystem",
        "v1_0_0"
    };

    constexpr SchemaVersion triggers{
        "Triggers",
        "v1_2_0"
    };

    constexpr SchemaVersion triggersCollection{
        "TriggersCollection",
        ""
    };

    constexpr SchemaVersion updateService{
        "UpdateService",
        "v1_11_1"
    };

    constexpr SchemaVersion virtualMedia{
        "VirtualMedia",
        "v1_5_1"
    };

    constexpr SchemaVersion virtualMediaCollection{
        "VirtualMediaCollection",
        ""
    };

    constexpr SchemaVersion vLanNetworkInterface{
        "VLanNetworkInterface",
        "v1_3_0"
    };

    constexpr SchemaVersion vLanNetworkInterfaceCollection{
        "VLanNetworkInterfaceCollection",
        ""
    };

    constexpr const std::array<const SchemaVersion*, 100> schemas {
        &accountService,
        &actionInfo,
        &assembly,
        &attributeRegistry,
        &bios,
        &cable,
        &cableCollection,
        &certificate,
        &certificateCollection,
        &certificateLocations,
        &certificateService,
        &chassis,
        &chassisCollection,
        &computerSystem,
        &computerSystemCollection,
        &drive,
        &driveCollection,
        &environmentMetrics,
        &ethernetInterface,
        &ethernetInterfaceCollection,
        &event,
        &eventDestination,
        &eventDestinationCollection,
        &eventService,
        &fan,
        &fanCollection,
        &iPAddresses,
        &jsonSchemaFile,
        &jsonSchemaFileCollection,
        &logEntry,
        &logEntryCollection,
        &logService,
        &logServiceCollection,
        &manager,
        &managerAccount,
        &managerAccountCollection,
        &managerCollection,
        &managerDiagnosticData,
        &managerNetworkProtocol,
        &memory,
        &memoryCollection,
        &message,
        &messageRegistry,
        &messageRegistryCollection,
        &messageRegistryFile,
        &messageRegistryFileCollection,
        &metricDefinition,
        &metricDefinitionCollection,
        &metricReport,
        &metricReportCollection,
        &metricReportDefinition,
        &metricReportDefinitionCollection,
        &operatingConfig,
        &operatingConfigCollection,
        &pCIeDevice,
        &pCIeDeviceCollection,
        &pCIeFunction,
        &pCIeFunctionCollection,
        &pCIeSlots,
        &physicalContext,
        &power,
        &powerSubsystem,
        &powerSupply,
        &powerSupplyCollection,
        &privileges,
        &processor,
        &processorCollection,
        &redfishError,
        &redfishExtensions,
        &redundancy,
        &resource,
        &role,
        &roleCollection,
        &sensor,
        &sensorCollection,
        &serviceRoot,
        &session,
        &sessionCollection,
        &sessionService,
        &settings,
        &softwareInventory,
        &softwareInventoryCollection,
        &storage,
        &storageCollection,
        &storageController,
        &storageControllerCollection,
        &task,
        &taskCollection,
        &taskService,
        &telemetryService,
        &thermal,
        &thermalMetrics,
        &thermalSubsystem,
        &triggers,
        &triggersCollection,
        &updateService,
        &virtualMedia,
        &virtualMediaCollection,
        &vLanNetworkInterface,
        &vLanNetworkInterfaceCollection,
    };
}
