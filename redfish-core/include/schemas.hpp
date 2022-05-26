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

namespace redfish
{
    inline constexpr std::array schemas {
        "AccountService",
        "ActionInfo",
        "Assembly",
        "AttributeRegistry",
        "Bios",
        "Cable",
        "Certificate",
        "CertificateLocations",
        "CertificateService",
        "Chassis",
        "ComputerSystem",
        "Drive",
        "EthernetInterface",
        "Event",
        "EventDestination",
        "EventService",
        "IPAddresses",
        "JsonSchemaFile",
        "LogEntry",
        "LogService",
        "Manager",
        "ManagerAccount",
        "ManagerDiagnosticData",
        "ManagerNetworkProtocol",
        "Memory",
        "Message",
        "MessageRegistry",
        "MessageRegistryFile",
        "MetricDefinition",
        "MetricReport",
        "MetricReportDefinition",
        "odata",
        "OperatingConfig",
        "PCIeDevice",
        "PCIeFunction",
        "PhysicalContext",
        "Power",
        "Privileges",
        "Processor",
        "redfish-error",
        "redfish-payload-annotations",
        "redfish-schema",
        "Redundancy",
        "Resource",
        "Role",
        "Sensor",
        "ServiceRoot",
        "Session",
        "SessionService",
        "Settings",
        "SoftwareInventory",
        "Storage",
        "StorageController",
        "Task",
        "TaskService",
        "TelemetryService",
        "Thermal",
        "Triggers",
        "UpdateService",
        "VirtualMedia",
        "VLanNetworkInterface",
    };

    constexpr const char* accountServiceType = "#AccountService.v1_10_0.AccountService";
    constexpr const char* actionInfoType = "#ActionInfo.v1_2_0.ActionInfo";
    constexpr const char* assemblyType = "#Assembly.v1_3_0.Assembly";
    constexpr const char* attributeRegistryType = "#AttributeRegistry.v1_3_6.AttributeRegistry";
    constexpr const char* biosType = "#Bios.v1_2_0.Bios";
    constexpr const char* cableType = "#Cable.v1_2_0.Cable";
    constexpr const char* certificateType = "#Certificate.v1_5_0.Certificate";
    constexpr const char* certificateLocationsType = "#CertificateLocations.v1_0_2.CertificateLocations";
    constexpr const char* certificateServiceType = "#CertificateService.v1_0_4.CertificateService";
    constexpr const char* chassisType = "#Chassis.v1_19_0.Chassis";
    constexpr const char* computerSystemType = "#ComputerSystem.v1_17_0.ComputerSystem";
    constexpr const char* driveType = "#Drive.v1_14_0.Drive";
    constexpr const char* ethernetInterfaceType = "#EthernetInterface.v1_8_0.EthernetInterface";
    constexpr const char* eventType = "#Event.v1_7_0.Event";
    constexpr const char* eventDestinationType = "#EventDestination.v1_11_2.EventDestination";
    constexpr const char* eventServiceType = "#EventService.v1_7_2.EventService";
    constexpr const char* iPAddressesType = "#IPAddresses.v1_1_3.IPAddresses";
    constexpr const char* jsonSchemaFileType = "#JsonSchemaFile.v1_1_4.JsonSchemaFile";
    constexpr const char* logEntryType = "#LogEntry.v1_11_0.LogEntry";
    constexpr const char* logServiceType = "#LogService.v1_3_0.LogService";
    constexpr const char* managerType = "#Manager.v1_14_0.Manager";
    constexpr const char* managerAccountType = "#ManagerAccount.v1_8_1.ManagerAccount";
    constexpr const char* managerDiagnosticDataType = "#ManagerDiagnosticData.v1_0_0.ManagerDiagnosticData";
    constexpr const char* managerNetworkProtocolType = "#ManagerNetworkProtocol.v1_8_0.ManagerNetworkProtocol";
    constexpr const char* memoryType = "#Memory.v1_14_0.Memory";
    constexpr const char* messageType = "#Message.v1_1_2.Message";
    constexpr const char* messageRegistryType = "#MessageRegistry.v1_5_0.MessageRegistry";
    constexpr const char* messageRegistryFileType = "#MessageRegistryFile.v1_1_3.MessageRegistryFile";
    constexpr const char* metricDefinitionType = "#MetricDefinition.v1_2_1.MetricDefinition";
    constexpr const char* metricReportType = "#MetricReport.v1_4_2.MetricReport";
    constexpr const char* metricReportDefinitionType = "#MetricReportDefinition.v1_4_1.MetricReportDefinition";
    constexpr const char* operatingConfigType = "#OperatingConfig.v1_0_2.OperatingConfig";
    constexpr const char* pCIeDeviceType = "#PCIeDevice.v1_9_0.PCIeDevice";
    constexpr const char* pCIeFunctionType = "#PCIeFunction.v1_3_0.PCIeFunction";
    constexpr const char* powerType = "#Power.v1_7_1.Power";
    constexpr const char* privilegesType = "#Privileges.v1_0_5.Privileges";
    constexpr const char* processorType = "#Processor.v1_14_0.Processor";
    constexpr const char* redfishErrorType = "#RedfishError.v1_0_1.RedfishError";
    constexpr const char* redfishExtensionsType = "#RedfishExtensions.v1_0_0.RedfishExtensions";
    constexpr const char* validationType = "#Validation.v1_0_0.Validation";
    constexpr const char* redundancyType = "#Redundancy.v1_4_0.Redundancy";
    constexpr const char* resourceType = "#Resource.v1_14_0.Resource";
    constexpr const char* roleType = "#Role.v1_3_1.Role";
    constexpr const char* sensorType = "#Sensor.v1_5_0.Sensor";
    constexpr const char* serviceRootType = "#ServiceRoot.v1_13_0.ServiceRoot";
    constexpr const char* sessionType = "#Session.v1_3_0.Session";
    constexpr const char* sessionServiceType = "#SessionService.v1_1_8.SessionService";
    constexpr const char* settingsType = "#Settings.v1_3_4.Settings";
    constexpr const char* softwareInventoryType = "#SoftwareInventory.v1_6_0.SoftwareInventory";
    constexpr const char* storageType = "#Storage.v1_12_0.Storage";
    constexpr const char* storageControllerType = "#StorageController.v1_5_0.StorageController";
    constexpr const char* taskType = "#Task.v1_5_1.Task";
    constexpr const char* taskServiceType = "#TaskService.v1_2_0.TaskService";
    constexpr const char* telemetryServiceType = "#TelemetryService.v1_3_1.TelemetryService";
    constexpr const char* thermalType = "#Thermal.v1_7_1.Thermal";
    constexpr const char* triggersType = "#Triggers.v1_2_0.Triggers";
    constexpr const char* updateServiceType = "#UpdateService.v1_11_0.UpdateService";
    constexpr const char* virtualMediaType = "#VirtualMedia.v1_5_1.VirtualMedia";
    constexpr const char* vLanNetworkInterfaceType = "#VLanNetworkInterface.v1_3_0.VLanNetworkInterface";
}
