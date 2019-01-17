# OpenBMC Webserver Development

1. ### Performance targets
    As OpenBMC is intended to be deployed on an embedded system, care should be
    taken to avoid expensive constructs, and memory usage.  In general, our
    performance and metric targets are:

    - Binaries and static files should take up < 1MB of filesystem size
    - Memory usage should remain below 10MB at all times
    - Application startup time should be less than 1 second on target hardware
      (AST2500)

2. ### Asynchronous programming
   Care should be taken to ensure that all code is written to be asynchronous in
   nature, to avoid blocking methods from stopping the processing of other
   tasks.  At this time the webserver uses boost::asio for it async framework.
   Threads should be avoided if possible, and instead use async tasks within
   boost::asio.

3. ### Secure coding guidelines
   Secure coding practices should be followed in all places in the webserver

    In general, this means:
    - All buffer boundaries must be checked before indexing or using values
    - All pointers and iterators must be checked for null before dereferencing
    - All input from outside the application is considered untrusted, and should
      be escaped, authorized and filtered accordingly.  This includes files in
      the filesystem.
    - All error statuses are checked and accounted for in control flow.
    - Where applicable, noexcept methods should be preferred to methods that use
      exceptions
    - Explicitly bounded types should be preferred over implicitly bounded types
      (like std::array<int, size> as opposed to int[size])
    - no use of [Banned
      functions](https://github.com/intel/safestringlib/wiki/SDL-List-of-Banned-Functions
      "Banned function list")

4. ### Error handling
   Error handling should be constructed in such a way that all possible errors
   return valid HTTP responses.  The following HTTP codes will be used commonly
    - 200 OK - Request was properly handled
    - 201 Created - Resource was created
    - 401 Unauthorized - Request didn't posses the necessary authentication
    - 403 Forbidden - Request was authenticated, but did not have the necessary
      permissions to accomplish the requested task
    - 404 Not found - The url was not found
    - 500 Internal error - Something has broken within the OpenBMC web server,
      and should be filed as a bug

    Where possible, 307 and 308 redirects should be avoided, as they introduce
    the possibility for subtle security bugs.

5. ### Startup times
   Given that the most common target of OpenBMC is an ARM11 processor, care
   needs to be taken to ensure startup times are low.  In general this means:

    - Minimizing the number of files read from disk at startup.  Unless a
      feature is explicitly intended to be runtime configurable, its logic
      should be "baked in" to the application at compile time.  For cases where
      the implementation is configurable at runtime, the default values should
      be included in application code to minimize the use of nonvolatile
      storage.
    - Avoid excessive memory usage and mallocs at startup.

6. ### Compiler features
    - At this point in time, the webserver sets a number of security flags in
      compile time options to prevent misuse.  The specific flags and what
      optimization levels they are enabled at are documented in the
      CMakeLists.txt file.
    - Exceptions are currently enabled for webserver builds, but their use is
      discouraged.  Long term, the intent is to disable exceptions, so any use
      of them for explicit control flow will likely be rejected in code review.
      Any use of exceptions should be cases where the program can be reasonably
      expected to crash if the exception occurs, as this will be the future
      behavior once exceptions are disabled.
    - Run time type information is disabled
    - Link time optimization is enabled

7. ### Authentication
   The webserver shall provide the following authentication mechanisms.
    - Basic authentication
    - Cookie authentication
    - Token authentication

    There shall be connection between the authentication mechanism used and
    resources that are available over it. The webserver shall employ an
    authentication scheme that is in line with the rest of OpenBMC, and allows
    users and privileges to be provisioned from other interfaces.

8. ### Web security
   The OpenBMC webserver shall follow the latest OWASP recommendations for
   authentication, session management, and security.

9. ### Performance
   The performance priorities for the OpenBMC webserver are (in order):
    1. Code is readable and clear
    2. Code follows secure guidelines
    3. Code is performant, and does not unnecessarily abstract concepts at the
       expense of performance
    4. Code does not employ constructs which require continuous system
       resources, unless required to meet performance targets.  (example:
       caching sensor values which are expected to change regularly)

10. ### Abstraction/interfacing
   In general, the OpenBMC webserver is built using the data driven design.
   Abstraction and Interface guarantees should be used when multiple
   implementations exist, but for implementations where only a single
   implementation exists, prefer to make the code correct and clean rather than
   implement a concrete interface.

11. ### phosphor webui
   The webserver should be capable of hosting phosphor-webui, and impelmenting
   the required flows to host the application.  In general, all access methods
   should be available to the webui.

12. ### Developing and Testing
  There are a variety of ways to develop and test bmcweb software changes.
  Here are the steps for using the SDK and QEMU.

  - Follow all [development environment setup](https://github.com/openbmc/docs/blob/master/development/dev-environment.md)
  directions in the development environment setup document. This will get
  QEMU started up and you in the SDK environment.
  - Follow all of the [gerrit setup](https://github.com/openbmc/docs/blob/master/development/gerrit-setup.md)
  directions in the gerrit setup document.
  - Clone bmcweb from gerrit
  ```
  git clone ssh://openbmc.gerrit/bmcweb/
  ```

  - Ensure it compiles
  ```
  cmake ./ && make
  ```
  **Note:** If you'd like to enable debug traces in bmcweb, use the
  following command for cmake
  ```
  cmake ./ -DCMAKE_BUILD_TYPE:type=Debug
  ```

  - Make your changes as needed, rebuild with `make`

  - Reduce binary size by stripping it when ready for testing
  ```
  arm-openbmc-linux-gnueabi-strip bmcweb
  ```
  **Note:** Stripping is not required and having the debug symbols could be
  useful depending on your testing. Leaving them will drastically increase
  your transfer time to the BMC.

  - Copy your bmcweb you want to test to /tmp/ in QEMU
  ```
  scp -P 2222 bmcweb root@127.0.0.1:/tmp/
  ```
  **Special Notes:**
  The address and port shown here (127.0.0.1 and 2222) reaches the QEMU session
  you set up in your development environment as described above.

  - Stop bmcweb service within your QEMU session
  ```
  systemctl stop bmcweb
  ```
  **Note:** bmcweb supports being started directly in parallel with the bmcweb
  running as a service. The standalone bmcweb will be available on port 18080.
  An advantage of this is you can compare between the two easily for testing.
  In QEMU you would need to open up port 18080 when starting QEMU. Your curl
  commands would need to use 18080 to communicate.

  - If running within a system that has read-only /usr/ filesystem, issue
  the following commands one time per QEMU boot to make the filesystem
  writeable
  ```
  mkdir -p /var/persist/usr
  mkdir -p /var/persist/work/usr
  mount -t overlay -o lowerdir=/usr,upperdir=/var/persist/usr,workdir=/var/persist/work/usr overlay /usr
  ```

  - Remove the existing bmcweb from the filesystem in QEMU
  ```
  rm /usr/bin/bmcweb
  ```

  - Link to your new bmcweb in /tmp/
  ```
  ln -sf /tmp/bmcweb /usr/bin/bmcweb
  ```

  - Test your changes. bmcweb will be started automatically upon your
  first REST or Redfish command
  ```
  curl -c cjar -b cjar -k -X POST https://127.0.0.1:2443/login -d "{\"data\": [ \"root\", \"0penBmc\" ] }"
  curl -c cjar -b cjar -k -X GET https://127.0.0.1:2443/xyz/openbmc_project/state/bmc0
  ```

  - Stop the bmcweb service and scp new file over to /tmp/ each time you
  want to retest a change.

  See the [REST](https://github.com/openbmc/docs/blob/master/REST-cheatsheet.md)
  and [Redfish](https://github.com/openbmc/docs/blob/master/REDFISH-cheatsheet.md) cheatsheets for valid commands.

13. ### Redfish

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
