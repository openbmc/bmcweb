# Redfish #

bmcweb provides an implementation of the [Redfish][1] API.  This document
details the Redfish schemas supported by bmcweb. This document also discusses
some of the details of that implementation and different implementations
available for certain areas.

## Redfish Schema

The redfish implementation shall pass the [Redfish Service
Validator](https://github.com/DMTF/Redfish-Service-Validator "Validator") with
no warnings or errors

The following redfish schemas and fields are targeted for OpenBMC.  This is a
living document, and these schemas are subject to change.

The latest Redfish schemas can be found [here](https://redfish.dmtf.org/schemas/)

If using a previously unused schema, you will need to add it to the included
schema list in scripts/update_schemas.py and run update_schemas.py.

Fields common to all schemas

- @odata.context
- @odata.id
- @odata.type
- Id
- Name


#### /redfish/v1/
##### ServiceRoot

- AccountService
- CertificateService
- Chassis
- JsonSchemas
- Managers
- RedfishVersion
- SessionService
- Systems
- UUID
- UpdateService

#### /redfish/v1/AccountService/
##### AccountService

- Description
- ServiceEnabled
- MinpasswordLength
- MaxPasswordLength
- Accounts
- Roles

#### /redfish/v1/AccountService/Accounts/
##### AccountCollection

- Description
- Members@odata.count
- Members

#### /redfish/v1/AccountService/Accounts/{ManagerAccountId}
##### ManagerAccount

- Description
- Enabled
- Password
- UserName
- RoleId
- Links/Role

#### /redfish/v1/AccountService/Roles/
##### RoleCollection

- Description
- Members@odata.count
- Members
  - By default will contain 3 roles, "Administrator", "Operator", and "ReadOnly"

#### /redfish/v1/AccountService/Roles/{RoleId}
##### Role

- Description
- IsPredefined
  - Will be set to true for all default roles.  If the given role is
    non-default, or has been modified from default, will be marked as false.
- AssignedPrivileges
  - For the default roles, the following privileges will be assigned by
    default
      - Administrator: Login, ConfigureManager, ConfigureUsers, ConfigureSelf,
        ConfigureComponents
      - Operator: Login, ConfigureComponents, ConfigureSelf
      - ReadOnly: Login, ConfigureSelf


#### /redfish/v1/Chassis
##### ChassisCollection

- Members@odata.count
- Members

#### /redfish/v1/Chassis/{ChassisId}
##### Chassis

- ChassisType
- Manufacturer
- Model
- SerialNumber
- PartNumber
- PowerState
- Thermal
  - Shall be included if component contains temperature sensors, otherwise
    shall be omitted.
- Power
  - Shall be included if component contains voltage/current sensing
    components, otherwise will be omitted.

#### /redfish/v1/Chassis/{ChassisId}/Thermal
##### Thermal
Temperatures Fans Redundancy

#### /redfish/v1/Chassis/{ChassisId}/Thermal#/Temperatures/{SensorName}
##### Temperature
- MemberId
- Status
- ReadingCelsius
- UpperThresholdNonCritical
- UpperThresholdCritical
- LowerThresholdNonCritical
- LowerThresholdCritical
- MinReadingRange
- MaxReadingRange

*threshold fields only present if defined for sensor, otherwise absent*

#### /redfish/v1/Chassis/{ChassisId}/Thermal#/Fans/{FanName}
##### Fan
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
- Redundancy

*threshold fields only present if defined for sensor, otherwise absent*

#### /redfish/v1/Chassis/{ChassisId}/Thermal#/Redundancy/{RedundancyName}
##### Redundancy
- MemberId
- RedundancySet
- Mode
- Status
- MinNumNeeded
- MaxNumSupported


#### /redfish/v1/Chassis/{ChassisId}/Power/
##### Power
PowerControl Voltages PowerSupplies Redundancy

#### /redfish/v1/Chassis/{ChassisId}/Power#/PowerControl/{ControlName}
##### PowerControl
- MemberId
- PowerConsumedWatts
- PowerMetrics/IntervalInMin
- PowerMetrics/MinConsumedWatts
- PowerMetrics/MaxConsumedWatts
- PowerMetrics/AverageConsumedWatts
- RelatedItem
  - Should list systems and related chassis

#### /redfish/v1/Chassis/{ChassisId}/Power#/Voltages/{VoltageName}
##### Voltage
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

#### /redfish/v1/Chassis/{ChassisId}/Power#/PowerSupplies/{PSUName}
##### PowerSupply
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

#### /redfish/v1/Chassis/{ChassisId}/Power#/Redundancy/{RedundancyName}
##### Redundancy
- MemberId
- RedundancySet
- Mode
- Status
- MinNumNeeded
- MaxNumSupported


#### /redfish/v1/EventService
##### EventService
- Id
- ServiceEnabled
- DeliveryRetryAttempts
  - Defaults to 3
- EventTypesForSubscription
  - Defaults to "Alert"
- Actions
- Subscriptions

#### /redfish/v1/EventService/Subscriptions
##### EventDestinationCollection
- Members@odata.count
- Members

#### /redfish/v1/EventService/Subscriptions/{EventName}
##### EventDestination
- Id
- Destination
- EventTypes
- Context
- OriginResources
- Protocol


#### /redfish/v1/Managers
##### ManagerCollection
- Members
- Members@odata.count

#### /redfish/v1/Managers/bmc
##### Manager
- Description
- LogServices
- GraphicalConsole
- UUID
- Model
- Links
- PowerState
- FirmwareVersion
- ManagerType
- ServiceEntryPointUUID
- DateTime
- NetworkProtocol
- Actions
- Status
- SerialConsole
- VirtualMedia
- EthernetInterfaces

#### /redfish/v1/Managers/bmc/EthernetInterfaces
##### EthernetInterfaceCollection
- Members
- Members@odata.count
- Description

#### /redfish/v1/Managers/bmc/EthernetInterfaces/{EthernetInterfaceId}
##### EthernetInterface
- Description
- VLAN
- MaxIPv6StaticAddresses

#### /redfish/v1/Managers/bmc/LogServices

The [LogService][2] resource provides properties for monitoring and configuring
events for the service or resource to which it is associated.

Within bmcweb, the LogService object resides under the System resource. It
tracks all events for the system.

The LogService supports multiple log entry types. bmcweb has support for
the `Event` type. This is the new Redfish-defined type.

bmcweb supports two different implementations of the
`LogService/EventLog/Entries` URI.

The default implementation uses rsyslog to write Redfish events from the journal
to the persistent /var/log/ filesystem. The bmcweb software then looks for these
files in /var/log/ and returns the appropriate Redfish EventLog Entries for
these. More details on adding events can be found [here][3]

The other implementation of EventLog Entries can be enabled by compiling bmcweb
with the `-DBMCWEB_ENABLE_REDFISH_DBUS_LOG_ENTRIES=ON` option. This will cause
bmcweb to look to [phosphor-logging][4] for any D-Bus log entries. These will
then be translated to Redfish EventLog Entries.

These two implementations do not work together, so choosing one will disable
the other.

#### /redfish/v1/Managers/bmc/LogServices
##### LogServiceCollection
- Members
- Members@odata.count
- Description

#### /redfish/v1/Managers/bmc/LogServices/RedfishLog
##### LogService
- Entries
- OverWritePolicy
- Actions
- Status
- DateTime
- MaxNumberOfRecords

#### /redfish/v1/Managers/bmc/LogServices/RedfishLog/Entries/{LogEntryId}
##### LogEntry
- Message
- Created
- EntryType

#### /redfish/v1/Managers/bmc/NetworkProtocol
##### ManagerNetworkProtocol
- Description
- SSDP
- HTTPS
- SSH
- VirtualMedia
- KVMIP
- Status


#### /redfish/v1/Registries
##### MessageRegistryFileCollection
- Members
  - Should support Base, CommonMessages, and EventingMessages
- Members@odata.count
- Description

#### /redfish/v1/Registries/{MessageRegistryFileId}
##### MessageRegistryFile
- Location
- Description
- Location@odata.count
- Languages@odata.count
- Languages
- Registry


#### /redfish/v1/SessionService
##### SessionService
- Description
- ServiceEnabled
- Status
- SessionTimeout
- Sessions

#### /redfish/v1/SessionService/Sessions
##### SessionCollection
- Members
- Members@odata.count
- Description


#### /redfish/v1/Systems
##### ComputerSystemCollection
- Members
  - Should support one system
- Members@odata.count

#### /redfish/v1/Systems/system
##### ComputerSystem
- Boot
- PartNumber
- IndicatorLED
- UUID
- LogServices
- SystemType
- Manufacturer
- Description
- Model
- Links
- PowerState
- BiosVersion
- Storage
- SerialNumber
- Processors
- ProcessorSummary
- Memory
- Actions
- Status
- EthernetInterfaces
- MemorySummary

#### /redfish/v1/Systems/system/EthernetInterfaces
##### EthernetInterfaceCollection
- Members
- Members@odata.count
- Description

#### /redfish/v1/Systems/system/LogServices
##### LogServiceCollection
- Members
  - Should default to one member, named SEL
- Members@odata.count
- Description

#### /redfish/v1/Systems/system/LogServices/SEL/Entries
##### LogEntryCollection
- Members
- Members@odata.count
- Description
- @odata.nextLink

#### /redfish/v1/Systems/system/LogServices/SEL/Entries/{LogEntryId}
##### LogEntry
- MessageArgs
- Severity
- SensorType
- Message
- MessageId
- Created
- EntryCode
- EntryType

#### /redfish/v1/Systems/system/Memory
##### MemoryCollection
- Members
- Members@odata.count

#### /redfish/v1/Systems/system/Memory/{MemoryId}
##### Memory
- MemoryType
- Description
- DeviceLocator
- Oem
- Metrics
- BaseModuleType
- Manufacturer
- MemoryDeviceType
- RankCount
- AllowedSpeedsMHz
- CapacityMiB
- DataWidthBits
- SerialNumber
- OperatingSpeedMhz
- ErrorCorrection
- PartNumber
- Status
- BusWidthBits
- MemoryMedia

#### /redfish/v1/Systems/system/Memory/{MemoryId}/MemoryMetrics
##### MemoryMetrics
- Description
- HealthData

#### /redfish/v1/Systems/system/Processors
##### ProcessorCollection
- Members
  - Should Support CPU1 and CPU2 for dual socket systems
- Members@odata.count

#### /redfish/v1/Systems/system/Processors/{ProcessorId}
##### Processor
- ProcessorArchitecture
- TotalCores
- ProcessorId
- MaxSpeedMHz
- Manufacturer
- Status
- Socket
- InstructionSet
- Model
- ProcessorType
- TotalThreads

#### /redfish/v1/Systems/system/Storage
##### StorageCollection
- Members
- Members@odata.count

#### /redfish/v1/Systems/system/Storage/{StorageId}
##### Storage
- Drives
- Links


#### /redfish/v1/UpdateService
##### UpdateService
- SoftwareInventory

#### /redfish/v1/UpdateService/FirmwareInventory
##### SoftwareInventoryCollection
- Members
- Should Support BMC, ME, CPLD and BIOS
- Members@odata.count

#### /redfish/v1/UpdateService/FirmwareInventory/{SoftwareInventoryId}
##### SoftwareInventory
- Version
- Updateable

[1]: https://www.dmtf.org/standards/redfish
[2]: https://redfish.dmtf.org/schemas/v1/LogService.json
[3]: https://github.com/openbmc/docs/blob/master/architecture/redfish-logging-in-bmcweb.md
[4]: https://github.com/openbmc/phosphor-logging
