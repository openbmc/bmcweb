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
#include "verb.hpp"

#include <array>
#include <map>
#include <string_view>
#include <vector>

// clang-format off

namespace redfish::privileges
{
const std::vector<Privileges> privilegeSetLogin = {{
    {"Login"}
}};
const std::vector<Privileges> privilegeSetConfigureComponents = {{
    {"ConfigureComponents"}
}};
const std::vector<Privileges> privilegeSetConfigureUsers = {{
    {"ConfigureUsers"}
}};
const std::vector<Privileges> privilegeSetConfigureManager = {{
    {"ConfigureManager"}
}};
const std::vector<Privileges> privilegeSetConfigureManagerOrConfigureComponents = {{
    {"ConfigureManager"},
    {"ConfigureComponents"}
}};
const std::vector<Privileges> privilegeSetConfigureManagerOrConfigureSelf = {{
    {"ConfigureManager"},
    {"ConfigureSelf"}
}};
const std::vector<Privileges> privilegeSetConfigureManagerOrConfigureUsersOrConfigureSelf = {{
    {"ConfigureManager"},
    {"ConfigureUsers"},
    {"ConfigureSelf"}
}};
const std::vector<Privileges> privilegeSetLoginOrNoAuth = {{
    {"Login"},
    {}
}};

enum class EntityTag : size_t {
	tagAccelerationFunction = 0,
	tagAccelerationFunctionCollection = 1,
	tagAccountService = 2,
	tagActionInfo = 3,
	tagAddressPool = 4,
	tagAddressPoolCollection = 5,
	tagAggregate = 6,
	tagAggregateCollection = 7,
	tagAggregationService = 8,
	tagAggregationSource = 9,
	tagAggregationSourceCollection = 10,
	tagAllowDeny = 11,
	tagAllowDenyCollection = 12,
	tagAssembly = 13,
	tagBattery = 14,
	tagBatteryCollection = 15,
	tagBatteryMetrics = 16,
	tagBios = 17,
	tagBootOption = 18,
	tagBootOptionCollection = 19,
	tagCable = 20,
	tagCableCollection = 21,
	tagCertificate = 22,
	tagCertificateCollection = 23,
	tagCertificateLocations = 24,
	tagCertificateService = 25,
	tagChassis = 26,
	tagChassisCollection = 27,
	tagCircuit = 28,
	tagCircuitCollection = 29,
	tagCompositionReservation = 30,
	tagCompositionReservationCollection = 31,
	tagCompositionService = 32,
	tagComputerSystem = 33,
	tagComputerSystemCollection = 34,
	tagConnection = 35,
	tagConnectionCollection = 36,
	tagConnectionMethod = 37,
	tagConnectionMethodCollection = 38,
	tagControl = 39,
	tagControlCollection = 40,
	tagDrive = 41,
	tagDriveCollection = 42,
	tagEndpoint = 43,
	tagEndpointCollection = 44,
	tagEndpointGroup = 45,
	tagEndpointGroupCollection = 46,
	tagEnvironmentMetrics = 47,
	tagEthernetInterface = 48,
	tagEthernetInterfaceCollection = 49,
	tagEventDestination = 50,
	tagEventDestinationCollection = 51,
	tagEventService = 52,
	tagExternalAccountProvider = 53,
	tagExternalAccountProviderCollection = 54,
	tagFabric = 55,
	tagFabricCollection = 56,
	tagFabricAdapter = 57,
	tagFabricAdapterCollection = 58,
	tagFacility = 59,
	tagFacilityCollection = 60,
	tagFan = 61,
	tagFanCollection = 62,
	tagGraphicsController = 63,
	tagGraphicsControllerCollection = 64,
	tagHostInterface = 65,
	tagHostInterfaceCollection = 66,
	tagJob = 67,
	tagJobCollection = 68,
	tagJobService = 69,
	tagJsonSchemaFile = 70,
	tagJsonSchemaFileCollection = 71,
	tagKey = 72,
	tagKeyCollection = 73,
	tagKeyPolicy = 74,
	tagKeyPolicyCollection = 75,
	tagKeyService = 76,
	tagLogEntry = 77,
	tagLogEntryCollection = 78,
	tagLogService = 79,
	tagLogServiceCollection = 80,
	tagManager = 81,
	tagManagerCollection = 82,
	tagManagerAccount = 83,
	tagManagerAccountCollection = 84,
	tagManagerDiagnosticData = 85,
	tagManagerNetworkProtocol = 86,
	tagMediaController = 87,
	tagMediaControllerCollection = 88,
	tagMemory = 89,
	tagMemoryCollection = 90,
	tagMemoryChunks = 91,
	tagMemoryChunksCollection = 92,
	tagMemoryDomain = 93,
	tagMemoryDomainCollection = 94,
	tagMemoryMetrics = 95,
	tagMessageRegistryFile = 96,
	tagMessageRegistryFileCollection = 97,
	tagMetricDefinition = 98,
	tagMetricDefinitionCollection = 99,
	tagMetricReport = 100,
	tagMetricReportCollection = 101,
	tagMetricReportDefinition = 102,
	tagMetricReportDefinitionCollection = 103,
	tagNetworkAdapter = 104,
	tagNetworkAdapterCollection = 105,
	tagNetworkAdapterMetrics = 106,
	tagNetworkDeviceFunction = 107,
	tagNetworkDeviceFunctionCollection = 108,
	tagNetworkDeviceFunctionMetrics = 109,
	tagNetworkInterface = 110,
	tagNetworkInterfaceCollection = 111,
	tagNetworkPort = 112,
	tagNetworkPortCollection = 113,
	tagOperatingConfig = 114,
	tagOperatingConfigCollection = 115,
	tagOutlet = 116,
	tagOutletCollection = 117,
	tagOutletGroup = 118,
	tagOutletGroupCollection = 119,
	tagPCIeDevice = 120,
	tagPCIeDeviceCollection = 121,
	tagPCIeFunction = 122,
	tagPCIeFunctionCollection = 123,
	tagPCIeSlots = 124,
	tagPort = 125,
	tagPortCollection = 126,
	tagPortMetrics = 127,
	tagPower = 128,
	tagPowerDistribution = 129,
	tagPowerDistributionCollection = 130,
	tagPowerDistributionMetrics = 131,
	tagPowerDomain = 132,
	tagPowerDomainCollection = 133,
	tagPowerEquipment = 134,
	tagPowerSubsystem = 135,
	tagPowerSupply = 136,
	tagPowerSupplyCollection = 137,
	tagPowerSupplyMetrics = 138,
	tagProcessor = 139,
	tagProcessorCollection = 140,
	tagProcessorMetrics = 141,
	tagResourceBlock = 142,
	tagResourceBlockCollection = 143,
	tagRole = 144,
	tagRoleCollection = 145,
	tagRouteEntry = 146,
	tagRouteEntryCollection = 147,
	tagRouteSetEntry = 148,
	tagRouteSetEntryCollection = 149,
	tagSecureBoot = 150,
	tagSecureBootDatabase = 151,
	tagSecureBootDatabaseCollection = 152,
	tagSensor = 153,
	tagSensorCollection = 154,
	tagSerialInterface = 155,
	tagSerialInterfaceCollection = 156,
	tagServiceRoot = 157,
	tagSession = 158,
	tagSessionCollection = 159,
	tagSessionService = 160,
	tagSignature = 161,
	tagSignatureCollection = 162,
	tagSimpleStorage = 163,
	tagSimpleStorageCollection = 164,
	tagSoftwareInventory = 165,
	tagSoftwareInventoryCollection = 166,
	tagStorage = 167,
	tagStorageCollection = 168,
	tagStorageController = 169,
	tagStorageControllerCollection = 170,
	tagSwitch = 171,
	tagSwitchCollection = 172,
	tagTask = 173,
	tagTaskCollection = 174,
	tagTaskService = 175,
	tagTelemetryService = 176,
	tagThermal = 177,
	tagThermalMetrics = 178,
	tagThermalSubsystem = 179,
	tagTriggers = 180,
	tagTriggersCollection = 181,
	tagUpdateService = 182,
	tagUSBController = 183,
	tagUSBControllerCollection = 184,
	tagVCATEntry = 185,
	tagVCATEntryCollection = 186,
	tagVLanNetworkInterface = 187,
	tagVLanNetworkInterfaceCollection = 188,
	tagVirtualMedia = 189,
	tagVirtualMediaCollection = 190,
	tagVolume = 191,
	tagVolumeCollection = 192,
	tagZone = 193,
	tagZoneCollection = 194,
	none = 195,
};

std::map<std::string_view, EntityTag> entityTagMap;

// AccelerationFunction
std::map<HttpVerb, std::vector<Privileges>> tagAccelerationFunctionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
};

// AccelerationFunctionCollection
std::map<HttpVerb, std::vector<Privileges>> tagAccelerationFunctionCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
};

// AccountService
std::map<HttpVerb, std::vector<Privileges>> tagAccountServiceMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureUsers},
	{HttpVerb::Put, privilegeSetConfigureUsers},
	{HttpVerb::Delete, privilegeSetConfigureUsers},
	{HttpVerb::Post, privilegeSetConfigureUsers},
};

// ActionInfo
std::map<HttpVerb, std::vector<Privileges>> tagActionInfoMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// AddressPool
std::map<HttpVerb, std::vector<Privileges>> tagAddressPoolMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
};

// AddressPoolCollection
std::map<HttpVerb, std::vector<Privileges>> tagAddressPoolCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
};

// Aggregate
std::map<HttpVerb, std::vector<Privileges>> tagAggregateMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManagerOrConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureManagerOrConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureManagerOrConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureManagerOrConfigureComponents},
};

// AggregateCollection
std::map<HttpVerb, std::vector<Privileges>> tagAggregateCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManagerOrConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureManagerOrConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureManagerOrConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureManagerOrConfigureComponents},
};

// AggregationService
std::map<HttpVerb, std::vector<Privileges>> tagAggregationServiceMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// AggregationSource
std::map<HttpVerb, std::vector<Privileges>> tagAggregationSourceMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// AggregationSourceCollection
std::map<HttpVerb, std::vector<Privileges>> tagAggregationSourceCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// AllowDeny
std::map<HttpVerb, std::vector<Privileges>> tagAllowDenyMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// AllowDenyCollection
std::map<HttpVerb, std::vector<Privileges>> tagAllowDenyCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// Assembly
std::map<HttpVerb, std::vector<Privileges>> tagAssemblyMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
};

// Battery
std::map<HttpVerb, std::vector<Privileges>> tagBatteryMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// BatteryCollection
std::map<HttpVerb, std::vector<Privileges>> tagBatteryCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// BatteryMetrics
std::map<HttpVerb, std::vector<Privileges>> tagBatteryMetricsMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// Bios
std::map<HttpVerb, std::vector<Privileges>> tagBiosMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// BootOption
std::map<HttpVerb, std::vector<Privileges>> tagBootOptionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
};

// BootOptionCollection
std::map<HttpVerb, std::vector<Privileges>> tagBootOptionCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
};

// Cable
std::map<HttpVerb, std::vector<Privileges>> tagCableMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
};

// CableCollection
std::map<HttpVerb, std::vector<Privileges>> tagCableCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
};

// Certificate
std::map<HttpVerb, std::vector<Privileges>> tagCertificateMap =  {
	{HttpVerb::Get, privilegeSetConfigureManager},
	{HttpVerb::Head, privilegeSetConfigureManager},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// CertificateCollection
std::map<HttpVerb, std::vector<Privileges>> tagCertificateCollectionMap =  {
	{HttpVerb::Get, privilegeSetConfigureManager},
	{HttpVerb::Head, privilegeSetConfigureManager},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// CertificateLocations
std::map<HttpVerb, std::vector<Privileges>> tagCertificateLocationsMap =  {
	{HttpVerb::Get, privilegeSetConfigureManager},
	{HttpVerb::Head, privilegeSetConfigureManager},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// CertificateService
std::map<HttpVerb, std::vector<Privileges>> tagCertificateServiceMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// Chassis
std::map<HttpVerb, std::vector<Privileges>> tagChassisMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
};

// ChassisCollection
std::map<HttpVerb, std::vector<Privileges>> tagChassisCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
};

// Circuit
std::map<HttpVerb, std::vector<Privileges>> tagCircuitMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
};

// CircuitCollection
std::map<HttpVerb, std::vector<Privileges>> tagCircuitCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
};

// CompositionReservation
std::map<HttpVerb, std::vector<Privileges>> tagCompositionReservationMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// CompositionReservationCollection
std::map<HttpVerb, std::vector<Privileges>> tagCompositionReservationCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// CompositionService
std::map<HttpVerb, std::vector<Privileges>> tagCompositionServiceMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// ComputerSystem
std::map<HttpVerb, std::vector<Privileges>> tagComputerSystemMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// ComputerSystemCollection
std::map<HttpVerb, std::vector<Privileges>> tagComputerSystemCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// Connection
std::map<HttpVerb, std::vector<Privileges>> tagConnectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// ConnectionCollection
std::map<HttpVerb, std::vector<Privileges>> tagConnectionCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// ConnectionMethod
std::map<HttpVerb, std::vector<Privileges>> tagConnectionMethodMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// ConnectionMethodCollection
std::map<HttpVerb, std::vector<Privileges>> tagConnectionMethodCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// Control
std::map<HttpVerb, std::vector<Privileges>> tagControlMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// ControlCollection
std::map<HttpVerb, std::vector<Privileges>> tagControlCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// Drive
std::map<HttpVerb, std::vector<Privileges>> tagDriveMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// DriveCollection
std::map<HttpVerb, std::vector<Privileges>> tagDriveCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// Endpoint
std::map<HttpVerb, std::vector<Privileges>> tagEndpointMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// EndpointCollection
std::map<HttpVerb, std::vector<Privileges>> tagEndpointCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// EndpointGroup
std::map<HttpVerb, std::vector<Privileges>> tagEndpointGroupMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// EndpointGroupCollection
std::map<HttpVerb, std::vector<Privileges>> tagEndpointGroupCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// EnvironmentMetrics
std::map<HttpVerb, std::vector<Privileges>> tagEnvironmentMetricsMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// EthernetInterface
std::map<HttpVerb, std::vector<Privileges>> tagEthernetInterfaceMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// EthernetInterfaceCollection
std::map<HttpVerb, std::vector<Privileges>> tagEthernetInterfaceCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// EventDestination
std::map<HttpVerb, std::vector<Privileges>> tagEventDestinationMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManagerOrConfigureSelf},
	{HttpVerb::Post, privilegeSetConfigureManagerOrConfigureSelf},
	{HttpVerb::Put, privilegeSetConfigureManagerOrConfigureSelf},
	{HttpVerb::Delete, privilegeSetConfigureManagerOrConfigureSelf},
};

// EventDestinationCollection
std::map<HttpVerb, std::vector<Privileges>> tagEventDestinationCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManagerOrConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureManagerOrConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureManagerOrConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureManagerOrConfigureComponents},
};

// EventService
std::map<HttpVerb, std::vector<Privileges>> tagEventServiceMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
};

// ExternalAccountProvider
std::map<HttpVerb, std::vector<Privileges>> tagExternalAccountProviderMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// ExternalAccountProviderCollection
std::map<HttpVerb, std::vector<Privileges>> tagExternalAccountProviderCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// Fabric
std::map<HttpVerb, std::vector<Privileges>> tagFabricMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// FabricCollection
std::map<HttpVerb, std::vector<Privileges>> tagFabricCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// FabricAdapter
std::map<HttpVerb, std::vector<Privileges>> tagFabricAdapterMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// FabricAdapterCollection
std::map<HttpVerb, std::vector<Privileges>> tagFabricAdapterCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// Facility
std::map<HttpVerb, std::vector<Privileges>> tagFacilityMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
};

// FacilityCollection
std::map<HttpVerb, std::vector<Privileges>> tagFacilityCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
};

// Fan
std::map<HttpVerb, std::vector<Privileges>> tagFanMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// FanCollection
std::map<HttpVerb, std::vector<Privileges>> tagFanCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// GraphicsController
std::map<HttpVerb, std::vector<Privileges>> tagGraphicsControllerMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
};

// GraphicsControllerCollection
std::map<HttpVerb, std::vector<Privileges>> tagGraphicsControllerCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
};

// HostInterface
std::map<HttpVerb, std::vector<Privileges>> tagHostInterfaceMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
};

// HostInterfaceCollection
std::map<HttpVerb, std::vector<Privileges>> tagHostInterfaceCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
};

// Job
std::map<HttpVerb, std::vector<Privileges>> tagJobMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// JobCollection
std::map<HttpVerb, std::vector<Privileges>> tagJobCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// JobService
std::map<HttpVerb, std::vector<Privileges>> tagJobServiceMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// JsonSchemaFile
std::map<HttpVerb, std::vector<Privileges>> tagJsonSchemaFileMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
};

// JsonSchemaFileCollection
std::map<HttpVerb, std::vector<Privileges>> tagJsonSchemaFileCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
};

// Key
std::map<HttpVerb, std::vector<Privileges>> tagKeyMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// KeyCollection
std::map<HttpVerb, std::vector<Privileges>> tagKeyCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// KeyPolicy
std::map<HttpVerb, std::vector<Privileges>> tagKeyPolicyMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// KeyPolicyCollection
std::map<HttpVerb, std::vector<Privileges>> tagKeyPolicyCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// KeyService
std::map<HttpVerb, std::vector<Privileges>> tagKeyServiceMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// LogEntry
std::map<HttpVerb, std::vector<Privileges>> tagLogEntryMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// LogEntryCollection
std::map<HttpVerb, std::vector<Privileges>> tagLogEntryCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// LogService
std::map<HttpVerb, std::vector<Privileges>> tagLogServiceMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// LogServiceCollection
std::map<HttpVerb, std::vector<Privileges>> tagLogServiceCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// Manager
std::map<HttpVerb, std::vector<Privileges>> tagManagerMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
};

// ManagerCollection
std::map<HttpVerb, std::vector<Privileges>> tagManagerCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
};

// ManagerAccount
std::map<HttpVerb, std::vector<Privileges>> tagManagerAccountMap =  {
	{HttpVerb::Get, privilegeSetConfigureManagerOrConfigureUsersOrConfigureSelf},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureUsers},
	{HttpVerb::Post, privilegeSetConfigureUsers},
	{HttpVerb::Put, privilegeSetConfigureUsers},
	{HttpVerb::Delete, privilegeSetConfigureUsers},
};

// ManagerAccountCollection
std::map<HttpVerb, std::vector<Privileges>> tagManagerAccountCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureUsers},
	{HttpVerb::Put, privilegeSetConfigureUsers},
	{HttpVerb::Delete, privilegeSetConfigureUsers},
	{HttpVerb::Post, privilegeSetConfigureUsers},
};

// ManagerDiagnosticData
std::map<HttpVerb, std::vector<Privileges>> tagManagerDiagnosticDataMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
};

// ManagerNetworkProtocol
std::map<HttpVerb, std::vector<Privileges>> tagManagerNetworkProtocolMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
};

// MediaController
std::map<HttpVerb, std::vector<Privileges>> tagMediaControllerMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// MediaControllerCollection
std::map<HttpVerb, std::vector<Privileges>> tagMediaControllerCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// Memory
std::map<HttpVerb, std::vector<Privileges>> tagMemoryMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// MemoryCollection
std::map<HttpVerb, std::vector<Privileges>> tagMemoryCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// MemoryChunks
std::map<HttpVerb, std::vector<Privileges>> tagMemoryChunksMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// MemoryChunksCollection
std::map<HttpVerb, std::vector<Privileges>> tagMemoryChunksCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// MemoryDomain
std::map<HttpVerb, std::vector<Privileges>> tagMemoryDomainMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// MemoryDomainCollection
std::map<HttpVerb, std::vector<Privileges>> tagMemoryDomainCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// MemoryMetrics
std::map<HttpVerb, std::vector<Privileges>> tagMemoryMetricsMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// MessageRegistryFile
std::map<HttpVerb, std::vector<Privileges>> tagMessageRegistryFileMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
};

// MessageRegistryFileCollection
std::map<HttpVerb, std::vector<Privileges>> tagMessageRegistryFileCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
};

// MetricDefinition
std::map<HttpVerb, std::vector<Privileges>> tagMetricDefinitionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// MetricDefinitionCollection
std::map<HttpVerb, std::vector<Privileges>> tagMetricDefinitionCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// MetricReport
std::map<HttpVerb, std::vector<Privileges>> tagMetricReportMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// MetricReportCollection
std::map<HttpVerb, std::vector<Privileges>> tagMetricReportCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// MetricReportDefinition
std::map<HttpVerb, std::vector<Privileges>> tagMetricReportDefinitionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// MetricReportDefinitionCollection
std::map<HttpVerb, std::vector<Privileges>> tagMetricReportDefinitionCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// NetworkAdapter
std::map<HttpVerb, std::vector<Privileges>> tagNetworkAdapterMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// NetworkAdapterCollection
std::map<HttpVerb, std::vector<Privileges>> tagNetworkAdapterCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// NetworkAdapterMetrics
std::map<HttpVerb, std::vector<Privileges>> tagNetworkAdapterMetricsMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// NetworkDeviceFunction
std::map<HttpVerb, std::vector<Privileges>> tagNetworkDeviceFunctionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// NetworkDeviceFunctionCollection
std::map<HttpVerb, std::vector<Privileges>> tagNetworkDeviceFunctionCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// NetworkDeviceFunctionMetrics
std::map<HttpVerb, std::vector<Privileges>> tagNetworkDeviceFunctionMetricsMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// NetworkInterface
std::map<HttpVerb, std::vector<Privileges>> tagNetworkInterfaceMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// NetworkInterfaceCollection
std::map<HttpVerb, std::vector<Privileges>> tagNetworkInterfaceCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// NetworkPort
std::map<HttpVerb, std::vector<Privileges>> tagNetworkPortMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// NetworkPortCollection
std::map<HttpVerb, std::vector<Privileges>> tagNetworkPortCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// OperatingConfig
std::map<HttpVerb, std::vector<Privileges>> tagOperatingConfigMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// OperatingConfigCollection
std::map<HttpVerb, std::vector<Privileges>> tagOperatingConfigCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// Outlet
std::map<HttpVerb, std::vector<Privileges>> tagOutletMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// OutletCollection
std::map<HttpVerb, std::vector<Privileges>> tagOutletCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// OutletGroup
std::map<HttpVerb, std::vector<Privileges>> tagOutletGroupMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// OutletGroupCollection
std::map<HttpVerb, std::vector<Privileges>> tagOutletGroupCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// PCIeDevice
std::map<HttpVerb, std::vector<Privileges>> tagPCIeDeviceMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// PCIeDeviceCollection
std::map<HttpVerb, std::vector<Privileges>> tagPCIeDeviceCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// PCIeFunction
std::map<HttpVerb, std::vector<Privileges>> tagPCIeFunctionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// PCIeFunctionCollection
std::map<HttpVerb, std::vector<Privileges>> tagPCIeFunctionCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// PCIeSlots
std::map<HttpVerb, std::vector<Privileges>> tagPCIeSlotsMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// Port
std::map<HttpVerb, std::vector<Privileges>> tagPortMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// PortCollection
std::map<HttpVerb, std::vector<Privileges>> tagPortCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// PortMetrics
std::map<HttpVerb, std::vector<Privileges>> tagPortMetricsMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// Power
std::map<HttpVerb, std::vector<Privileges>> tagPowerMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// PowerDistribution
std::map<HttpVerb, std::vector<Privileges>> tagPowerDistributionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// PowerDistributionCollection
std::map<HttpVerb, std::vector<Privileges>> tagPowerDistributionCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// PowerDistributionMetrics
std::map<HttpVerb, std::vector<Privileges>> tagPowerDistributionMetricsMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// PowerDomain
std::map<HttpVerb, std::vector<Privileges>> tagPowerDomainMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// PowerDomainCollection
std::map<HttpVerb, std::vector<Privileges>> tagPowerDomainCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// PowerEquipment
std::map<HttpVerb, std::vector<Privileges>> tagPowerEquipmentMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// PowerSubsystem
std::map<HttpVerb, std::vector<Privileges>> tagPowerSubsystemMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// PowerSupply
std::map<HttpVerb, std::vector<Privileges>> tagPowerSupplyMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// PowerSupplyCollection
std::map<HttpVerb, std::vector<Privileges>> tagPowerSupplyCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// PowerSupplyMetrics
std::map<HttpVerb, std::vector<Privileges>> tagPowerSupplyMetricsMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// Processor
std::map<HttpVerb, std::vector<Privileges>> tagProcessorMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
};

// ProcessorCollection
std::map<HttpVerb, std::vector<Privileges>> tagProcessorCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
};

// ProcessorMetrics
std::map<HttpVerb, std::vector<Privileges>> tagProcessorMetricsMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
};

// ResourceBlock
std::map<HttpVerb, std::vector<Privileges>> tagResourceBlockMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
};

// ResourceBlockCollection
std::map<HttpVerb, std::vector<Privileges>> tagResourceBlockCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
};

// Role
std::map<HttpVerb, std::vector<Privileges>> tagRoleMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// RoleCollection
std::map<HttpVerb, std::vector<Privileges>> tagRoleCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// RouteEntry
std::map<HttpVerb, std::vector<Privileges>> tagRouteEntryMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
};

// RouteEntryCollection
std::map<HttpVerb, std::vector<Privileges>> tagRouteEntryCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
};

// RouteSetEntry
std::map<HttpVerb, std::vector<Privileges>> tagRouteSetEntryMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
};

// RouteSetEntryCollection
std::map<HttpVerb, std::vector<Privileges>> tagRouteSetEntryCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
};

// SecureBoot
std::map<HttpVerb, std::vector<Privileges>> tagSecureBootMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// SecureBootDatabase
std::map<HttpVerb, std::vector<Privileges>> tagSecureBootDatabaseMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// SecureBootDatabaseCollection
std::map<HttpVerb, std::vector<Privileges>> tagSecureBootDatabaseCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// Sensor
std::map<HttpVerb, std::vector<Privileges>> tagSensorMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// SensorCollection
std::map<HttpVerb, std::vector<Privileges>> tagSensorCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// SerialInterface
std::map<HttpVerb, std::vector<Privileges>> tagSerialInterfaceMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// SerialInterfaceCollection
std::map<HttpVerb, std::vector<Privileges>> tagSerialInterfaceCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// ServiceRoot
std::map<HttpVerb, std::vector<Privileges>> tagServiceRootMap =  {
	{HttpVerb::Get, privilegeSetLoginOrNoAuth},
	{HttpVerb::Head, privilegeSetLoginOrNoAuth},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// Session
std::map<HttpVerb, std::vector<Privileges>> tagSessionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManagerOrConfigureSelf},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// SessionCollection
std::map<HttpVerb, std::vector<Privileges>> tagSessionCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetLogin},
};

// SessionService
std::map<HttpVerb, std::vector<Privileges>> tagSessionServiceMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// Signature
std::map<HttpVerb, std::vector<Privileges>> tagSignatureMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// SignatureCollection
std::map<HttpVerb, std::vector<Privileges>> tagSignatureCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// SimpleStorage
std::map<HttpVerb, std::vector<Privileges>> tagSimpleStorageMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// SimpleStorageCollection
std::map<HttpVerb, std::vector<Privileges>> tagSimpleStorageCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// SoftwareInventory
std::map<HttpVerb, std::vector<Privileges>> tagSoftwareInventoryMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// SoftwareInventoryCollection
std::map<HttpVerb, std::vector<Privileges>> tagSoftwareInventoryCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// Storage
std::map<HttpVerb, std::vector<Privileges>> tagStorageMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// StorageCollection
std::map<HttpVerb, std::vector<Privileges>> tagStorageCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// StorageController
std::map<HttpVerb, std::vector<Privileges>> tagStorageControllerMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// StorageControllerCollection
std::map<HttpVerb, std::vector<Privileges>> tagStorageControllerCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// Switch
std::map<HttpVerb, std::vector<Privileges>> tagSwitchMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// SwitchCollection
std::map<HttpVerb, std::vector<Privileges>> tagSwitchCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// Task
std::map<HttpVerb, std::vector<Privileges>> tagTaskMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// TaskCollection
std::map<HttpVerb, std::vector<Privileges>> tagTaskCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// TaskService
std::map<HttpVerb, std::vector<Privileges>> tagTaskServiceMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// TelemetryService
std::map<HttpVerb, std::vector<Privileges>> tagTelemetryServiceMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// Thermal
std::map<HttpVerb, std::vector<Privileges>> tagThermalMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// ThermalMetrics
std::map<HttpVerb, std::vector<Privileges>> tagThermalMetricsMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// ThermalSubsystem
std::map<HttpVerb, std::vector<Privileges>> tagThermalSubsystemMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// Triggers
std::map<HttpVerb, std::vector<Privileges>> tagTriggersMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// TriggersCollection
std::map<HttpVerb, std::vector<Privileges>> tagTriggersCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// UpdateService
std::map<HttpVerb, std::vector<Privileges>> tagUpdateServiceMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// USBController
std::map<HttpVerb, std::vector<Privileges>> tagUSBControllerMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
};

// USBControllerCollection
std::map<HttpVerb, std::vector<Privileges>> tagUSBControllerCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
};

// VCATEntry
std::map<HttpVerb, std::vector<Privileges>> tagVCATEntryMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
};

// VCATEntryCollection
std::map<HttpVerb, std::vector<Privileges>> tagVCATEntryCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
};

// VLanNetworkInterface
std::map<HttpVerb, std::vector<Privileges>> tagVLanNetworkInterfaceMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// VLanNetworkInterfaceCollection
std::map<HttpVerb, std::vector<Privileges>> tagVLanNetworkInterfaceCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// VirtualMedia
std::map<HttpVerb, std::vector<Privileges>> tagVirtualMediaMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// VirtualMediaCollection
std::map<HttpVerb, std::vector<Privileges>> tagVirtualMediaCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureManager},
	{HttpVerb::Put, privilegeSetConfigureManager},
	{HttpVerb::Delete, privilegeSetConfigureManager},
	{HttpVerb::Post, privilegeSetConfigureManager},
};

// Volume
std::map<HttpVerb, std::vector<Privileges>> tagVolumeMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// VolumeCollection
std::map<HttpVerb, std::vector<Privileges>> tagVolumeCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// Zone
std::map<HttpVerb, std::vector<Privileges>> tagZoneMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

// ZoneCollection
std::map<HttpVerb, std::vector<Privileges>> tagZoneCollectionMap =  {
	{HttpVerb::Get, privilegeSetLogin},
	{HttpVerb::Head, privilegeSetLogin},
	{HttpVerb::Patch, privilegeSetConfigureComponents},
	{HttpVerb::Post, privilegeSetConfigureComponents},
	{HttpVerb::Put, privilegeSetConfigureComponents},
	{HttpVerb::Delete, privilegeSetConfigureComponents},
};

std::map<EntityTag,std::map<HttpVerb,std::vector<Privileges>>> privilegeSetMap = 

 {
	{EntityTag::tagAccelerationFunction, tagAccelerationFunctionMap},
	{EntityTag::tagAccelerationFunctionCollection, tagAccelerationFunctionCollectionMap},
	{EntityTag::tagAccountService, tagAccountServiceMap},
	{EntityTag::tagActionInfo, tagActionInfoMap},
	{EntityTag::tagAddressPool, tagAddressPoolMap},
	{EntityTag::tagAddressPoolCollection, tagAddressPoolCollectionMap},
	{EntityTag::tagAggregate, tagAggregateMap},
	{EntityTag::tagAggregateCollection, tagAggregateCollectionMap},
	{EntityTag::tagAggregationService, tagAggregationServiceMap},
	{EntityTag::tagAggregationSource, tagAggregationSourceMap},
	{EntityTag::tagAggregationSourceCollection, tagAggregationSourceCollectionMap},
	{EntityTag::tagAllowDeny, tagAllowDenyMap},
	{EntityTag::tagAllowDenyCollection, tagAllowDenyCollectionMap},
	{EntityTag::tagAssembly, tagAssemblyMap},
	{EntityTag::tagBattery, tagBatteryMap},
	{EntityTag::tagBatteryCollection, tagBatteryCollectionMap},
	{EntityTag::tagBatteryMetrics, tagBatteryMetricsMap},
	{EntityTag::tagBios, tagBiosMap},
	{EntityTag::tagBootOption, tagBootOptionMap},
	{EntityTag::tagBootOptionCollection, tagBootOptionCollectionMap},
	{EntityTag::tagCable, tagCableMap},
	{EntityTag::tagCableCollection, tagCableCollectionMap},
	{EntityTag::tagCertificate, tagCertificateMap},
	{EntityTag::tagCertificateCollection, tagCertificateCollectionMap},
	{EntityTag::tagCertificateLocations, tagCertificateLocationsMap},
	{EntityTag::tagCertificateService, tagCertificateServiceMap},
	{EntityTag::tagChassis, tagChassisMap},
	{EntityTag::tagChassisCollection, tagChassisCollectionMap},
	{EntityTag::tagCircuit, tagCircuitMap},
	{EntityTag::tagCircuitCollection, tagCircuitCollectionMap},
	{EntityTag::tagCompositionReservation, tagCompositionReservationMap},
	{EntityTag::tagCompositionReservationCollection, tagCompositionReservationCollectionMap},
	{EntityTag::tagCompositionService, tagCompositionServiceMap},
	{EntityTag::tagComputerSystem, tagComputerSystemMap},
	{EntityTag::tagComputerSystemCollection, tagComputerSystemCollectionMap},
	{EntityTag::tagConnection, tagConnectionMap},
	{EntityTag::tagConnectionCollection, tagConnectionCollectionMap},
	{EntityTag::tagConnectionMethod, tagConnectionMethodMap},
	{EntityTag::tagConnectionMethodCollection, tagConnectionMethodCollectionMap},
	{EntityTag::tagControl, tagControlMap},
	{EntityTag::tagControlCollection, tagControlCollectionMap},
	{EntityTag::tagDrive, tagDriveMap},
	{EntityTag::tagDriveCollection, tagDriveCollectionMap},
	{EntityTag::tagEndpoint, tagEndpointMap},
	{EntityTag::tagEndpointCollection, tagEndpointCollectionMap},
	{EntityTag::tagEndpointGroup, tagEndpointGroupMap},
	{EntityTag::tagEndpointGroupCollection, tagEndpointGroupCollectionMap},
	{EntityTag::tagEnvironmentMetrics, tagEnvironmentMetricsMap},
	{EntityTag::tagEthernetInterface, tagEthernetInterfaceMap},
	{EntityTag::tagEthernetInterfaceCollection, tagEthernetInterfaceCollectionMap},
	{EntityTag::tagEventDestination, tagEventDestinationMap},
	{EntityTag::tagEventDestinationCollection, tagEventDestinationCollectionMap},
	{EntityTag::tagEventService, tagEventServiceMap},
	{EntityTag::tagExternalAccountProvider, tagExternalAccountProviderMap},
	{EntityTag::tagExternalAccountProviderCollection, tagExternalAccountProviderCollectionMap},
	{EntityTag::tagFabric, tagFabricMap},
	{EntityTag::tagFabricCollection, tagFabricCollectionMap},
	{EntityTag::tagFabricAdapter, tagFabricAdapterMap},
	{EntityTag::tagFabricAdapterCollection, tagFabricAdapterCollectionMap},
	{EntityTag::tagFacility, tagFacilityMap},
	{EntityTag::tagFacilityCollection, tagFacilityCollectionMap},
	{EntityTag::tagFan, tagFanMap},
	{EntityTag::tagFanCollection, tagFanCollectionMap},
	{EntityTag::tagGraphicsController, tagGraphicsControllerMap},
	{EntityTag::tagGraphicsControllerCollection, tagGraphicsControllerCollectionMap},
	{EntityTag::tagHostInterface, tagHostInterfaceMap},
	{EntityTag::tagHostInterfaceCollection, tagHostInterfaceCollectionMap},
	{EntityTag::tagJob, tagJobMap},
	{EntityTag::tagJobCollection, tagJobCollectionMap},
	{EntityTag::tagJobService, tagJobServiceMap},
	{EntityTag::tagJsonSchemaFile, tagJsonSchemaFileMap},
	{EntityTag::tagJsonSchemaFileCollection, tagJsonSchemaFileCollectionMap},
	{EntityTag::tagKey, tagKeyMap},
	{EntityTag::tagKeyCollection, tagKeyCollectionMap},
	{EntityTag::tagKeyPolicy, tagKeyPolicyMap},
	{EntityTag::tagKeyPolicyCollection, tagKeyPolicyCollectionMap},
	{EntityTag::tagKeyService, tagKeyServiceMap},
	{EntityTag::tagLogEntry, tagLogEntryMap},
	{EntityTag::tagLogEntryCollection, tagLogEntryCollectionMap},
	{EntityTag::tagLogService, tagLogServiceMap},
	{EntityTag::tagLogServiceCollection, tagLogServiceCollectionMap},
	{EntityTag::tagManager, tagManagerMap},
	{EntityTag::tagManagerCollection, tagManagerCollectionMap},
	{EntityTag::tagManagerAccount, tagManagerAccountMap},
	{EntityTag::tagManagerAccountCollection, tagManagerAccountCollectionMap},
	{EntityTag::tagManagerDiagnosticData, tagManagerDiagnosticDataMap},
	{EntityTag::tagManagerNetworkProtocol, tagManagerNetworkProtocolMap},
	{EntityTag::tagMediaController, tagMediaControllerMap},
	{EntityTag::tagMediaControllerCollection, tagMediaControllerCollectionMap},
	{EntityTag::tagMemory, tagMemoryMap},
	{EntityTag::tagMemoryCollection, tagMemoryCollectionMap},
	{EntityTag::tagMemoryChunks, tagMemoryChunksMap},
	{EntityTag::tagMemoryChunksCollection, tagMemoryChunksCollectionMap},
	{EntityTag::tagMemoryDomain, tagMemoryDomainMap},
	{EntityTag::tagMemoryDomainCollection, tagMemoryDomainCollectionMap},
	{EntityTag::tagMemoryMetrics, tagMemoryMetricsMap},
	{EntityTag::tagMessageRegistryFile, tagMessageRegistryFileMap},
	{EntityTag::tagMessageRegistryFileCollection, tagMessageRegistryFileCollectionMap},
	{EntityTag::tagMetricDefinition, tagMetricDefinitionMap},
	{EntityTag::tagMetricDefinitionCollection, tagMetricDefinitionCollectionMap},
	{EntityTag::tagMetricReport, tagMetricReportMap},
	{EntityTag::tagMetricReportCollection, tagMetricReportCollectionMap},
	{EntityTag::tagMetricReportDefinition, tagMetricReportDefinitionMap},
	{EntityTag::tagMetricReportDefinitionCollection, tagMetricReportDefinitionCollectionMap},
	{EntityTag::tagNetworkAdapter, tagNetworkAdapterMap},
	{EntityTag::tagNetworkAdapterCollection, tagNetworkAdapterCollectionMap},
	{EntityTag::tagNetworkAdapterMetrics, tagNetworkAdapterMetricsMap},
	{EntityTag::tagNetworkDeviceFunction, tagNetworkDeviceFunctionMap},
	{EntityTag::tagNetworkDeviceFunctionCollection, tagNetworkDeviceFunctionCollectionMap},
	{EntityTag::tagNetworkDeviceFunctionMetrics, tagNetworkDeviceFunctionMetricsMap},
	{EntityTag::tagNetworkInterface, tagNetworkInterfaceMap},
	{EntityTag::tagNetworkInterfaceCollection, tagNetworkInterfaceCollectionMap},
	{EntityTag::tagNetworkPort, tagNetworkPortMap},
	{EntityTag::tagNetworkPortCollection, tagNetworkPortCollectionMap},
	{EntityTag::tagOperatingConfig, tagOperatingConfigMap},
	{EntityTag::tagOperatingConfigCollection, tagOperatingConfigCollectionMap},
	{EntityTag::tagOutlet, tagOutletMap},
	{EntityTag::tagOutletCollection, tagOutletCollectionMap},
	{EntityTag::tagOutletGroup, tagOutletGroupMap},
	{EntityTag::tagOutletGroupCollection, tagOutletGroupCollectionMap},
	{EntityTag::tagPCIeDevice, tagPCIeDeviceMap},
	{EntityTag::tagPCIeDeviceCollection, tagPCIeDeviceCollectionMap},
	{EntityTag::tagPCIeFunction, tagPCIeFunctionMap},
	{EntityTag::tagPCIeFunctionCollection, tagPCIeFunctionCollectionMap},
	{EntityTag::tagPCIeSlots, tagPCIeSlotsMap},
	{EntityTag::tagPort, tagPortMap},
	{EntityTag::tagPortCollection, tagPortCollectionMap},
	{EntityTag::tagPortMetrics, tagPortMetricsMap},
	{EntityTag::tagPower, tagPowerMap},
	{EntityTag::tagPowerDistribution, tagPowerDistributionMap},
	{EntityTag::tagPowerDistributionCollection, tagPowerDistributionCollectionMap},
	{EntityTag::tagPowerDistributionMetrics, tagPowerDistributionMetricsMap},
	{EntityTag::tagPowerDomain, tagPowerDomainMap},
	{EntityTag::tagPowerDomainCollection, tagPowerDomainCollectionMap},
	{EntityTag::tagPowerEquipment, tagPowerEquipmentMap},
	{EntityTag::tagPowerSubsystem, tagPowerSubsystemMap},
	{EntityTag::tagPowerSupply, tagPowerSupplyMap},
	{EntityTag::tagPowerSupplyCollection, tagPowerSupplyCollectionMap},
	{EntityTag::tagPowerSupplyMetrics, tagPowerSupplyMetricsMap},
	{EntityTag::tagProcessor, tagProcessorMap},
	{EntityTag::tagProcessorCollection, tagProcessorCollectionMap},
	{EntityTag::tagProcessorMetrics, tagProcessorMetricsMap},
	{EntityTag::tagResourceBlock, tagResourceBlockMap},
	{EntityTag::tagResourceBlockCollection, tagResourceBlockCollectionMap},
	{EntityTag::tagRole, tagRoleMap},
	{EntityTag::tagRoleCollection, tagRoleCollectionMap},
	{EntityTag::tagRouteEntry, tagRouteEntryMap},
	{EntityTag::tagRouteEntryCollection, tagRouteEntryCollectionMap},
	{EntityTag::tagRouteSetEntry, tagRouteSetEntryMap},
	{EntityTag::tagRouteSetEntryCollection, tagRouteSetEntryCollectionMap},
	{EntityTag::tagSecureBoot, tagSecureBootMap},
	{EntityTag::tagSecureBootDatabase, tagSecureBootDatabaseMap},
	{EntityTag::tagSecureBootDatabaseCollection, tagSecureBootDatabaseCollectionMap},
	{EntityTag::tagSensor, tagSensorMap},
	{EntityTag::tagSensorCollection, tagSensorCollectionMap},
	{EntityTag::tagSerialInterface, tagSerialInterfaceMap},
	{EntityTag::tagSerialInterfaceCollection, tagSerialInterfaceCollectionMap},
	{EntityTag::tagServiceRoot, tagServiceRootMap},
	{EntityTag::tagSession, tagSessionMap},
	{EntityTag::tagSessionCollection, tagSessionCollectionMap},
	{EntityTag::tagSessionService, tagSessionServiceMap},
	{EntityTag::tagSignature, tagSignatureMap},
	{EntityTag::tagSignatureCollection, tagSignatureCollectionMap},
	{EntityTag::tagSimpleStorage, tagSimpleStorageMap},
	{EntityTag::tagSimpleStorageCollection, tagSimpleStorageCollectionMap},
	{EntityTag::tagSoftwareInventory, tagSoftwareInventoryMap},
	{EntityTag::tagSoftwareInventoryCollection, tagSoftwareInventoryCollectionMap},
	{EntityTag::tagStorage, tagStorageMap},
	{EntityTag::tagStorageCollection, tagStorageCollectionMap},
	{EntityTag::tagStorageController, tagStorageControllerMap},
	{EntityTag::tagStorageControllerCollection, tagStorageControllerCollectionMap},
	{EntityTag::tagSwitch, tagSwitchMap},
	{EntityTag::tagSwitchCollection, tagSwitchCollectionMap},
	{EntityTag::tagTask, tagTaskMap},
	{EntityTag::tagTaskCollection, tagTaskCollectionMap},
	{EntityTag::tagTaskService, tagTaskServiceMap},
	{EntityTag::tagTelemetryService, tagTelemetryServiceMap},
	{EntityTag::tagThermal, tagThermalMap},
	{EntityTag::tagThermalMetrics, tagThermalMetricsMap},
	{EntityTag::tagThermalSubsystem, tagThermalSubsystemMap},
	{EntityTag::tagTriggers, tagTriggersMap},
	{EntityTag::tagTriggersCollection, tagTriggersCollectionMap},
	{EntityTag::tagUpdateService, tagUpdateServiceMap},
	{EntityTag::tagUSBController, tagUSBControllerMap},
	{EntityTag::tagUSBControllerCollection, tagUSBControllerCollectionMap},
	{EntityTag::tagVCATEntry, tagVCATEntryMap},
	{EntityTag::tagVCATEntryCollection, tagVCATEntryCollectionMap},
	{EntityTag::tagVLanNetworkInterface, tagVLanNetworkInterfaceMap},
	{EntityTag::tagVLanNetworkInterfaceCollection, tagVLanNetworkInterfaceCollectionMap},
	{EntityTag::tagVirtualMedia, tagVirtualMediaMap},
	{EntityTag::tagVirtualMediaCollection, tagVirtualMediaCollectionMap},
	{EntityTag::tagVolume, tagVolumeMap},
	{EntityTag::tagVolumeCollection, tagVolumeCollectionMap},
	{EntityTag::tagZone, tagZoneMap},
	{EntityTag::tagZoneCollection, tagZoneCollectionMap},
};

inline const std::vector<Privileges>
    getPrivilegesFromUrlAndMethod(std::string_view url,
                                  HttpVerb method)
{
    return privilegeSetMap[entityTagMap[url]][method];
}
} // namespace redfish::privileges
// clang-format on
