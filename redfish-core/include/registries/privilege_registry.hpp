#pragma once
// privilege_registry.hpp is generated.  Do not edit directly
#include <privileges.hpp>

namespace redfish::privileges
{
// clang-format off
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
// clang-format on

// AccelerationFunction
const auto& getAccelerationFunction = privilegeSetLogin;
const auto& headAccelerationFunction = privilegeSetLogin;
const auto& patchAccelerationFunction = privilegeSetConfigureComponents;
const auto& putAccelerationFunction = privilegeSetConfigureComponents;
const auto& deleteAccelerationFunction = privilegeSetConfigureComponents;
const auto& postAccelerationFunction = privilegeSetConfigureComponents;

// AccelerationFunctionCollection
const auto& getAccelerationFunctionCollection = privilegeSetLogin;
const auto& headAccelerationFunctionCollection = privilegeSetLogin;
const auto& patchAccelerationFunctionCollection =
    privilegeSetConfigureComponents;
const auto& putAccelerationFunctionCollection = privilegeSetConfigureComponents;
const auto& deleteAccelerationFunctionCollection =
    privilegeSetConfigureComponents;
const auto& postAccelerationFunctionCollection =
    privilegeSetConfigureComponents;

// AccountService
const auto& getAccountService = privilegeSetLogin;
const auto& headAccountService = privilegeSetLogin;
const auto& patchAccountService = privilegeSetConfigureUsers;
const auto& putAccountService = privilegeSetConfigureUsers;
const auto& deleteAccountService = privilegeSetConfigureUsers;
const auto& postAccountService = privilegeSetConfigureUsers;

// ActionInfo
const auto& getActionInfo = privilegeSetLogin;
const auto& headActionInfo = privilegeSetLogin;
const auto& patchActionInfo = privilegeSetConfigureManager;
const auto& putActionInfo = privilegeSetConfigureManager;
const auto& deleteActionInfo = privilegeSetConfigureManager;
const auto& postActionInfo = privilegeSetConfigureManager;

// AddressPool
const auto& getAddressPool = privilegeSetLogin;
const auto& headAddressPool = privilegeSetLogin;
const auto& patchAddressPool = privilegeSetConfigureComponents;
const auto& putAddressPool = privilegeSetConfigureComponents;
const auto& deleteAddressPool = privilegeSetConfigureComponents;
const auto& postAddressPool = privilegeSetConfigureComponents;

// AddressPoolCollection
const auto& getAddressPoolCollection = privilegeSetLogin;
const auto& headAddressPoolCollection = privilegeSetLogin;
const auto& patchAddressPoolCollection = privilegeSetConfigureComponents;
const auto& putAddressPoolCollection = privilegeSetConfigureComponents;
const auto& deleteAddressPoolCollection = privilegeSetConfigureComponents;
const auto& postAddressPoolCollection = privilegeSetConfigureComponents;

// Aggregate
const auto& getAggregate = privilegeSetLogin;
const auto& headAggregate = privilegeSetLogin;
const auto& patchAggregate = privilegeSetConfigureManagerOrConfigureComponents;
const auto& putAggregate = privilegeSetConfigureManagerOrConfigureComponents;
const auto& deleteAggregate = privilegeSetConfigureManagerOrConfigureComponents;
const auto& postAggregate = privilegeSetConfigureManagerOrConfigureComponents;

// AggregateCollection
const auto& getAggregateCollection = privilegeSetLogin;
const auto& headAggregateCollection = privilegeSetLogin;
const auto& patchAggregateCollection =
    privilegeSetConfigureManagerOrConfigureComponents;
const auto& putAggregateCollection =
    privilegeSetConfigureManagerOrConfigureComponents;
const auto& deleteAggregateCollection =
    privilegeSetConfigureManagerOrConfigureComponents;
const auto& postAggregateCollection =
    privilegeSetConfigureManagerOrConfigureComponents;

// AggregationService
const auto& getAggregationService = privilegeSetLogin;
const auto& headAggregationService = privilegeSetLogin;
const auto& patchAggregationService = privilegeSetConfigureManager;
const auto& putAggregationService = privilegeSetConfigureManager;
const auto& deleteAggregationService = privilegeSetConfigureManager;
const auto& postAggregationService = privilegeSetConfigureManager;

// AggregationSource
const auto& getAggregationSource = privilegeSetLogin;
const auto& headAggregationSource = privilegeSetLogin;
const auto& patchAggregationSource = privilegeSetConfigureManager;
const auto& putAggregationSource = privilegeSetConfigureManager;
const auto& deleteAggregationSource = privilegeSetConfigureManager;
const auto& postAggregationSource = privilegeSetConfigureManager;

// AggregationSourceCollection
const auto& getAggregationSourceCollection = privilegeSetLogin;
const auto& headAggregationSourceCollection = privilegeSetLogin;
const auto& patchAggregationSourceCollection = privilegeSetConfigureManager;
const auto& putAggregationSourceCollection = privilegeSetConfigureManager;
const auto& deleteAggregationSourceCollection = privilegeSetConfigureManager;
const auto& postAggregationSourceCollection = privilegeSetConfigureManager;

// Assembly
const auto& getAssembly = privilegeSetLogin;
const auto& headAssembly = privilegeSetLogin;
const auto& patchAssembly = privilegeSetConfigureComponents;
const auto& putAssembly = privilegeSetConfigureComponents;
const auto& deleteAssembly = privilegeSetConfigureComponents;
const auto& postAssembly = privilegeSetConfigureComponents;

// Bios
const auto& getBios = privilegeSetLogin;
const auto& headBios = privilegeSetLogin;
const auto& patchBios = privilegeSetConfigureComponents;
const auto& postBios = privilegeSetConfigureComponents;
const auto& putBios = privilegeSetConfigureComponents;
const auto& deleteBios = privilegeSetConfigureComponents;

// BootOption
const auto& getBootOption = privilegeSetLogin;
const auto& headBootOption = privilegeSetLogin;
const auto& patchBootOption = privilegeSetConfigureComponents;
const auto& putBootOption = privilegeSetConfigureComponents;
const auto& deleteBootOption = privilegeSetConfigureComponents;
const auto& postBootOption = privilegeSetConfigureComponents;

// BootOptionCollection
const auto& getBootOptionCollection = privilegeSetLogin;
const auto& headBootOptionCollection = privilegeSetLogin;
const auto& patchBootOptionCollection = privilegeSetConfigureComponents;
const auto& putBootOptionCollection = privilegeSetConfigureComponents;
const auto& deleteBootOptionCollection = privilegeSetConfigureComponents;
const auto& postBootOptionCollection = privilegeSetConfigureComponents;

// Certificate
const auto& getCertificate = privilegeSetConfigureManager;
const auto& headCertificate = privilegeSetConfigureManager;
const auto& patchCertificate = privilegeSetConfigureManager;
const auto& putCertificate = privilegeSetConfigureManager;
const auto& deleteCertificate = privilegeSetConfigureManager;
const auto& postCertificate = privilegeSetConfigureManager;

// CertificateCollection
const auto& getCertificateCollection = privilegeSetConfigureManager;
const auto& headCertificateCollection = privilegeSetConfigureManager;
const auto& patchCertificateCollection = privilegeSetConfigureManager;
const auto& putCertificateCollection = privilegeSetConfigureManager;
const auto& deleteCertificateCollection = privilegeSetConfigureManager;
const auto& postCertificateCollection = privilegeSetConfigureManager;

// CertificateLocations
const auto& getCertificateLocations = privilegeSetConfigureManager;
const auto& headCertificateLocations = privilegeSetConfigureManager;
const auto& patchCertificateLocations = privilegeSetConfigureManager;
const auto& putCertificateLocations = privilegeSetConfigureManager;
const auto& deleteCertificateLocations = privilegeSetConfigureManager;
const auto& postCertificateLocations = privilegeSetConfigureManager;

// CertificateService
const auto& getCertificateService = privilegeSetLogin;
const auto& headCertificateService = privilegeSetLogin;
const auto& patchCertificateService = privilegeSetConfigureManager;
const auto& putCertificateService = privilegeSetConfigureManager;
const auto& deleteCertificateService = privilegeSetConfigureManager;
const auto& postCertificateService = privilegeSetConfigureManager;

// Chassis
const auto& getChassis = privilegeSetLogin;
const auto& headChassis = privilegeSetLogin;
const auto& patchChassis = privilegeSetConfigureComponents;
const auto& putChassis = privilegeSetConfigureComponents;
const auto& deleteChassis = privilegeSetConfigureComponents;
const auto& postChassis = privilegeSetConfigureComponents;

// ChassisCollection
const auto& getChassisCollection = privilegeSetLogin;
const auto& headChassisCollection = privilegeSetLogin;
const auto& patchChassisCollection = privilegeSetConfigureComponents;
const auto& putChassisCollection = privilegeSetConfigureComponents;
const auto& deleteChassisCollection = privilegeSetConfigureComponents;
const auto& postChassisCollection = privilegeSetConfigureComponents;

// Circuit
const auto& getCircuit = privilegeSetLogin;
const auto& headCircuit = privilegeSetLogin;
const auto& patchCircuit = privilegeSetConfigureComponents;
const auto& putCircuit = privilegeSetConfigureComponents;
const auto& deleteCircuit = privilegeSetConfigureComponents;
const auto& postCircuit = privilegeSetConfigureComponents;

// CircuitCollection
const auto& getCircuitCollection = privilegeSetLogin;
const auto& headCircuitCollection = privilegeSetLogin;
const auto& patchCircuitCollection = privilegeSetConfigureComponents;
const auto& putCircuitCollection = privilegeSetConfigureComponents;
const auto& deleteCircuitCollection = privilegeSetConfigureComponents;
const auto& postCircuitCollection = privilegeSetConfigureComponents;

// CompositionService
const auto& getCompositionService = privilegeSetLogin;
const auto& headCompositionService = privilegeSetLogin;
const auto& patchCompositionService = privilegeSetConfigureManager;
const auto& putCompositionService = privilegeSetConfigureManager;
const auto& deleteCompositionService = privilegeSetConfigureManager;
const auto& postCompositionService = privilegeSetConfigureManager;

// ComputerSystem
const auto& getComputerSystem = privilegeSetLogin;
const auto& headComputerSystem = privilegeSetLogin;
const auto& patchComputerSystem = privilegeSetConfigureComponents;
const auto& postComputerSystem = privilegeSetConfigureComponents;
const auto& putComputerSystem = privilegeSetConfigureComponents;
const auto& deleteComputerSystem = privilegeSetConfigureComponents;

// ComputerSystemCollection
const auto& getComputerSystemCollection = privilegeSetLogin;
const auto& headComputerSystemCollection = privilegeSetLogin;
const auto& patchComputerSystemCollection = privilegeSetConfigureComponents;
const auto& postComputerSystemCollection = privilegeSetConfigureComponents;
const auto& putComputerSystemCollection = privilegeSetConfigureComponents;
const auto& deleteComputerSystemCollection = privilegeSetConfigureComponents;

// Connection
const auto& getConnection = privilegeSetLogin;
const auto& headConnection = privilegeSetLogin;
const auto& patchConnection = privilegeSetConfigureComponents;
const auto& postConnection = privilegeSetConfigureComponents;
const auto& putConnection = privilegeSetConfigureComponents;
const auto& deleteConnection = privilegeSetConfigureComponents;

// ConnectionCollection
const auto& getConnectionCollection = privilegeSetLogin;
const auto& headConnectionCollection = privilegeSetLogin;
const auto& patchConnectionCollection = privilegeSetConfigureComponents;
const auto& postConnectionCollection = privilegeSetConfigureComponents;
const auto& putConnectionCollection = privilegeSetConfigureComponents;
const auto& deleteConnectionCollection = privilegeSetConfigureComponents;

// ConnectionMethod
const auto& getConnectionMethod = privilegeSetLogin;
const auto& headConnectionMethod = privilegeSetLogin;
const auto& patchConnectionMethod = privilegeSetConfigureManager;
const auto& putConnectionMethod = privilegeSetConfigureManager;
const auto& deleteConnectionMethod = privilegeSetConfigureManager;
const auto& postConnectionMethod = privilegeSetConfigureManager;

// ConnectionMethodCollection
const auto& getConnectionMethodCollection = privilegeSetLogin;
const auto& headConnectionMethodCollection = privilegeSetLogin;
const auto& patchConnectionMethodCollection = privilegeSetConfigureManager;
const auto& putConnectionMethodCollection = privilegeSetConfigureManager;
const auto& deleteConnectionMethodCollection = privilegeSetConfigureManager;
const auto& postConnectionMethodCollection = privilegeSetConfigureManager;

// Drive
const auto& getDrive = privilegeSetLogin;
const auto& headDrive = privilegeSetLogin;
const auto& patchDrive = privilegeSetConfigureComponents;
const auto& postDrive = privilegeSetConfigureComponents;
const auto& putDrive = privilegeSetConfigureComponents;
const auto& deleteDrive = privilegeSetConfigureComponents;

// DriveCollection
const auto& getDriveCollection = privilegeSetLogin;
const auto& headDriveCollection = privilegeSetLogin;
const auto& patchDriveCollection = privilegeSetConfigureComponents;
const auto& postDriveCollection = privilegeSetConfigureComponents;
const auto& putDriveCollection = privilegeSetConfigureComponents;
const auto& deleteDriveCollection = privilegeSetConfigureComponents;

// Endpoint
const auto& getEndpoint = privilegeSetLogin;
const auto& headEndpoint = privilegeSetLogin;
const auto& patchEndpoint = privilegeSetConfigureComponents;
const auto& postEndpoint = privilegeSetConfigureComponents;
const auto& putEndpoint = privilegeSetConfigureComponents;
const auto& deleteEndpoint = privilegeSetConfigureComponents;

// EndpointCollection
const auto& getEndpointCollection = privilegeSetLogin;
const auto& headEndpointCollection = privilegeSetLogin;
const auto& patchEndpointCollection = privilegeSetConfigureComponents;
const auto& postEndpointCollection = privilegeSetConfigureComponents;
const auto& putEndpointCollection = privilegeSetConfigureComponents;
const auto& deleteEndpointCollection = privilegeSetConfigureComponents;

// EndpointGroup
const auto& getEndpointGroup = privilegeSetLogin;
const auto& headEndpointGroup = privilegeSetLogin;
const auto& patchEndpointGroup = privilegeSetConfigureComponents;
const auto& postEndpointGroup = privilegeSetConfigureComponents;
const auto& putEndpointGroup = privilegeSetConfigureComponents;
const auto& deleteEndpointGroup = privilegeSetConfigureComponents;

// EndpointGroupCollection
const auto& getEndpointGroupCollection = privilegeSetLogin;
const auto& headEndpointGroupCollection = privilegeSetLogin;
const auto& patchEndpointGroupCollection = privilegeSetConfigureComponents;
const auto& postEndpointGroupCollection = privilegeSetConfigureComponents;
const auto& putEndpointGroupCollection = privilegeSetConfigureComponents;
const auto& deleteEndpointGroupCollection = privilegeSetConfigureComponents;

// EthernetInterface
const auto& getEthernetInterface = privilegeSetLogin;
const auto& headEthernetInterface = privilegeSetLogin;
const auto& patchEthernetInterface = privilegeSetConfigureComponents;
const auto& postEthernetInterface = privilegeSetConfigureComponents;
const auto& putEthernetInterface = privilegeSetConfigureComponents;
const auto& deleteEthernetInterface = privilegeSetConfigureComponents;

// EthernetInterfaceCollection
const auto& getEthernetInterfaceCollection = privilegeSetLogin;
const auto& headEthernetInterfaceCollection = privilegeSetLogin;
const auto& patchEthernetInterfaceCollection = privilegeSetConfigureComponents;
const auto& postEthernetInterfaceCollection = privilegeSetConfigureComponents;
const auto& putEthernetInterfaceCollection = privilegeSetConfigureComponents;
const auto& deleteEthernetInterfaceCollection = privilegeSetConfigureComponents;

// EventDestination
const auto& getEventDestination = privilegeSetLogin;
const auto& headEventDestination = privilegeSetLogin;
const auto& patchEventDestination = privilegeSetConfigureManagerOrConfigureSelf;
const auto& postEventDestination = privilegeSetConfigureManagerOrConfigureSelf;
const auto& putEventDestination = privilegeSetConfigureManagerOrConfigureSelf;
const auto& deleteEventDestination =
    privilegeSetConfigureManagerOrConfigureSelf;

// EventDestinationCollection
const auto& getEventDestinationCollection = privilegeSetLogin;
const auto& headEventDestinationCollection = privilegeSetLogin;
const auto& patchEventDestinationCollection =
    privilegeSetConfigureManagerOrConfigureComponents;
const auto& postEventDestinationCollection =
    privilegeSetConfigureManagerOrConfigureComponents;
const auto& putEventDestinationCollection =
    privilegeSetConfigureManagerOrConfigureComponents;
const auto& deleteEventDestinationCollection =
    privilegeSetConfigureManagerOrConfigureComponents;

// EventService
const auto& getEventService = privilegeSetLogin;
const auto& headEventService = privilegeSetLogin;
const auto& patchEventService = privilegeSetConfigureManager;
const auto& postEventService = privilegeSetConfigureManager;
const auto& putEventService = privilegeSetConfigureManager;
const auto& deleteEventService = privilegeSetConfigureManager;

// ExternalAccountProvider
const auto& getExternalAccountProvider = privilegeSetLogin;
const auto& headExternalAccountProvider = privilegeSetLogin;
const auto& patchExternalAccountProvider = privilegeSetConfigureManager;
const auto& putExternalAccountProvider = privilegeSetConfigureManager;
const auto& deleteExternalAccountProvider = privilegeSetConfigureManager;
const auto& postExternalAccountProvider = privilegeSetConfigureManager;

// ExternalAccountProviderCollection
const auto& getExternalAccountProviderCollection = privilegeSetLogin;
const auto& headExternalAccountProviderCollection = privilegeSetLogin;
const auto& patchExternalAccountProviderCollection =
    privilegeSetConfigureManager;
const auto& putExternalAccountProviderCollection = privilegeSetConfigureManager;
const auto& deleteExternalAccountProviderCollection =
    privilegeSetConfigureManager;
const auto& postExternalAccountProviderCollection =
    privilegeSetConfigureManager;

// Fabric
const auto& getFabric = privilegeSetLogin;
const auto& headFabric = privilegeSetLogin;
const auto& patchFabric = privilegeSetConfigureComponents;
const auto& postFabric = privilegeSetConfigureComponents;
const auto& putFabric = privilegeSetConfigureComponents;
const auto& deleteFabric = privilegeSetConfigureComponents;

// FabricCollection
const auto& getFabricCollection = privilegeSetLogin;
const auto& headFabricCollection = privilegeSetLogin;
const auto& patchFabricCollection = privilegeSetConfigureComponents;
const auto& postFabricCollection = privilegeSetConfigureComponents;
const auto& putFabricCollection = privilegeSetConfigureComponents;
const auto& deleteFabricCollection = privilegeSetConfigureComponents;

// FabricAdapter
const auto& getFabricAdapter = privilegeSetLogin;
const auto& headFabricAdapter = privilegeSetLogin;
const auto& patchFabricAdapter = privilegeSetConfigureComponents;
const auto& postFabricAdapter = privilegeSetConfigureComponents;
const auto& putFabricAdapter = privilegeSetConfigureComponents;
const auto& deleteFabricAdapter = privilegeSetConfigureComponents;

// FabricAdapterCollection
const auto& getFabricAdapterCollection = privilegeSetLogin;
const auto& headFabricAdapterCollection = privilegeSetLogin;
const auto& patchFabricAdapterCollection = privilegeSetConfigureComponents;
const auto& postFabricAdapterCollection = privilegeSetConfigureComponents;
const auto& putFabricAdapterCollection = privilegeSetConfigureComponents;
const auto& deleteFabricAdapterCollection = privilegeSetConfigureComponents;

// Facility
const auto& getFacility = privilegeSetLogin;
const auto& headFacility = privilegeSetLogin;
const auto& patchFacility = privilegeSetConfigureComponents;
const auto& putFacility = privilegeSetConfigureComponents;
const auto& deleteFacility = privilegeSetConfigureComponents;
const auto& postFacility = privilegeSetConfigureComponents;

// FacilityCollection
const auto& getFacilityCollection = privilegeSetLogin;
const auto& headFacilityCollection = privilegeSetLogin;
const auto& patchFacilityCollection = privilegeSetConfigureComponents;
const auto& putFacilityCollection = privilegeSetConfigureComponents;
const auto& deleteFacilityCollection = privilegeSetConfigureComponents;
const auto& postFacilityCollection = privilegeSetConfigureComponents;

// HostInterface
const auto& getHostInterface = privilegeSetLogin;
const auto& headHostInterface = privilegeSetLogin;
const auto& patchHostInterface = privilegeSetConfigureManager;
const auto& postHostInterface = privilegeSetConfigureManager;
const auto& putHostInterface = privilegeSetConfigureManager;
const auto& deleteHostInterface = privilegeSetConfigureManager;

// HostInterfaceCollection
const auto& getHostInterfaceCollection = privilegeSetLogin;
const auto& headHostInterfaceCollection = privilegeSetLogin;
const auto& patchHostInterfaceCollection = privilegeSetConfigureManager;
const auto& postHostInterfaceCollection = privilegeSetConfigureManager;
const auto& putHostInterfaceCollection = privilegeSetConfigureManager;
const auto& deleteHostInterfaceCollection = privilegeSetConfigureManager;

// Job
const auto& getJob = privilegeSetLogin;
const auto& headJob = privilegeSetLogin;
const auto& patchJob = privilegeSetConfigureManager;
const auto& putJob = privilegeSetConfigureManager;
const auto& deleteJob = privilegeSetConfigureManager;
const auto& postJob = privilegeSetConfigureManager;

// JobCollection
const auto& getJobCollection = privilegeSetLogin;
const auto& headJobCollection = privilegeSetLogin;
const auto& patchJobCollection = privilegeSetConfigureManager;
const auto& putJobCollection = privilegeSetConfigureManager;
const auto& deleteJobCollection = privilegeSetConfigureManager;
const auto& postJobCollection = privilegeSetConfigureManager;

// JobService
const auto& getJobService = privilegeSetLogin;
const auto& headJobService = privilegeSetLogin;
const auto& patchJobService = privilegeSetConfigureManager;
const auto& putJobService = privilegeSetConfigureManager;
const auto& deleteJobService = privilegeSetConfigureManager;
const auto& postJobService = privilegeSetConfigureManager;

// JsonSchemaFile
const auto& getJsonSchemaFile = privilegeSetLogin;
const auto& headJsonSchemaFile = privilegeSetLogin;
const auto& patchJsonSchemaFile = privilegeSetConfigureManager;
const auto& postJsonSchemaFile = privilegeSetConfigureManager;
const auto& putJsonSchemaFile = privilegeSetConfigureManager;
const auto& deleteJsonSchemaFile = privilegeSetConfigureManager;

// JsonSchemaFileCollection
const auto& getJsonSchemaFileCollection = privilegeSetLogin;
const auto& headJsonSchemaFileCollection = privilegeSetLogin;
const auto& patchJsonSchemaFileCollection = privilegeSetConfigureManager;
const auto& postJsonSchemaFileCollection = privilegeSetConfigureManager;
const auto& putJsonSchemaFileCollection = privilegeSetConfigureManager;
const auto& deleteJsonSchemaFileCollection = privilegeSetConfigureManager;

// LogEntry
const auto& getLogEntry = privilegeSetLogin;
const auto& headLogEntry = privilegeSetLogin;
const auto& patchLogEntry = privilegeSetConfigureManager;
const auto& putLogEntry = privilegeSetConfigureManager;
const auto& deleteLogEntry = privilegeSetConfigureManager;
const auto& postLogEntry = privilegeSetConfigureManager;

// LogEntryCollection
const auto& getLogEntryCollection = privilegeSetLogin;
const auto& headLogEntryCollection = privilegeSetLogin;
const auto& patchLogEntryCollection = privilegeSetConfigureManager;
const auto& putLogEntryCollection = privilegeSetConfigureManager;
const auto& deleteLogEntryCollection = privilegeSetConfigureManager;
const auto& postLogEntryCollection = privilegeSetConfigureManager;

// LogService
const auto& getLogService = privilegeSetLogin;
const auto& headLogService = privilegeSetLogin;
const auto& patchLogService = privilegeSetConfigureManager;
const auto& putLogService = privilegeSetConfigureManager;
const auto& deleteLogService = privilegeSetConfigureManager;
const auto& postLogService = privilegeSetConfigureManager;

// LogServiceCollection
const auto& getLogServiceCollection = privilegeSetLogin;
const auto& headLogServiceCollection = privilegeSetLogin;
const auto& patchLogServiceCollection = privilegeSetConfigureManager;
const auto& putLogServiceCollection = privilegeSetConfigureManager;
const auto& deleteLogServiceCollection = privilegeSetConfigureManager;
const auto& postLogServiceCollection = privilegeSetConfigureManager;

// Manager
const auto& getManager = privilegeSetLogin;
const auto& headManager = privilegeSetLogin;
const auto& patchManager = privilegeSetConfigureManager;
const auto& postManager = privilegeSetConfigureManager;
const auto& putManager = privilegeSetConfigureManager;
const auto& deleteManager = privilegeSetConfigureManager;

// ManagerCollection
const auto& getManagerCollection = privilegeSetLogin;
const auto& headManagerCollection = privilegeSetLogin;
const auto& patchManagerCollection = privilegeSetConfigureManager;
const auto& postManagerCollection = privilegeSetConfigureManager;
const auto& putManagerCollection = privilegeSetConfigureManager;
const auto& deleteManagerCollection = privilegeSetConfigureManager;

// ManagerAccount
const auto& getManagerAccount =
    privilegeSetConfigureManagerOrConfigureUsersOrConfigureSelf;
const auto& headManagerAccount = privilegeSetLogin;
const auto& patchManagerAccount = privilegeSetConfigureUsers;
const auto& postManagerAccount = privilegeSetConfigureUsers;
const auto& putManagerAccount = privilegeSetConfigureUsers;
const auto& deleteManagerAccount = privilegeSetConfigureUsers;

// ManagerAccountCollection
const auto& getManagerAccountCollection = privilegeSetLogin;
const auto& headManagerAccountCollection = privilegeSetLogin;
const auto& patchManagerAccountCollection = privilegeSetConfigureUsers;
const auto& putManagerAccountCollection = privilegeSetConfigureUsers;
const auto& deleteManagerAccountCollection = privilegeSetConfigureUsers;
const auto& postManagerAccountCollection = privilegeSetConfigureUsers;

// ManagerNetworkProtocol
const auto& getManagerNetworkProtocol = privilegeSetLogin;
const auto& headManagerNetworkProtocol = privilegeSetLogin;
const auto& patchManagerNetworkProtocol = privilegeSetConfigureManager;
const auto& postManagerNetworkProtocol = privilegeSetConfigureManager;
const auto& putManagerNetworkProtocol = privilegeSetConfigureManager;
const auto& deleteManagerNetworkProtocol = privilegeSetConfigureManager;

// MediaController
const auto& getMediaController = privilegeSetLogin;
const auto& headMediaController = privilegeSetLogin;
const auto& patchMediaController = privilegeSetConfigureComponents;
const auto& postMediaController = privilegeSetConfigureComponents;
const auto& putMediaController = privilegeSetConfigureComponents;
const auto& deleteMediaController = privilegeSetConfigureComponents;

// MediaControllerCollection
const auto& getMediaControllerCollection = privilegeSetLogin;
const auto& headMediaControllerCollection = privilegeSetLogin;
const auto& patchMediaControllerCollection = privilegeSetConfigureComponents;
const auto& postMediaControllerCollection = privilegeSetConfigureComponents;
const auto& putMediaControllerCollection = privilegeSetConfigureComponents;
const auto& deleteMediaControllerCollection = privilegeSetConfigureComponents;

// Memory
const auto& getMemory = privilegeSetLogin;
const auto& headMemory = privilegeSetLogin;
const auto& patchMemory = privilegeSetConfigureComponents;
const auto& postMemory = privilegeSetConfigureComponents;
const auto& putMemory = privilegeSetConfigureComponents;
const auto& deleteMemory = privilegeSetConfigureComponents;

// MemoryCollection
const auto& getMemoryCollection = privilegeSetLogin;
const auto& headMemoryCollection = privilegeSetLogin;
const auto& patchMemoryCollection = privilegeSetConfigureComponents;
const auto& postMemoryCollection = privilegeSetConfigureComponents;
const auto& putMemoryCollection = privilegeSetConfigureComponents;
const auto& deleteMemoryCollection = privilegeSetConfigureComponents;

// MemoryChunks
const auto& getMemoryChunks = privilegeSetLogin;
const auto& headMemoryChunks = privilegeSetLogin;
const auto& patchMemoryChunks = privilegeSetConfigureComponents;
const auto& postMemoryChunks = privilegeSetConfigureComponents;
const auto& putMemoryChunks = privilegeSetConfigureComponents;
const auto& deleteMemoryChunks = privilegeSetConfigureComponents;

// MemoryChunksCollection
const auto& getMemoryChunksCollection = privilegeSetLogin;
const auto& headMemoryChunksCollection = privilegeSetLogin;
const auto& patchMemoryChunksCollection = privilegeSetConfigureComponents;
const auto& postMemoryChunksCollection = privilegeSetConfigureComponents;
const auto& putMemoryChunksCollection = privilegeSetConfigureComponents;
const auto& deleteMemoryChunksCollection = privilegeSetConfigureComponents;

// MemoryDomain
const auto& getMemoryDomain = privilegeSetLogin;
const auto& headMemoryDomain = privilegeSetLogin;
const auto& patchMemoryDomain = privilegeSetConfigureComponents;
const auto& postMemoryDomain = privilegeSetConfigureComponents;
const auto& putMemoryDomain = privilegeSetConfigureComponents;
const auto& deleteMemoryDomain = privilegeSetConfigureComponents;

// MemoryDomainCollection
const auto& getMemoryDomainCollection = privilegeSetLogin;
const auto& headMemoryDomainCollection = privilegeSetLogin;
const auto& patchMemoryDomainCollection = privilegeSetConfigureComponents;
const auto& postMemoryDomainCollection = privilegeSetConfigureComponents;
const auto& putMemoryDomainCollection = privilegeSetConfigureComponents;
const auto& deleteMemoryDomainCollection = privilegeSetConfigureComponents;

// MemoryMetrics
const auto& getMemoryMetrics = privilegeSetLogin;
const auto& headMemoryMetrics = privilegeSetLogin;
const auto& patchMemoryMetrics = privilegeSetConfigureComponents;
const auto& postMemoryMetrics = privilegeSetConfigureComponents;
const auto& putMemoryMetrics = privilegeSetConfigureComponents;
const auto& deleteMemoryMetrics = privilegeSetConfigureComponents;

// MessageRegistryFile
const auto& getMessageRegistryFile = privilegeSetLogin;
const auto& headMessageRegistryFile = privilegeSetLogin;
const auto& patchMessageRegistryFile = privilegeSetConfigureManager;
const auto& postMessageRegistryFile = privilegeSetConfigureManager;
const auto& putMessageRegistryFile = privilegeSetConfigureManager;
const auto& deleteMessageRegistryFile = privilegeSetConfigureManager;

// MessageRegistryFileCollection
const auto& getMessageRegistryFileCollection = privilegeSetLogin;
const auto& headMessageRegistryFileCollection = privilegeSetLogin;
const auto& patchMessageRegistryFileCollection = privilegeSetConfigureManager;
const auto& postMessageRegistryFileCollection = privilegeSetConfigureManager;
const auto& putMessageRegistryFileCollection = privilegeSetConfigureManager;
const auto& deleteMessageRegistryFileCollection = privilegeSetConfigureManager;

// MetricDefinition
const auto& getMetricDefinition = privilegeSetLogin;
const auto& headMetricDefinition = privilegeSetLogin;
const auto& patchMetricDefinition = privilegeSetConfigureManager;
const auto& putMetricDefinition = privilegeSetConfigureManager;
const auto& deleteMetricDefinition = privilegeSetConfigureManager;
const auto& postMetricDefinition = privilegeSetConfigureManager;

// MetricDefinitionCollection
const auto& getMetricDefinitionCollection = privilegeSetLogin;
const auto& headMetricDefinitionCollection = privilegeSetLogin;
const auto& patchMetricDefinitionCollection = privilegeSetConfigureManager;
const auto& putMetricDefinitionCollection = privilegeSetConfigureManager;
const auto& deleteMetricDefinitionCollection = privilegeSetConfigureManager;
const auto& postMetricDefinitionCollection = privilegeSetConfigureManager;

// MetricReport
const auto& getMetricReport = privilegeSetLogin;
const auto& headMetricReport = privilegeSetLogin;
const auto& patchMetricReport = privilegeSetConfigureManager;
const auto& putMetricReport = privilegeSetConfigureManager;
const auto& deleteMetricReport = privilegeSetConfigureManager;
const auto& postMetricReport = privilegeSetConfigureManager;

// MetricReportCollection
const auto& getMetricReportCollection = privilegeSetLogin;
const auto& headMetricReportCollection = privilegeSetLogin;
const auto& patchMetricReportCollection = privilegeSetConfigureManager;
const auto& putMetricReportCollection = privilegeSetConfigureManager;
const auto& deleteMetricReportCollection = privilegeSetConfigureManager;
const auto& postMetricReportCollection = privilegeSetConfigureManager;

// MetricReportDefinition
const auto& getMetricReportDefinition = privilegeSetLogin;
const auto& headMetricReportDefinition = privilegeSetLogin;
const auto& patchMetricReportDefinition = privilegeSetConfigureManager;
const auto& putMetricReportDefinition = privilegeSetConfigureManager;
const auto& deleteMetricReportDefinition = privilegeSetConfigureManager;
const auto& postMetricReportDefinition = privilegeSetConfigureManager;

// MetricReportDefinitionCollection
const auto& getMetricReportDefinitionCollection = privilegeSetLogin;
const auto& headMetricReportDefinitionCollection = privilegeSetLogin;
const auto& patchMetricReportDefinitionCollection =
    privilegeSetConfigureManager;
const auto& putMetricReportDefinitionCollection = privilegeSetConfigureManager;
const auto& deleteMetricReportDefinitionCollection =
    privilegeSetConfigureManager;
const auto& postMetricReportDefinitionCollection = privilegeSetConfigureManager;

// NetworkAdapter
const auto& getNetworkAdapter = privilegeSetLogin;
const auto& headNetworkAdapter = privilegeSetLogin;
const auto& patchNetworkAdapter = privilegeSetConfigureComponents;
const auto& postNetworkAdapter = privilegeSetConfigureComponents;
const auto& putNetworkAdapter = privilegeSetConfigureComponents;
const auto& deleteNetworkAdapter = privilegeSetConfigureComponents;

// NetworkAdapterCollection
const auto& getNetworkAdapterCollection = privilegeSetLogin;
const auto& headNetworkAdapterCollection = privilegeSetLogin;
const auto& patchNetworkAdapterCollection = privilegeSetConfigureComponents;
const auto& postNetworkAdapterCollection = privilegeSetConfigureComponents;
const auto& putNetworkAdapterCollection = privilegeSetConfigureComponents;
const auto& deleteNetworkAdapterCollection = privilegeSetConfigureComponents;

// NetworkDeviceFunction
const auto& getNetworkDeviceFunction = privilegeSetLogin;
const auto& headNetworkDeviceFunction = privilegeSetLogin;
const auto& patchNetworkDeviceFunction = privilegeSetConfigureComponents;
const auto& postNetworkDeviceFunction = privilegeSetConfigureComponents;
const auto& putNetworkDeviceFunction = privilegeSetConfigureComponents;
const auto& deleteNetworkDeviceFunction = privilegeSetConfigureComponents;

// NetworkDeviceFunctionCollection
const auto& getNetworkDeviceFunctionCollection = privilegeSetLogin;
const auto& headNetworkDeviceFunctionCollection = privilegeSetLogin;
const auto& patchNetworkDeviceFunctionCollection =
    privilegeSetConfigureComponents;
const auto& postNetworkDeviceFunctionCollection =
    privilegeSetConfigureComponents;
const auto& putNetworkDeviceFunctionCollection =
    privilegeSetConfigureComponents;
const auto& deleteNetworkDeviceFunctionCollection =
    privilegeSetConfigureComponents;

// NetworkInterface
const auto& getNetworkInterface = privilegeSetLogin;
const auto& headNetworkInterface = privilegeSetLogin;
const auto& patchNetworkInterface = privilegeSetConfigureComponents;
const auto& postNetworkInterface = privilegeSetConfigureComponents;
const auto& putNetworkInterface = privilegeSetConfigureComponents;
const auto& deleteNetworkInterface = privilegeSetConfigureComponents;

// NetworkInterfaceCollection
const auto& getNetworkInterfaceCollection = privilegeSetLogin;
const auto& headNetworkInterfaceCollection = privilegeSetLogin;
const auto& patchNetworkInterfaceCollection = privilegeSetConfigureComponents;
const auto& postNetworkInterfaceCollection = privilegeSetConfigureComponents;
const auto& putNetworkInterfaceCollection = privilegeSetConfigureComponents;
const auto& deleteNetworkInterfaceCollection = privilegeSetConfigureComponents;

// NetworkPort
const auto& getNetworkPort = privilegeSetLogin;
const auto& headNetworkPort = privilegeSetLogin;
const auto& patchNetworkPort = privilegeSetConfigureComponents;
const auto& postNetworkPort = privilegeSetConfigureComponents;
const auto& putNetworkPort = privilegeSetConfigureComponents;
const auto& deleteNetworkPort = privilegeSetConfigureComponents;

// NetworkPortCollection
const auto& getNetworkPortCollection = privilegeSetLogin;
const auto& headNetworkPortCollection = privilegeSetLogin;
const auto& patchNetworkPortCollection = privilegeSetConfigureComponents;
const auto& postNetworkPortCollection = privilegeSetConfigureComponents;
const auto& putNetworkPortCollection = privilegeSetConfigureComponents;
const auto& deleteNetworkPortCollection = privilegeSetConfigureComponents;

// OperatingConfig
const auto& getOperatingConfig = privilegeSetLogin;
const auto& headOperatingConfig = privilegeSetLogin;
const auto& patchOperatingConfig = privilegeSetConfigureComponents;
const auto& postOperatingConfig = privilegeSetConfigureComponents;
const auto& putOperatingConfig = privilegeSetConfigureComponents;
const auto& deleteOperatingConfig = privilegeSetConfigureComponents;

// OperatingConfigCollection
const auto& getOperatingConfigCollection = privilegeSetLogin;
const auto& headOperatingConfigCollection = privilegeSetLogin;
const auto& patchOperatingConfigCollection = privilegeSetConfigureComponents;
const auto& postOperatingConfigCollection = privilegeSetConfigureComponents;
const auto& putOperatingConfigCollection = privilegeSetConfigureComponents;
const auto& deleteOperatingConfigCollection = privilegeSetConfigureComponents;

// Outlet
const auto& getOutlet = privilegeSetLogin;
const auto& headOutlet = privilegeSetLogin;
const auto& patchOutlet = privilegeSetConfigureComponents;
const auto& postOutlet = privilegeSetConfigureComponents;
const auto& putOutlet = privilegeSetConfigureComponents;
const auto& deleteOutlet = privilegeSetConfigureComponents;

// OutletCollection
const auto& getOutletCollection = privilegeSetLogin;
const auto& headOutletCollection = privilegeSetLogin;
const auto& patchOutletCollection = privilegeSetConfigureComponents;
const auto& postOutletCollection = privilegeSetConfigureComponents;
const auto& putOutletCollection = privilegeSetConfigureComponents;
const auto& deleteOutletCollection = privilegeSetConfigureComponents;

// OutletGroup
const auto& getOutletGroup = privilegeSetLogin;
const auto& headOutletGroup = privilegeSetLogin;
const auto& patchOutletGroup = privilegeSetConfigureComponents;
const auto& postOutletGroup = privilegeSetConfigureComponents;
const auto& putOutletGroup = privilegeSetConfigureComponents;
const auto& deleteOutletGroup = privilegeSetConfigureComponents;

// OutletGroupCollection
const auto& getOutletGroupCollection = privilegeSetLogin;
const auto& headOutletGroupCollection = privilegeSetLogin;
const auto& patchOutletGroupCollection = privilegeSetConfigureComponents;
const auto& postOutletGroupCollection = privilegeSetConfigureComponents;
const auto& putOutletGroupCollection = privilegeSetConfigureComponents;
const auto& deleteOutletGroupCollection = privilegeSetConfigureComponents;

// PCIeDevice
const auto& getPCIeDevice = privilegeSetLogin;
const auto& headPCIeDevice = privilegeSetLogin;
const auto& patchPCIeDevice = privilegeSetConfigureComponents;
const auto& postPCIeDevice = privilegeSetConfigureComponents;
const auto& putPCIeDevice = privilegeSetConfigureComponents;
const auto& deletePCIeDevice = privilegeSetConfigureComponents;

// PCIeDeviceCollection
const auto& getPCIeDeviceCollection = privilegeSetLogin;
const auto& headPCIeDeviceCollection = privilegeSetLogin;
const auto& patchPCIeDeviceCollection = privilegeSetConfigureComponents;
const auto& postPCIeDeviceCollection = privilegeSetConfigureComponents;
const auto& putPCIeDeviceCollection = privilegeSetConfigureComponents;
const auto& deletePCIeDeviceCollection = privilegeSetConfigureComponents;

// PCIeFunction
const auto& getPCIeFunction = privilegeSetLogin;
const auto& headPCIeFunction = privilegeSetLogin;
const auto& patchPCIeFunction = privilegeSetConfigureComponents;
const auto& postPCIeFunction = privilegeSetConfigureComponents;
const auto& putPCIeFunction = privilegeSetConfigureComponents;
const auto& deletePCIeFunction = privilegeSetConfigureComponents;

// PCIeFunctionCollection
const auto& getPCIeFunctionCollection = privilegeSetLogin;
const auto& headPCIeFunctionCollection = privilegeSetLogin;
const auto& patchPCIeFunctionCollection = privilegeSetConfigureComponents;
const auto& postPCIeFunctionCollection = privilegeSetConfigureComponents;
const auto& putPCIeFunctionCollection = privilegeSetConfigureComponents;
const auto& deletePCIeFunctionCollection = privilegeSetConfigureComponents;

// PCIeSlots
const auto& getPCIeSlots = privilegeSetLogin;
const auto& headPCIeSlots = privilegeSetLogin;
const auto& patchPCIeSlots = privilegeSetConfigureComponents;
const auto& postPCIeSlots = privilegeSetConfigureComponents;
const auto& putPCIeSlots = privilegeSetConfigureComponents;
const auto& deletePCIeSlots = privilegeSetConfigureComponents;

// Port
const auto& getPort = privilegeSetLogin;
const auto& headPort = privilegeSetLogin;
const auto& patchPort = privilegeSetConfigureComponents;
const auto& postPort = privilegeSetConfigureComponents;
const auto& putPort = privilegeSetConfigureComponents;
const auto& deletePort = privilegeSetConfigureComponents;

// PortCollection
const auto& getPortCollection = privilegeSetLogin;
const auto& headPortCollection = privilegeSetLogin;
const auto& patchPortCollection = privilegeSetConfigureComponents;
const auto& postPortCollection = privilegeSetConfigureComponents;
const auto& putPortCollection = privilegeSetConfigureComponents;
const auto& deletePortCollection = privilegeSetConfigureComponents;

// PortMetrics
const auto& getPortMetrics = privilegeSetLogin;
const auto& headPortMetrics = privilegeSetLogin;
const auto& patchPortMetrics = privilegeSetConfigureComponents;
const auto& postPortMetrics = privilegeSetConfigureComponents;
const auto& putPortMetrics = privilegeSetConfigureComponents;
const auto& deletePortMetrics = privilegeSetConfigureComponents;

// Power
const auto& getPower = privilegeSetLogin;
const auto& headPower = privilegeSetLogin;
const auto& patchPower = privilegeSetConfigureManager;
const auto& putPower = privilegeSetConfigureManager;
const auto& deletePower = privilegeSetConfigureManager;
const auto& postPower = privilegeSetConfigureManager;

// PowerDistribution
const auto& getPowerDistribution = privilegeSetLogin;
const auto& headPowerDistribution = privilegeSetLogin;
const auto& patchPowerDistribution = privilegeSetConfigureComponents;
const auto& postPowerDistribution = privilegeSetConfigureComponents;
const auto& putPowerDistribution = privilegeSetConfigureComponents;
const auto& deletePowerDistribution = privilegeSetConfigureComponents;

// PowerDistributionCollection
const auto& getPowerDistributionCollection = privilegeSetLogin;
const auto& headPowerDistributionCollection = privilegeSetLogin;
const auto& patchPowerDistributionCollection = privilegeSetConfigureComponents;
const auto& postPowerDistributionCollection = privilegeSetConfigureComponents;
const auto& putPowerDistributionCollection = privilegeSetConfigureComponents;
const auto& deletePowerDistributionCollection = privilegeSetConfigureComponents;

// PowerDistributionMetrics
const auto& getPowerDistributionMetrics = privilegeSetLogin;
const auto& headPowerDistributionMetrics = privilegeSetLogin;
const auto& patchPowerDistributionMetrics = privilegeSetConfigureComponents;
const auto& postPowerDistributionMetrics = privilegeSetConfigureComponents;
const auto& putPowerDistributionMetrics = privilegeSetConfigureComponents;
const auto& deletePowerDistributionMetrics = privilegeSetConfigureComponents;

// Processor
const auto& getProcessor = privilegeSetLogin;
const auto& headProcessor = privilegeSetLogin;
const auto& patchProcessor = privilegeSetConfigureComponents;
const auto& putProcessor = privilegeSetConfigureComponents;
const auto& deleteProcessor = privilegeSetConfigureComponents;
const auto& postProcessor = privilegeSetConfigureComponents;

// ProcessorCollection
const auto& getProcessorCollection = privilegeSetLogin;
const auto& headProcessorCollection = privilegeSetLogin;
const auto& patchProcessorCollection = privilegeSetConfigureComponents;
const auto& putProcessorCollection = privilegeSetConfigureComponents;
const auto& deleteProcessorCollection = privilegeSetConfigureComponents;
const auto& postProcessorCollection = privilegeSetConfigureComponents;

// ProcessorMetrics
const auto& getProcessorMetrics = privilegeSetLogin;
const auto& headProcessorMetrics = privilegeSetLogin;
const auto& patchProcessorMetrics = privilegeSetConfigureComponents;
const auto& putProcessorMetrics = privilegeSetConfigureComponents;
const auto& deleteProcessorMetrics = privilegeSetConfigureComponents;
const auto& postProcessorMetrics = privilegeSetConfigureComponents;

// ResourceBlock
const auto& getResourceBlock = privilegeSetLogin;
const auto& headResourceBlock = privilegeSetLogin;
const auto& patchResourceBlock = privilegeSetConfigureComponents;
const auto& putResourceBlock = privilegeSetConfigureComponents;
const auto& deleteResourceBlock = privilegeSetConfigureComponents;
const auto& postResourceBlock = privilegeSetConfigureComponents;

// ResourceBlockCollection
const auto& getResourceBlockCollection = privilegeSetLogin;
const auto& headResourceBlockCollection = privilegeSetLogin;
const auto& patchResourceBlockCollection = privilegeSetConfigureComponents;
const auto& putResourceBlockCollection = privilegeSetConfigureComponents;
const auto& deleteResourceBlockCollection = privilegeSetConfigureComponents;
const auto& postResourceBlockCollection = privilegeSetConfigureComponents;

// Role
const auto& getRole = privilegeSetLogin;
const auto& headRole = privilegeSetLogin;
const auto& patchRole = privilegeSetConfigureManager;
const auto& putRole = privilegeSetConfigureManager;
const auto& deleteRole = privilegeSetConfigureManager;
const auto& postRole = privilegeSetConfigureManager;

// RoleCollection
const auto& getRoleCollection = privilegeSetLogin;
const auto& headRoleCollection = privilegeSetLogin;
const auto& patchRoleCollection = privilegeSetConfigureManager;
const auto& putRoleCollection = privilegeSetConfigureManager;
const auto& deleteRoleCollection = privilegeSetConfigureManager;
const auto& postRoleCollection = privilegeSetConfigureManager;

// RouteEntry
const auto& getRouteEntry = privilegeSetLogin;
const auto& headRouteEntry = privilegeSetLogin;
const auto& patchRouteEntry = privilegeSetConfigureComponents;
const auto& putRouteEntry = privilegeSetConfigureComponents;
const auto& deleteRouteEntry = privilegeSetConfigureComponents;
const auto& postRouteEntry = privilegeSetConfigureComponents;

// RouteEntryCollection
const auto& getRouteEntryCollection = privilegeSetLogin;
const auto& headRouteEntryCollection = privilegeSetLogin;
const auto& patchRouteEntryCollection = privilegeSetConfigureComponents;
const auto& putRouteEntryCollection = privilegeSetConfigureComponents;
const auto& deleteRouteEntryCollection = privilegeSetConfigureComponents;
const auto& postRouteEntryCollection = privilegeSetConfigureComponents;

// RouteEntrySet
const auto& getRouteEntrySet = privilegeSetLogin;
const auto& headRouteEntrySet = privilegeSetLogin;
const auto& patchRouteEntrySet = privilegeSetConfigureComponents;
const auto& putRouteEntrySet = privilegeSetConfigureComponents;
const auto& deleteRouteEntrySet = privilegeSetConfigureComponents;
const auto& postRouteEntrySet = privilegeSetConfigureComponents;

// RouteEntrySetCollection
const auto& getRouteEntrySetCollection = privilegeSetLogin;
const auto& headRouteEntrySetCollection = privilegeSetLogin;
const auto& patchRouteEntrySetCollection = privilegeSetConfigureComponents;
const auto& putRouteEntrySetCollection = privilegeSetConfigureComponents;
const auto& deleteRouteEntrySetCollection = privilegeSetConfigureComponents;
const auto& postRouteEntrySetCollection = privilegeSetConfigureComponents;

// SecureBoot
const auto& getSecureBoot = privilegeSetLogin;
const auto& headSecureBoot = privilegeSetLogin;
const auto& patchSecureBoot = privilegeSetConfigureComponents;
const auto& postSecureBoot = privilegeSetConfigureComponents;
const auto& putSecureBoot = privilegeSetConfigureComponents;
const auto& deleteSecureBoot = privilegeSetConfigureComponents;

// SecureBootDatabase
const auto& getSecureBootDatabase = privilegeSetLogin;
const auto& headSecureBootDatabase = privilegeSetLogin;
const auto& patchSecureBootDatabase = privilegeSetConfigureComponents;
const auto& postSecureBootDatabase = privilegeSetConfigureComponents;
const auto& putSecureBootDatabase = privilegeSetConfigureComponents;
const auto& deleteSecureBootDatabase = privilegeSetConfigureComponents;

// SecureBootDatabaseCollection
const auto& getSecureBootDatabaseCollection = privilegeSetLogin;
const auto& headSecureBootDatabaseCollection = privilegeSetLogin;
const auto& patchSecureBootDatabaseCollection = privilegeSetConfigureComponents;
const auto& postSecureBootDatabaseCollection = privilegeSetConfigureComponents;
const auto& putSecureBootDatabaseCollection = privilegeSetConfigureComponents;
const auto& deleteSecureBootDatabaseCollection =
    privilegeSetConfigureComponents;

// Sensor
const auto& getSensor = privilegeSetLogin;
const auto& headSensor = privilegeSetLogin;
const auto& patchSensor = privilegeSetConfigureComponents;
const auto& postSensor = privilegeSetConfigureComponents;
const auto& putSensor = privilegeSetConfigureComponents;
const auto& deleteSensor = privilegeSetConfigureComponents;

// SensorCollection
const auto& getSensorCollection = privilegeSetLogin;
const auto& headSensorCollection = privilegeSetLogin;
const auto& patchSensorCollection = privilegeSetConfigureComponents;
const auto& postSensorCollection = privilegeSetConfigureComponents;
const auto& putSensorCollection = privilegeSetConfigureComponents;
const auto& deleteSensorCollection = privilegeSetConfigureComponents;

// SerialInterface
const auto& getSerialInterface = privilegeSetLogin;
const auto& headSerialInterface = privilegeSetLogin;
const auto& patchSerialInterface = privilegeSetConfigureManager;
const auto& putSerialInterface = privilegeSetConfigureManager;
const auto& deleteSerialInterface = privilegeSetConfigureManager;
const auto& postSerialInterface = privilegeSetConfigureManager;

// SerialInterfaceCollection
const auto& getSerialInterfaceCollection = privilegeSetLogin;
const auto& headSerialInterfaceCollection = privilegeSetLogin;
const auto& patchSerialInterfaceCollection = privilegeSetConfigureManager;
const auto& putSerialInterfaceCollection = privilegeSetConfigureManager;
const auto& deleteSerialInterfaceCollection = privilegeSetConfigureManager;
const auto& postSerialInterfaceCollection = privilegeSetConfigureManager;

// ServiceRoot
const auto& getServiceRoot = privilegeSetLoginOrNoAuth;
const auto& headServiceRoot = privilegeSetLoginOrNoAuth;
const auto& patchServiceRoot = privilegeSetConfigureManager;
const auto& putServiceRoot = privilegeSetConfigureManager;
const auto& deleteServiceRoot = privilegeSetConfigureManager;
const auto& postServiceRoot = privilegeSetConfigureManager;

// Session
const auto& getSession = privilegeSetLogin;
const auto& headSession = privilegeSetLogin;
const auto& patchSession = privilegeSetConfigureManager;
const auto& putSession = privilegeSetConfigureManager;
const auto& deleteSession = privilegeSetConfigureManagerOrConfigureSelf;
const auto& postSession = privilegeSetConfigureManager;

// SessionCollection
const auto& getSessionCollection = privilegeSetLogin;
const auto& headSessionCollection = privilegeSetLogin;
const auto& patchSessionCollection = privilegeSetConfigureManager;
const auto& putSessionCollection = privilegeSetConfigureManager;
const auto& deleteSessionCollection = privilegeSetConfigureManager;
const auto& postSessionCollection = privilegeSetLogin;

// SessionService
const auto& getSessionService = privilegeSetLogin;
const auto& headSessionService = privilegeSetLogin;
const auto& patchSessionService = privilegeSetConfigureManager;
const auto& putSessionService = privilegeSetConfigureManager;
const auto& deleteSessionService = privilegeSetConfigureManager;
const auto& postSessionService = privilegeSetConfigureManager;

// Signature
const auto& getSignature = privilegeSetLogin;
const auto& headSignature = privilegeSetLogin;
const auto& patchSignature = privilegeSetConfigureComponents;
const auto& postSignature = privilegeSetConfigureComponents;
const auto& putSignature = privilegeSetConfigureComponents;
const auto& deleteSignature = privilegeSetConfigureComponents;

// SignatureCollection
const auto& getSignatureCollection = privilegeSetLogin;
const auto& headSignatureCollection = privilegeSetLogin;
const auto& patchSignatureCollection = privilegeSetConfigureComponents;
const auto& postSignatureCollection = privilegeSetConfigureComponents;
const auto& putSignatureCollection = privilegeSetConfigureComponents;
const auto& deleteSignatureCollection = privilegeSetConfigureComponents;

// SimpleStorage
const auto& getSimpleStorage = privilegeSetLogin;
const auto& headSimpleStorage = privilegeSetLogin;
const auto& patchSimpleStorage = privilegeSetConfigureComponents;
const auto& postSimpleStorage = privilegeSetConfigureComponents;
const auto& putSimpleStorage = privilegeSetConfigureComponents;
const auto& deleteSimpleStorage = privilegeSetConfigureComponents;

// SimpleStorageCollection
const auto& getSimpleStorageCollection = privilegeSetLogin;
const auto& headSimpleStorageCollection = privilegeSetLogin;
const auto& patchSimpleStorageCollection = privilegeSetConfigureComponents;
const auto& postSimpleStorageCollection = privilegeSetConfigureComponents;
const auto& putSimpleStorageCollection = privilegeSetConfigureComponents;
const auto& deleteSimpleStorageCollection = privilegeSetConfigureComponents;

// SoftwareInventory
const auto& getSoftwareInventory = privilegeSetLogin;
const auto& headSoftwareInventory = privilegeSetLogin;
const auto& patchSoftwareInventory = privilegeSetConfigureComponents;
const auto& postSoftwareInventory = privilegeSetConfigureComponents;
const auto& putSoftwareInventory = privilegeSetConfigureComponents;
const auto& deleteSoftwareInventory = privilegeSetConfigureComponents;

// SoftwareInventoryCollection
const auto& getSoftwareInventoryCollection = privilegeSetLogin;
const auto& headSoftwareInventoryCollection = privilegeSetLogin;
const auto& patchSoftwareInventoryCollection = privilegeSetConfigureComponents;
const auto& postSoftwareInventoryCollection = privilegeSetConfigureComponents;
const auto& putSoftwareInventoryCollection = privilegeSetConfigureComponents;
const auto& deleteSoftwareInventoryCollection = privilegeSetConfigureComponents;

// Storage
const auto& getStorage = privilegeSetLogin;
const auto& headStorage = privilegeSetLogin;
const auto& patchStorage = privilegeSetConfigureComponents;
const auto& postStorage = privilegeSetConfigureComponents;
const auto& putStorage = privilegeSetConfigureComponents;
const auto& deleteStorage = privilegeSetConfigureComponents;

// StorageCollection
const auto& getStorageCollection = privilegeSetLogin;
const auto& headStorageCollection = privilegeSetLogin;
const auto& patchStorageCollection = privilegeSetConfigureComponents;
const auto& postStorageCollection = privilegeSetConfigureComponents;
const auto& putStorageCollection = privilegeSetConfigureComponents;
const auto& deleteStorageCollection = privilegeSetConfigureComponents;

// StorageController
const auto& getStorageController = privilegeSetLogin;
const auto& headStorageController = privilegeSetLogin;
const auto& patchStorageController = privilegeSetConfigureComponents;
const auto& postStorageController = privilegeSetConfigureComponents;
const auto& putStorageController = privilegeSetConfigureComponents;
const auto& deleteStorageController = privilegeSetConfigureComponents;

// StorageControllerCollection
const auto& getStorageControllerCollection = privilegeSetLogin;
const auto& headStorageControllerCollection = privilegeSetLogin;
const auto& patchStorageControllerCollection = privilegeSetConfigureComponents;
const auto& postStorageControllerCollection = privilegeSetConfigureComponents;
const auto& putStorageControllerCollection = privilegeSetConfigureComponents;
const auto& deleteStorageControllerCollection = privilegeSetConfigureComponents;

// Switch
const auto& getSwitch = privilegeSetLogin;
const auto& headSwitch = privilegeSetLogin;
const auto& patchSwitch = privilegeSetConfigureComponents;
const auto& postSwitch = privilegeSetConfigureComponents;
const auto& putSwitch = privilegeSetConfigureComponents;
const auto& deleteSwitch = privilegeSetConfigureComponents;

// SwitchCollection
const auto& getSwitchCollection = privilegeSetLogin;
const auto& headSwitchCollection = privilegeSetLogin;
const auto& patchSwitchCollection = privilegeSetConfigureComponents;
const auto& postSwitchCollection = privilegeSetConfigureComponents;
const auto& putSwitchCollection = privilegeSetConfigureComponents;
const auto& deleteSwitchCollection = privilegeSetConfigureComponents;

// Task
const auto& getTask = privilegeSetLogin;
const auto& headTask = privilegeSetLogin;
const auto& patchTask = privilegeSetConfigureManager;
const auto& putTask = privilegeSetConfigureManager;
const auto& deleteTask = privilegeSetConfigureManager;
const auto& postTask = privilegeSetConfigureManager;

// TaskCollection
const auto& getTaskCollection = privilegeSetLogin;
const auto& headTaskCollection = privilegeSetLogin;
const auto& patchTaskCollection = privilegeSetConfigureManager;
const auto& putTaskCollection = privilegeSetConfigureManager;
const auto& deleteTaskCollection = privilegeSetConfigureManager;
const auto& postTaskCollection = privilegeSetConfigureManager;

// TaskService
const auto& getTaskService = privilegeSetLogin;
const auto& headTaskService = privilegeSetLogin;
const auto& patchTaskService = privilegeSetConfigureManager;
const auto& putTaskService = privilegeSetConfigureManager;
const auto& deleteTaskService = privilegeSetConfigureManager;
const auto& postTaskService = privilegeSetConfigureManager;

// TelemetryService
const auto& getTelemetryService = privilegeSetLogin;
const auto& headTelemetryService = privilegeSetLogin;
const auto& patchTelemetryService = privilegeSetConfigureManager;
const auto& putTelemetryService = privilegeSetConfigureManager;
const auto& deleteTelemetryService = privilegeSetConfigureManager;
const auto& postTelemetryService = privilegeSetConfigureManager;

// Thermal
const auto& getThermal = privilegeSetLogin;
const auto& headThermal = privilegeSetLogin;
const auto& patchThermal = privilegeSetConfigureManager;
const auto& putThermal = privilegeSetConfigureManager;
const auto& deleteThermal = privilegeSetConfigureManager;
const auto& postThermal = privilegeSetConfigureManager;

// Triggers
const auto& getTriggers = privilegeSetLogin;
const auto& headTriggers = privilegeSetLogin;
const auto& patchTriggers = privilegeSetConfigureManager;
const auto& putTriggers = privilegeSetConfigureManager;
const auto& deleteTriggers = privilegeSetConfigureManager;
const auto& postTriggers = privilegeSetConfigureManager;

// TriggersCollection
const auto& getTriggersCollection = privilegeSetLogin;
const auto& headTriggersCollection = privilegeSetLogin;
const auto& patchTriggersCollection = privilegeSetConfigureManager;
const auto& putTriggersCollection = privilegeSetConfigureManager;
const auto& deleteTriggersCollection = privilegeSetConfigureManager;
const auto& postTriggersCollection = privilegeSetConfigureManager;

// UpdateService
const auto& getUpdateService = privilegeSetLogin;
const auto& headUpdateService = privilegeSetLogin;
const auto& patchUpdateService = privilegeSetConfigureComponents;
const auto& postUpdateService = privilegeSetConfigureComponents;
const auto& putUpdateService = privilegeSetConfigureComponents;
const auto& deleteUpdateService = privilegeSetConfigureComponents;

// VCATEntry
const auto& getVCATEntry = privilegeSetLogin;
const auto& headVCATEntry = privilegeSetLogin;
const auto& patchVCATEntry = privilegeSetConfigureComponents;
const auto& putVCATEntry = privilegeSetConfigureComponents;
const auto& deleteVCATEntry = privilegeSetConfigureComponents;
const auto& postVCATEntry = privilegeSetConfigureComponents;

// VCATEntryCollection
const auto& getVCATEntryCollection = privilegeSetLogin;
const auto& headVCATEntryCollection = privilegeSetLogin;
const auto& patchVCATEntryCollection = privilegeSetConfigureComponents;
const auto& putVCATEntryCollection = privilegeSetConfigureComponents;
const auto& deleteVCATEntryCollection = privilegeSetConfigureComponents;
const auto& postVCATEntryCollection = privilegeSetConfigureComponents;

// VLanNetworkInterface
const auto& getVLanNetworkInterface = privilegeSetLogin;
const auto& headVLanNetworkInterface = privilegeSetLogin;
const auto& patchVLanNetworkInterface = privilegeSetConfigureManager;
const auto& putVLanNetworkInterface = privilegeSetConfigureManager;
const auto& deleteVLanNetworkInterface = privilegeSetConfigureManager;
const auto& postVLanNetworkInterface = privilegeSetConfigureManager;

// VLanNetworkInterfaceCollection
const auto& getVLanNetworkInterfaceCollection = privilegeSetLogin;
const auto& headVLanNetworkInterfaceCollection = privilegeSetLogin;
const auto& patchVLanNetworkInterfaceCollection = privilegeSetConfigureManager;
const auto& putVLanNetworkInterfaceCollection = privilegeSetConfigureManager;
const auto& deleteVLanNetworkInterfaceCollection = privilegeSetConfigureManager;
const auto& postVLanNetworkInterfaceCollection = privilegeSetConfigureManager;

// VirtualMedia
const auto& getVirtualMedia = privilegeSetLogin;
const auto& headVirtualMedia = privilegeSetLogin;
const auto& patchVirtualMedia = privilegeSetConfigureManager;
const auto& putVirtualMedia = privilegeSetConfigureManager;
const auto& deleteVirtualMedia = privilegeSetConfigureManager;
const auto& postVirtualMedia = privilegeSetConfigureManager;

// VirtualMediaCollection
const auto& getVirtualMediaCollection = privilegeSetLogin;
const auto& headVirtualMediaCollection = privilegeSetLogin;
const auto& patchVirtualMediaCollection = privilegeSetConfigureManager;
const auto& putVirtualMediaCollection = privilegeSetConfigureManager;
const auto& deleteVirtualMediaCollection = privilegeSetConfigureManager;
const auto& postVirtualMediaCollection = privilegeSetConfigureManager;

// Volume
const auto& getVolume = privilegeSetLogin;
const auto& headVolume = privilegeSetLogin;
const auto& patchVolume = privilegeSetConfigureComponents;
const auto& postVolume = privilegeSetConfigureComponents;
const auto& putVolume = privilegeSetConfigureComponents;
const auto& deleteVolume = privilegeSetConfigureComponents;

// VolumeCollection
const auto& getVolumeCollection = privilegeSetLogin;
const auto& headVolumeCollection = privilegeSetLogin;
const auto& patchVolumeCollection = privilegeSetConfigureComponents;
const auto& postVolumeCollection = privilegeSetConfigureComponents;
const auto& putVolumeCollection = privilegeSetConfigureComponents;
const auto& deleteVolumeCollection = privilegeSetConfigureComponents;

// Zone
const auto& getZone = privilegeSetLogin;
const auto& headZone = privilegeSetLogin;
const auto& patchZone = privilegeSetConfigureComponents;
const auto& postZone = privilegeSetConfigureComponents;
const auto& putZone = privilegeSetConfigureComponents;
const auto& deleteZone = privilegeSetConfigureComponents;

// ZoneCollection
const auto& getZoneCollection = privilegeSetLogin;
const auto& headZoneCollection = privilegeSetLogin;
const auto& patchZoneCollection = privilegeSetConfigureComponents;
const auto& postZoneCollection = privilegeSetConfigureComponents;
const auto& putZoneCollection = privilegeSetConfigureComponents;
const auto& deleteZoneCollection = privilegeSetConfigureComponents;

} // namespace redfish::privileges
