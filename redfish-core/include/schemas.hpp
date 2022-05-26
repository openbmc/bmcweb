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
#include "schema_common.hpp"

#include <array>

namespace redfish::schemas
{
    constexpr SchemaVersion accelerationFunction{
        "AccelerationFunction",
        1,
        0,
        3,
        false
    };

    constexpr SchemaVersion accelerationFunctionCollection{
        "AccelerationFunctionCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion accountService{
        "AccountService",
        1,
        11,
        1,
        false
    };

    constexpr SchemaVersion actionInfo{
        "ActionInfo",
        1,
        3,
        0,
        false
    };

    constexpr SchemaVersion addressPool{
        "AddressPool",
        1,
        2,
        1,
        false
    };

    constexpr SchemaVersion addressPoolCollection{
        "AddressPoolCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion aggregate{
        "Aggregate",
        1,
        0,
        1,
        false
    };

    constexpr SchemaVersion aggregateCollection{
        "AggregateCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion aggregationService{
        "AggregationService",
        1,
        0,
        1,
        false
    };

    constexpr SchemaVersion aggregationSource{
        "AggregationSource",
        1,
        2,
        0,
        false
    };

    constexpr SchemaVersion aggregationSourceCollection{
        "AggregationSourceCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion allowDeny{
        "AllowDeny",
        1,
        0,
        0,
        false
    };

    constexpr SchemaVersion allowDenyCollection{
        "AllowDenyCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion assembly{
        "Assembly",
        1,
        4,
        0,
        false
    };

    constexpr SchemaVersion attributeRegistry{
        "AttributeRegistry",
        1,
        3,
        6,
        false
    };

    constexpr SchemaVersion battery{
        "Battery",
        1,
        2,
        0,
        false
    };

    constexpr SchemaVersion batteryCollection{
        "BatteryCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion batteryMetrics{
        "BatteryMetrics",
        1,
        0,
        1,
        false
    };

    constexpr SchemaVersion bios{
        "Bios",
        1,
        2,
        0,
        false
    };

    constexpr SchemaVersion bootOption{
        "BootOption",
        1,
        0,
        4,
        false
    };

    constexpr SchemaVersion bootOptionCollection{
        "BootOptionCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion cable{
        "Cable",
        1,
        2,
        0,
        false
    };

    constexpr SchemaVersion cableCollection{
        "CableCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion certificate{
        "Certificate",
        1,
        6,
        0,
        false
    };

    constexpr SchemaVersion certificateCollection{
        "CertificateCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion certificateLocations{
        "CertificateLocations",
        1,
        0,
        2,
        false
    };

    constexpr SchemaVersion certificateService{
        "CertificateService",
        1,
        0,
        4,
        false
    };

    constexpr SchemaVersion chassis{
        "Chassis",
        1,
        21,
        0,
        false
    };

    constexpr SchemaVersion chassisCollection{
        "ChassisCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion circuit{
        "Circuit",
        1,
        7,
        0,
        false
    };

    constexpr SchemaVersion circuitCollection{
        "CircuitCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion collectionCapabilities{
        "CollectionCapabilities",
        1,
        4,
        0,
        false
    };

    constexpr SchemaVersion componentIntegrity{
        "ComponentIntegrity",
        1,
        2,
        0,
        false
    };

    constexpr SchemaVersion componentIntegrityCollection{
        "ComponentIntegrityCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion compositionReservation{
        "CompositionReservation",
        1,
        0,
        0,
        false
    };

    constexpr SchemaVersion compositionReservationCollection{
        "CompositionReservationCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion compositionService{
        "CompositionService",
        1,
        2,
        0,
        false
    };

    constexpr SchemaVersion computerSystem{
        "ComputerSystem",
        1,
        19,
        0,
        false
    };

    constexpr SchemaVersion computerSystemCollection{
        "ComputerSystemCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion connection{
        "Connection",
        1,
        1,
        0,
        false
    };

    constexpr SchemaVersion connectionCollection{
        "ConnectionCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion connectionMethod{
        "ConnectionMethod",
        1,
        0,
        0,
        false
    };

    constexpr SchemaVersion connectionMethodCollection{
        "ConnectionMethodCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion control{
        "Control",
        1,
        2,
        0,
        false
    };

    constexpr SchemaVersion controlCollection{
        "ControlCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion drive{
        "Drive",
        1,
        15,
        0,
        false
    };

    constexpr SchemaVersion driveCollection{
        "DriveCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion endpoint{
        "Endpoint",
        1,
        7,
        0,
        false
    };

    constexpr SchemaVersion endpointCollection{
        "EndpointCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion endpointGroup{
        "EndpointGroup",
        1,
        3,
        2,
        false
    };

    constexpr SchemaVersion endpointGroupCollection{
        "EndpointGroupCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion environmentMetrics{
        "EnvironmentMetrics",
        1,
        3,
        0,
        false
    };

    constexpr SchemaVersion ethernetInterface{
        "EthernetInterface",
        1,
        9,
        0,
        false
    };

    constexpr SchemaVersion ethernetInterfaceCollection{
        "EthernetInterfaceCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion event{
        "Event",
        1,
        7,
        1,
        false
    };

    constexpr SchemaVersion eventDestination{
        "EventDestination",
        1,
        12,
        0,
        false
    };

    constexpr SchemaVersion eventDestinationCollection{
        "EventDestinationCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion eventService{
        "EventService",
        1,
        8,
        0,
        false
    };

    constexpr SchemaVersion externalAccountProvider{
        "ExternalAccountProvider",
        1,
        4,
        1,
        false
    };

    constexpr SchemaVersion externalAccountProviderCollection{
        "ExternalAccountProviderCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion fabric{
        "Fabric",
        1,
        3,
        0,
        false
    };

    constexpr SchemaVersion fabricAdapter{
        "FabricAdapter",
        1,
        4,
        0,
        false
    };

    constexpr SchemaVersion fabricAdapterCollection{
        "FabricAdapterCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion fabricCollection{
        "FabricCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion facility{
        "Facility",
        1,
        3,
        0,
        false
    };

    constexpr SchemaVersion facilityCollection{
        "FacilityCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion fan{
        "Fan",
        1,
        3,
        0,
        false
    };

    constexpr SchemaVersion fanCollection{
        "FanCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion graphicsController{
        "GraphicsController",
        1,
        0,
        0,
        false
    };

    constexpr SchemaVersion graphicsControllerCollection{
        "GraphicsControllerCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion hostInterface{
        "HostInterface",
        1,
        3,
        0,
        false
    };

    constexpr SchemaVersion hostInterfaceCollection{
        "HostInterfaceCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion iPAddresses{
        "IPAddresses",
        1,
        1,
        3,
        false
    };

    constexpr SchemaVersion job{
        "Job",
        1,
        1,
        1,
        false
    };

    constexpr SchemaVersion jobCollection{
        "JobCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion jobService{
        "JobService",
        1,
        0,
        4,
        false
    };

    constexpr SchemaVersion jsonSchemaFile{
        "JsonSchemaFile",
        1,
        1,
        4,
        false
    };

    constexpr SchemaVersion jsonSchemaFileCollection{
        "JsonSchemaFileCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion key{
        "Key",
        1,
        1,
        0,
        false
    };

    constexpr SchemaVersion keyCollection{
        "KeyCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion keyPolicy{
        "KeyPolicy",
        1,
        0,
        0,
        false
    };

    constexpr SchemaVersion keyPolicyCollection{
        "KeyPolicyCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion keyService{
        "KeyService",
        1,
        0,
        0,
        false
    };

    constexpr SchemaVersion license{
        "License",
        1,
        0,
        0,
        false
    };

    constexpr SchemaVersion licenseCollection{
        "LicenseCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion licenseService{
        "LicenseService",
        1,
        0,
        0,
        false
    };

    constexpr SchemaVersion logEntry{
        "LogEntry",
        1,
        13,
        0,
        false
    };

    constexpr SchemaVersion logEntryCollection{
        "LogEntryCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion logService{
        "LogService",
        1,
        3,
        1,
        false
    };

    constexpr SchemaVersion logServiceCollection{
        "LogServiceCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion manager{
        "Manager",
        1,
        16,
        0,
        false
    };

    constexpr SchemaVersion managerAccount{
        "ManagerAccount",
        1,
        9,
        0,
        false
    };

    constexpr SchemaVersion managerAccountCollection{
        "ManagerAccountCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion managerCollection{
        "ManagerCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion managerDiagnosticData{
        "ManagerDiagnosticData",
        1,
        1,
        0,
        false
    };

    constexpr SchemaVersion managerNetworkProtocol{
        "ManagerNetworkProtocol",
        1,
        9,
        0,
        false
    };

    constexpr SchemaVersion manifest{
        "Manifest",
        1,
        1,
        0,
        false
    };

    constexpr SchemaVersion mediaController{
        "MediaController",
        1,
        3,
        0,
        false
    };

    constexpr SchemaVersion mediaControllerCollection{
        "MediaControllerCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion memory{
        "Memory",
        1,
        16,
        0,
        false
    };

    constexpr SchemaVersion memoryChunks{
        "MemoryChunks",
        1,
        4,
        2,
        false
    };

    constexpr SchemaVersion memoryChunksCollection{
        "MemoryChunksCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion memoryCollection{
        "MemoryCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion memoryDomain{
        "MemoryDomain",
        1,
        4,
        0,
        false
    };

    constexpr SchemaVersion memoryDomainCollection{
        "MemoryDomainCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion memoryMetrics{
        "MemoryMetrics",
        1,
        5,
        0,
        false
    };

    constexpr SchemaVersion message{
        "Message",
        1,
        1,
        2,
        false
    };

    constexpr SchemaVersion messageRegistry{
        "MessageRegistry",
        1,
        5,
        0,
        false
    };

    constexpr SchemaVersion messageRegistryCollection{
        "MessageRegistryCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion messageRegistryFile{
        "MessageRegistryFile",
        1,
        1,
        3,
        false
    };

    constexpr SchemaVersion messageRegistryFileCollection{
        "MessageRegistryFileCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion metricDefinition{
        "MetricDefinition",
        1,
        3,
        1,
        false
    };

    constexpr SchemaVersion metricDefinitionCollection{
        "MetricDefinitionCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion metricReport{
        "MetricReport",
        1,
        5,
        0,
        false
    };

    constexpr SchemaVersion metricReportCollection{
        "MetricReportCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion metricReportDefinition{
        "MetricReportDefinition",
        1,
        4,
        2,
        false
    };

    constexpr SchemaVersion metricReportDefinitionCollection{
        "MetricReportDefinitionCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion networkAdapter{
        "NetworkAdapter",
        1,
        9,
        0,
        false
    };

    constexpr SchemaVersion networkAdapterCollection{
        "NetworkAdapterCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion networkAdapterMetrics{
        "NetworkAdapterMetrics",
        1,
        0,
        0,
        false
    };

    constexpr SchemaVersion networkDeviceFunction{
        "NetworkDeviceFunction",
        1,
        9,
        0,
        false
    };

    constexpr SchemaVersion networkDeviceFunctionCollection{
        "NetworkDeviceFunctionCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion networkDeviceFunctionMetrics{
        "NetworkDeviceFunctionMetrics",
        1,
        1,
        0,
        false
    };

    constexpr SchemaVersion networkInterface{
        "NetworkInterface",
        1,
        2,
        1,
        false
    };

    constexpr SchemaVersion networkInterfaceCollection{
        "NetworkInterfaceCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion networkPort{
        "NetworkPort",
        1,
        4,
        1,
        false
    };

    constexpr SchemaVersion networkPortCollection{
        "NetworkPortCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion operatingConfig{
        "OperatingConfig",
        1,
        0,
        2,
        false
    };

    constexpr SchemaVersion operatingConfigCollection{
        "OperatingConfigCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion outlet{
        "Outlet",
        1,
        4,
        0,
        false
    };

    constexpr SchemaVersion outletCollection{
        "OutletCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion outletGroup{
        "OutletGroup",
        1,
        1,
        0,
        false
    };

    constexpr SchemaVersion outletGroupCollection{
        "OutletGroupCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion pCIeDevice{
        "PCIeDevice",
        1,
        10,
        0,
        false
    };

    constexpr SchemaVersion pCIeDeviceCollection{
        "PCIeDeviceCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion pCIeFunction{
        "PCIeFunction",
        1,
        4,
        0,
        false
    };

    constexpr SchemaVersion pCIeFunctionCollection{
        "PCIeFunctionCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion pCIeSlots{
        "PCIeSlots",
        1,
        5,
        0,
        false
    };

    constexpr SchemaVersion physicalContext{
        "PhysicalContext",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion port{
        "Port",
        1,
        7,
        0,
        false
    };

    constexpr SchemaVersion portCollection{
        "PortCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion portMetrics{
        "PortMetrics",
        1,
        3,
        0,
        false
    };

    constexpr SchemaVersion power{
        "Power",
        1,
        7,
        1,
        false
    };

    constexpr SchemaVersion powerDistribution{
        "PowerDistribution",
        1,
        2,
        2,
        false
    };

    constexpr SchemaVersion powerDistributionCollection{
        "PowerDistributionCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion powerDistributionMetrics{
        "PowerDistributionMetrics",
        1,
        3,
        0,
        false
    };

    constexpr SchemaVersion powerDomain{
        "PowerDomain",
        1,
        2,
        0,
        false
    };

    constexpr SchemaVersion powerDomainCollection{
        "PowerDomainCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion powerEquipment{
        "PowerEquipment",
        1,
        2,
        0,
        false
    };

    constexpr SchemaVersion powerSubsystem{
        "PowerSubsystem",
        1,
        1,
        0,
        false
    };

    constexpr SchemaVersion powerSupply{
        "PowerSupply",
        1,
        5,
        0,
        false
    };

    constexpr SchemaVersion powerSupplyCollection{
        "PowerSupplyCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion powerSupplyMetrics{
        "PowerSupplyMetrics",
        1,
        0,
        1,
        false
    };

    constexpr SchemaVersion privilegeRegistry{
        "PrivilegeRegistry",
        1,
        1,
        4,
        false
    };

    constexpr SchemaVersion privileges{
        "Privileges",
        1,
        0,
        5,
        false
    };

    constexpr SchemaVersion processor{
        "Processor",
        1,
        16,
        0,
        false
    };

    constexpr SchemaVersion processorCollection{
        "ProcessorCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion processorMetrics{
        "ProcessorMetrics",
        1,
        6,
        0,
        false
    };

    constexpr SchemaVersion protocol{
        "Protocol",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion redfishError{
        "RedfishError",
        1,
        0,
        1,
        false
    };

    constexpr SchemaVersion redfishExtensions{
        "RedfishExtensions",
        1,
        0,
        0,
        false
    };

    constexpr SchemaVersion validation{
        "Validation",
        1,
        0,
        0,
        false
    };

    constexpr SchemaVersion redundancy{
        "Redundancy",
        1,
        4,
        1,
        false
    };

    constexpr SchemaVersion registeredClient{
        "RegisteredClient",
        1,
        0,
        0,
        false
    };

    constexpr SchemaVersion registeredClientCollection{
        "RegisteredClientCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion resource{
        "Resource",
        1,
        14,
        1,
        false
    };

    constexpr SchemaVersion resourceBlock{
        "ResourceBlock",
        1,
        4,
        0,
        false
    };

    constexpr SchemaVersion resourceBlockCollection{
        "ResourceBlockCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion role{
        "Role",
        1,
        3,
        1,
        false
    };

    constexpr SchemaVersion roleCollection{
        "RoleCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion routeEntry{
        "RouteEntry",
        1,
        0,
        1,
        false
    };

    constexpr SchemaVersion routeEntryCollection{
        "RouteEntryCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion routeSetEntry{
        "RouteSetEntry",
        1,
        0,
        1,
        false
    };

    constexpr SchemaVersion routeSetEntryCollection{
        "RouteSetEntryCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion schedule{
        "Schedule",
        1,
        2,
        3,
        false
    };

    constexpr SchemaVersion secureBoot{
        "SecureBoot",
        1,
        1,
        0,
        false
    };

    constexpr SchemaVersion secureBootDatabase{
        "SecureBootDatabase",
        1,
        0,
        1,
        false
    };

    constexpr SchemaVersion secureBootDatabaseCollection{
        "SecureBootDatabaseCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion securityPolicy{
        "SecurityPolicy",
        1,
        0,
        0,
        false
    };

    constexpr SchemaVersion sensor{
        "Sensor",
        1,
        6,
        0,
        false
    };

    constexpr SchemaVersion sensorCollection{
        "SensorCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion serialInterface{
        "SerialInterface",
        1,
        1,
        8,
        false
    };

    constexpr SchemaVersion serialInterfaceCollection{
        "SerialInterfaceCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion serviceConditions{
        "ServiceConditions",
        1,
        0,
        0,
        false
    };

    constexpr SchemaVersion serviceRoot{
        "ServiceRoot",
        1,
        14,
        0,
        false
    };

    constexpr SchemaVersion session{
        "Session",
        1,
        5,
        0,
        false
    };

    constexpr SchemaVersion sessionCollection{
        "SessionCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion sessionService{
        "SessionService",
        1,
        1,
        8,
        false
    };

    constexpr SchemaVersion settings{
        "Settings",
        1,
        3,
        5,
        false
    };

    constexpr SchemaVersion signature{
        "Signature",
        1,
        0,
        2,
        false
    };

    constexpr SchemaVersion signatureCollection{
        "SignatureCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion simpleStorage{
        "SimpleStorage",
        1,
        3,
        1,
        false
    };

    constexpr SchemaVersion simpleStorageCollection{
        "SimpleStorageCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion softwareInventory{
        "SoftwareInventory",
        1,
        8,
        0,
        false
    };

    constexpr SchemaVersion softwareInventoryCollection{
        "SoftwareInventoryCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion storage{
        "Storage",
        1,
        13,
        0,
        false
    };

    constexpr SchemaVersion storageCollection{
        "StorageCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion storageController{
        "StorageController",
        1,
        6,
        0,
        false
    };

    constexpr SchemaVersion storageControllerCollection{
        "StorageControllerCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion switchSchema{
        "Switch",
        1,
        8,
        0,
        false
    };

    constexpr SchemaVersion switchCollection{
        "SwitchCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion switchMetrics{
        "SwitchMetrics",
        1,
        0,
        0,
        false
    };

    constexpr SchemaVersion task{
        "Task",
        1,
        6,
        1,
        false
    };

    constexpr SchemaVersion taskCollection{
        "TaskCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion taskService{
        "TaskService",
        1,
        2,
        0,
        false
    };

    constexpr SchemaVersion telemetryService{
        "TelemetryService",
        1,
        3,
        1,
        false
    };

    constexpr SchemaVersion thermal{
        "Thermal",
        1,
        7,
        1,
        false
    };

    constexpr SchemaVersion thermalMetrics{
        "ThermalMetrics",
        1,
        0,
        1,
        false
    };

    constexpr SchemaVersion thermalSubsystem{
        "ThermalSubsystem",
        1,
        0,
        0,
        false
    };

    constexpr SchemaVersion triggers{
        "Triggers",
        1,
        2,
        0,
        false
    };

    constexpr SchemaVersion triggersCollection{
        "TriggersCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion trustedComponent{
        "TrustedComponent",
        1,
        0,
        0,
        false
    };

    constexpr SchemaVersion trustedComponentCollection{
        "TrustedComponentCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion updateService{
        "UpdateService",
        1,
        11,
        1,
        false
    };

    constexpr SchemaVersion uSBController{
        "USBController",
        1,
        0,
        0,
        false
    };

    constexpr SchemaVersion uSBControllerCollection{
        "USBControllerCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion vCATEntry{
        "VCATEntry",
        1,
        0,
        1,
        false
    };

    constexpr SchemaVersion vCATEntryCollection{
        "VCATEntryCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion virtualMedia{
        "VirtualMedia",
        1,
        5,
        1,
        false
    };

    constexpr SchemaVersion virtualMediaCollection{
        "VirtualMediaCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion vLanNetworkInterface{
        "VLanNetworkInterface",
        1,
        3,
        0,
        false
    };

    constexpr SchemaVersion vLanNetworkInterfaceCollection{
        "VLanNetworkInterfaceCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion volume{
        "Volume",
        1,
        8,
        0,
        false
    };

    constexpr SchemaVersion volumeCollection{
        "VolumeCollection",
        0,
        0,
        0,
        false
    };

    constexpr SchemaVersion zone{
        "Zone",
        1,
        6,
        1,
        false
    };

    constexpr SchemaVersion zoneCollection{
        "ZoneCollection",
        0,
        0,
        0,
        false
    };

    constexpr const std::array<const SchemaVersion, 226> schemas {
        accelerationFunction,
        accelerationFunctionCollection,
        accountService,
        actionInfo,
        addressPool,
        addressPoolCollection,
        aggregate,
        aggregateCollection,
        aggregationService,
        aggregationSource,
        aggregationSourceCollection,
        allowDeny,
        allowDenyCollection,
        assembly,
        attributeRegistry,
        battery,
        batteryCollection,
        batteryMetrics,
        bios,
        bootOption,
        bootOptionCollection,
        cable,
        cableCollection,
        certificate,
        certificateCollection,
        certificateLocations,
        certificateService,
        chassis,
        chassisCollection,
        circuit,
        circuitCollection,
        collectionCapabilities,
        componentIntegrity,
        componentIntegrityCollection,
        compositionReservation,
        compositionReservationCollection,
        compositionService,
        computerSystem,
        computerSystemCollection,
        connection,
        connectionCollection,
        connectionMethod,
        connectionMethodCollection,
        control,
        controlCollection,
        drive,
        driveCollection,
        endpoint,
        endpointCollection,
        endpointGroup,
        endpointGroupCollection,
        environmentMetrics,
        ethernetInterface,
        ethernetInterfaceCollection,
        event,
        eventDestination,
        eventDestinationCollection,
        eventService,
        externalAccountProvider,
        externalAccountProviderCollection,
        fabric,
        fabricAdapter,
        fabricAdapterCollection,
        fabricCollection,
        facility,
        facilityCollection,
        fan,
        fanCollection,
        graphicsController,
        graphicsControllerCollection,
        hostInterface,
        hostInterfaceCollection,
        iPAddresses,
        job,
        jobCollection,
        jobService,
        jsonSchemaFile,
        jsonSchemaFileCollection,
        key,
        keyCollection,
        keyPolicy,
        keyPolicyCollection,
        keyService,
        license,
        licenseCollection,
        licenseService,
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
        manifest,
        mediaController,
        mediaControllerCollection,
        memory,
        memoryChunks,
        memoryChunksCollection,
        memoryCollection,
        memoryDomain,
        memoryDomainCollection,
        memoryMetrics,
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
        networkAdapter,
        networkAdapterCollection,
        networkAdapterMetrics,
        networkDeviceFunction,
        networkDeviceFunctionCollection,
        networkDeviceFunctionMetrics,
        networkInterface,
        networkInterfaceCollection,
        networkPort,
        networkPortCollection,
        operatingConfig,
        operatingConfigCollection,
        outlet,
        outletCollection,
        outletGroup,
        outletGroupCollection,
        pCIeDevice,
        pCIeDeviceCollection,
        pCIeFunction,
        pCIeFunctionCollection,
        pCIeSlots,
        physicalContext,
        port,
        portCollection,
        portMetrics,
        power,
        powerDistribution,
        powerDistributionCollection,
        powerDistributionMetrics,
        powerDomain,
        powerDomainCollection,
        powerEquipment,
        powerSubsystem,
        powerSupply,
        powerSupplyCollection,
        powerSupplyMetrics,
        privilegeRegistry,
        privileges,
        processor,
        processorCollection,
        processorMetrics,
        protocol,
        redfishError,
        redfishExtensions,
        validation,
        redundancy,
        registeredClient,
        registeredClientCollection,
        resource,
        resourceBlock,
        resourceBlockCollection,
        role,
        roleCollection,
        routeEntry,
        routeEntryCollection,
        routeSetEntry,
        routeSetEntryCollection,
        schedule,
        secureBoot,
        secureBootDatabase,
        secureBootDatabaseCollection,
        securityPolicy,
        sensor,
        sensorCollection,
        serialInterface,
        serialInterfaceCollection,
        serviceConditions,
        serviceRoot,
        session,
        sessionCollection,
        sessionService,
        settings,
        signature,
        signatureCollection,
        simpleStorage,
        simpleStorageCollection,
        softwareInventory,
        softwareInventoryCollection,
        storage,
        storageCollection,
        storageController,
        storageControllerCollection,
        switchSchema,
        switchCollection,
        switchMetrics,
        task,
        taskCollection,
        taskService,
        telemetryService,
        thermal,
        thermalMetrics,
        thermalSubsystem,
        triggers,
        triggersCollection,
        trustedComponent,
        trustedComponentCollection,
        updateService,
        uSBController,
        uSBControllerCollection,
        vCATEntry,
        vCATEntryCollection,
        virtualMedia,
        virtualMediaCollection,
        vLanNetworkInterface,
        vLanNetworkInterfaceCollection,
        volume,
        volumeCollection,
        zone,
        zoneCollection,
    };
}
