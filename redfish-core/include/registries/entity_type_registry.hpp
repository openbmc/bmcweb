#pragma once
/****************************************************************
 *                 READ THIS WARNING FIRST
 * This is an auto-generated header which contains definitions
 * for Redfish DMTF defined messages.
 * DO NOT modify this registry outside of running the
 * generate_odatatype_registry script.  The definitions contained within
 * this file are owned by DMTF.  Any modifications to these files
 * should be first pushed to the relevant registry in the DMTF
 * github organization.
 ***************************************************************/

#include <array>
#include <string_view>

// clang-format off

namespace redfish
{
const std::array<std::string_view, 0> AccelerationFunctionUris = {{
}};

const std::array<std::string_view, 0> AccelerationFunctionCollectionUris = {{
}};

const std::array<std::string_view, 2> AccountServiceUris = {{
    "/redfish/v1/AccountService/",
    "/redfish/v1/AccountService/PrivilegeMap",
}};

const std::array<std::string_view, 5> ActionInfoUris = {{
    "/redfish/v1/Systems/system/ResetActionInfo/",
    "/redfish/v1/Chassis/<str>/ResetActionInfo/",
    "/redfish/v1/Systems/hypervisor/ResetActionInfo/",
    "/redfish/v1/Managers/bmc/ResetActionInfo/",
    "/redfish/v1/Systems/<str>/ResetActionInfo/",
}};

const std::array<std::string_view, 0> AddressPoolUris = {{
}};

const std::array<std::string_view, 0> AddressPoolCollectionUris = {{
}};

const std::array<std::string_view, 0> AggregateUris = {{
}};

const std::array<std::string_view, 0> AggregateCollectionUris = {{
}};

const std::array<std::string_view, 0> AggregationServiceUris = {{
}};

const std::array<std::string_view, 0> AggregationSourceUris = {{
}};

const std::array<std::string_view, 0> AggregationSourceCollectionUris = {{
}};

const std::array<std::string_view, 0> AllowDenyUris = {{
}};

const std::array<std::string_view, 0> AllowDenyCollectionUris = {{
}};

const std::array<std::string_view, 0> AssemblyUris = {{
}};

const std::array<std::string_view, 0> BatteryUris = {{
}};

const std::array<std::string_view, 0> BatteryCollectionUris = {{
}};

const std::array<std::string_view, 0> BatteryMetricsUris = {{
}};

const std::array<std::string_view, 1> BiosUris = {{
    "/redfish/v1/Systems/<str>/Bios/",
}};

const std::array<std::string_view, 0> BootOptionUris = {{
}};

const std::array<std::string_view, 0> BootOptionCollectionUris = {{
}};

const std::array<std::string_view, 1> CableUris = {{
    "/redfish/v1/Cables/<str>/",
}};

const std::array<std::string_view, 1> CableCollectionUris = {{
    "/redfish/v1/Cables/",
}};

const std::array<std::string_view, 4> CertificateUris = {{
    "/redfish/v1/Managers/bmc/Truststore/Certificates/<str>/",
    "/redfish/v1/AccountService/LDAP/Certificates/<str>/",
    "/redfish/v1/Managers/bmc/NetworkProtocol/HTTPS/Certificates/<str>/",
    "/redfish/v1/Managers/bmc/Truststore/Certificates/",
}};

const std::array<std::string_view, 3> CertificateCollectionUris = {{
    "/redfish/v1/Managers/bmc/NetworkProtocol/HTTPS/Certificates/",
    "/redfish/v1/AccountService/LDAP/Certificates/",
    "/redfish/v1/Managers/bmc/Truststore/Certificates/",
}};

const std::array<std::string_view, 1> CertificateLocationsUris = {{
    "/redfish/v1/CertificateService/CertificateLocations/",
}};

const std::array<std::string_view, 1> CertificateServiceUris = {{
    "/redfish/v1/CertificateService/",
}};

const std::array<std::string_view, 2> ChassisUris = {{
    "/redfish/v1/Chassis/<str>/Drives/<str>/",
    "/redfish/v1/Chassis/<str>/",
}};

const std::array<std::string_view, 1> ChassisCollectionUris = {{
    "/redfish/v1/Chassis/",
}};

const std::array<std::string_view, 0> CircuitUris = {{
}};

const std::array<std::string_view, 0> CircuitCollectionUris = {{
}};

const std::array<std::string_view, 0> CompositionReservationUris = {{
}};

const std::array<std::string_view, 0> CompositionReservationCollectionUris = {{
}};

const std::array<std::string_view, 0> CompositionServiceUris = {{
}};

const std::array<std::string_view, 3> ComputerSystemUris = {{
    "/redfish/v1/Systems/<str>/",
    "/redfish/v1/Systems/system/",
    "/redfish/v1/Systems/hypervisor/",
}};

const std::array<std::string_view, 1> ComputerSystemCollectionUris = {{
    "/redfish/v1/Systems/",
}};

const std::array<std::string_view, 0> ConnectionUris = {{
}};

const std::array<std::string_view, 0> ConnectionCollectionUris = {{
}};

const std::array<std::string_view, 0> ConnectionMethodUris = {{
}};

const std::array<std::string_view, 0> ConnectionMethodCollectionUris = {{
}};

const std::array<std::string_view, 0> ControlUris = {{
}};

const std::array<std::string_view, 0> ControlCollectionUris = {{
}};

const std::array<std::string_view, 1> DriveUris = {{
    "/redfish/v1/Systems/<str>/Storage/1/Drives/<str>/",
}};

const std::array<std::string_view, 1> DriveCollectionUris = {{
    "/redfish/v1/Chassis/<str>/Drives/",
}};

const std::array<std::string_view, 0> EndpointUris = {{
}};

const std::array<std::string_view, 0> EndpointCollectionUris = {{
}};

const std::array<std::string_view, 0> EndpointGroupUris = {{
}};

const std::array<std::string_view, 0> EndpointGroupCollectionUris = {{
}};

const std::array<std::string_view, 1> EnvironmentMetricsUris = {{
    "/redfish/v1/Chassis/<str>/EnvironmentMetrics/",
}};

const std::array<std::string_view, 2> EthernetInterfaceUris = {{
    "/redfish/v1/Managers/bmc/EthernetInterfaces/<str>/",
    "/redfish/v1/Systems/hypervisor/EthernetInterfaces/<str>/",
}};

const std::array<std::string_view, 2> EthernetInterfaceCollectionUris = {{
    "/redfish/v1/Managers/bmc/EthernetInterfaces/",
    "/redfish/v1/Systems/hypervisor/EthernetInterfaces/",
}};

const std::array<std::string_view, 1> EventDestinationUris = {{
    "/redfish/v1/EventService/Subscriptions/<str>/",
}};

const std::array<std::string_view, 1> EventDestinationCollectionUris = {{
    "/redfish/v1/EventService/Subscriptions/",
}};

const std::array<std::string_view, 1> EventServiceUris = {{
    "/redfish/v1/EventService/",
}};

const std::array<std::string_view, 0> ExternalAccountProviderUris = {{
}};

const std::array<std::string_view, 0> ExternalAccountProviderCollectionUris = {{
}};

const std::array<std::string_view, 0> FabricUris = {{
}};

const std::array<std::string_view, 0> FabricCollectionUris = {{
}};

const std::array<std::string_view, 0> FabricAdapterUris = {{
}};

const std::array<std::string_view, 0> FabricAdapterCollectionUris = {{
}};

const std::array<std::string_view, 0> FacilityUris = {{
}};

const std::array<std::string_view, 0> FacilityCollectionUris = {{
}};

const std::array<std::string_view, 0> FanUris = {{
}};

const std::array<std::string_view, 0> FanCollectionUris = {{
}};

const std::array<std::string_view, 0> GraphicsControllerUris = {{
}};

const std::array<std::string_view, 0> GraphicsControllerCollectionUris = {{
}};

const std::array<std::string_view, 0> HostInterfaceUris = {{
}};

const std::array<std::string_view, 0> HostInterfaceCollectionUris = {{
}};

const std::array<std::string_view, 0> JobUris = {{
}};

const std::array<std::string_view, 0> JobCollectionUris = {{
}};

const std::array<std::string_view, 0> JobServiceUris = {{
}};

const std::array<std::string_view, 1> JsonSchemaFileUris = {{
    "/redfish/v1/JsonSchemas/",
}};

const std::array<std::string_view, 1> JsonSchemaFileCollectionUris = {{
    "/redfish/v1/JsonSchemas/<str>/",
}};

const std::array<std::string_view, 0> KeyUris = {{
}};

const std::array<std::string_view, 0> KeyCollectionUris = {{
}};

const std::array<std::string_view, 0> KeyPolicyUris = {{
}};

const std::array<std::string_view, 0> KeyPolicyCollectionUris = {{
}};

const std::array<std::string_view, 0> KeyServiceUris = {{
}};

const std::array<std::string_view, 11> LogEntryUris = {{
    "/redfish/v1/Managers/bmc/LogServices/FaultLog/Entries/<str>/",
    "/redfish/v1/Systems/<str>/LogServices/Dump/Entries/<str>/",
    "/redfish/v1/Systems/<str>/LogServices/Crashdump/Entries/<str>/<str>/",
    "/redfish/v1/Systems/<str>/LogServices/PostCodes/Entries/<str>/attachment/",
    "/redfish/v1/Systems/<str>/LogServices/HostLogger/Entries/<str>/",
    "/redfish/v1/Systems/<str>/LogServices/EventLog/Entries/<str>/attachment",
    "/redfish/v1/Managers/bmc/LogServices/Journal/Entries/<str>/",
    "/redfish/v1/Managers/bmc/LogServices/Dump/Entries/<str>/",
    "/redfish/v1/Systems/<str>/LogServices/HostLogger/Entries/",
    "/redfish/v1/Systems/<str>/LogServices/EventLog/Entries/<str>/",
    "/redfish/v1/Systems/<str>/LogServices/PostCodes/Entries/<str>/",
}};

const std::array<std::string_view, 6> LogEntryCollectionUris = {{
    "/redfish/v1/Managers/bmc/LogServices/Dump/Entries/",
    "/redfish/v1/Systems/<str>/LogServices/PostCodes/Entries/",
    "/redfish/v1/Managers/bmc/LogServices/Journal/Entries/",
    "/redfish/v1/Systems/<str>/LogServices/EventLog/Entries/",
    "/redfish/v1/Systems/<str>/LogServices/Dump/Entries/",
    "/redfish/v1/Managers/bmc/LogServices/FaultLog/Entries/",
}};

const std::array<std::string_view, 7> LogServiceUris = {{
    "/redfish/v1/Systems/<str>/LogServices/HostLogger/",
    "/redfish/v1/Systems/<str>/LogServices/EventLog/",
    "/redfish/v1/Managers/bmc/LogServices/Dump/",
    "/redfish/v1/Systems/<str>/LogServices/PostCodes/",
    "/redfish/v1/Managers/bmc/LogServices/FaultLog/",
    "/redfish/v1/Managers/bmc/LogServices/Journal/",
    "/redfish/v1/Systems/<str>/LogServices/Dump/",
}};

const std::array<std::string_view, 2> LogServiceCollectionUris = {{
    "/redfish/v1/Managers/bmc/LogServices/",
    "/redfish/v1/Systems/<str>/LogServices/",
}};

const std::array<std::string_view, 1> ManagerUris = {{
    "/redfish/v1/Managers/bmc/",
}};

const std::array<std::string_view, 1> ManagerCollectionUris = {{
    "/redfish/v1/Managers/",
}};

const std::array<std::string_view, 1> ManagerAccountUris = {{
    "/redfish/v1/AccountService/Accounts/<str>/",
}};

const std::array<std::string_view, 1> ManagerAccountCollectionUris = {{
    "/redfish/v1/AccountService/Accounts/",
}};

const std::array<std::string_view, 1> ManagerDiagnosticDataUris = {{
    "/redfish/v1/Managers/bmc/ManagerDiagnosticData",
}};

const std::array<std::string_view, 1> ManagerNetworkProtocolUris = {{
    "/redfish/v1/Managers/bmc/NetworkProtocol/",
}};

const std::array<std::string_view, 0> MediaControllerUris = {{
}};

const std::array<std::string_view, 0> MediaControllerCollectionUris = {{
}};

const std::array<std::string_view, 1> MemoryUris = {{
    "/redfish/v1/Systems/<str>/Memory/<str>/",
}};

const std::array<std::string_view, 1> MemoryCollectionUris = {{
    "/redfish/v1/Systems/<str>/Memory/",
}};

const std::array<std::string_view, 0> MemoryChunksUris = {{
}};

const std::array<std::string_view, 0> MemoryChunksCollectionUris = {{
}};

const std::array<std::string_view, 0> MemoryDomainUris = {{
}};

const std::array<std::string_view, 0> MemoryDomainCollectionUris = {{
}};

const std::array<std::string_view, 0> MemoryMetricsUris = {{
}};

const std::array<std::string_view, 2> MessageRegistryFileUris = {{
    "/redfish/v1/Registries/<str>/<str>/",
    "/redfish/v1/Registries/<str>/",
}};

const std::array<std::string_view, 1> MessageRegistryFileCollectionUris = {{
    "/redfish/v1/Registries/",
}};

const std::array<std::string_view, 0> MetricDefinitionUris = {{
}};

const std::array<std::string_view, 0> MetricDefinitionCollectionUris = {{
}};

const std::array<std::string_view, 1> MetricReportUris = {{
    "/redfish/v1/TelemetryService/MetricReports/<str>/",
}};

const std::array<std::string_view, 1> MetricReportCollectionUris = {{
    "/redfish/v1/TelemetryService/MetricReports/",
}};

const std::array<std::string_view, 1> MetricReportDefinitionUris = {{
    "/redfish/v1/TelemetryService/MetricReportDefinitions/<str>/",
}};

const std::array<std::string_view, 2> MetricReportDefinitionCollectionUris = {{
    "/redfish/v1/TelemetryService/MetricReportDefinitions/",
    "/redfish/v1/TelemetryService/MetricReportDefinitions/<str>/",
}};

const std::array<std::string_view, 0> NetworkAdapterUris = {{
}};

const std::array<std::string_view, 0> NetworkAdapterCollectionUris = {{
}};

const std::array<std::string_view, 0> NetworkAdapterMetricsUris = {{
}};

const std::array<std::string_view, 0> NetworkDeviceFunctionUris = {{
}};

const std::array<std::string_view, 0> NetworkDeviceFunctionCollectionUris = {{
}};

const std::array<std::string_view, 0> NetworkDeviceFunctionMetricsUris = {{
}};

const std::array<std::string_view, 0> NetworkInterfaceUris = {{
}};

const std::array<std::string_view, 0> NetworkInterfaceCollectionUris = {{
}};

const std::array<std::string_view, 0> NetworkPortUris = {{
}};

const std::array<std::string_view, 0> NetworkPortCollectionUris = {{
}};

const std::array<std::string_view, 1> OperatingConfigUris = {{
    "/redfish/v1/Systems/system/Processors/<str>/OperatingConfigs/<str>/",
}};

const std::array<std::string_view, 1> OperatingConfigCollectionUris = {{
    "/redfish/v1/Systems/system/Processors/<str>/OperatingConfigs/",
}};

const std::array<std::string_view, 0> OutletUris = {{
}};

const std::array<std::string_view, 0> OutletCollectionUris = {{
}};

const std::array<std::string_view, 0> OutletGroupUris = {{
}};

const std::array<std::string_view, 0> OutletGroupCollectionUris = {{
}};

const std::array<std::string_view, 1> PCIeDeviceUris = {{
    "/redfish/v1/Systems/<str>/PCIeDevices/<str>/",
}};

const std::array<std::string_view, 1> PCIeDeviceCollectionUris = {{
    "/redfish/v1/Systems/<str>/PCIeDevices/",
}};

const std::array<std::string_view, 1> PCIeFunctionUris = {{
    "/redfish/v1/Systems/system/PCIeDevices/<str>/PCIeFunctions/<str>/",
}};

const std::array<std::string_view, 1> PCIeFunctionCollectionUris = {{
    "/redfish/v1/Systems/system/PCIeDevices/<str>/PCIeFunctions/",
}};

const std::array<std::string_view, 1> PCIeSlotsUris = {{
    "/redfish/v1/Chassis/<str>/PCIeSlots/",
}};

const std::array<std::string_view, 0> PortUris = {{
}};

const std::array<std::string_view, 0> PortCollectionUris = {{
}};

const std::array<std::string_view, 0> PortMetricsUris = {{
}};

const std::array<std::string_view, 1> PowerUris = {{
    "/redfish/v1/Chassis/<str>/Power/",
}};

const std::array<std::string_view, 0> PowerDistributionUris = {{
}};

const std::array<std::string_view, 0> PowerDistributionCollectionUris = {{
}};

const std::array<std::string_view, 0> PowerDistributionMetricsUris = {{
}};

const std::array<std::string_view, 0> PowerDomainUris = {{
}};

const std::array<std::string_view, 0> PowerDomainCollectionUris = {{
}};

const std::array<std::string_view, 0> PowerEquipmentUris = {{
}};

const std::array<std::string_view, 1> PowerSubsystemUris = {{
    "/redfish/v1/Chassis/<str>/PowerSubsystem/",
}};

const std::array<std::string_view, 0> PowerSupplyUris = {{
}};

const std::array<std::string_view, 0> PowerSupplyCollectionUris = {{
}};

const std::array<std::string_view, 0> PowerSupplyMetricsUris = {{
}};

const std::array<std::string_view, 1> ProcessorUris = {{
    "/redfish/v1/Systems/<str>/Processors/<str>/",
}};

const std::array<std::string_view, 1> ProcessorCollectionUris = {{
    "/redfish/v1/Systems/<str>/Processors/",
}};

const std::array<std::string_view, 0> ProcessorMetricsUris = {{
}};

const std::array<std::string_view, 0> ResourceBlockUris = {{
}};

const std::array<std::string_view, 0> ResourceBlockCollectionUris = {{
}};

const std::array<std::string_view, 1> RoleUris = {{
    "/redfish/v1/AccountService/Roles/<str>/",
}};

const std::array<std::string_view, 1> RoleCollectionUris = {{
    "/redfish/v1/AccountService/Roles/",
}};

const std::array<std::string_view, 0> RouteEntryUris = {{
}};

const std::array<std::string_view, 0> RouteEntryCollectionUris = {{
}};

const std::array<std::string_view, 0> RouteSetEntryUris = {{
}};

const std::array<std::string_view, 0> RouteSetEntryCollectionUris = {{
}};

const std::array<std::string_view, 0> SecureBootUris = {{
}};

const std::array<std::string_view, 0> SecureBootDatabaseUris = {{
}};

const std::array<std::string_view, 0> SecureBootDatabaseCollectionUris = {{
}};

const std::array<std::string_view, 1> SensorUris = {{
    "/redfish/v1/Chassis/<str>/Sensors/<str>/",
}};

const std::array<std::string_view, 1> SensorCollectionUris = {{
    "/redfish/v1/Chassis/<str>/Sensors/",
}};

const std::array<std::string_view, 0> SerialInterfaceUris = {{
}};

const std::array<std::string_view, 0> SerialInterfaceCollectionUris = {{
}};

const std::array<std::string_view, 1> ServiceRootUris = {{
    "/redfish/v1/",
}};

const std::array<std::string_view, 1> SessionUris = {{
    "/redfish/v1/SessionService/Sessions/<str>/",
}};

const std::array<std::string_view, 1> SessionCollectionUris = {{
    "/redfish/v1/SessionService/Sessions/",
}};

const std::array<std::string_view, 1> SessionServiceUris = {{
    "/redfish/v1/SessionService/",
}};

const std::array<std::string_view, 0> SignatureUris = {{
}};

const std::array<std::string_view, 0> SignatureCollectionUris = {{
}};

const std::array<std::string_view, 0> SimpleStorageUris = {{
}};

const std::array<std::string_view, 0> SimpleStorageCollectionUris = {{
}};

const std::array<std::string_view, 1> SoftwareInventoryUris = {{
    "/redfish/v1/UpdateService/FirmwareInventory/<str>/",
}};

const std::array<std::string_view, 1> SoftwareInventoryCollectionUris = {{
    "/redfish/v1/UpdateService/FirmwareInventory/",
}};

const std::array<std::string_view, 1> StorageUris = {{
    "/redfish/v1/Systems/system/Storage/1/",
}};

const std::array<std::string_view, 1> StorageCollectionUris = {{
    "/redfish/v1/Systems/<str>/Storage/",
}};

const std::array<std::string_view, 0> StorageControllerUris = {{
}};

const std::array<std::string_view, 0> StorageControllerCollectionUris = {{
}};

const std::array<std::string_view, 0> SwitchUris = {{
}};

const std::array<std::string_view, 0> SwitchCollectionUris = {{
}};

const std::array<std::string_view, 2> TaskUris = {{
    "/redfish/v1/TaskService/Tasks/<str>/",
    "/redfish/v1/TaskService/Tasks/<str>/Monitor/",
}};

const std::array<std::string_view, 1> TaskCollectionUris = {{
    "/redfish/v1/TaskService/Tasks/",
}};

const std::array<std::string_view, 1> TaskServiceUris = {{
    "/redfish/v1/TaskService/",
}};

const std::array<std::string_view, 1> TelemetryServiceUris = {{
    "/redfish/v1/TelemetryService/",
}};

const std::array<std::string_view, 1> ThermalUris = {{
    "/redfish/v1/Chassis/<str>/Thermal/",
}};

const std::array<std::string_view, 0> ThermalMetricsUris = {{
}};

const std::array<std::string_view, 1> ThermalSubsystemUris = {{
    "/redfish/v1/Chassis/<str>/ThermalSubsystem/",
}};

const std::array<std::string_view, 1> TriggersUris = {{
    "/redfish/v1/TelemetryService/Triggers/<str>/",
}};

const std::array<std::string_view, 1> TriggersCollectionUris = {{
    "/redfish/v1/TelemetryService/Triggers/",
}};

const std::array<std::string_view, 2> UpdateServiceUris = {{
    "/redfish/v1/UpdateService/update/",
    "/redfish/v1/UpdateService/",
}};

const std::array<std::string_view, 0> USBControllerUris = {{
}};

const std::array<std::string_view, 0> USBControllerCollectionUris = {{
}};

const std::array<std::string_view, 0> VCATEntryUris = {{
}};

const std::array<std::string_view, 0> VCATEntryCollectionUris = {{
}};

const std::array<std::string_view, 1> VLanNetworkInterfaceUris = {{
    "/redfish/v1/Managers/bmc/EthernetInterfaces/<str>/VLANs/<str>/",
}};

const std::array<std::string_view, 1> VLanNetworkInterfaceCollectionUris = {{
    "/redfish/v1/Managers/bmc/EthernetInterfaces/<str>/VLANs/",
}};

const std::array<std::string_view, 1> VirtualMediaUris = {{
    "/redfish/v1/Managers/<str>/VirtualMedia/<str>/",
}};

const std::array<std::string_view, 1> VirtualMediaCollectionUris = {{
    "/redfish/v1/Managers/<str>/VirtualMedia/",
}};

const std::array<std::string_view, 0> VolumeUris = {{
}};

const std::array<std::string_view, 0> VolumeCollectionUris = {{
}};

const std::array<std::string_view, 0> ZoneUris = {{
}};

const std::array<std::string_view, 0> ZoneCollectionUris = {{
}};

const static std::array<const std::span<const std::string_view>, 195> entityTypeToUriMap{{
    AccelerationFunctionUris,
    AccelerationFunctionCollectionUris,
    AccountServiceUris,
    ActionInfoUris,
    AddressPoolUris,
    AddressPoolCollectionUris,
    AggregateUris,
    AggregateCollectionUris,
    AggregationServiceUris,
    AggregationSourceUris,
    AggregationSourceCollectionUris,
    AllowDenyUris,
    AllowDenyCollectionUris,
    AssemblyUris,
    BatteryUris,
    BatteryCollectionUris,
    BatteryMetricsUris,
    BiosUris,
    BootOptionUris,
    BootOptionCollectionUris,
    CableUris,
    CableCollectionUris,
    CertificateUris,
    CertificateCollectionUris,
    CertificateLocationsUris,
    CertificateServiceUris,
    ChassisUris,
    ChassisCollectionUris,
    CircuitUris,
    CircuitCollectionUris,
    CompositionReservationUris,
    CompositionReservationCollectionUris,
    CompositionServiceUris,
    ComputerSystemUris,
    ComputerSystemCollectionUris,
    ConnectionUris,
    ConnectionCollectionUris,
    ConnectionMethodUris,
    ConnectionMethodCollectionUris,
    ControlUris,
    ControlCollectionUris,
    DriveUris,
    DriveCollectionUris,
    EndpointUris,
    EndpointCollectionUris,
    EndpointGroupUris,
    EndpointGroupCollectionUris,
    EnvironmentMetricsUris,
    EthernetInterfaceUris,
    EthernetInterfaceCollectionUris,
    EventDestinationUris,
    EventDestinationCollectionUris,
    EventServiceUris,
    ExternalAccountProviderUris,
    ExternalAccountProviderCollectionUris,
    FabricUris,
    FabricCollectionUris,
    FabricAdapterUris,
    FabricAdapterCollectionUris,
    FacilityUris,
    FacilityCollectionUris,
    FanUris,
    FanCollectionUris,
    GraphicsControllerUris,
    GraphicsControllerCollectionUris,
    HostInterfaceUris,
    HostInterfaceCollectionUris,
    JobUris,
    JobCollectionUris,
    JobServiceUris,
    JsonSchemaFileUris,
    JsonSchemaFileCollectionUris,
    KeyUris,
    KeyCollectionUris,
    KeyPolicyUris,
    KeyPolicyCollectionUris,
    KeyServiceUris,
    LogEntryUris,
    LogEntryCollectionUris,
    LogServiceUris,
    LogServiceCollectionUris,
    ManagerUris,
    ManagerCollectionUris,
    ManagerAccountUris,
    ManagerAccountCollectionUris,
    ManagerDiagnosticDataUris,
    ManagerNetworkProtocolUris,
    MediaControllerUris,
    MediaControllerCollectionUris,
    MemoryUris,
    MemoryCollectionUris,
    MemoryChunksUris,
    MemoryChunksCollectionUris,
    MemoryDomainUris,
    MemoryDomainCollectionUris,
    MemoryMetricsUris,
    MessageRegistryFileUris,
    MessageRegistryFileCollectionUris,
    MetricDefinitionUris,
    MetricDefinitionCollectionUris,
    MetricReportUris,
    MetricReportCollectionUris,
    MetricReportDefinitionUris,
    MetricReportDefinitionCollectionUris,
    NetworkAdapterUris,
    NetworkAdapterCollectionUris,
    NetworkAdapterMetricsUris,
    NetworkDeviceFunctionUris,
    NetworkDeviceFunctionCollectionUris,
    NetworkDeviceFunctionMetricsUris,
    NetworkInterfaceUris,
    NetworkInterfaceCollectionUris,
    NetworkPortUris,
    NetworkPortCollectionUris,
    OperatingConfigUris,
    OperatingConfigCollectionUris,
    OutletUris,
    OutletCollectionUris,
    OutletGroupUris,
    OutletGroupCollectionUris,
    PCIeDeviceUris,
    PCIeDeviceCollectionUris,
    PCIeFunctionUris,
    PCIeFunctionCollectionUris,
    PCIeSlotsUris,
    PortUris,
    PortCollectionUris,
    PortMetricsUris,
    PowerUris,
    PowerDistributionUris,
    PowerDistributionCollectionUris,
    PowerDistributionMetricsUris,
    PowerDomainUris,
    PowerDomainCollectionUris,
    PowerEquipmentUris,
    PowerSubsystemUris,
    PowerSupplyUris,
    PowerSupplyCollectionUris,
    PowerSupplyMetricsUris,
    ProcessorUris,
    ProcessorCollectionUris,
    ProcessorMetricsUris,
    ResourceBlockUris,
    ResourceBlockCollectionUris,
    RoleUris,
    RoleCollectionUris,
    RouteEntryUris,
    RouteEntryCollectionUris,
    RouteSetEntryUris,
    RouteSetEntryCollectionUris,
    SecureBootUris,
    SecureBootDatabaseUris,
    SecureBootDatabaseCollectionUris,
    SensorUris,
    SensorCollectionUris,
    SerialInterfaceUris,
    SerialInterfaceCollectionUris,
    ServiceRootUris,
    SessionUris,
    SessionCollectionUris,
    SessionServiceUris,
    SignatureUris,
    SignatureCollectionUris,
    SimpleStorageUris,
    SimpleStorageCollectionUris,
    SoftwareInventoryUris,
    SoftwareInventoryCollectionUris,
    StorageUris,
    StorageCollectionUris,
    StorageControllerUris,
    StorageControllerCollectionUris,
    SwitchUris,
    SwitchCollectionUris,
    TaskUris,
    TaskCollectionUris,
    TaskServiceUris,
    TelemetryServiceUris,
    ThermalUris,
    ThermalMetricsUris,
    ThermalSubsystemUris,
    TriggersUris,
    TriggersCollectionUris,
    UpdateServiceUris,
    USBControllerUris,
    USBControllerCollectionUris,
    VCATEntryUris,
    VCATEntryCollectionUris,
    VLanNetworkInterfaceUris,
    VLanNetworkInterfaceCollectionUris,
    VirtualMediaUris,
    VirtualMediaCollectionUris,
    VolumeUris,
    VolumeCollectionUris,
    ZoneUris,
    ZoneCollectionUris,
}};

inline std::span<const std::string_view> getUriFromEntityTag(redfish::privileges::EntityTag entity)
{
    size_t entityIndex = static_cast<size_t>(entity);
    return entityTypeToUriMap[entityIndex];
}} // namespace redfish::privileges
// clang-format on
