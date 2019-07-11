# Redfish #

## Redfish Schema

The redfish implementation shall pass the [Redfish Service
Validator](https://github.com/DMTF/Redfish-Service-Validator "Validator") with
no warnings or errors

The following redfish schemas and fields are targeted for OpenBMC.  This is a
living document, and these schemas are subject to change.

The latest Redfish schemas can be found [here](https://redfish.dmtf.org/schemas/)

Fields common to all schemas

- @odata.context
- @odata.id
- @odata.type
- Id
- Name


#### /redfish/v1/
##### ServiceRoot

- RedfishVersion
- UUID


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

#### /redfish/v1/AccountService/Accounts/<AccountName>
##### Account

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
  - By default will contain 3 roles, "Administrator", "Operator", and "User"

#### /redfish/v1/AccountService/Roles/<RoleName>
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
      - Operator: Login, ConfigureComponents
      - User: Login


#### /redfish/v1/Chassis
##### ChassisCollection

- Members@odata.count
- Members

#### /redfish/v1/Chassis/<ChassisName>
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

#### /redfish/v1/Chassis/<ChassisName>/Thermal
##### Thermal
Temperatures Fans Redundancy

#### /redfish/v1/Chassis/<ChassisName>/Thermal#/Temperatures/<SensorName>
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

#### /redfish/v1/Chassis/<ChassisName>/Thermal#/Fans/<FanName>
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

#### /redfish/v1/Chassis/<ChassisName>/Thermal#/Redundancy/<RedundancyName>
##### Fan
- MemberId
- RedundancySet
- Mode
- Status
- MinNumNeeded
- MaxNumSupported


#### /redfish/v1/Chassis/<ChassisName>/Power/
##### Thermal
PowerControl Voltages PowerSupplies Redundancy

#### /redfish/v1/Chassis/<ChassisName>/Power#/PowerControl/<ControlName>
##### PowerControl
- MemberId
- PowerConsumedWatts
- PowerMetrics/IntervalInMin
- PowerMetrics/MinConsumedWatts
- PowerMetrics/MaxConsumedWatts
- PowerMetrics/AverageConsumedWatts
- RelatedItem
  - Should list systems and related chassis

#### /redfish/v1/Chassis/<ChassisName>/Power#/Voltages/<VoltageName>
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

#### /redfish/v1/Chassis/<ChassisName>/Power#/PowerSupplies/<PSUName>
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

#### /redfish/v1/Chassis/{ChassisName}/Power#/Redundancy/<RedundancyName>
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

#### /redfish/v1/EventService/Subscriptions/{EventName}/
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

#### /redfish/v1/Managers/BMC
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

#### /redfish/v1/Managers/BMC/EthernetInterfaces
##### EthernetInterfaceCollection
- Members
- Members@odata.count
- Description

#### /redfish/v1/Managers/BMC/EthernetInterfaces/{InterfaceName}
##### EthernetInterface
- Description
- VLAN
- MaxIPv6StaticAddresses

#### /redfish/v1/Managers/BMC/LogServices
##### LogServiceCollection
- Members
- Members@odata.count
- Description

#### /redfish/v1/Managers/BMC/LogServices/RedfishLog
##### LogService
- Entries
- OverWritePolicy
- Actions
- Status
- DateTime
- MaxNumberOfRecords

#### /redfish/v1/Managers/BMC/LogServices/RedfishLog/Entries/{entry}
##### LogEntry
- Message
- Created
- EntryType

#### /redfish/v1/Managers/BMC/NetworkProtocol
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

#### /redfish/v1/Registries/<MessageRegistry>
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

#### /redfish/v1/Systems/{SystemName}
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

#### /redfish/v1/Systems/{SystemName}/EthernetInterfaces
##### EthernetInterfaceCollection
- Members
- Members@odata.count
- Description

#### /redfish/v1/Systems/{SystemName}/LogServices
##### LogServiceCollection
- Members
  - Should default to one member, named SEL
- Members@odata.count
- Description

#### /redfish/v1/Systems/{SystemName}/LogServices/SEL/Entries
##### LogEntryCollection
- Members
- Members@odata.count
- Description
- @odata.nextLink

#### /redfish/v1/Systems/{SystemName}/LogServices/SEL/Entries/{entryNumber}
##### LogEntry
- MessageArgs
- Severity
- SensorType
- Message
- MessageId
- Created
- EntryCode
- EntryType

#### /redfish/v1/Systems/{SystemName}/Memory
##### MemoryCollection
- Members
- Members@odata.count

#### /redfish/v1/Systems/{SystemName}/Memory/Memory1
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

#### /redfish/v1/Systems/{SystemName}/Memory/Memory1/MemoryMetrics
##### MemoryMetrics
- Description
- HealthData

#### /redfish/v1/Systems/{SystemName}/Processors
##### ProcessorCollection
- Members
  - Should Support CPU1 and CPU2 for dual socket systems
- Members@odata.count

#### /redfish/v1/Systems/{SystemName}/Processors/{CPUName}
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

#### /redfish/v1/Systems/{SystemName}/Storage
##### StorageCollection
- Members
- Members@odata.count

#### /redfish/v1/Systems/{SystemName}/Storage/{storageIndex>
##### Storage
- Drives
- Links


#### /redfish/v1/UpdateService
##### UpdateService
- SoftwareInventory

#### /redfish/v1/UpdateService/SoftwareInventory
##### SoftwareInventoryCollection
- Members
- Should Support BMC, ME, CPLD and BIOS
- Members@odata.count

#### /redfish/v1/UpdateService/SoftwareInventory/{MemberName}
##### SoftwareInventory
- Version
