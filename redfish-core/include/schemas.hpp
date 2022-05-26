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
        1,
        11,
        1
    };

    constexpr SchemaVersion actionInfo{
        "ActionInfo",
        1,
        3,
        0
    };

    constexpr SchemaVersion assembly{
        "Assembly",
        1,
        4,
        0
    };

    constexpr SchemaVersion attributeRegistry{
        "AttributeRegistry",
        1,
        3,
        6
    };

    constexpr SchemaVersion bios{
        "Bios",
        1,
        2,
        0
    };

    constexpr SchemaVersion cable{
        "Cable",
        1,
        2,
        0
    };

    constexpr SchemaVersion cableCollection{
        "CableCollection",
        0,
        0,
        0
    };

    constexpr SchemaVersion certificate{
        "Certificate",
        1,
        6,
        0
    };

    constexpr SchemaVersion certificateCollection{
        "CertificateCollection",
        0,
        0,
        0
    };

    constexpr SchemaVersion certificateLocations{
        "CertificateLocations",
        1,
        0,
        2
    };

    constexpr SchemaVersion certificateService{
        "CertificateService",
        1,
        0,
        4
    };

    constexpr SchemaVersion chassis{
        "Chassis",
        1,
        21,
        0
    };

    constexpr SchemaVersion chassisCollection{
        "ChassisCollection",
        0,
        0,
        0
    };

    constexpr SchemaVersion computerSystem{
        "ComputerSystem",
        1,
        19,
        0
    };

    constexpr SchemaVersion computerSystemCollection{
        "ComputerSystemCollection",
        0,
        0,
        0
    };

    constexpr SchemaVersion drive{
        "Drive",
        1,
        15,
        0
    };

    constexpr SchemaVersion driveCollection{
        "DriveCollection",
        0,
        0,
        0
    };

    constexpr SchemaVersion environmentMetrics{
        "EnvironmentMetrics",
        1,
        3,
        0
    };

    constexpr SchemaVersion ethernetInterface{
        "EthernetInterface",
        1,
        9,
        0
    };

    constexpr SchemaVersion ethernetInterfaceCollection{
        "EthernetInterfaceCollection",
        0,
        0,
        0
    };

    constexpr SchemaVersion event{
        "Event",
        1,
        7,
        1
    };

    constexpr SchemaVersion eventDestination{
        "EventDestination",
        1,
        12,
        0
    };

    constexpr SchemaVersion eventDestinationCollection{
        "EventDestinationCollection",
        0,
        0,
        0
    };

    constexpr SchemaVersion eventService{
        "EventService",
        1,
        8,
        0
    };

    constexpr SchemaVersion fan{
        "Fan",
        1,
        3,
        0
    };

    constexpr SchemaVersion fanCollection{
        "FanCollection",
        0,
        0,
        0
    };

    constexpr SchemaVersion iPAddresses{
        "IPAddresses",
        1,
        1,
        3
    };

    constexpr SchemaVersion jsonSchemaFile{
        "JsonSchemaFile",
        1,
        1,
        4
    };

    constexpr SchemaVersion jsonSchemaFileCollection{
        "JsonSchemaFileCollection",
        0,
        0,
        0
    };

    constexpr SchemaVersion logEntry{
        "LogEntry",
        1,
        13,
        0
    };

    constexpr SchemaVersion logEntryCollection{
        "LogEntryCollection",
        0,
        0,
        0
    };

    constexpr SchemaVersion logService{
        "LogService",
        1,
        3,
        1
    };

    constexpr SchemaVersion logServiceCollection{
        "LogServiceCollection",
        0,
        0,
        0
    };

    constexpr SchemaVersion manager{
        "Manager",
        1,
        16,
        0
    };

    constexpr SchemaVersion managerAccount{
        "ManagerAccount",
        1,
        9,
        0
    };

    constexpr SchemaVersion managerAccountCollection{
        "ManagerAccountCollection",
        0,
        0,
        0
    };

    constexpr SchemaVersion managerCollection{
        "ManagerCollection",
        0,
        0,
        0
    };

    constexpr SchemaVersion managerDiagnosticData{
        "ManagerDiagnosticData",
        1,
        1,
        0
    };

    constexpr SchemaVersion managerNetworkProtocol{
        "ManagerNetworkProtocol",
        1,
        9,
        0
    };

    constexpr SchemaVersion memory{
        "Memory",
        1,
        16,
        0
    };

    constexpr SchemaVersion memoryCollection{
        "MemoryCollection",
        0,
        0,
        0
    };

    constexpr SchemaVersion message{
        "Message",
        1,
        1,
        2
    };

    constexpr SchemaVersion messageRegistry{
        "MessageRegistry",
        1,
        5,
        0
    };

    constexpr SchemaVersion messageRegistryCollection{
        "MessageRegistryCollection",
        0,
        0,
        0
    };

    constexpr SchemaVersion messageRegistryFile{
        "MessageRegistryFile",
        1,
        1,
        3
    };

    constexpr SchemaVersion messageRegistryFileCollection{
        "MessageRegistryFileCollection",
        0,
        0,
        0
    };

    constexpr SchemaVersion metricDefinition{
        "MetricDefinition",
        1,
        3,
        1
    };

    constexpr SchemaVersion metricDefinitionCollection{
        "MetricDefinitionCollection",
        0,
        0,
        0
    };

    constexpr SchemaVersion metricReport{
        "MetricReport",
        1,
        5,
        0
    };

    constexpr SchemaVersion metricReportCollection{
        "MetricReportCollection",
        0,
        0,
        0
    };

    constexpr SchemaVersion metricReportDefinition{
        "MetricReportDefinition",
        1,
        4,
        2
    };

    constexpr SchemaVersion metricReportDefinitionCollection{
        "MetricReportDefinitionCollection",
        0,
        0,
        0
    };

    constexpr SchemaVersion operatingConfig{
        "OperatingConfig",
        1,
        0,
        2
    };

    constexpr SchemaVersion operatingConfigCollection{
        "OperatingConfigCollection",
        0,
        0,
        0
    };

    constexpr SchemaVersion pCIeDevice{
        "PCIeDevice",
        1,
        10,
        0
    };

    constexpr SchemaVersion pCIeDeviceCollection{
        "PCIeDeviceCollection",
        0,
        0,
        0
    };

    constexpr SchemaVersion pCIeFunction{
        "PCIeFunction",
        1,
        4,
        0
    };

    constexpr SchemaVersion pCIeFunctionCollection{
        "PCIeFunctionCollection",
        0,
        0,
        0
    };

    constexpr SchemaVersion pCIeSlots{
        "PCIeSlots",
        1,
        5,
        0
    };

    constexpr SchemaVersion physicalContext{
        "PhysicalContext",
        0,
        0,
        0
    };

    constexpr SchemaVersion power{
        "Power",
        1,
        7,
        1
    };

    constexpr SchemaVersion powerSubsystem{
        "PowerSubsystem",
        1,
        1,
        0
    };

    constexpr SchemaVersion powerSupply{
        "PowerSupply",
        1,
        5,
        0
    };

    constexpr SchemaVersion powerSupplyCollection{
        "PowerSupplyCollection",
        0,
        0,
        0
    };

    constexpr SchemaVersion privileges{
        "Privileges",
        1,
        0,
        5
    };

    constexpr SchemaVersion processor{
        "Processor",
        1,
        16,
        0
    };

    constexpr SchemaVersion processorCollection{
        "ProcessorCollection",
        0,
        0,
        0
    };

    constexpr SchemaVersion redfishError{
        "RedfishError",
        1,
        0,
        1
    };

    constexpr SchemaVersion redfishExtensions{
        "RedfishExtensions",
        1,
        0,
        0
    };

    constexpr SchemaVersion redundancy{
        "Redundancy",
        1,
        4,
        1
    };

    constexpr SchemaVersion resource{
        "Resource",
        1,
        14,
        1
    };

    constexpr SchemaVersion role{
        "Role",
        1,
        3,
        1
    };

    constexpr SchemaVersion roleCollection{
        "RoleCollection",
        0,
        0,
        0
    };

    constexpr SchemaVersion sensor{
        "Sensor",
        1,
        6,
        0
    };

    constexpr SchemaVersion sensorCollection{
        "SensorCollection",
        0,
        0,
        0
    };

    constexpr SchemaVersion serviceRoot{
        "ServiceRoot",
        1,
        14,
        0
    };

    constexpr SchemaVersion session{
        "Session",
        1,
        5,
        0
    };

    constexpr SchemaVersion sessionCollection{
        "SessionCollection",
        0,
        0,
        0
    };

    constexpr SchemaVersion sessionService{
        "SessionService",
        1,
        1,
        8
    };

    constexpr SchemaVersion settings{
        "Settings",
        1,
        3,
        5
    };

    constexpr SchemaVersion softwareInventory{
        "SoftwareInventory",
        1,
        8,
        0
    };

    constexpr SchemaVersion softwareInventoryCollection{
        "SoftwareInventoryCollection",
        0,
        0,
        0
    };

    constexpr SchemaVersion storage{
        "Storage",
        1,
        13,
        0
    };

    constexpr SchemaVersion storageCollection{
        "StorageCollection",
        0,
        0,
        0
    };

    constexpr SchemaVersion storageController{
        "StorageController",
        1,
        6,
        0
    };

    constexpr SchemaVersion storageControllerCollection{
        "StorageControllerCollection",
        0,
        0,
        0
    };

    constexpr SchemaVersion task{
        "Task",
        1,
        6,
        1
    };

    constexpr SchemaVersion taskCollection{
        "TaskCollection",
        0,
        0,
        0
    };

    constexpr SchemaVersion taskService{
        "TaskService",
        1,
        2,
        0
    };

    constexpr SchemaVersion telemetryService{
        "TelemetryService",
        1,
        3,
        1
    };

    constexpr SchemaVersion thermal{
        "Thermal",
        1,
        7,
        1
    };

    constexpr SchemaVersion thermalMetrics{
        "ThermalMetrics",
        1,
        0,
        1
    };

    constexpr SchemaVersion thermalSubsystem{
        "ThermalSubsystem",
        1,
        0,
        0
    };

    constexpr SchemaVersion triggers{
        "Triggers",
        1,
        2,
        0
    };

    constexpr SchemaVersion triggersCollection{
        "TriggersCollection",
        0,
        0,
        0
    };

    constexpr SchemaVersion updateService{
        "UpdateService",
        1,
        11,
        1
    };

    constexpr SchemaVersion virtualMedia{
        "VirtualMedia",
        1,
        5,
        1
    };

    constexpr SchemaVersion virtualMediaCollection{
        "VirtualMediaCollection",
        0,
        0,
        0
    };

    constexpr SchemaVersion vLanNetworkInterface{
        "VLanNetworkInterface",
        1,
        3,
        0
    };

    constexpr SchemaVersion vLanNetworkInterfaceCollection{
        "VLanNetworkInterfaceCollection",
        0,
        0,
        0
    };

    constexpr const std::array<const SchemaVersion, 100> schemas {
        accountService,
        actionInfo,
        assembly,
        attributeRegistry,
        bios,
        cable,
        cableCollection,
        certificate,
        certificateCollection,
        certificateLocations,
        certificateService,
        chassis,
        chassisCollection,
        computerSystem,
        computerSystemCollection,
        drive,
        driveCollection,
        environmentMetrics,
        ethernetInterface,
        ethernetInterfaceCollection,
        event,
        eventDestination,
        eventDestinationCollection,
        eventService,
        fan,
        fanCollection,
        iPAddresses,
        jsonSchemaFile,
        jsonSchemaFileCollection,
        logEntry,
        logEntryCollection,
        logService,
        logServiceCollection,
        manager,
        managerAccount,
        managerAccountCollection,
        managerCollection,
        managerDiagnosticData,
        managerNetworkProtocol,
        memory,
        memoryCollection,
        message,
        messageRegistry,
        messageRegistryCollection,
        messageRegistryFile,
        messageRegistryFileCollection,
        metricDefinition,
        metricDefinitionCollection,
        metricReport,
        metricReportCollection,
        metricReportDefinition,
        metricReportDefinitionCollection,
        operatingConfig,
        operatingConfigCollection,
        pCIeDevice,
        pCIeDeviceCollection,
        pCIeFunction,
        pCIeFunctionCollection,
        pCIeSlots,
        physicalContext,
        power,
        powerSubsystem,
        powerSupply,
        powerSupplyCollection,
        privileges,
        processor,
        processorCollection,
        redfishError,
        redfishExtensions,
        redundancy,
        resource,
        role,
        roleCollection,
        sensor,
        sensorCollection,
        serviceRoot,
        session,
        sessionCollection,
        sessionService,
        settings,
        softwareInventory,
        softwareInventoryCollection,
        storage,
        storageCollection,
        storageController,
        storageControllerCollection,
        task,
        taskCollection,
        taskService,
        telemetryService,
        thermal,
        thermalMetrics,
        thermalSubsystem,
        triggers,
        triggersCollection,
        updateService,
        virtualMedia,
        virtualMediaCollection,
        vLanNetworkInterface,
        vLanNetworkInterfaceCollection,
    };
}
