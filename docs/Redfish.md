# Redfish

bmcweb provides an implementation of the [Redfish][1] API. This document details
the Redfish schemas supported by bmcweb. This document also discusses some of
the details of that implementation and different implementations available for
certain areas.

## Redfish Schema

The redfish implementation shall pass the [Redfish Service Validator][2] with no
warnings or errors

The following redfish schemas and fields are targeted for OpenBMC. This is a
living document, and these schemas are subject to change.

The latest [Redfish schemas][3] are available from DMTF

If using a previously unused schema, you will need to add it to the included
schema list in `scripts/update_schemas.py` and run `update_schemas.py`.

Fields common to all schemas

- @odata.id
- @odata.type
- Id
- Name

### /redfish/v1/

#### ServiceRoot

- AccountService
- AggregationService
- Cables
- CertificateService
- Chassis
- EventService
- JsonSchemas
- Links/ManagerProvidingService
- Links/Sessions
- Managers
- RedfishVersion
- Registries
- ServiceIdentification
- SessionService
- Systems
- Tasks
- TelemetryService
- UUID
- UpdateService

### /redfish/v1/AccountService/

#### AccountService

- AccountLockoutDuration
- AccountLockoutThreshold
- Accounts
- Description
- HTTPBasicAuth
- LDAP
- MaxPasswordLength
- MinPasswordLength
- MultiFactorAuth/ClientCertificate/Certificates
- MultiFactorAuth/ClientCertificate/CertificateMappingAttribute
- MultiFactorAuth/ClientCertificate/Enabled
- MultiFactorAuth/ClientCertificate/RespondToUnauthenticatedClients
- Oem/OpenBMC/AuthMethods/BasicAuth
- Oem/OpenBMC/AuthMethods/Cookie
- Oem/OpenBMC/AuthMethods/SessionToken
- Oem/OpenBMC/AuthMethods/TLS
- Oem/OpenBMC/AuthMethods/XToken
- Roles
- ServiceEnabled

### /redfish/v1/AccountService/MultiFactorAuth/ClientCertificate/Certificates

- Members
- `Members@odata.count`

### /redfish/v1/AccountService/MultiFactorAuth/ClientCertificate/Certificates/{Certificate}

- CertificateString
- Id
- Issuer/City
- Issuer/CommonName
- Issuer/Country
- Issuer/Organization
- Issuer/OrganizationalUnit
- Issuer/State
- KeyUsage
- Subject/City
- Subject/Country
- Subject/CommonName
- Subject/Organization
- Subject/OrganizationalUnit
- Subject/State
- ValidNotAfter
- ValidNotBefore

### /redfish/v1/AggregationService/

#### AggregationService

- AggregationSources
- Description
- ServiceEnabled

### /redfish/v1/AggregationService/AggregationSources

#### AggregationSourceCollection

- Members
- `Members@odata.count`

### /redfish/v1/AggregationService/AggregationSources/{AggregationSourceId}

#### AggregationSource

- HostName
- Password

### /redfish/v1/AccountService/Accounts/

#### ManagerAccountCollection

- Description
- Members
- `Members@odata.count`

### /redfish/v1/AccountService/Accounts/{ManagerAccountId}/

#### ManagerAccount

- AccountTypes
- Description
- Enabled
- Links/Role
- Locked
- `Locked@Redfish.AllowableValues`
- Password
- PasswordChangeRequired
- RoleId
- StrictAccountTypes
- UserName

### /redfish/v1/AccountService/LDAP/Certificates/

#### CertificateCollection

- Description
- Members
- `Members@odata.count`

### /redfish/v1/AccountService/Roles/

#### RoleCollection

- Description
- Members
  - By default will contain 3 roles, "Administrator", "Operator", and "ReadOnly"
- `Members@odata.count`

### /redfish/v1/AccountService/Roles/{RoleId}/

#### Role

- AssignedPrivileges
  - For the default roles, the following privileges will be assigned by default
    - Administrator: Login, ConfigureManager, ConfigureUsers, ConfigureSelf,
      ConfigureComponents
    - Operator: Login, ConfigureComponents, ConfigureSelf
    - ReadOnly: Login, ConfigureSelf
- Description
- IsPredefined
  - Will be set to true for all default roles. If the given role is non-default,
    or has been modified from default, will be marked as false.
- OemPrivileges
- RoleId

### /redfish/v1/Cables/

#### CableCollection

- Description
- Members
- `Members@odata.count`

### /redfish/v1/Cables/{CableId}/

#### Cable

- CableType
- LengthMeters
- Status

### /redfish/v1/CertificateService/

#### CertificateService

- Actions
- CertificateLocations
- Description

### /redfish/v1/CertificateService/CertificateLocations/

#### CertificateLocations

- Description
- Links/Certificates
- Links/Certificates@odata.count

### /redfish/v1/Chassis/

#### ChassisCollection

- Members
- `Members@odata.count`

### /redfish/v1/Chassis/{ChassisId}/

#### Chassis

- Actions
- AssetTag
- ChassisType
- Drives
- HotPluggable
- Links/ComputerSystems
- Links/ManagedBy
- Location/PartLocation/ServiceLabel
- LocationIndicatorActive
- Manufacturer
- Model
- PartNumber
- Power
- PowerSubsystem
- PowerState
- PhysicalSecurity
- Sensors
- SerialNumber
- SparePartNumber
- Status
- Thermal
- ThermalSubsystem
- UUID
- Version

### /redfish/v1/Chassis/{ChassisId}/Drive/

#### Drive

- Members (This is dependent on a entity manager association from Chassis to
  Drives, The name of the association is `chassis<->drive`)

### /redfish/v1/Chassis/{ChassisId}/Drive/{DriveId}/

#### Drive

- Drives
- `Drives@odata.count`
- Status (this is dependent on a entity manager association from Chassis to
  Drives)

### /redfish/v1/Chassis/{ChassisId}/EnvironmentMetrics/

#### EnvironmentMetrics

### /redfish/v1/Chassis/{ChassisId}/Power/

#### Power

- PowerControl
- PowerSupplies
- Redundancy
- Voltages

### /redfish/v1/Chassis/{ChassisId}/Sensors/

#### SensorCollection

- Description
- Members
- `Members@odata.count`

### /redfish/v1/Chassis/{ChassisId}/Sensors/{Id}/

#### Sensor

- Reading
- ReadingRangeMax
- ReadingRangeMin
- ReadingType
- ReadingUnits
- SpeedRPM
- Status
- Thresholds

### /redfish/v1/Chassis/{ChassisId}/Thermal/

#### Thermal

- Fans
- Redundancy
- Temperatures

### /redfish/v1/Chassis/{ChassisId}/Thermal#/Temperatures/{SensorName}/

#### Temperature

- MemberId
- Status
- ReadingCelsius
- UpperThresholdNonCritical
- UpperThresholdCritical
- LowerThresholdNonCritical
- LowerThresholdCritical
- MinReadingRange
- MaxReadingRange _threshold fields only present if defined for sensor,
  otherwise absent_

### /redfish/v1/Chassis/{ChassisId}/Thermal#/Fans/{FanName}/

#### Fan

- MemberId
- Status
- Reading
- ReadingUnits
- UpperThresholdNonCritical
- UpperThresholdCritical
- LowerThresholdNonCritical
- LowerThresholdCritical
- MinReadingRange
- MaxReadingRange
- Redundancy _threshold fields only present if defined for sensor, otherwise
  absent_

### /redfish/v1/Chassis/{ChassisId}/Thermal#/Redundancy/{RedundancyName}/

#### Redundancy

- MemberId
- RedundancySet
- Mode
- Status
- MinNumNeeded
- MaxNumSupported

### /redfish/v1/Chassis/{ChassisId}/ThermalSubsystem

#### ThermalSubsystem

- Status
- ThermalMetrics

#### /redfish/v1/Chassis/{ChassisId}/ThermalSubsystem/ThermalMetrics/

##### ThermalMetrics

- TemperatureReadingsCelsius[]/DataSourceUri
- TemperatureReadingsCelsius[]/Reading
- `TemperatureReadingsCelsius@odata.count`

#### /redfish/v1/Chassis/{ChassisId}/ThermalSubsystem/Fans

##### FansCollection

- Description
- Members
- `Members@odata.count`

#### /redfish/v1/Chassis/{ChassisId}/ThermalSubsystem/Fans/{FanName}/

#### Fan

- Location
- LocationIndicatorActive
- Manufacturer
- Model
- PartNumber
- SerialNumber
- SparePartNumber
- Status

### /redfish/v1/Chassis/{ChassisId}/Power#/PowerControl/{ControlName}/

#### PowerControl

- MemberId
- PowerConsumedWatts
- PowerMetrics/IntervalInMin
- PowerMetrics/MinConsumedWatts
- PowerMetrics/MaxConsumedWatts
- PowerMetrics/AverageConsumedWatts
- RelatedItem
  - Should list systems and related chassis

### /redfish/v1/Chassis/{ChassisId}/Power#/Voltages/{VoltageName}/

#### Voltage

- MemberId
- Status
- ReadingVolts
- UpperThresholdNonCritical
- UpperThresholdCritical
- LowerThresholdNonCritical
- LowerThresholdCritical
- MinReadingRange
- MaxReadingRange
- PhysicalContext
- RelatedItem

### /redfish/v1/Chassis/{ChassisId}/Power#/PowerSupplies/{PSUName}/

#### PowerSupply

- MemberId
- Status
- LininputVoltage
- Model
- manufacturer
- FirmwareVersion
- SerialNumber
- PartNumber
- RelatedItem
- Redundancy

### /redfish/v1/Chassis/{ChassisId}/Power#/Redundancy/{RedundancyName}/

#### Redundancy

- MemberId
- RedundancySet
- Mode
- Status
- MinNumNeeded
- MaxNumSupported

#### /redfish/v1/Chassis/{ChassisId}/PowerSubsystem/PowerSupplies

##### PowerSupplies

- Description
- Members
- `Members@odata.count`

#### /redfish/v1/Chassis/{ChassisId}/PowerSubsystem/PowerSupplies/{PowerSupplyId}

##### PowerSupply

- EfficiencyRatings
  - EfficiencyPercent
- FirmwareVersion
- Location
- LocationIndicatorActive
- Manufacturer
- Model
- PartNumber
- SerialNumber
- SparePartNumber
- Status

### /redfish/v1/EventService/

#### EventService

- Actions
- SubmitTestEvent
  - eventGroupId
  - eventId
  - eventTimestamp
  - message
  - messageArgs
  - messageId
  - originOfCondition
  - resolution
  - severity
- DeliveryRetryAttempts
  - Defaults to 3
- DeliveryRetryIntervalSeconds
- EventFormatTypes
- RegistryPrefixes
- ResourceTypes
- SSEFilterPropertiesSupported
- ServiceEnabled
- Status
- Subscriptions

### /redfish/v1/EventService/Subscriptions/

#### EventDestinationCollection

- Members
- `Members@odata.count`

### /redfish/v1/EventService/Subscriptions/{EventName}/

#### EventDestination

- Id
- Destination
- EventTypes
- Context
- HeartbeatIntervalMinutes
- OriginResources
- RegistryPrefixes
- Protocol
- SendHeartbeat

### /redfish/v1/JsonSchemas/

#### JsonSchemaFileCollection

- Description
- `Members@odata.count`
- Members

### /redfish/v1/JsonSchemas/{Id}/

#### JsonSchemaFile

- Schema
- Description
- Languages
- `Languages@odata.count`
- Location
- `Location@odata.count`

### /redfish/v1/Managers/

#### ManagerCollection

- Members
- `Members@odata.count`

### /redfish/v1/Managers/bmc/

#### Manager

- Actions
- DateTime
- DateTimeLocalOffset
- Description
- EthernetInterfaces
- FirmwareVersion
- GraphicalConsole
- LastResetTime
- Links/ActiveSoftwareImage
- Links/ManagerForChassis
- Links/ManagerForChassis@odata.count
- Links/ManagerForServers
- Links/ManagerForServers@odata.count
- Links/ManagerInChassis
- Links/SoftwareImages
- Links/SoftwareImages@odata.count
- LocationIndicatorActive
- LogServices
- ManagerType
- Manufacturer
- Model
- NetworkProtocol
- Oem
- PartNumber
- PowerState
- SerialNumber
- ServiceEntryPointUUID
- ServiceIdentification
- SparePartNumber
- Status
- UUID

### /redfish/v1/Managers/bmc/EthernetInterfaces/

#### EthernetInterfaceCollection

- Description
- Members
- `Members@odata.count`

### /redfish/v1/Managers/bmc/EthernetInterfaces/{EthernetInterfaceId}/

#### EthernetInterface

- DHCPv4
- DHCPv6
- Description
- EthernetInterfaceType
- FQDN
- HostName
- IPv4Addresses
- IPv4StaticAddresses
- IPv6AddressPolicyTable
- IPv6Addresses
- IPv6DefaultGateway
- IPv6StaticAddresses
- IPv6StaticDefaultGateways
- InterfaceEnabled
- Links/RelatedInterfaces
- LinkStatus
- MACAddress
- NameServers
- SpeedMbps
- StatelessAddressAutoConfig
- StaticNameServers
- Status
- VLAN/VLANEnable
- VLAN/VLANId
- VLAN/Tagged

### /redfish/v1/Managers/bmc/LogServices/

The [LogService][4] resource provides properties for monitoring and configuring
events for the service or resource to which it is associated.

Within bmcweb, the LogService object resides under the System resource. It
tracks all events for the system.

The LogService supports multiple log entry types. bmcweb has support for the
`Event` type. This is the new Redfish-defined type.

bmcweb supports two different implementations of the
`LogService/EventLog/Entries` URI.

The default implementation uses rsyslog to write Redfish events from the journal
to the persistent /var/log/ filesystem. The bmcweb software then looks for these
files in /var/log/ and returns the appropriate Redfish EventLog Entries for
these. [More details][5] on adding events are available.

The other implementation of EventLog Entries can be enabled by compiling bmcweb
with the `-DBMCWEB_ENABLE_REDFISH_DBUS_LOG_ENTRIES=ON` option. This will cause
bmcweb to look to [phosphor-logging][6] for any D-Bus log entries. These will
then be translated to Redfish EventLog Entries.

These two implementations do not work together, so choosing one will disable the
other.

#### LogServiceCollection

- Description
- Members
- `Members@odata.count`

### /redfish/v1/Managers/bmc/LogServices/RedfishLog/

#### LogService

- Entries
- OverWritePolicy
- Actions
- Status
- DateTime
- MaxNumberOfRecords

### /redfish/v1/Managers/bmc/LogServices/RedfishLog/Entries/{LogEntryId}/

#### LogEntry

- Message
- Created
- EntryType

### /redfish/v1/Managers/bmc/ManagerDiagnosticData/

#### ManagerDiagnosticData

- ServiceRootUptimeSeconds
- FreeStorageSpaceKiB
- MemoryStatistics/AvailableBytes
- MemoryStatistics/BuffersAndCacheBytes
- MemoryStatistics/FreeBytes
- MemoryStatistics/SharedBytes
- MemoryStatistics/TotalBytes
- ProcessorStatistics/KernelPercent
- ProcessorStatistics/UserPercent

### /redfish/v1/Managers/bmc/NetworkProtocol/

#### ManagerNetworkProtocol

- Description
- FQDN
- HTTP
- HTTPS
- HostName
- IPMI
- NTP
- SSH
- Status

### /redfish/v1/Managers/bmc/NetworkProtocol/HTTPS/Certificates/

#### CertificateCollection

- Description
- Members
- `Members@odata.count`

### /redfish/v1/Managers/bmc/NetworkProtocol/HTTPS/Certificates/{CertificateId}/

#### Certificate

- CertificateString
- Description
- Issuer
- KeyUsage
- Subject
- ValidNotAfter
- ValidNotBefore

### /redfish/v1/Managers/bmc/Truststore/Certificates/

#### CertificateCollection

- Description
- error

### /redfish/v1/Registries/

#### MessageRegistryFileCollection

- Description
- Members
  - Should support Base, CommonMessages, and EventingMessages
- `Members@odata.count`

### /redfish/v1/Registries/{MessageRegistryFileId}/

#### MessageRegistryFile

- Description
- Languages
- `Languages@odata.count`
- Location
- `Location@odata.count`
- Registry

### /redfish/v1/SessionService/

#### SessionService

- Description
- ServiceEnabled
- SessionTimeout
- Sessions

### /redfish/v1/SessionService/Sessions/

#### SessionCollection

- Description
- Members
- `Members@odata.count`

### /redfish/v1/SessionService/Sessions/{SessionId}/

#### Session

- ClientOriginIPAddress
- Description
- Roles
- UserName

### /redfish/v1/Systems/

#### ComputerSystemCollection

- Members
  - Should support one system
- `Members@odata.count`

### /redfish/v1/Systems/system/Bios/

#### Bios

- Actions
- Description
- Links/ActiveSoftwareImage
- Links/SoftwareImages
- Links/SoftwareImages@odata.count

### /redfish/v1/Systems/system/

#### ComputerSystem

- Actions
- AssetTag
- Bios
- BiosVersion
- Boot
- BootProgress
- Description
- FabricAdapters
- HostWatchdogTimer
- IdlePowerSaver/Enable
- IdlePowerSaver/EnterUtilizationPercent
- IdlePowerSaver/EnterDwellTimeSeconds
- IdlePowerSaver/ExitUtilizationPercent
- IdlePowerSaver/ExitDwellTimeSeconds
- IndicatorLED
- LastResetTime
- Links/Chassis
- Links/ManagedBy
- LocationIndicatorActive
- LogServices
- Manufacturer
- Memory
- MemorySummary
- Model
- PCIeDevices
- PartNumber
- PowerMode
- PowerRestorePolicy
- PowerState
- ProcessorSummary
- Processors
- SerialConsole/IPMI/ServiceEnabled
- SerialConsole/MaxConcurrentSessions
- SerialConsole/SSH/HotKeySequenceDisplay
- SerialConsole/SSH/Port
- SerialConsole/SSH/ServiceEnabled
- SerialNumber
- Status
- Storage
- SubModel
- SystemType

### /redfish/v1/Systems/system/EthernetInterfaces/

#### EthernetInterfaceCollection

- Members
- `Members@odata.count`
- Description

### /redfish/v1/Systems/system/FabricAdapters/

#### FabricAdapterCollection

- Members
- `Members@odata.count`

### /redfish/v1/Systems/system/FabricAdapters/{FabricAdapterId}/

#### FabricAdapter

- Location
- LocationIndicatorActive
- Model
- PartNumber
- SerialNumber
- SparePartNumber
- Status

### /redfish/v1/Systems/system/LogServices/

#### LogServiceCollection

- Description
- Members
  - Should default to one member, named SEL
- `Members@odata.count`

### /redfish/v1/Systems/system/LogServices/EventLog/

#### LogService

- Actions
- DateTime
- DateTimeLocalOffset
- Description
- Entries
- OverWritePolicy

### /redfish/v1/Systems/system/LogServices/EventLog/Entries/

#### LogEntryCollection

- Description
- Members
- `Members@odata.count`

### /redfish/v1/Systems/system/LogServices/EventLog/Entries/{LogEntryId}/

#### LogEntry

- AdditionalDataURI
- Created
- EntryType
- Message
- Modified
- Resolved
- Severity

### /redfish/v1/Systems/system/LogServices/SEL/Entries/

#### LogEntryCollection

- Members
- `Members@odata.count`
- Description
- @odata.nextLink

### /redfish/v1/Systems/system/LogServices/SEL/Entries/{LogEntryId}/

#### LogEntry

- MessageArgs
- Severity
- SensorType
- Message
- MessageId
- Created
- EntryCode
- EntryType

### /redfish/v1/Systems/system/Memory/

#### MemoryCollection

- Members
- `Members@odata.count`

### /redfish/v1/Systems/system/Memory/{MemoryId}/

#### Memory

- AllowedSpeedsMHz
- BaseModuleType
- BusWidthBits
- CapacityMiB
- DataWidthBits
- ErrorCorrection
- FirmwareRevision
- LocationIndicatorActive
- Manufacturer
- Model
- OperatingSpeedMhz
- PartNumber
- RankCount
- SerialNumber
- SparePartNumber
- Status

### /redfish/v1/Systems/system/Memory/{MemoryId}/MemoryMetrics/

#### MemoryMetrics

- Description
- HealthData

### /redfish/v1/Systems/system/PCIeDevices/

#### PCIeDeviceCollection

- Description
- Members
- `Members@odata.count`

### /redfish/v1/Systems/system/PCIeDevices/{PCIeDevice}/

- Manufacturer
- Model
- PartNumber
- PCIeInterface
  - LanesInUse
  - MaxLanes
  - MaxPCIeType
  - PCIeType
- SerialNumber
- Slot
  - Lanes
  - PCIeType
  - SlotType
- SparePartNumber
- Status

### /redfish/v1/Systems/system/Processors/

#### ProcessorCollection

- Members
  - Should Support CPU1 and CPU2 for dual socket systems
- `Members@odata.count`

### /redfish/v1/Systems/system/Processors/{ProcessorId}/

#### Processor

- InstructionSet
- LocationIndicatorActive
- Manufacturer
- MaxSpeedMHz
- PartNumber
- ProcessorArchitecture
- ProcessorId
- ProcessorType
- SerialNumber
- Socket
- SparePartNumber
- Status
- ThrottleCauses
- Throttled
- TotalCores
- TotalThreads
- Version

### /redfish/v1/Systems/system/ResetActionInfo/

#### ActionInfo

- Parameters/AllowableValues
- Parameters/DataType
- Parameters/Required

### /redfish/v1/Systems/system/Storage/

#### StorageCollection

- Members
- `Members@odata.count`

### /redfish/v1/Systems/system/Storage/{StorageId}/

#### Storage

- Drives
- `Drives@odata.count`
- Status

### /redfish/v1/Systems/system/Storage/{StorageId}/Drive/{DriveId}/

#### Storage

- CapacityBytes
- EncryptionStatus
- Links
- Status

### /redfish/v1/TaskService/

#### TaskService

- CompletedTaskOverWritePolicy
- DateTime
- LifeCycleEventOnTaskStateChange
- ServiceEnabled
- Status
- Tasks

### /redfish/v1/TaskService/Tasks/

#### TaskCollection

- Members
- `Members@odata.count`

### /redfish/v1/TelemetryService/

#### TelemetryService

- MaxReports
- MetricReportDefinitions
- MetricReports
- MinCollectionInterval
- Status
- Triggers
- SupportedCollectionFunctions

### /redfish/v1/TelemetryService/MetricReportDefinitions/

#### MetricReportDefinitionCollection

- Members
- `Members@odata.count`

### /redfish/v1/TelemetryService/MetricReportDefinitions/{MetricReportDefinitionId}/

#### MetricReportDefinition

- AppendLimit
- Id
- MetricReport
- MetricReportDefinitionEnabled
- MetricReportDefinitionType
- Metrics
- Name
- ReportActions
- ReportUpdates
- Schedule
- Status

### /redfish/v1/TelemetryService/MetricReports/

#### MetricReportCollection

- Members
- `Members@odata.count`

### /redfish/v1/TelemetryService/MetricReports/{MetricReportId}/

#### MetricReport

- Id
- MetricReportDefinition
- MetricValues
- Name
- Timestamp

### /redfish/v1/TelemetryService/Triggers/

#### TriggersCollection

- Members
- `Members@odata.count`

### /redfish/v1/UpdateService/

#### UpdateService

- Actions
- Description
- FirmwareInventory
- HttpPushUri
- HttpPushUriOptions
- MaxImageSizeBytes
- MultipartHttpPushUri
- ServiceEnabled

### /redfish/v1/UpdateService/FirmwareInventory/

#### SoftwareInventoryCollection

- Members
  - Should Support BMC, ME, CPLD and BIOS
- `Members@odata.count`

### /redfish/v1/UpdateService/FirmwareInventory/{SoftwareInventoryId}/

#### SoftwareInventory

- Description
- LowestSupportedVersion
- `RelatedItem@odata.count`
- RelatedItem
- Status
- Updateable
- Version

[1]: https://www.dmtf.org/standards/redfish
[2]: https://github.com/DMTF/Redfish-Service-Validator
[3]: https://redfish.dmtf.org/schemas/
[4]: https://redfish.dmtf.org/schemas/v1/LogService.json
[5]:
  https://github.com/openbmc/docs/blob/master/architecture/redfish-logging-in-bmcweb.md
[6]: https://github.com/openbmc/phosphor-logging
