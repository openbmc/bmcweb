#pragma once
/****************************************************************
 *                 READ THIS WARNING FIRST
 * This is an auto-generated header which contains definitions
 * for Redfish DMTF defined messages.
 * DO NOT modify this registry outside of running the
 * parse_registries.py script.  The definitions contained within
 * this file are owned by DMTF.  Any modifications to these files
 * should be first pushed to the relevant registry in the DMTF
 * github organization.
 ***************************************************************/
#include "privileges.hpp"

#include <array>

// clang-format off

namespace redfish::privileges
{
const std::array<Privileges, 1> privilegeSetLogin = {{
    {"Login"}
}};
const std::array<Privileges, 1> privilegeSetConfigureComponents = {{
    {"ConfigureComponents"}
}};
const std::array<Privileges, 1> privilegeSetConfigureUsers = {{
    {"ConfigureUsers"}
}};
const std::array<Privileges, 1> privilegeSetConfigureManager = {{
    {"ConfigureManager"}
}};
const std::array<Privileges, 2> privilegeSetConfigureManagerOrConfigureComponents = {{
    {"ConfigureManager"},
    {"ConfigureComponents"}
}};
const std::array<Privileges, 2> privilegeSetConfigureManagerOrConfigureSelf = {{
    {"ConfigureManager"},
    {"ConfigureSelf"}
}};
const std::array<Privileges, 3> privilegeSetConfigureManagerOrConfigureUsersOrConfigureSelf = {{
    {"ConfigureManager"},
    {"ConfigureUsers"},
    {"ConfigureSelf"}
}};
const std::array<Privileges, 2> privilegeSetLoginOrNoAuth = {{
    {"Login"},
    {}
}};
// AccelerationFunction
const static auto& getAccelerationFunction = privilegeSetLogin;
const static auto& headAccelerationFunction = privilegeSetLogin;
const static auto& patchAccelerationFunction = privilegeSetConfigureComponents;
const static auto& putAccelerationFunction = privilegeSetConfigureComponents;
const static auto& deleteAccelerationFunction = privilegeSetConfigureComponents;
const static auto& postAccelerationFunction = privilegeSetConfigureComponents;

// AccelerationFunctionCollection
const static auto& getAccelerationFunctionCollection = privilegeSetLogin;
const static auto& headAccelerationFunctionCollection = privilegeSetLogin;
const static auto& patchAccelerationFunctionCollection = privilegeSetConfigureComponents;
const static auto& putAccelerationFunctionCollection = privilegeSetConfigureComponents;
const static auto& deleteAccelerationFunctionCollection = privilegeSetConfigureComponents;
const static auto& postAccelerationFunctionCollection = privilegeSetConfigureComponents;

// AccountService
const static auto& getAccountService = privilegeSetLogin;
const static auto& headAccountService = privilegeSetLogin;
const static auto& patchAccountService = privilegeSetConfigureUsers;
const static auto& putAccountService = privilegeSetConfigureUsers;
const static auto& deleteAccountService = privilegeSetConfigureUsers;
const static auto& postAccountService = privilegeSetConfigureUsers;

// ActionInfo
const static auto& getActionInfo = privilegeSetLogin;
const static auto& headActionInfo = privilegeSetLogin;
const static auto& patchActionInfo = privilegeSetConfigureManager;
const static auto& putActionInfo = privilegeSetConfigureManager;
const static auto& deleteActionInfo = privilegeSetConfigureManager;
const static auto& postActionInfo = privilegeSetConfigureManager;

// AddressPool
const static auto& getAddressPool = privilegeSetLogin;
const static auto& headAddressPool = privilegeSetLogin;
const static auto& patchAddressPool = privilegeSetConfigureComponents;
const static auto& putAddressPool = privilegeSetConfigureComponents;
const static auto& deleteAddressPool = privilegeSetConfigureComponents;
const static auto& postAddressPool = privilegeSetConfigureComponents;

// AddressPoolCollection
const static auto& getAddressPoolCollection = privilegeSetLogin;
const static auto& headAddressPoolCollection = privilegeSetLogin;
const static auto& patchAddressPoolCollection = privilegeSetConfigureComponents;
const static auto& putAddressPoolCollection = privilegeSetConfigureComponents;
const static auto& deleteAddressPoolCollection = privilegeSetConfigureComponents;
const static auto& postAddressPoolCollection = privilegeSetConfigureComponents;

// Aggregate
const static auto& getAggregate = privilegeSetLogin;
const static auto& headAggregate = privilegeSetLogin;
const static auto& patchAggregate = privilegeSetConfigureManagerOrConfigureComponents;
const static auto& putAggregate = privilegeSetConfigureManagerOrConfigureComponents;
const static auto& deleteAggregate = privilegeSetConfigureManagerOrConfigureComponents;
const static auto& postAggregate = privilegeSetConfigureManagerOrConfigureComponents;

// AggregateCollection
const static auto& getAggregateCollection = privilegeSetLogin;
const static auto& headAggregateCollection = privilegeSetLogin;
const static auto& patchAggregateCollection = privilegeSetConfigureManagerOrConfigureComponents;
const static auto& putAggregateCollection = privilegeSetConfigureManagerOrConfigureComponents;
const static auto& deleteAggregateCollection = privilegeSetConfigureManagerOrConfigureComponents;
const static auto& postAggregateCollection = privilegeSetConfigureManagerOrConfigureComponents;

// AggregationService
const static auto& getAggregationService = privilegeSetLogin;
const static auto& headAggregationService = privilegeSetLogin;
const static auto& patchAggregationService = privilegeSetConfigureManager;
const static auto& putAggregationService = privilegeSetConfigureManager;
const static auto& deleteAggregationService = privilegeSetConfigureManager;
const static auto& postAggregationService = privilegeSetConfigureManager;

// AggregationSource
const static auto& getAggregationSource = privilegeSetLogin;
const static auto& headAggregationSource = privilegeSetLogin;
const static auto& patchAggregationSource = privilegeSetConfigureManager;
const static auto& putAggregationSource = privilegeSetConfigureManager;
const static auto& deleteAggregationSource = privilegeSetConfigureManager;
const static auto& postAggregationSource = privilegeSetConfigureManager;

// AggregationSourceCollection
const static auto& getAggregationSourceCollection = privilegeSetLogin;
const static auto& headAggregationSourceCollection = privilegeSetLogin;
const static auto& patchAggregationSourceCollection = privilegeSetConfigureManager;
const static auto& putAggregationSourceCollection = privilegeSetConfigureManager;
const static auto& deleteAggregationSourceCollection = privilegeSetConfigureManager;
const static auto& postAggregationSourceCollection = privilegeSetConfigureManager;

// AllowDeny
const static auto& getAllowDeny = privilegeSetLogin;
const static auto& headAllowDeny = privilegeSetLogin;
const static auto& patchAllowDeny = privilegeSetConfigureManager;
const static auto& putAllowDeny = privilegeSetConfigureManager;
const static auto& deleteAllowDeny = privilegeSetConfigureManager;
const static auto& postAllowDeny = privilegeSetConfigureManager;

// AllowDenyCollection
const static auto& getAllowDenyCollection = privilegeSetLogin;
const static auto& headAllowDenyCollection = privilegeSetLogin;
const static auto& patchAllowDenyCollection = privilegeSetConfigureManager;
const static auto& putAllowDenyCollection = privilegeSetConfigureManager;
const static auto& deleteAllowDenyCollection = privilegeSetConfigureManager;
const static auto& postAllowDenyCollection = privilegeSetConfigureManager;

// Assembly
const static auto& getAssembly = privilegeSetLogin;
const static auto& headAssembly = privilegeSetLogin;
const static auto& patchAssembly = privilegeSetConfigureComponents;
const static auto& putAssembly = privilegeSetConfigureComponents;
const static auto& deleteAssembly = privilegeSetConfigureComponents;
const static auto& postAssembly = privilegeSetConfigureComponents;

// Battery
const static auto& getBattery = privilegeSetLogin;
const static auto& headBattery = privilegeSetLogin;
const static auto& patchBattery = privilegeSetConfigureManager;
const static auto& putBattery = privilegeSetConfigureManager;
const static auto& deleteBattery = privilegeSetConfigureManager;
const static auto& postBattery = privilegeSetConfigureManager;

// BatteryCollection
const static auto& getBatteryCollection = privilegeSetLogin;
const static auto& headBatteryCollection = privilegeSetLogin;
const static auto& patchBatteryCollection = privilegeSetConfigureManager;
const static auto& putBatteryCollection = privilegeSetConfigureManager;
const static auto& deleteBatteryCollection = privilegeSetConfigureManager;
const static auto& postBatteryCollection = privilegeSetConfigureManager;

// BatteryMetrics
const static auto& getBatteryMetrics = privilegeSetLogin;
const static auto& headBatteryMetrics = privilegeSetLogin;
const static auto& patchBatteryMetrics = privilegeSetConfigureManager;
const static auto& putBatteryMetrics = privilegeSetConfigureManager;
const static auto& deleteBatteryMetrics = privilegeSetConfigureManager;
const static auto& postBatteryMetrics = privilegeSetConfigureManager;

// Bios
const static auto& getBios = privilegeSetLogin;
const static auto& headBios = privilegeSetLogin;
const static auto& patchBios = privilegeSetConfigureComponents;
const static auto& postBios = privilegeSetConfigureComponents;
const static auto& putBios = privilegeSetConfigureComponents;
const static auto& deleteBios = privilegeSetConfigureComponents;

// BootOption
const static auto& getBootOption = privilegeSetLogin;
const static auto& headBootOption = privilegeSetLogin;
const static auto& patchBootOption = privilegeSetConfigureComponents;
const static auto& putBootOption = privilegeSetConfigureComponents;
const static auto& deleteBootOption = privilegeSetConfigureComponents;
const static auto& postBootOption = privilegeSetConfigureComponents;

// BootOptionCollection
const static auto& getBootOptionCollection = privilegeSetLogin;
const static auto& headBootOptionCollection = privilegeSetLogin;
const static auto& patchBootOptionCollection = privilegeSetConfigureComponents;
const static auto& putBootOptionCollection = privilegeSetConfigureComponents;
const static auto& deleteBootOptionCollection = privilegeSetConfigureComponents;
const static auto& postBootOptionCollection = privilegeSetConfigureComponents;

// Cable
const static auto& getCable = privilegeSetLogin;
const static auto& headCable = privilegeSetLogin;
const static auto& patchCable = privilegeSetConfigureComponents;
const static auto& putCable = privilegeSetConfigureComponents;
const static auto& deleteCable = privilegeSetConfigureComponents;
const static auto& postCable = privilegeSetConfigureComponents;

// CableCollection
const static auto& getCableCollection = privilegeSetLogin;
const static auto& headCableCollection = privilegeSetLogin;
const static auto& patchCableCollection = privilegeSetConfigureComponents;
const static auto& putCableCollection = privilegeSetConfigureComponents;
const static auto& deleteCableCollection = privilegeSetConfigureComponents;
const static auto& postCableCollection = privilegeSetConfigureComponents;

// Certificate
const static auto& getCertificate = privilegeSetConfigureManager;
const static auto& headCertificate = privilegeSetConfigureManager;
const static auto& patchCertificate = privilegeSetConfigureManager;
const static auto& putCertificate = privilegeSetConfigureManager;
const static auto& deleteCertificate = privilegeSetConfigureManager;
const static auto& postCertificate = privilegeSetConfigureManager;

// CertificateCollection
const static auto& getCertificateCollection = privilegeSetConfigureManager;
const static auto& headCertificateCollection = privilegeSetConfigureManager;
const static auto& patchCertificateCollection = privilegeSetConfigureManager;
const static auto& putCertificateCollection = privilegeSetConfigureManager;
const static auto& deleteCertificateCollection = privilegeSetConfigureManager;
const static auto& postCertificateCollection = privilegeSetConfigureManager;

// CertificateLocations
const static auto& getCertificateLocations = privilegeSetConfigureManager;
const static auto& headCertificateLocations = privilegeSetConfigureManager;
const static auto& patchCertificateLocations = privilegeSetConfigureManager;
const static auto& putCertificateLocations = privilegeSetConfigureManager;
const static auto& deleteCertificateLocations = privilegeSetConfigureManager;
const static auto& postCertificateLocations = privilegeSetConfigureManager;

// CertificateService
const static auto& getCertificateService = privilegeSetLogin;
const static auto& headCertificateService = privilegeSetLogin;
const static auto& patchCertificateService = privilegeSetConfigureManager;
const static auto& putCertificateService = privilegeSetConfigureManager;
const static auto& deleteCertificateService = privilegeSetConfigureManager;
const static auto& postCertificateService = privilegeSetConfigureManager;

// Chassis
const static auto& getChassis = privilegeSetLogin;
const static auto& headChassis = privilegeSetLogin;
const static auto& patchChassis = privilegeSetConfigureComponents;
const static auto& putChassis = privilegeSetConfigureComponents;
const static auto& deleteChassis = privilegeSetConfigureComponents;
const static auto& postChassis = privilegeSetConfigureComponents;

// ChassisCollection
const static auto& getChassisCollection = privilegeSetLogin;
const static auto& headChassisCollection = privilegeSetLogin;
const static auto& patchChassisCollection = privilegeSetConfigureComponents;
const static auto& putChassisCollection = privilegeSetConfigureComponents;
const static auto& deleteChassisCollection = privilegeSetConfigureComponents;
const static auto& postChassisCollection = privilegeSetConfigureComponents;

// Circuit
const static auto& getCircuit = privilegeSetLogin;
const static auto& headCircuit = privilegeSetLogin;
const static auto& patchCircuit = privilegeSetConfigureComponents;
const static auto& putCircuit = privilegeSetConfigureComponents;
const static auto& deleteCircuit = privilegeSetConfigureComponents;
const static auto& postCircuit = privilegeSetConfigureComponents;

// CircuitCollection
const static auto& getCircuitCollection = privilegeSetLogin;
const static auto& headCircuitCollection = privilegeSetLogin;
const static auto& patchCircuitCollection = privilegeSetConfigureComponents;
const static auto& putCircuitCollection = privilegeSetConfigureComponents;
const static auto& deleteCircuitCollection = privilegeSetConfigureComponents;
const static auto& postCircuitCollection = privilegeSetConfigureComponents;

// CompositionReservation
const static auto& getCompositionReservation = privilegeSetLogin;
const static auto& headCompositionReservation = privilegeSetLogin;
const static auto& patchCompositionReservation = privilegeSetConfigureManager;
const static auto& putCompositionReservation = privilegeSetConfigureManager;
const static auto& deleteCompositionReservation = privilegeSetConfigureManager;
const static auto& postCompositionReservation = privilegeSetConfigureManager;

// CompositionReservationCollection
const static auto& getCompositionReservationCollection = privilegeSetLogin;
const static auto& headCompositionReservationCollection = privilegeSetLogin;
const static auto& patchCompositionReservationCollection = privilegeSetConfigureManager;
const static auto& putCompositionReservationCollection = privilegeSetConfigureManager;
const static auto& deleteCompositionReservationCollection = privilegeSetConfigureManager;
const static auto& postCompositionReservationCollection = privilegeSetConfigureManager;

// CompositionService
const static auto& getCompositionService = privilegeSetLogin;
const static auto& headCompositionService = privilegeSetLogin;
const static auto& patchCompositionService = privilegeSetConfigureManager;
const static auto& putCompositionService = privilegeSetConfigureManager;
const static auto& deleteCompositionService = privilegeSetConfigureManager;
const static auto& postCompositionService = privilegeSetConfigureManager;

// ComputerSystem
const static auto& getComputerSystem = privilegeSetLogin;
const static auto& headComputerSystem = privilegeSetLogin;
const static auto& patchComputerSystem = privilegeSetConfigureComponents;
const static auto& postComputerSystem = privilegeSetConfigureComponents;
const static auto& putComputerSystem = privilegeSetConfigureComponents;
const static auto& deleteComputerSystem = privilegeSetConfigureComponents;

// ComputerSystemCollection
const static auto& getComputerSystemCollection = privilegeSetLogin;
const static auto& headComputerSystemCollection = privilegeSetLogin;
const static auto& patchComputerSystemCollection = privilegeSetConfigureComponents;
const static auto& postComputerSystemCollection = privilegeSetConfigureComponents;
const static auto& putComputerSystemCollection = privilegeSetConfigureComponents;
const static auto& deleteComputerSystemCollection = privilegeSetConfigureComponents;

// Connection
const static auto& getConnection = privilegeSetLogin;
const static auto& headConnection = privilegeSetLogin;
const static auto& patchConnection = privilegeSetConfigureComponents;
const static auto& postConnection = privilegeSetConfigureComponents;
const static auto& putConnection = privilegeSetConfigureComponents;
const static auto& deleteConnection = privilegeSetConfigureComponents;

// ConnectionCollection
const static auto& getConnectionCollection = privilegeSetLogin;
const static auto& headConnectionCollection = privilegeSetLogin;
const static auto& patchConnectionCollection = privilegeSetConfigureComponents;
const static auto& postConnectionCollection = privilegeSetConfigureComponents;
const static auto& putConnectionCollection = privilegeSetConfigureComponents;
const static auto& deleteConnectionCollection = privilegeSetConfigureComponents;

// ConnectionMethod
const static auto& getConnectionMethod = privilegeSetLogin;
const static auto& headConnectionMethod = privilegeSetLogin;
const static auto& patchConnectionMethod = privilegeSetConfigureManager;
const static auto& putConnectionMethod = privilegeSetConfigureManager;
const static auto& deleteConnectionMethod = privilegeSetConfigureManager;
const static auto& postConnectionMethod = privilegeSetConfigureManager;

// ConnectionMethodCollection
const static auto& getConnectionMethodCollection = privilegeSetLogin;
const static auto& headConnectionMethodCollection = privilegeSetLogin;
const static auto& patchConnectionMethodCollection = privilegeSetConfigureManager;
const static auto& putConnectionMethodCollection = privilegeSetConfigureManager;
const static auto& deleteConnectionMethodCollection = privilegeSetConfigureManager;
const static auto& postConnectionMethodCollection = privilegeSetConfigureManager;

// Control
const static auto& getControl = privilegeSetLogin;
const static auto& headControl = privilegeSetLogin;
const static auto& patchControl = privilegeSetConfigureManager;
const static auto& putControl = privilegeSetConfigureManager;
const static auto& deleteControl = privilegeSetConfigureManager;
const static auto& postControl = privilegeSetConfigureManager;

// ControlCollection
const static auto& getControlCollection = privilegeSetLogin;
const static auto& headControlCollection = privilegeSetLogin;
const static auto& patchControlCollection = privilegeSetConfigureManager;
const static auto& putControlCollection = privilegeSetConfigureManager;
const static auto& deleteControlCollection = privilegeSetConfigureManager;
const static auto& postControlCollection = privilegeSetConfigureManager;

// Drive
const static auto& getDrive = privilegeSetLogin;
const static auto& headDrive = privilegeSetLogin;
const static auto& patchDrive = privilegeSetConfigureComponents;
const static auto& postDrive = privilegeSetConfigureComponents;
const static auto& putDrive = privilegeSetConfigureComponents;
const static auto& deleteDrive = privilegeSetConfigureComponents;

// DriveCollection
const static auto& getDriveCollection = privilegeSetLogin;
const static auto& headDriveCollection = privilegeSetLogin;
const static auto& patchDriveCollection = privilegeSetConfigureComponents;
const static auto& postDriveCollection = privilegeSetConfigureComponents;
const static auto& putDriveCollection = privilegeSetConfigureComponents;
const static auto& deleteDriveCollection = privilegeSetConfigureComponents;

// Endpoint
const static auto& getEndpoint = privilegeSetLogin;
const static auto& headEndpoint = privilegeSetLogin;
const static auto& patchEndpoint = privilegeSetConfigureComponents;
const static auto& postEndpoint = privilegeSetConfigureComponents;
const static auto& putEndpoint = privilegeSetConfigureComponents;
const static auto& deleteEndpoint = privilegeSetConfigureComponents;

// EndpointCollection
const static auto& getEndpointCollection = privilegeSetLogin;
const static auto& headEndpointCollection = privilegeSetLogin;
const static auto& patchEndpointCollection = privilegeSetConfigureComponents;
const static auto& postEndpointCollection = privilegeSetConfigureComponents;
const static auto& putEndpointCollection = privilegeSetConfigureComponents;
const static auto& deleteEndpointCollection = privilegeSetConfigureComponents;

// EndpointGroup
const static auto& getEndpointGroup = privilegeSetLogin;
const static auto& headEndpointGroup = privilegeSetLogin;
const static auto& patchEndpointGroup = privilegeSetConfigureComponents;
const static auto& postEndpointGroup = privilegeSetConfigureComponents;
const static auto& putEndpointGroup = privilegeSetConfigureComponents;
const static auto& deleteEndpointGroup = privilegeSetConfigureComponents;

// EndpointGroupCollection
const static auto& getEndpointGroupCollection = privilegeSetLogin;
const static auto& headEndpointGroupCollection = privilegeSetLogin;
const static auto& patchEndpointGroupCollection = privilegeSetConfigureComponents;
const static auto& postEndpointGroupCollection = privilegeSetConfigureComponents;
const static auto& putEndpointGroupCollection = privilegeSetConfigureComponents;
const static auto& deleteEndpointGroupCollection = privilegeSetConfigureComponents;

// EnvironmentMetrics
const static auto& getEnvironmentMetrics = privilegeSetLogin;
const static auto& headEnvironmentMetrics = privilegeSetLogin;
const static auto& patchEnvironmentMetrics = privilegeSetConfigureManager;
const static auto& putEnvironmentMetrics = privilegeSetConfigureManager;
const static auto& deleteEnvironmentMetrics = privilegeSetConfigureManager;
const static auto& postEnvironmentMetrics = privilegeSetConfigureManager;

// EthernetInterface
const static auto& getEthernetInterface = privilegeSetLogin;
const static auto& headEthernetInterface = privilegeSetLogin;
const static auto& patchEthernetInterface = privilegeSetConfigureComponents;
const static auto& postEthernetInterface = privilegeSetConfigureComponents;
const static auto& putEthernetInterface = privilegeSetConfigureComponents;
const static auto& deleteEthernetInterface = privilegeSetConfigureComponents;

// EthernetInterfaceCollection
const static auto& getEthernetInterfaceCollection = privilegeSetLogin;
const static auto& headEthernetInterfaceCollection = privilegeSetLogin;
const static auto& patchEthernetInterfaceCollection = privilegeSetConfigureComponents;
const static auto& postEthernetInterfaceCollection = privilegeSetConfigureComponents;
const static auto& putEthernetInterfaceCollection = privilegeSetConfigureComponents;
const static auto& deleteEthernetInterfaceCollection = privilegeSetConfigureComponents;

// EventDestination
const static auto& getEventDestination = privilegeSetLogin;
const static auto& headEventDestination = privilegeSetLogin;
const static auto& patchEventDestination = privilegeSetConfigureManagerOrConfigureSelf;
const static auto& postEventDestination = privilegeSetConfigureManagerOrConfigureSelf;
const static auto& putEventDestination = privilegeSetConfigureManagerOrConfigureSelf;
const static auto& deleteEventDestination = privilegeSetConfigureManagerOrConfigureSelf;

// EventDestinationCollection
const static auto& getEventDestinationCollection = privilegeSetLogin;
const static auto& headEventDestinationCollection = privilegeSetLogin;
const static auto& patchEventDestinationCollection = privilegeSetConfigureManagerOrConfigureComponents;
const static auto& postEventDestinationCollection = privilegeSetConfigureManagerOrConfigureComponents;
const static auto& putEventDestinationCollection = privilegeSetConfigureManagerOrConfigureComponents;
const static auto& deleteEventDestinationCollection = privilegeSetConfigureManagerOrConfigureComponents;

// EventService
const static auto& getEventService = privilegeSetLogin;
const static auto& headEventService = privilegeSetLogin;
const static auto& patchEventService = privilegeSetConfigureManager;
const static auto& postEventService = privilegeSetConfigureManager;
const static auto& putEventService = privilegeSetConfigureManager;
const static auto& deleteEventService = privilegeSetConfigureManager;

// ExternalAccountProvider
const static auto& getExternalAccountProvider = privilegeSetLogin;
const static auto& headExternalAccountProvider = privilegeSetLogin;
const static auto& patchExternalAccountProvider = privilegeSetConfigureManager;
const static auto& putExternalAccountProvider = privilegeSetConfigureManager;
const static auto& deleteExternalAccountProvider = privilegeSetConfigureManager;
const static auto& postExternalAccountProvider = privilegeSetConfigureManager;

// ExternalAccountProviderCollection
const static auto& getExternalAccountProviderCollection = privilegeSetLogin;
const static auto& headExternalAccountProviderCollection = privilegeSetLogin;
const static auto& patchExternalAccountProviderCollection = privilegeSetConfigureManager;
const static auto& putExternalAccountProviderCollection = privilegeSetConfigureManager;
const static auto& deleteExternalAccountProviderCollection = privilegeSetConfigureManager;
const static auto& postExternalAccountProviderCollection = privilegeSetConfigureManager;

// Fabric
const static auto& getFabric = privilegeSetLogin;
const static auto& headFabric = privilegeSetLogin;
const static auto& patchFabric = privilegeSetConfigureComponents;
const static auto& postFabric = privilegeSetConfigureComponents;
const static auto& putFabric = privilegeSetConfigureComponents;
const static auto& deleteFabric = privilegeSetConfigureComponents;

// FabricCollection
const static auto& getFabricCollection = privilegeSetLogin;
const static auto& headFabricCollection = privilegeSetLogin;
const static auto& patchFabricCollection = privilegeSetConfigureComponents;
const static auto& postFabricCollection = privilegeSetConfigureComponents;
const static auto& putFabricCollection = privilegeSetConfigureComponents;
const static auto& deleteFabricCollection = privilegeSetConfigureComponents;

// FabricAdapter
const static auto& getFabricAdapter = privilegeSetLogin;
const static auto& headFabricAdapter = privilegeSetLogin;
const static auto& patchFabricAdapter = privilegeSetConfigureComponents;
const static auto& postFabricAdapter = privilegeSetConfigureComponents;
const static auto& putFabricAdapter = privilegeSetConfigureComponents;
const static auto& deleteFabricAdapter = privilegeSetConfigureComponents;

// FabricAdapterCollection
const static auto& getFabricAdapterCollection = privilegeSetLogin;
const static auto& headFabricAdapterCollection = privilegeSetLogin;
const static auto& patchFabricAdapterCollection = privilegeSetConfigureComponents;
const static auto& postFabricAdapterCollection = privilegeSetConfigureComponents;
const static auto& putFabricAdapterCollection = privilegeSetConfigureComponents;
const static auto& deleteFabricAdapterCollection = privilegeSetConfigureComponents;

// Facility
const static auto& getFacility = privilegeSetLogin;
const static auto& headFacility = privilegeSetLogin;
const static auto& patchFacility = privilegeSetConfigureComponents;
const static auto& putFacility = privilegeSetConfigureComponents;
const static auto& deleteFacility = privilegeSetConfigureComponents;
const static auto& postFacility = privilegeSetConfigureComponents;

// FacilityCollection
const static auto& getFacilityCollection = privilegeSetLogin;
const static auto& headFacilityCollection = privilegeSetLogin;
const static auto& patchFacilityCollection = privilegeSetConfigureComponents;
const static auto& putFacilityCollection = privilegeSetConfigureComponents;
const static auto& deleteFacilityCollection = privilegeSetConfigureComponents;
const static auto& postFacilityCollection = privilegeSetConfigureComponents;

// Fan
const static auto& getFan = privilegeSetLogin;
const static auto& headFan = privilegeSetLogin;
const static auto& patchFan = privilegeSetConfigureManager;
const static auto& putFan = privilegeSetConfigureManager;
const static auto& deleteFan = privilegeSetConfigureManager;
const static auto& postFan = privilegeSetConfigureManager;

// FanCollection
const static auto& getFanCollection = privilegeSetLogin;
const static auto& headFanCollection = privilegeSetLogin;
const static auto& patchFanCollection = privilegeSetConfigureManager;
const static auto& putFanCollection = privilegeSetConfigureManager;
const static auto& deleteFanCollection = privilegeSetConfigureManager;
const static auto& postFanCollection = privilegeSetConfigureManager;

// GraphicsController
const static auto& getGraphicsController = privilegeSetLogin;
const static auto& headGraphicsController = privilegeSetLogin;
const static auto& patchGraphicsController = privilegeSetConfigureComponents;
const static auto& putGraphicsController = privilegeSetConfigureComponents;
const static auto& deleteGraphicsController = privilegeSetConfigureComponents;
const static auto& postGraphicsController = privilegeSetConfigureComponents;

// GraphicsControllerCollection
const static auto& getGraphicsControllerCollection = privilegeSetLogin;
const static auto& headGraphicsControllerCollection = privilegeSetLogin;
const static auto& patchGraphicsControllerCollection = privilegeSetConfigureComponents;
const static auto& putGraphicsControllerCollection = privilegeSetConfigureComponents;
const static auto& deleteGraphicsControllerCollection = privilegeSetConfigureComponents;
const static auto& postGraphicsControllerCollection = privilegeSetConfigureComponents;

// HostInterface
const static auto& getHostInterface = privilegeSetLogin;
const static auto& headHostInterface = privilegeSetLogin;
const static auto& patchHostInterface = privilegeSetConfigureManager;
const static auto& postHostInterface = privilegeSetConfigureManager;
const static auto& putHostInterface = privilegeSetConfigureManager;
const static auto& deleteHostInterface = privilegeSetConfigureManager;

// HostInterfaceCollection
const static auto& getHostInterfaceCollection = privilegeSetLogin;
const static auto& headHostInterfaceCollection = privilegeSetLogin;
const static auto& patchHostInterfaceCollection = privilegeSetConfigureManager;
const static auto& postHostInterfaceCollection = privilegeSetConfigureManager;
const static auto& putHostInterfaceCollection = privilegeSetConfigureManager;
const static auto& deleteHostInterfaceCollection = privilegeSetConfigureManager;

// Job
const static auto& getJob = privilegeSetLogin;
const static auto& headJob = privilegeSetLogin;
const static auto& patchJob = privilegeSetConfigureManager;
const static auto& putJob = privilegeSetConfigureManager;
const static auto& deleteJob = privilegeSetConfigureManager;
const static auto& postJob = privilegeSetConfigureManager;

// JobCollection
const static auto& getJobCollection = privilegeSetLogin;
const static auto& headJobCollection = privilegeSetLogin;
const static auto& patchJobCollection = privilegeSetConfigureManager;
const static auto& putJobCollection = privilegeSetConfigureManager;
const static auto& deleteJobCollection = privilegeSetConfigureManager;
const static auto& postJobCollection = privilegeSetConfigureManager;

// JobService
const static auto& getJobService = privilegeSetLogin;
const static auto& headJobService = privilegeSetLogin;
const static auto& patchJobService = privilegeSetConfigureManager;
const static auto& putJobService = privilegeSetConfigureManager;
const static auto& deleteJobService = privilegeSetConfigureManager;
const static auto& postJobService = privilegeSetConfigureManager;

// JsonSchemaFile
const static auto& getJsonSchemaFile = privilegeSetLogin;
const static auto& headJsonSchemaFile = privilegeSetLogin;
const static auto& patchJsonSchemaFile = privilegeSetConfigureManager;
const static auto& postJsonSchemaFile = privilegeSetConfigureManager;
const static auto& putJsonSchemaFile = privilegeSetConfigureManager;
const static auto& deleteJsonSchemaFile = privilegeSetConfigureManager;

// JsonSchemaFileCollection
const static auto& getJsonSchemaFileCollection = privilegeSetLogin;
const static auto& headJsonSchemaFileCollection = privilegeSetLogin;
const static auto& patchJsonSchemaFileCollection = privilegeSetConfigureManager;
const static auto& postJsonSchemaFileCollection = privilegeSetConfigureManager;
const static auto& putJsonSchemaFileCollection = privilegeSetConfigureManager;
const static auto& deleteJsonSchemaFileCollection = privilegeSetConfigureManager;

// Key
const static auto& getKey = privilegeSetLogin;
const static auto& headKey = privilegeSetLogin;
const static auto& patchKey = privilegeSetConfigureManager;
const static auto& putKey = privilegeSetConfigureManager;
const static auto& deleteKey = privilegeSetConfigureManager;
const static auto& postKey = privilegeSetConfigureManager;

// KeyCollection
const static auto& getKeyCollection = privilegeSetLogin;
const static auto& headKeyCollection = privilegeSetLogin;
const static auto& patchKeyCollection = privilegeSetConfigureManager;
const static auto& putKeyCollection = privilegeSetConfigureManager;
const static auto& deleteKeyCollection = privilegeSetConfigureManager;
const static auto& postKeyCollection = privilegeSetConfigureManager;

// KeyPolicy
const static auto& getKeyPolicy = privilegeSetLogin;
const static auto& headKeyPolicy = privilegeSetLogin;
const static auto& patchKeyPolicy = privilegeSetConfigureManager;
const static auto& putKeyPolicy = privilegeSetConfigureManager;
const static auto& deleteKeyPolicy = privilegeSetConfigureManager;
const static auto& postKeyPolicy = privilegeSetConfigureManager;

// KeyPolicyCollection
const static auto& getKeyPolicyCollection = privilegeSetLogin;
const static auto& headKeyPolicyCollection = privilegeSetLogin;
const static auto& patchKeyPolicyCollection = privilegeSetConfigureManager;
const static auto& putKeyPolicyCollection = privilegeSetConfigureManager;
const static auto& deleteKeyPolicyCollection = privilegeSetConfigureManager;
const static auto& postKeyPolicyCollection = privilegeSetConfigureManager;

// KeyService
const static auto& getKeyService = privilegeSetLogin;
const static auto& headKeyService = privilegeSetLogin;
const static auto& patchKeyService = privilegeSetConfigureManager;
const static auto& putKeyService = privilegeSetConfigureManager;
const static auto& deleteKeyService = privilegeSetConfigureManager;
const static auto& postKeyService = privilegeSetConfigureManager;

// LogEntry
const static auto& getLogEntry = privilegeSetLogin;
const static auto& headLogEntry = privilegeSetLogin;
const static auto& patchLogEntry = privilegeSetConfigureManager;
const static auto& putLogEntry = privilegeSetConfigureManager;
const static auto& deleteLogEntry = privilegeSetConfigureManager;
const static auto& postLogEntry = privilegeSetConfigureManager;

// LogEntryCollection
const static auto& getLogEntryCollection = privilegeSetLogin;
const static auto& headLogEntryCollection = privilegeSetLogin;
const static auto& patchLogEntryCollection = privilegeSetConfigureManager;
const static auto& putLogEntryCollection = privilegeSetConfigureManager;
const static auto& deleteLogEntryCollection = privilegeSetConfigureManager;
const static auto& postLogEntryCollection = privilegeSetConfigureManager;

// LogService
const static auto& getLogService = privilegeSetLogin;
const static auto& headLogService = privilegeSetLogin;
const static auto& patchLogService = privilegeSetConfigureManager;
const static auto& putLogService = privilegeSetConfigureManager;
const static auto& deleteLogService = privilegeSetConfigureManager;
const static auto& postLogService = privilegeSetConfigureManager;

// LogServiceCollection
const static auto& getLogServiceCollection = privilegeSetLogin;
const static auto& headLogServiceCollection = privilegeSetLogin;
const static auto& patchLogServiceCollection = privilegeSetConfigureManager;
const static auto& putLogServiceCollection = privilegeSetConfigureManager;
const static auto& deleteLogServiceCollection = privilegeSetConfigureManager;
const static auto& postLogServiceCollection = privilegeSetConfigureManager;

// Manager
const static auto& getManager = privilegeSetLogin;
const static auto& headManager = privilegeSetLogin;
const static auto& patchManager = privilegeSetConfigureManager;
const static auto& postManager = privilegeSetConfigureManager;
const static auto& putManager = privilegeSetConfigureManager;
const static auto& deleteManager = privilegeSetConfigureManager;

// ManagerCollection
const static auto& getManagerCollection = privilegeSetLogin;
const static auto& headManagerCollection = privilegeSetLogin;
const static auto& patchManagerCollection = privilegeSetConfigureManager;
const static auto& postManagerCollection = privilegeSetConfigureManager;
const static auto& putManagerCollection = privilegeSetConfigureManager;
const static auto& deleteManagerCollection = privilegeSetConfigureManager;

// ManagerAccount
const static auto& getManagerAccount = privilegeSetConfigureManagerOrConfigureUsersOrConfigureSelf;
const static auto& headManagerAccount = privilegeSetLogin;
const static auto& patchManagerAccount = privilegeSetConfigureUsers;
const static auto& postManagerAccount = privilegeSetConfigureUsers;
const static auto& putManagerAccount = privilegeSetConfigureUsers;
const static auto& deleteManagerAccount = privilegeSetConfigureUsers;

// ManagerAccountCollection
const static auto& getManagerAccountCollection = privilegeSetLogin;
const static auto& headManagerAccountCollection = privilegeSetLogin;
const static auto& patchManagerAccountCollection = privilegeSetConfigureUsers;
const static auto& putManagerAccountCollection = privilegeSetConfigureUsers;
const static auto& deleteManagerAccountCollection = privilegeSetConfigureUsers;
const static auto& postManagerAccountCollection = privilegeSetConfigureUsers;

// ManagerDiagnosticData
const static auto& getManagerDiagnosticData = privilegeSetLogin;
const static auto& headManagerDiagnosticData = privilegeSetLogin;
const static auto& patchManagerDiagnosticData = privilegeSetConfigureManager;
const static auto& postManagerDiagnosticData = privilegeSetConfigureManager;
const static auto& putManagerDiagnosticData = privilegeSetConfigureManager;

// ManagerNetworkProtocol
const static auto& getManagerNetworkProtocol = privilegeSetLogin;
const static auto& headManagerNetworkProtocol = privilegeSetLogin;
const static auto& patchManagerNetworkProtocol = privilegeSetConfigureManager;
const static auto& postManagerNetworkProtocol = privilegeSetConfigureManager;
const static auto& putManagerNetworkProtocol = privilegeSetConfigureManager;
const static auto& deleteManagerNetworkProtocol = privilegeSetConfigureManager;

// MediaController
const static auto& getMediaController = privilegeSetLogin;
const static auto& headMediaController = privilegeSetLogin;
const static auto& patchMediaController = privilegeSetConfigureComponents;
const static auto& postMediaController = privilegeSetConfigureComponents;
const static auto& putMediaController = privilegeSetConfigureComponents;
const static auto& deleteMediaController = privilegeSetConfigureComponents;

// MediaControllerCollection
const static auto& getMediaControllerCollection = privilegeSetLogin;
const static auto& headMediaControllerCollection = privilegeSetLogin;
const static auto& patchMediaControllerCollection = privilegeSetConfigureComponents;
const static auto& postMediaControllerCollection = privilegeSetConfigureComponents;
const static auto& putMediaControllerCollection = privilegeSetConfigureComponents;
const static auto& deleteMediaControllerCollection = privilegeSetConfigureComponents;

// Memory
const static auto& getMemory = privilegeSetLogin;
const static auto& headMemory = privilegeSetLogin;
const static auto& patchMemory = privilegeSetConfigureComponents;
const static auto& postMemory = privilegeSetConfigureComponents;
const static auto& putMemory = privilegeSetConfigureComponents;
const static auto& deleteMemory = privilegeSetConfigureComponents;

// MemoryCollection
const static auto& getMemoryCollection = privilegeSetLogin;
const static auto& headMemoryCollection = privilegeSetLogin;
const static auto& patchMemoryCollection = privilegeSetConfigureComponents;
const static auto& postMemoryCollection = privilegeSetConfigureComponents;
const static auto& putMemoryCollection = privilegeSetConfigureComponents;
const static auto& deleteMemoryCollection = privilegeSetConfigureComponents;

// MemoryChunks
const static auto& getMemoryChunks = privilegeSetLogin;
const static auto& headMemoryChunks = privilegeSetLogin;
const static auto& patchMemoryChunks = privilegeSetConfigureComponents;
const static auto& postMemoryChunks = privilegeSetConfigureComponents;
const static auto& putMemoryChunks = privilegeSetConfigureComponents;
const static auto& deleteMemoryChunks = privilegeSetConfigureComponents;

// MemoryChunksCollection
const static auto& getMemoryChunksCollection = privilegeSetLogin;
const static auto& headMemoryChunksCollection = privilegeSetLogin;
const static auto& patchMemoryChunksCollection = privilegeSetConfigureComponents;
const static auto& postMemoryChunksCollection = privilegeSetConfigureComponents;
const static auto& putMemoryChunksCollection = privilegeSetConfigureComponents;
const static auto& deleteMemoryChunksCollection = privilegeSetConfigureComponents;

// MemoryDomain
const static auto& getMemoryDomain = privilegeSetLogin;
const static auto& headMemoryDomain = privilegeSetLogin;
const static auto& patchMemoryDomain = privilegeSetConfigureComponents;
const static auto& postMemoryDomain = privilegeSetConfigureComponents;
const static auto& putMemoryDomain = privilegeSetConfigureComponents;
const static auto& deleteMemoryDomain = privilegeSetConfigureComponents;

// MemoryDomainCollection
const static auto& getMemoryDomainCollection = privilegeSetLogin;
const static auto& headMemoryDomainCollection = privilegeSetLogin;
const static auto& patchMemoryDomainCollection = privilegeSetConfigureComponents;
const static auto& postMemoryDomainCollection = privilegeSetConfigureComponents;
const static auto& putMemoryDomainCollection = privilegeSetConfigureComponents;
const static auto& deleteMemoryDomainCollection = privilegeSetConfigureComponents;

// MemoryMetrics
const static auto& getMemoryMetrics = privilegeSetLogin;
const static auto& headMemoryMetrics = privilegeSetLogin;
const static auto& patchMemoryMetrics = privilegeSetConfigureComponents;
const static auto& postMemoryMetrics = privilegeSetConfigureComponents;
const static auto& putMemoryMetrics = privilegeSetConfigureComponents;
const static auto& deleteMemoryMetrics = privilegeSetConfigureComponents;

// MessageRegistryFile
const static auto& getMessageRegistryFile = privilegeSetLogin;
const static auto& headMessageRegistryFile = privilegeSetLogin;
const static auto& patchMessageRegistryFile = privilegeSetConfigureManager;
const static auto& postMessageRegistryFile = privilegeSetConfigureManager;
const static auto& putMessageRegistryFile = privilegeSetConfigureManager;
const static auto& deleteMessageRegistryFile = privilegeSetConfigureManager;

// MessageRegistryFileCollection
const static auto& getMessageRegistryFileCollection = privilegeSetLogin;
const static auto& headMessageRegistryFileCollection = privilegeSetLogin;
const static auto& patchMessageRegistryFileCollection = privilegeSetConfigureManager;
const static auto& postMessageRegistryFileCollection = privilegeSetConfigureManager;
const static auto& putMessageRegistryFileCollection = privilegeSetConfigureManager;
const static auto& deleteMessageRegistryFileCollection = privilegeSetConfigureManager;

// MetricDefinition
const static auto& getMetricDefinition = privilegeSetLogin;
const static auto& headMetricDefinition = privilegeSetLogin;
const static auto& patchMetricDefinition = privilegeSetConfigureManager;
const static auto& putMetricDefinition = privilegeSetConfigureManager;
const static auto& deleteMetricDefinition = privilegeSetConfigureManager;
const static auto& postMetricDefinition = privilegeSetConfigureManager;

// MetricDefinitionCollection
const static auto& getMetricDefinitionCollection = privilegeSetLogin;
const static auto& headMetricDefinitionCollection = privilegeSetLogin;
const static auto& patchMetricDefinitionCollection = privilegeSetConfigureManager;
const static auto& putMetricDefinitionCollection = privilegeSetConfigureManager;
const static auto& deleteMetricDefinitionCollection = privilegeSetConfigureManager;
const static auto& postMetricDefinitionCollection = privilegeSetConfigureManager;

// MetricReport
const static auto& getMetricReport = privilegeSetLogin;
const static auto& headMetricReport = privilegeSetLogin;
const static auto& patchMetricReport = privilegeSetConfigureManager;
const static auto& putMetricReport = privilegeSetConfigureManager;
const static auto& deleteMetricReport = privilegeSetConfigureManager;
const static auto& postMetricReport = privilegeSetConfigureManager;

// MetricReportCollection
const static auto& getMetricReportCollection = privilegeSetLogin;
const static auto& headMetricReportCollection = privilegeSetLogin;
const static auto& patchMetricReportCollection = privilegeSetConfigureManager;
const static auto& putMetricReportCollection = privilegeSetConfigureManager;
const static auto& deleteMetricReportCollection = privilegeSetConfigureManager;
const static auto& postMetricReportCollection = privilegeSetConfigureManager;

// MetricReportDefinition
const static auto& getMetricReportDefinition = privilegeSetLogin;
const static auto& headMetricReportDefinition = privilegeSetLogin;
const static auto& patchMetricReportDefinition = privilegeSetConfigureManager;
const static auto& putMetricReportDefinition = privilegeSetConfigureManager;
const static auto& deleteMetricReportDefinition = privilegeSetConfigureManager;
const static auto& postMetricReportDefinition = privilegeSetConfigureManager;

// MetricReportDefinitionCollection
const static auto& getMetricReportDefinitionCollection = privilegeSetLogin;
const static auto& headMetricReportDefinitionCollection = privilegeSetLogin;
const static auto& patchMetricReportDefinitionCollection = privilegeSetConfigureManager;
const static auto& putMetricReportDefinitionCollection = privilegeSetConfigureManager;
const static auto& deleteMetricReportDefinitionCollection = privilegeSetConfigureManager;
const static auto& postMetricReportDefinitionCollection = privilegeSetConfigureManager;

// NetworkAdapter
const static auto& getNetworkAdapter = privilegeSetLogin;
const static auto& headNetworkAdapter = privilegeSetLogin;
const static auto& patchNetworkAdapter = privilegeSetConfigureComponents;
const static auto& postNetworkAdapter = privilegeSetConfigureComponents;
const static auto& putNetworkAdapter = privilegeSetConfigureComponents;
const static auto& deleteNetworkAdapter = privilegeSetConfigureComponents;

// NetworkAdapterCollection
const static auto& getNetworkAdapterCollection = privilegeSetLogin;
const static auto& headNetworkAdapterCollection = privilegeSetLogin;
const static auto& patchNetworkAdapterCollection = privilegeSetConfigureComponents;
const static auto& postNetworkAdapterCollection = privilegeSetConfigureComponents;
const static auto& putNetworkAdapterCollection = privilegeSetConfigureComponents;
const static auto& deleteNetworkAdapterCollection = privilegeSetConfigureComponents;

// NetworkAdapterMetrics
const static auto& getNetworkAdapterMetrics = privilegeSetLogin;
const static auto& headNetworkAdapterMetrics = privilegeSetLogin;
const static auto& patchNetworkAdapterMetrics = privilegeSetConfigureManager;
const static auto& putNetworkAdapterMetrics = privilegeSetConfigureManager;
const static auto& deleteNetworkAdapterMetrics = privilegeSetConfigureManager;
const static auto& postNetworkAdapterMetrics = privilegeSetConfigureManager;

// NetworkDeviceFunction
const static auto& getNetworkDeviceFunction = privilegeSetLogin;
const static auto& headNetworkDeviceFunction = privilegeSetLogin;
const static auto& patchNetworkDeviceFunction = privilegeSetConfigureComponents;
const static auto& postNetworkDeviceFunction = privilegeSetConfigureComponents;
const static auto& putNetworkDeviceFunction = privilegeSetConfigureComponents;
const static auto& deleteNetworkDeviceFunction = privilegeSetConfigureComponents;

// NetworkDeviceFunctionCollection
const static auto& getNetworkDeviceFunctionCollection = privilegeSetLogin;
const static auto& headNetworkDeviceFunctionCollection = privilegeSetLogin;
const static auto& patchNetworkDeviceFunctionCollection = privilegeSetConfigureComponents;
const static auto& postNetworkDeviceFunctionCollection = privilegeSetConfigureComponents;
const static auto& putNetworkDeviceFunctionCollection = privilegeSetConfigureComponents;
const static auto& deleteNetworkDeviceFunctionCollection = privilegeSetConfigureComponents;

// NetworkDeviceFunctionMetrics
const static auto& getNetworkDeviceFunctionMetrics = privilegeSetLogin;
const static auto& headNetworkDeviceFunctionMetrics = privilegeSetLogin;
const static auto& patchNetworkDeviceFunctionMetrics = privilegeSetConfigureManager;
const static auto& putNetworkDeviceFunctionMetrics = privilegeSetConfigureManager;
const static auto& deleteNetworkDeviceFunctionMetrics = privilegeSetConfigureManager;
const static auto& postNetworkDeviceFunctionMetrics = privilegeSetConfigureManager;

// NetworkInterface
const static auto& getNetworkInterface = privilegeSetLogin;
const static auto& headNetworkInterface = privilegeSetLogin;
const static auto& patchNetworkInterface = privilegeSetConfigureComponents;
const static auto& postNetworkInterface = privilegeSetConfigureComponents;
const static auto& putNetworkInterface = privilegeSetConfigureComponents;
const static auto& deleteNetworkInterface = privilegeSetConfigureComponents;

// NetworkInterfaceCollection
const static auto& getNetworkInterfaceCollection = privilegeSetLogin;
const static auto& headNetworkInterfaceCollection = privilegeSetLogin;
const static auto& patchNetworkInterfaceCollection = privilegeSetConfigureComponents;
const static auto& postNetworkInterfaceCollection = privilegeSetConfigureComponents;
const static auto& putNetworkInterfaceCollection = privilegeSetConfigureComponents;
const static auto& deleteNetworkInterfaceCollection = privilegeSetConfigureComponents;

// NetworkPort
const static auto& getNetworkPort = privilegeSetLogin;
const static auto& headNetworkPort = privilegeSetLogin;
const static auto& patchNetworkPort = privilegeSetConfigureComponents;
const static auto& postNetworkPort = privilegeSetConfigureComponents;
const static auto& putNetworkPort = privilegeSetConfigureComponents;
const static auto& deleteNetworkPort = privilegeSetConfigureComponents;

// NetworkPortCollection
const static auto& getNetworkPortCollection = privilegeSetLogin;
const static auto& headNetworkPortCollection = privilegeSetLogin;
const static auto& patchNetworkPortCollection = privilegeSetConfigureComponents;
const static auto& postNetworkPortCollection = privilegeSetConfigureComponents;
const static auto& putNetworkPortCollection = privilegeSetConfigureComponents;
const static auto& deleteNetworkPortCollection = privilegeSetConfigureComponents;

// OperatingConfig
const static auto& getOperatingConfig = privilegeSetLogin;
const static auto& headOperatingConfig = privilegeSetLogin;
const static auto& patchOperatingConfig = privilegeSetConfigureComponents;
const static auto& postOperatingConfig = privilegeSetConfigureComponents;
const static auto& putOperatingConfig = privilegeSetConfigureComponents;
const static auto& deleteOperatingConfig = privilegeSetConfigureComponents;

// OperatingConfigCollection
const static auto& getOperatingConfigCollection = privilegeSetLogin;
const static auto& headOperatingConfigCollection = privilegeSetLogin;
const static auto& patchOperatingConfigCollection = privilegeSetConfigureComponents;
const static auto& postOperatingConfigCollection = privilegeSetConfigureComponents;
const static auto& putOperatingConfigCollection = privilegeSetConfigureComponents;
const static auto& deleteOperatingConfigCollection = privilegeSetConfigureComponents;

// Outlet
const static auto& getOutlet = privilegeSetLogin;
const static auto& headOutlet = privilegeSetLogin;
const static auto& patchOutlet = privilegeSetConfigureComponents;
const static auto& postOutlet = privilegeSetConfigureComponents;
const static auto& putOutlet = privilegeSetConfigureComponents;
const static auto& deleteOutlet = privilegeSetConfigureComponents;

// OutletCollection
const static auto& getOutletCollection = privilegeSetLogin;
const static auto& headOutletCollection = privilegeSetLogin;
const static auto& patchOutletCollection = privilegeSetConfigureComponents;
const static auto& postOutletCollection = privilegeSetConfigureComponents;
const static auto& putOutletCollection = privilegeSetConfigureComponents;
const static auto& deleteOutletCollection = privilegeSetConfigureComponents;

// OutletGroup
const static auto& getOutletGroup = privilegeSetLogin;
const static auto& headOutletGroup = privilegeSetLogin;
const static auto& patchOutletGroup = privilegeSetConfigureComponents;
const static auto& postOutletGroup = privilegeSetConfigureComponents;
const static auto& putOutletGroup = privilegeSetConfigureComponents;
const static auto& deleteOutletGroup = privilegeSetConfigureComponents;

// OutletGroupCollection
const static auto& getOutletGroupCollection = privilegeSetLogin;
const static auto& headOutletGroupCollection = privilegeSetLogin;
const static auto& patchOutletGroupCollection = privilegeSetConfigureComponents;
const static auto& postOutletGroupCollection = privilegeSetConfigureComponents;
const static auto& putOutletGroupCollection = privilegeSetConfigureComponents;
const static auto& deleteOutletGroupCollection = privilegeSetConfigureComponents;

// PCIeDevice
const static auto& getPCIeDevice = privilegeSetLogin;
const static auto& headPCIeDevice = privilegeSetLogin;
const static auto& patchPCIeDevice = privilegeSetConfigureComponents;
const static auto& postPCIeDevice = privilegeSetConfigureComponents;
const static auto& putPCIeDevice = privilegeSetConfigureComponents;
const static auto& deletePCIeDevice = privilegeSetConfigureComponents;

// PCIeDeviceCollection
const static auto& getPCIeDeviceCollection = privilegeSetLogin;
const static auto& headPCIeDeviceCollection = privilegeSetLogin;
const static auto& patchPCIeDeviceCollection = privilegeSetConfigureComponents;
const static auto& postPCIeDeviceCollection = privilegeSetConfigureComponents;
const static auto& putPCIeDeviceCollection = privilegeSetConfigureComponents;
const static auto& deletePCIeDeviceCollection = privilegeSetConfigureComponents;

// PCIeFunction
const static auto& getPCIeFunction = privilegeSetLogin;
const static auto& headPCIeFunction = privilegeSetLogin;
const static auto& patchPCIeFunction = privilegeSetConfigureComponents;
const static auto& postPCIeFunction = privilegeSetConfigureComponents;
const static auto& putPCIeFunction = privilegeSetConfigureComponents;
const static auto& deletePCIeFunction = privilegeSetConfigureComponents;

// PCIeFunctionCollection
const static auto& getPCIeFunctionCollection = privilegeSetLogin;
const static auto& headPCIeFunctionCollection = privilegeSetLogin;
const static auto& patchPCIeFunctionCollection = privilegeSetConfigureComponents;
const static auto& postPCIeFunctionCollection = privilegeSetConfigureComponents;
const static auto& putPCIeFunctionCollection = privilegeSetConfigureComponents;
const static auto& deletePCIeFunctionCollection = privilegeSetConfigureComponents;

// PCIeSlots
const static auto& getPCIeSlots = privilegeSetLogin;
const static auto& headPCIeSlots = privilegeSetLogin;
const static auto& patchPCIeSlots = privilegeSetConfigureComponents;
const static auto& postPCIeSlots = privilegeSetConfigureComponents;
const static auto& putPCIeSlots = privilegeSetConfigureComponents;
const static auto& deletePCIeSlots = privilegeSetConfigureComponents;

// Port
const static auto& getPort = privilegeSetLogin;
const static auto& headPort = privilegeSetLogin;
const static auto& patchPort = privilegeSetConfigureComponents;
const static auto& postPort = privilegeSetConfigureComponents;
const static auto& putPort = privilegeSetConfigureComponents;
const static auto& deletePort = privilegeSetConfigureComponents;

// PortCollection
const static auto& getPortCollection = privilegeSetLogin;
const static auto& headPortCollection = privilegeSetLogin;
const static auto& patchPortCollection = privilegeSetConfigureComponents;
const static auto& postPortCollection = privilegeSetConfigureComponents;
const static auto& putPortCollection = privilegeSetConfigureComponents;
const static auto& deletePortCollection = privilegeSetConfigureComponents;

// PortMetrics
const static auto& getPortMetrics = privilegeSetLogin;
const static auto& headPortMetrics = privilegeSetLogin;
const static auto& patchPortMetrics = privilegeSetConfigureComponents;
const static auto& postPortMetrics = privilegeSetConfigureComponents;
const static auto& putPortMetrics = privilegeSetConfigureComponents;
const static auto& deletePortMetrics = privilegeSetConfigureComponents;

// Power
const static auto& getPower = privilegeSetLogin;
const static auto& headPower = privilegeSetLogin;
const static auto& patchPower = privilegeSetConfigureManager;
const static auto& putPower = privilegeSetConfigureManager;
const static auto& deletePower = privilegeSetConfigureManager;
const static auto& postPower = privilegeSetConfigureManager;

// PowerDistribution
const static auto& getPowerDistribution = privilegeSetLogin;
const static auto& headPowerDistribution = privilegeSetLogin;
const static auto& patchPowerDistribution = privilegeSetConfigureComponents;
const static auto& postPowerDistribution = privilegeSetConfigureComponents;
const static auto& putPowerDistribution = privilegeSetConfigureComponents;
const static auto& deletePowerDistribution = privilegeSetConfigureComponents;

// PowerDistributionCollection
const static auto& getPowerDistributionCollection = privilegeSetLogin;
const static auto& headPowerDistributionCollection = privilegeSetLogin;
const static auto& patchPowerDistributionCollection = privilegeSetConfigureComponents;
const static auto& postPowerDistributionCollection = privilegeSetConfigureComponents;
const static auto& putPowerDistributionCollection = privilegeSetConfigureComponents;
const static auto& deletePowerDistributionCollection = privilegeSetConfigureComponents;

// PowerDistributionMetrics
const static auto& getPowerDistributionMetrics = privilegeSetLogin;
const static auto& headPowerDistributionMetrics = privilegeSetLogin;
const static auto& patchPowerDistributionMetrics = privilegeSetConfigureComponents;
const static auto& postPowerDistributionMetrics = privilegeSetConfigureComponents;
const static auto& putPowerDistributionMetrics = privilegeSetConfigureComponents;
const static auto& deletePowerDistributionMetrics = privilegeSetConfigureComponents;

// PowerDomain
const static auto& getPowerDomain = privilegeSetLogin;
const static auto& headPowerDomain = privilegeSetLogin;
const static auto& patchPowerDomain = privilegeSetConfigureManager;
const static auto& putPowerDomain = privilegeSetConfigureManager;
const static auto& deletePowerDomain = privilegeSetConfigureManager;
const static auto& postPowerDomain = privilegeSetConfigureManager;

// PowerDomainCollection
const static auto& getPowerDomainCollection = privilegeSetLogin;
const static auto& headPowerDomainCollection = privilegeSetLogin;
const static auto& patchPowerDomainCollection = privilegeSetConfigureManager;
const static auto& putPowerDomainCollection = privilegeSetConfigureManager;
const static auto& deletePowerDomainCollection = privilegeSetConfigureManager;
const static auto& postPowerDomainCollection = privilegeSetConfigureManager;

// PowerEquipment
const static auto& getPowerEquipment = privilegeSetLogin;
const static auto& headPowerEquipment = privilegeSetLogin;
const static auto& patchPowerEquipment = privilegeSetConfigureManager;
const static auto& putPowerEquipment = privilegeSetConfigureManager;
const static auto& deletePowerEquipment = privilegeSetConfigureManager;
const static auto& postPowerEquipment = privilegeSetConfigureManager;

// PowerSubsystem
const static auto& getPowerSubsystem = privilegeSetLogin;
const static auto& headPowerSubsystem = privilegeSetLogin;
const static auto& patchPowerSubsystem = privilegeSetConfigureManager;
const static auto& putPowerSubsystem = privilegeSetConfigureManager;
const static auto& deletePowerSubsystem = privilegeSetConfigureManager;
const static auto& postPowerSubsystem = privilegeSetConfigureManager;

// PowerSupply
const static auto& getPowerSupply = privilegeSetLogin;
const static auto& headPowerSupply = privilegeSetLogin;
const static auto& patchPowerSupply = privilegeSetConfigureManager;
const static auto& putPowerSupply = privilegeSetConfigureManager;
const static auto& deletePowerSupply = privilegeSetConfigureManager;
const static auto& postPowerSupply = privilegeSetConfigureManager;

// PowerSupplyCollection
const static auto& getPowerSupplyCollection = privilegeSetLogin;
const static auto& headPowerSupplyCollection = privilegeSetLogin;
const static auto& patchPowerSupplyCollection = privilegeSetConfigureManager;
const static auto& putPowerSupplyCollection = privilegeSetConfigureManager;
const static auto& deletePowerSupplyCollection = privilegeSetConfigureManager;
const static auto& postPowerSupplyCollection = privilegeSetConfigureManager;

// PowerSupplyMetrics
const static auto& getPowerSupplyMetrics = privilegeSetLogin;
const static auto& headPowerSupplyMetrics = privilegeSetLogin;
const static auto& patchPowerSupplyMetrics = privilegeSetConfigureManager;
const static auto& putPowerSupplyMetrics = privilegeSetConfigureManager;
const static auto& deletePowerSupplyMetrics = privilegeSetConfigureManager;
const static auto& postPowerSupplyMetrics = privilegeSetConfigureManager;

// Processor
const static auto& getProcessor = privilegeSetLogin;
const static auto& headProcessor = privilegeSetLogin;
const static auto& patchProcessor = privilegeSetConfigureComponents;
const static auto& putProcessor = privilegeSetConfigureComponents;
const static auto& deleteProcessor = privilegeSetConfigureComponents;
const static auto& postProcessor = privilegeSetConfigureComponents;

// ProcessorCollection
const static auto& getProcessorCollection = privilegeSetLogin;
const static auto& headProcessorCollection = privilegeSetLogin;
const static auto& patchProcessorCollection = privilegeSetConfigureComponents;
const static auto& putProcessorCollection = privilegeSetConfigureComponents;
const static auto& deleteProcessorCollection = privilegeSetConfigureComponents;
const static auto& postProcessorCollection = privilegeSetConfigureComponents;

// ProcessorMetrics
const static auto& getProcessorMetrics = privilegeSetLogin;
const static auto& headProcessorMetrics = privilegeSetLogin;
const static auto& patchProcessorMetrics = privilegeSetConfigureComponents;
const static auto& putProcessorMetrics = privilegeSetConfigureComponents;
const static auto& deleteProcessorMetrics = privilegeSetConfigureComponents;
const static auto& postProcessorMetrics = privilegeSetConfigureComponents;

// ResourceBlock
const static auto& getResourceBlock = privilegeSetLogin;
const static auto& headResourceBlock = privilegeSetLogin;
const static auto& patchResourceBlock = privilegeSetConfigureComponents;
const static auto& putResourceBlock = privilegeSetConfigureComponents;
const static auto& deleteResourceBlock = privilegeSetConfigureComponents;
const static auto& postResourceBlock = privilegeSetConfigureComponents;

// ResourceBlockCollection
const static auto& getResourceBlockCollection = privilegeSetLogin;
const static auto& headResourceBlockCollection = privilegeSetLogin;
const static auto& patchResourceBlockCollection = privilegeSetConfigureComponents;
const static auto& putResourceBlockCollection = privilegeSetConfigureComponents;
const static auto& deleteResourceBlockCollection = privilegeSetConfigureComponents;
const static auto& postResourceBlockCollection = privilegeSetConfigureComponents;

// Role
const static auto& getRole = privilegeSetLogin;
const static auto& headRole = privilegeSetLogin;
const static auto& patchRole = privilegeSetConfigureManager;
const static auto& putRole = privilegeSetConfigureManager;
const static auto& deleteRole = privilegeSetConfigureManager;
const static auto& postRole = privilegeSetConfigureManager;

// RoleCollection
const static auto& getRoleCollection = privilegeSetLogin;
const static auto& headRoleCollection = privilegeSetLogin;
const static auto& patchRoleCollection = privilegeSetConfigureManager;
const static auto& putRoleCollection = privilegeSetConfigureManager;
const static auto& deleteRoleCollection = privilegeSetConfigureManager;
const static auto& postRoleCollection = privilegeSetConfigureManager;

// RouteEntry
const static auto& getRouteEntry = privilegeSetLogin;
const static auto& headRouteEntry = privilegeSetLogin;
const static auto& patchRouteEntry = privilegeSetConfigureComponents;
const static auto& putRouteEntry = privilegeSetConfigureComponents;
const static auto& deleteRouteEntry = privilegeSetConfigureComponents;
const static auto& postRouteEntry = privilegeSetConfigureComponents;

// RouteEntryCollection
const static auto& getRouteEntryCollection = privilegeSetLogin;
const static auto& headRouteEntryCollection = privilegeSetLogin;
const static auto& patchRouteEntryCollection = privilegeSetConfigureComponents;
const static auto& putRouteEntryCollection = privilegeSetConfigureComponents;
const static auto& deleteRouteEntryCollection = privilegeSetConfigureComponents;
const static auto& postRouteEntryCollection = privilegeSetConfigureComponents;

// RouteSetEntry
const static auto& getRouteSetEntry = privilegeSetLogin;
const static auto& headRouteSetEntry = privilegeSetLogin;
const static auto& patchRouteSetEntry = privilegeSetConfigureComponents;
const static auto& putRouteSetEntry = privilegeSetConfigureComponents;
const static auto& deleteRouteSetEntry = privilegeSetConfigureComponents;
const static auto& postRouteSetEntry = privilegeSetConfigureComponents;

// RouteSetEntryCollection
const static auto& getRouteSetEntryCollection = privilegeSetLogin;
const static auto& headRouteSetEntryCollection = privilegeSetLogin;
const static auto& patchRouteSetEntryCollection = privilegeSetConfigureComponents;
const static auto& putRouteSetEntryCollection = privilegeSetConfigureComponents;
const static auto& deleteRouteSetEntryCollection = privilegeSetConfigureComponents;
const static auto& postRouteSetEntryCollection = privilegeSetConfigureComponents;

// SecureBoot
const static auto& getSecureBoot = privilegeSetLogin;
const static auto& headSecureBoot = privilegeSetLogin;
const static auto& patchSecureBoot = privilegeSetConfigureComponents;
const static auto& postSecureBoot = privilegeSetConfigureComponents;
const static auto& putSecureBoot = privilegeSetConfigureComponents;
const static auto& deleteSecureBoot = privilegeSetConfigureComponents;

// SecureBootDatabase
const static auto& getSecureBootDatabase = privilegeSetLogin;
const static auto& headSecureBootDatabase = privilegeSetLogin;
const static auto& patchSecureBootDatabase = privilegeSetConfigureComponents;
const static auto& postSecureBootDatabase = privilegeSetConfigureComponents;
const static auto& putSecureBootDatabase = privilegeSetConfigureComponents;
const static auto& deleteSecureBootDatabase = privilegeSetConfigureComponents;

// SecureBootDatabaseCollection
const static auto& getSecureBootDatabaseCollection = privilegeSetLogin;
const static auto& headSecureBootDatabaseCollection = privilegeSetLogin;
const static auto& patchSecureBootDatabaseCollection = privilegeSetConfigureComponents;
const static auto& postSecureBootDatabaseCollection = privilegeSetConfigureComponents;
const static auto& putSecureBootDatabaseCollection = privilegeSetConfigureComponents;
const static auto& deleteSecureBootDatabaseCollection = privilegeSetConfigureComponents;

// Sensor
const static auto& getSensor = privilegeSetLogin;
const static auto& headSensor = privilegeSetLogin;
const static auto& patchSensor = privilegeSetConfigureComponents;
const static auto& postSensor = privilegeSetConfigureComponents;
const static auto& putSensor = privilegeSetConfigureComponents;
const static auto& deleteSensor = privilegeSetConfigureComponents;

// SensorCollection
const static auto& getSensorCollection = privilegeSetLogin;
const static auto& headSensorCollection = privilegeSetLogin;
const static auto& patchSensorCollection = privilegeSetConfigureComponents;
const static auto& postSensorCollection = privilegeSetConfigureComponents;
const static auto& putSensorCollection = privilegeSetConfigureComponents;
const static auto& deleteSensorCollection = privilegeSetConfigureComponents;

// SerialInterface
const static auto& getSerialInterface = privilegeSetLogin;
const static auto& headSerialInterface = privilegeSetLogin;
const static auto& patchSerialInterface = privilegeSetConfigureManager;
const static auto& putSerialInterface = privilegeSetConfigureManager;
const static auto& deleteSerialInterface = privilegeSetConfigureManager;
const static auto& postSerialInterface = privilegeSetConfigureManager;

// SerialInterfaceCollection
const static auto& getSerialInterfaceCollection = privilegeSetLogin;
const static auto& headSerialInterfaceCollection = privilegeSetLogin;
const static auto& patchSerialInterfaceCollection = privilegeSetConfigureManager;
const static auto& putSerialInterfaceCollection = privilegeSetConfigureManager;
const static auto& deleteSerialInterfaceCollection = privilegeSetConfigureManager;
const static auto& postSerialInterfaceCollection = privilegeSetConfigureManager;

// ServiceRoot
const static auto& getServiceRoot = privilegeSetLoginOrNoAuth;
const static auto& headServiceRoot = privilegeSetLoginOrNoAuth;
const static auto& patchServiceRoot = privilegeSetConfigureManager;
const static auto& putServiceRoot = privilegeSetConfigureManager;
const static auto& deleteServiceRoot = privilegeSetConfigureManager;
const static auto& postServiceRoot = privilegeSetConfigureManager;

// Session
const static auto& getSession = privilegeSetLogin;
const static auto& headSession = privilegeSetLogin;
const static auto& patchSession = privilegeSetConfigureManager;
const static auto& putSession = privilegeSetConfigureManager;
const static auto& deleteSession = privilegeSetConfigureManagerOrConfigureSelf;
const static auto& postSession = privilegeSetConfigureManager;

// SessionCollection
const static auto& getSessionCollection = privilegeSetLogin;
const static auto& headSessionCollection = privilegeSetLogin;
const static auto& patchSessionCollection = privilegeSetConfigureManager;
const static auto& putSessionCollection = privilegeSetConfigureManager;
const static auto& deleteSessionCollection = privilegeSetConfigureManager;
const static auto& postSessionCollection = privilegeSetLogin;

// SessionService
const static auto& getSessionService = privilegeSetLogin;
const static auto& headSessionService = privilegeSetLogin;
const static auto& patchSessionService = privilegeSetConfigureManager;
const static auto& putSessionService = privilegeSetConfigureManager;
const static auto& deleteSessionService = privilegeSetConfigureManager;
const static auto& postSessionService = privilegeSetConfigureManager;

// Signature
const static auto& getSignature = privilegeSetLogin;
const static auto& headSignature = privilegeSetLogin;
const static auto& patchSignature = privilegeSetConfigureComponents;
const static auto& postSignature = privilegeSetConfigureComponents;
const static auto& putSignature = privilegeSetConfigureComponents;
const static auto& deleteSignature = privilegeSetConfigureComponents;

// SignatureCollection
const static auto& getSignatureCollection = privilegeSetLogin;
const static auto& headSignatureCollection = privilegeSetLogin;
const static auto& patchSignatureCollection = privilegeSetConfigureComponents;
const static auto& postSignatureCollection = privilegeSetConfigureComponents;
const static auto& putSignatureCollection = privilegeSetConfigureComponents;
const static auto& deleteSignatureCollection = privilegeSetConfigureComponents;

// SimpleStorage
const static auto& getSimpleStorage = privilegeSetLogin;
const static auto& headSimpleStorage = privilegeSetLogin;
const static auto& patchSimpleStorage = privilegeSetConfigureComponents;
const static auto& postSimpleStorage = privilegeSetConfigureComponents;
const static auto& putSimpleStorage = privilegeSetConfigureComponents;
const static auto& deleteSimpleStorage = privilegeSetConfigureComponents;

// SimpleStorageCollection
const static auto& getSimpleStorageCollection = privilegeSetLogin;
const static auto& headSimpleStorageCollection = privilegeSetLogin;
const static auto& patchSimpleStorageCollection = privilegeSetConfigureComponents;
const static auto& postSimpleStorageCollection = privilegeSetConfigureComponents;
const static auto& putSimpleStorageCollection = privilegeSetConfigureComponents;
const static auto& deleteSimpleStorageCollection = privilegeSetConfigureComponents;

// SoftwareInventory
const static auto& getSoftwareInventory = privilegeSetLogin;
const static auto& headSoftwareInventory = privilegeSetLogin;
const static auto& patchSoftwareInventory = privilegeSetConfigureComponents;
const static auto& postSoftwareInventory = privilegeSetConfigureComponents;
const static auto& putSoftwareInventory = privilegeSetConfigureComponents;
const static auto& deleteSoftwareInventory = privilegeSetConfigureComponents;

// SoftwareInventoryCollection
const static auto& getSoftwareInventoryCollection = privilegeSetLogin;
const static auto& headSoftwareInventoryCollection = privilegeSetLogin;
const static auto& patchSoftwareInventoryCollection = privilegeSetConfigureComponents;
const static auto& postSoftwareInventoryCollection = privilegeSetConfigureComponents;
const static auto& putSoftwareInventoryCollection = privilegeSetConfigureComponents;
const static auto& deleteSoftwareInventoryCollection = privilegeSetConfigureComponents;

// Storage
const static auto& getStorage = privilegeSetLogin;
const static auto& headStorage = privilegeSetLogin;
const static auto& patchStorage = privilegeSetConfigureComponents;
const static auto& postStorage = privilegeSetConfigureComponents;
const static auto& putStorage = privilegeSetConfigureComponents;
const static auto& deleteStorage = privilegeSetConfigureComponents;

// StorageCollection
const static auto& getStorageCollection = privilegeSetLogin;
const static auto& headStorageCollection = privilegeSetLogin;
const static auto& patchStorageCollection = privilegeSetConfigureComponents;
const static auto& postStorageCollection = privilegeSetConfigureComponents;
const static auto& putStorageCollection = privilegeSetConfigureComponents;
const static auto& deleteStorageCollection = privilegeSetConfigureComponents;

// StorageController
const static auto& getStorageController = privilegeSetLogin;
const static auto& headStorageController = privilegeSetLogin;
const static auto& patchStorageController = privilegeSetConfigureComponents;
const static auto& postStorageController = privilegeSetConfigureComponents;
const static auto& putStorageController = privilegeSetConfigureComponents;
const static auto& deleteStorageController = privilegeSetConfigureComponents;

// StorageControllerCollection
const static auto& getStorageControllerCollection = privilegeSetLogin;
const static auto& headStorageControllerCollection = privilegeSetLogin;
const static auto& patchStorageControllerCollection = privilegeSetConfigureComponents;
const static auto& postStorageControllerCollection = privilegeSetConfigureComponents;
const static auto& putStorageControllerCollection = privilegeSetConfigureComponents;
const static auto& deleteStorageControllerCollection = privilegeSetConfigureComponents;

// Switch
const static auto& getSwitch = privilegeSetLogin;
const static auto& headSwitch = privilegeSetLogin;
const static auto& patchSwitch = privilegeSetConfigureComponents;
const static auto& postSwitch = privilegeSetConfigureComponents;
const static auto& putSwitch = privilegeSetConfigureComponents;
const static auto& deleteSwitch = privilegeSetConfigureComponents;

// SwitchCollection
const static auto& getSwitchCollection = privilegeSetLogin;
const static auto& headSwitchCollection = privilegeSetLogin;
const static auto& patchSwitchCollection = privilegeSetConfigureComponents;
const static auto& postSwitchCollection = privilegeSetConfigureComponents;
const static auto& putSwitchCollection = privilegeSetConfigureComponents;
const static auto& deleteSwitchCollection = privilegeSetConfigureComponents;

// Task
const static auto& getTask = privilegeSetLogin;
const static auto& headTask = privilegeSetLogin;
const static auto& patchTask = privilegeSetConfigureManager;
const static auto& putTask = privilegeSetConfigureManager;
const static auto& deleteTask = privilegeSetConfigureManager;
const static auto& postTask = privilegeSetConfigureManager;

// TaskCollection
const static auto& getTaskCollection = privilegeSetLogin;
const static auto& headTaskCollection = privilegeSetLogin;
const static auto& patchTaskCollection = privilegeSetConfigureManager;
const static auto& putTaskCollection = privilegeSetConfigureManager;
const static auto& deleteTaskCollection = privilegeSetConfigureManager;
const static auto& postTaskCollection = privilegeSetConfigureManager;

// TaskService
const static auto& getTaskService = privilegeSetLogin;
const static auto& headTaskService = privilegeSetLogin;
const static auto& patchTaskService = privilegeSetConfigureManager;
const static auto& putTaskService = privilegeSetConfigureManager;
const static auto& deleteTaskService = privilegeSetConfigureManager;
const static auto& postTaskService = privilegeSetConfigureManager;

// TelemetryService
const static auto& getTelemetryService = privilegeSetLogin;
const static auto& headTelemetryService = privilegeSetLogin;
const static auto& patchTelemetryService = privilegeSetConfigureManager;
const static auto& putTelemetryService = privilegeSetConfigureManager;
const static auto& deleteTelemetryService = privilegeSetConfigureManager;
const static auto& postTelemetryService = privilegeSetConfigureManager;

// Thermal
const static auto& getThermal = privilegeSetLogin;
const static auto& headThermal = privilegeSetLogin;
const static auto& patchThermal = privilegeSetConfigureManager;
const static auto& putThermal = privilegeSetConfigureManager;
const static auto& deleteThermal = privilegeSetConfigureManager;
const static auto& postThermal = privilegeSetConfigureManager;

// ThermalMetrics
const static auto& getThermalMetrics = privilegeSetLogin;
const static auto& headThermalMetrics = privilegeSetLogin;
const static auto& patchThermalMetrics = privilegeSetConfigureManager;
const static auto& putThermalMetrics = privilegeSetConfigureManager;
const static auto& deleteThermalMetrics = privilegeSetConfigureManager;
const static auto& postThermalMetrics = privilegeSetConfigureManager;

// ThermalSubsystem
const static auto& getThermalSubsystem = privilegeSetLogin;
const static auto& headThermalSubsystem = privilegeSetLogin;
const static auto& patchThermalSubsystem = privilegeSetConfigureManager;
const static auto& putThermalSubsystem = privilegeSetConfigureManager;
const static auto& deleteThermalSubsystem = privilegeSetConfigureManager;
const static auto& postThermalSubsystem = privilegeSetConfigureManager;

// Triggers
const static auto& getTriggers = privilegeSetLogin;
const static auto& headTriggers = privilegeSetLogin;
const static auto& patchTriggers = privilegeSetConfigureManager;
const static auto& putTriggers = privilegeSetConfigureManager;
const static auto& deleteTriggers = privilegeSetConfigureManager;
const static auto& postTriggers = privilegeSetConfigureManager;

// TriggersCollection
const static auto& getTriggersCollection = privilegeSetLogin;
const static auto& headTriggersCollection = privilegeSetLogin;
const static auto& patchTriggersCollection = privilegeSetConfigureManager;
const static auto& putTriggersCollection = privilegeSetConfigureManager;
const static auto& deleteTriggersCollection = privilegeSetConfigureManager;
const static auto& postTriggersCollection = privilegeSetConfigureManager;

// UpdateService
const static auto& getUpdateService = privilegeSetLogin;
const static auto& headUpdateService = privilegeSetLogin;
const static auto& patchUpdateService = privilegeSetConfigureComponents;
const static auto& postUpdateService = privilegeSetConfigureComponents;
const static auto& putUpdateService = privilegeSetConfigureComponents;
const static auto& deleteUpdateService = privilegeSetConfigureComponents;

// USBController
const static auto& getUSBController = privilegeSetLogin;
const static auto& headUSBController = privilegeSetLogin;
const static auto& patchUSBController = privilegeSetConfigureComponents;
const static auto& putUSBController = privilegeSetConfigureComponents;
const static auto& deleteUSBController = privilegeSetConfigureComponents;
const static auto& postUSBController = privilegeSetConfigureComponents;

// USBControllerCollection
const static auto& getUSBControllerCollection = privilegeSetLogin;
const static auto& headUSBControllerCollection = privilegeSetLogin;
const static auto& patchUSBControllerCollection = privilegeSetConfigureComponents;
const static auto& putUSBControllerCollection = privilegeSetConfigureComponents;
const static auto& deleteUSBControllerCollection = privilegeSetConfigureComponents;
const static auto& postUSBControllerCollection = privilegeSetConfigureComponents;

// VCATEntry
const static auto& getVCATEntry = privilegeSetLogin;
const static auto& headVCATEntry = privilegeSetLogin;
const static auto& patchVCATEntry = privilegeSetConfigureComponents;
const static auto& putVCATEntry = privilegeSetConfigureComponents;
const static auto& deleteVCATEntry = privilegeSetConfigureComponents;
const static auto& postVCATEntry = privilegeSetConfigureComponents;

// VCATEntryCollection
const static auto& getVCATEntryCollection = privilegeSetLogin;
const static auto& headVCATEntryCollection = privilegeSetLogin;
const static auto& patchVCATEntryCollection = privilegeSetConfigureComponents;
const static auto& putVCATEntryCollection = privilegeSetConfigureComponents;
const static auto& deleteVCATEntryCollection = privilegeSetConfigureComponents;
const static auto& postVCATEntryCollection = privilegeSetConfigureComponents;

// VLanNetworkInterface
const static auto& getVLanNetworkInterface = privilegeSetLogin;
const static auto& headVLanNetworkInterface = privilegeSetLogin;
const static auto& patchVLanNetworkInterface = privilegeSetConfigureManager;
const static auto& putVLanNetworkInterface = privilegeSetConfigureManager;
const static auto& deleteVLanNetworkInterface = privilegeSetConfigureManager;
const static auto& postVLanNetworkInterface = privilegeSetConfigureManager;

// VLanNetworkInterfaceCollection
const static auto& getVLanNetworkInterfaceCollection = privilegeSetLogin;
const static auto& headVLanNetworkInterfaceCollection = privilegeSetLogin;
const static auto& patchVLanNetworkInterfaceCollection = privilegeSetConfigureManager;
const static auto& putVLanNetworkInterfaceCollection = privilegeSetConfigureManager;
const static auto& deleteVLanNetworkInterfaceCollection = privilegeSetConfigureManager;
const static auto& postVLanNetworkInterfaceCollection = privilegeSetConfigureManager;

// VirtualMedia
const static auto& getVirtualMedia = privilegeSetLogin;
const static auto& headVirtualMedia = privilegeSetLogin;
const static auto& patchVirtualMedia = privilegeSetConfigureManager;
const static auto& putVirtualMedia = privilegeSetConfigureManager;
const static auto& deleteVirtualMedia = privilegeSetConfigureManager;
const static auto& postVirtualMedia = privilegeSetConfigureManager;

// VirtualMediaCollection
const static auto& getVirtualMediaCollection = privilegeSetLogin;
const static auto& headVirtualMediaCollection = privilegeSetLogin;
const static auto& patchVirtualMediaCollection = privilegeSetConfigureManager;
const static auto& putVirtualMediaCollection = privilegeSetConfigureManager;
const static auto& deleteVirtualMediaCollection = privilegeSetConfigureManager;
const static auto& postVirtualMediaCollection = privilegeSetConfigureManager;

// Volume
const static auto& getVolume = privilegeSetLogin;
const static auto& headVolume = privilegeSetLogin;
const static auto& patchVolume = privilegeSetConfigureComponents;
const static auto& postVolume = privilegeSetConfigureComponents;
const static auto& putVolume = privilegeSetConfigureComponents;
const static auto& deleteVolume = privilegeSetConfigureComponents;

// VolumeCollection
const static auto& getVolumeCollection = privilegeSetLogin;
const static auto& headVolumeCollection = privilegeSetLogin;
const static auto& patchVolumeCollection = privilegeSetConfigureComponents;
const static auto& postVolumeCollection = privilegeSetConfigureComponents;
const static auto& putVolumeCollection = privilegeSetConfigureComponents;
const static auto& deleteVolumeCollection = privilegeSetConfigureComponents;

// Zone
const static auto& getZone = privilegeSetLogin;
const static auto& headZone = privilegeSetLogin;
const static auto& patchZone = privilegeSetConfigureComponents;
const static auto& postZone = privilegeSetConfigureComponents;
const static auto& putZone = privilegeSetConfigureComponents;
const static auto& deleteZone = privilegeSetConfigureComponents;

// ZoneCollection
const static auto& getZoneCollection = privilegeSetLogin;
const static auto& headZoneCollection = privilegeSetLogin;
const static auto& patchZoneCollection = privilegeSetConfigureComponents;
const static auto& postZoneCollection = privilegeSetConfigureComponents;
const static auto& putZoneCollection = privilegeSetConfigureComponents;
const static auto& deleteZoneCollection = privilegeSetConfigureComponents;

} // namespace redfish::privileges
// clang-format on
