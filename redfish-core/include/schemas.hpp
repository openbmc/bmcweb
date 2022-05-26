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
#include <array>
#include <schema_common.hpp>
// clang-format off

namespace redfish
{
    constexpr SchemaVersion accountServiceType{
        "AccountService",
        "v1_10_0"
    };

    constexpr SchemaVersion actionInfoType{
        "ActionInfo",
        "v1_2_0"
    };

    constexpr SchemaVersion assemblyType{
        "Assembly",
        "v1_3_0"
    };

    constexpr SchemaVersion attributeRegistryType{
        "AttributeRegistry",
        "v1_3_6"
    };

    constexpr SchemaVersion biosType{
        "Bios",
        "v1_2_0"
    };

    constexpr SchemaVersion cableType{
        "Cable",
        "v1_2_0"
    };

    constexpr SchemaVersion cableCollectionType{
        "CableCollection",
        ""
    };

    constexpr SchemaVersion certificateType{
        "Certificate",
        "v1_5_0"
    };

    constexpr SchemaVersion certificateCollectionType{
        "CertificateCollection",
        ""
    };

    constexpr SchemaVersion certificateLocationsType{
        "CertificateLocations",
        "v1_0_2"
    };

    constexpr SchemaVersion certificateServiceType{
        "CertificateService",
        "v1_0_4"
    };

    constexpr SchemaVersion chassisType{
        "Chassis",
        "v1_19_0"
    };

    constexpr SchemaVersion chassisCollectionType{
        "ChassisCollection",
        ""
    };

    constexpr SchemaVersion computerSystemType{
        "ComputerSystem",
        "v1_17_0"
    };

    constexpr SchemaVersion computerSystemCollectionType{
        "ComputerSystemCollection",
        ""
    };

    constexpr SchemaVersion driveType{
        "Drive",
        "v1_14_0"
    };

    constexpr SchemaVersion driveCollectionType{
        "DriveCollection",
        ""
    };

    constexpr SchemaVersion ethernetInterfaceType{
        "EthernetInterface",
        "v1_8_0"
    };

    constexpr SchemaVersion ethernetInterfaceCollectionType{
        "EthernetInterfaceCollection",
        ""
    };

    constexpr SchemaVersion eventType{
        "Event",
        "v1_7_0"
    };

    constexpr SchemaVersion eventDestinationType{
        "EventDestination",
        "v1_11_2"
    };

    constexpr SchemaVersion eventDestinationCollectionType{
        "EventDestinationCollection",
        ""
    };

    constexpr SchemaVersion eventServiceType{
        "EventService",
        "v1_7_2"
    };

    constexpr SchemaVersion iPAddressesType{
        "IPAddresses",
        "v1_1_3"
    };

    constexpr SchemaVersion jsonSchemaFileType{
        "JsonSchemaFile",
        "v1_1_4"
    };

    constexpr SchemaVersion jsonSchemaFileCollectionType{
        "JsonSchemaFileCollection",
        ""
    };

    constexpr SchemaVersion logEntryType{
        "LogEntry",
        "v1_11_0"
    };

    constexpr SchemaVersion logEntryCollectionType{
        "LogEntryCollection",
        ""
    };

    constexpr SchemaVersion logServiceType{
        "LogService",
        "v1_3_0"
    };

    constexpr SchemaVersion logServiceCollectionType{
        "LogServiceCollection",
        ""
    };

    constexpr SchemaVersion managerType{
        "Manager",
        "v1_14_0"
    };

    constexpr SchemaVersion managerAccountType{
        "ManagerAccount",
        "v1_8_1"
    };

    constexpr SchemaVersion managerAccountCollectionType{
        "ManagerAccountCollection",
        ""
    };

    constexpr SchemaVersion managerCollectionType{
        "ManagerCollection",
        ""
    };

    constexpr SchemaVersion managerDiagnosticDataType{
        "ManagerDiagnosticData",
        "v1_0_0"
    };

    constexpr SchemaVersion managerNetworkProtocolType{
        "ManagerNetworkProtocol",
        "v1_8_0"
    };

    constexpr SchemaVersion memoryType{
        "Memory",
        "v1_14_0"
    };

    constexpr SchemaVersion memoryCollectionType{
        "MemoryCollection",
        ""
    };

    constexpr SchemaVersion messageType{
        "Message",
        "v1_1_2"
    };

    constexpr SchemaVersion messageRegistryType{
        "MessageRegistry",
        "v1_5_0"
    };

    constexpr SchemaVersion messageRegistryCollectionType{
        "MessageRegistryCollection",
        ""
    };

    constexpr SchemaVersion messageRegistryFileType{
        "MessageRegistryFile",
        "v1_1_3"
    };

    constexpr SchemaVersion messageRegistryFileCollectionType{
        "MessageRegistryFileCollection",
        ""
    };

    constexpr SchemaVersion metricDefinitionType{
        "MetricDefinition",
        "v1_2_1"
    };

    constexpr SchemaVersion metricDefinitionCollectionType{
        "MetricDefinitionCollection",
        ""
    };

    constexpr SchemaVersion metricReportType{
        "MetricReport",
        "v1_4_2"
    };

    constexpr SchemaVersion metricReportCollectionType{
        "MetricReportCollection",
        ""
    };

    constexpr SchemaVersion metricReportDefinitionType{
        "MetricReportDefinition",
        "v1_4_1"
    };

    constexpr SchemaVersion metricReportDefinitionCollectionType{
        "MetricReportDefinitionCollection",
        ""
    };

    constexpr SchemaVersion operatingConfigType{
        "OperatingConfig",
        "v1_0_2"
    };

    constexpr SchemaVersion operatingConfigCollectionType{
        "OperatingConfigCollection",
        ""
    };

    constexpr SchemaVersion pCIeDeviceType{
        "PCIeDevice",
        "v1_9_0"
    };

    constexpr SchemaVersion pCIeDeviceCollectionType{
        "PCIeDeviceCollection",
        ""
    };

    constexpr SchemaVersion pCIeFunctionType{
        "PCIeFunction",
        "v1_3_0"
    };

    constexpr SchemaVersion pCIeFunctionCollectionType{
        "PCIeFunctionCollection",
        ""
    };

    constexpr SchemaVersion physicalContextType{
        "PhysicalContext",
        ""
    };

    constexpr SchemaVersion powerType{
        "Power",
        "v1_7_1"
    };

    constexpr SchemaVersion privilegesType{
        "Privileges",
        "v1_0_5"
    };

    constexpr SchemaVersion processorType{
        "Processor",
        "v1_14_0"
    };

    constexpr SchemaVersion processorCollectionType{
        "ProcessorCollection",
        ""
    };

    constexpr SchemaVersion redfishErrorType{
        "RedfishError",
        "v1_0_1"
    };

    constexpr SchemaVersion redfishExtensionsType{
        "RedfishExtensions",
        "v1_0_0"
    };

    constexpr SchemaVersion validationType{
        "Validation",
        "v1_0_0"
    };

    constexpr SchemaVersion redundancyType{
        "Redundancy",
        "v1_4_0"
    };

    constexpr SchemaVersion resourceType{
        "Resource",
        "v1_14_0"
    };

    constexpr SchemaVersion roleType{
        "Role",
        "v1_3_1"
    };

    constexpr SchemaVersion roleCollectionType{
        "RoleCollection",
        ""
    };

    constexpr SchemaVersion sensorType{
        "Sensor",
        "v1_5_0"
    };

    constexpr SchemaVersion sensorCollectionType{
        "SensorCollection",
        ""
    };

    constexpr SchemaVersion serviceRootType{
        "ServiceRoot",
        "v1_13_0"
    };

    constexpr SchemaVersion sessionType{
        "Session",
        "v1_3_0"
    };

    constexpr SchemaVersion sessionCollectionType{
        "SessionCollection",
        ""
    };

    constexpr SchemaVersion sessionServiceType{
        "SessionService",
        "v1_1_8"
    };

    constexpr SchemaVersion settingsType{
        "Settings",
        "v1_3_4"
    };

    constexpr SchemaVersion softwareInventoryType{
        "SoftwareInventory",
        "v1_6_0"
    };

    constexpr SchemaVersion softwareInventoryCollectionType{
        "SoftwareInventoryCollection",
        ""
    };

    constexpr SchemaVersion storageType{
        "Storage",
        "v1_12_0"
    };

    constexpr SchemaVersion storageCollectionType{
        "StorageCollection",
        ""
    };

    constexpr SchemaVersion storageControllerType{
        "StorageController",
        "v1_5_0"
    };

    constexpr SchemaVersion storageControllerCollectionType{
        "StorageControllerCollection",
        ""
    };

    constexpr SchemaVersion taskType{
        "Task",
        "v1_5_1"
    };

    constexpr SchemaVersion taskCollectionType{
        "TaskCollection",
        ""
    };

    constexpr SchemaVersion taskServiceType{
        "TaskService",
        "v1_2_0"
    };

    constexpr SchemaVersion telemetryServiceType{
        "TelemetryService",
        "v1_3_1"
    };

    constexpr SchemaVersion thermalType{
        "Thermal",
        "v1_7_1"
    };

    constexpr SchemaVersion triggersType{
        "Triggers",
        "v1_2_0"
    };

    constexpr SchemaVersion triggersCollectionType{
        "TriggersCollection",
        ""
    };

    constexpr SchemaVersion updateServiceType{
        "UpdateService",
        "v1_11_0"
    };

    constexpr SchemaVersion virtualMediaType{
        "VirtualMedia",
        "v1_5_1"
    };

    constexpr SchemaVersion virtualMediaCollectionType{
        "VirtualMediaCollection",
        ""
    };

    constexpr SchemaVersion vLanNetworkInterfaceType{
        "VLanNetworkInterface",
        "v1_3_0"
    };

    constexpr SchemaVersion vLanNetworkInterfaceCollectionType{
        "VLanNetworkInterfaceCollection",
        ""
    };

    constexpr const std::array<const SchemaVersion*, 92> schemas {
        &accountServiceType,
        &actionInfoType,
        &assemblyType,
        &attributeRegistryType,
        &biosType,
        &cableType,
        &cableCollectionType,
        &certificateType,
        &certificateCollectionType,
        &certificateLocationsType,
        &certificateServiceType,
        &chassisType,
        &chassisCollectionType,
        &computerSystemType,
        &computerSystemCollectionType,
        &driveType,
        &driveCollectionType,
        &ethernetInterfaceType,
        &ethernetInterfaceCollectionType,
        &eventType,
        &eventDestinationType,
        &eventDestinationCollectionType,
        &eventServiceType,
        &iPAddressesType,
        &jsonSchemaFileType,
        &jsonSchemaFileCollectionType,
        &logEntryType,
        &logEntryCollectionType,
        &logServiceType,
        &logServiceCollectionType,
        &managerType,
        &managerAccountType,
        &managerAccountCollectionType,
        &managerCollectionType,
        &managerDiagnosticDataType,
        &managerNetworkProtocolType,
        &memoryType,
        &memoryCollectionType,
        &messageType,
        &messageRegistryType,
        &messageRegistryCollectionType,
        &messageRegistryFileType,
        &messageRegistryFileCollectionType,
        &metricDefinitionType,
        &metricDefinitionCollectionType,
        &metricReportType,
        &metricReportCollectionType,
        &metricReportDefinitionType,
        &metricReportDefinitionCollectionType,
        &operatingConfigType,
        &operatingConfigCollectionType,
        &pCIeDeviceType,
        &pCIeDeviceCollectionType,
        &pCIeFunctionType,
        &pCIeFunctionCollectionType,
        &physicalContextType,
        &powerType,
        &privilegesType,
        &processorType,
        &processorCollectionType,
        &redfishErrorType,
        &redfishExtensionsType,
        &validationType,
        &redundancyType,
        &resourceType,
        &roleType,
        &roleCollectionType,
        &sensorType,
        &sensorCollectionType,
        &serviceRootType,
        &sessionType,
        &sessionCollectionType,
        &sessionServiceType,
        &settingsType,
        &softwareInventoryType,
        &softwareInventoryCollectionType,
        &storageType,
        &storageCollectionType,
        &storageControllerType,
        &storageControllerCollectionType,
        &taskType,
        &taskCollectionType,
        &taskServiceType,
        &telemetryServiceType,
        &thermalType,
        &triggersType,
        &triggersCollectionType,
        &updateServiceType,
        &virtualMediaType,
        &virtualMediaCollectionType,
        &vLanNetworkInterfaceType,
        &vLanNetworkInterfaceCollectionType,
    };

}
