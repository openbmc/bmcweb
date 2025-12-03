# Redfish multi-host implementation

This document shall provide an overview of handling a multi-host platform via
Redfish requests. A multi-host platform is one where a single BMC directly
manages and interfaces with multiple hosts or computing nodes. This should not
be confused with bmcweb's aggregation feature, which aggregates resources from
multiple hosts or computing nodes managed by satellite BMCs. The document
details the backends and mechanisms used to form a correct response to these
requests and includes lists of the resources that have been implemented as well
as those that are not yet supported.

## Design

### Associations

In accordance with the physical-topology
[design document](https://github.com/openbmc/docs/blob/master/designs/physical-topology.md),
bmcweb uses dbus associations to link a given ComputerSystem or Chassis with its
specific resources.

### xyz.openbmc_project.Inventory.Decorator.ManagedHost

In places, where associations do not exist yet, bmcweb utilizes
[xyz.openbmc_project.Inventory.Decorator.ManagedHost](https://github.com/openbmc/phosphor-dbus-interfaces/blob/1730647ae60100bbbaac1f5d010f459d9eb91331/yaml/xyz/openbmc_project/Inventory/Decorator/ManagedHost.interface.yaml)
to create dbus object paths. The HostIndex assigns a unique index to each host,
which is expected to remain consistent across boots.

### Indexing

Architecturally, index 0 is reserved for single-host systems. On multi-host it
is expected that indexing of the hosts starts at index 1 to avoid introducing
subtle bugs, as documented in
[openbmc/phosphor-dbus-interfaces](https://github.com/openbmc/phosphor-dbus-interfaces/blob/900af2c70e9c045508f60c029583b6cc30eb596a/yaml/xyz/openbmc_project/State/README.md?plain=1#L75).

### BMCWEB_REDFISH_SYSTEM_URI_NAME

The Redfish identifier for a ComputerSystem is currently derived from the dbus
object that implements `xyz.openbmc_project.Inventory.Decorator.ManagedHost`.
BMCWEB_REDFISH_SYSTEM_URI_NAME is only relevant for a single-host machine.

## Implemented

**/redfish/v1/Systems**\
Collection built via DBus using
xyz.openbmc_project.Inventory.Decorator.ManagedHost interface

**/redfish/v1/Systems/{computerSystemId}**\
In general built via DBus from
xyz.openbmc_project.Inventory.Decorator.ManagedHost

**Boot/BootSourceOverrideEnabled**\
**Service**: xyz.openbmc_project.Settings\
**Path**: /xyz/openbmc_project/control/hostN/boot/one_time

**Boot/BootSourceOverrideMode**\
xyz.openbmc_project.Settings\
/xyz/openbmc_project/control/hostN/boot

**Boot/BootSourceOverrideTarget**\
xyz.openbmc_project.Settings\
/xyz/openbmc_project/control/hostN/boot

**Boot/TrustedModuleRequiredToBoot**\
xyz.openbmc_project.Settings\
/xyz/openbmc_project/control/hostN/TPMEnable

**Boot/AutomaticRetryConfig**\
xyz.openbmc_project.Settings\
/xyz/openbmc_project/control/hostN/auto_reboot

**Boot/AutomaticRetryAttempts**\
xyz.openbmc_project.State.HostN\
/xyz/openbmc_project/state/hostN

**BootProgress/LastState**\
xyz.openbmc_project.State.HostN\
/xyz/openbmc_project/state/hostN

**BootProgress/LastStateTime**\
xyz.openbmc_project.State.HostN\
/xyz/openbmc_project/state/hostN

**PowerRestorePolicy**\
xyz.openbmc_project.Settings\
/xyz/openbmc_project/control/hostN/power_restore_policy

**PowerState**\
xyz.openbmc_project.State.HostN\
/xyz/openbmc_project/state/hostN

**/redfish/v1/Systems/{computerSystemId}/ResetActionInfo**\
**/redfish/v1/Systems/{computerSystemId}/Actions/ComputerSystem.Reset**\
Built from DBus using xyz.openbmc_project.Inventory.Decorator.ManagedHost\
xyz.openbmc_project.State.HostN\
/xyz/openbmc_project/state/hostN

### Logging

**/redfish/v1/Systems/{computerSystemId}/LogServices**\
Currently no mechanism in place. Fallback to hardcoded links.

#### PostCodes

**/redfish/v1/Systems/{computerSystemId}/LogServices/PostCodes/Entries**\
**/redfish/v1/Systems/{computerSystemId}/LogServices/PostCodes/Entries/\<str\>**\
**/redfish/v1/Systems/{computerSystemId}/LogServices/PostCodes/Entries/\<str\>/attachment**\
**/redfish/v1/Systems/{computerSystemId}/LogServices/PostCodes/Actions/LogService.ClearLog**\

Built from DBus using xyz.openbmc_project.Inventory.Decorator.ManagedHost\
xyz.openbmc_project.State.Boot.PostCodeN\
/xyz/openbmc_project/State/Boot/PostCodeN

#### EventLog

Ideally, EventLog responses would be scoped to the requested ComputerSystem, but
bmcweb doesn't currently support filtering EventLog entries by host. As a
workaround, the redfish-eventlog-location=managers meson option moves the
EventLog tree under /redfish/v1/Managers/{managerId}, providing a way to access
logs on multi-host platforms.

**/redfish/v1/Managers/{managerId}/LogServices/EventLog**\
**/redfish/v1/Managers/{managerId}/LogServices/EventLog/Entries**\
**/redfish/v1/Managers/{managerId}/LogServices/EventLog/Entries/\<str\>/attachment**\
**/redfish/v1/Managers/{managerId}/LogServices/EventLog/Entries/Actions/LogService.ClearLog**

### Inventory

**/redfish/v1/Systems/{computerSystemId}/Memory**\
**/redfish/v1/Systems/{computerSystemId}/Memory/\<str\>**\
**/redfish/v1/Systems/{ComputerSystem}/Processors**\
**/redfish/v1/Systems/{computerSystemId}/Processors/\<str\>**\
Built from DBus using associations

### Update Service

**/redfish/v1/UpdateService/FirmwareInventory/\<str\>**\
Built from DBus using associations

## Not implemented

The following is a list of the Redfish resources currently not implemented for
multi-host platforms. This list is subject to change. Further designs for the
respective backends need to be made, before any of these can be supported.

Excluding the PATCH properties, the resources are currently gated off by the
`BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM` constant.

### PATCH properties under /redfish/v1/Systems/{computerSystemId}

The following properties aren't yet implemented for multi-host.

- AssetTag
- PowerMode
- HostWatchdogTimer/FunctionEnabled
- HostWatchdogTimer/TimeoutAction
- IdlePowerSaver/Enabled
- IdlePowerSaver/EnterDwellTimeSeconds
- IdlePowerSaver/EnterUtilizationPercent
- IdlePowerSaver/ExitDwellTimeSeconds
- IdlePowerSaver/ExitUtilizationPercent

### Logging

#### EventLog

/redfish/v1/Systems/{computerSystemId}/LogServices/EventLog\
/redfish/v1/Systems/{computerSystemId}/LogServices/EventLog/Entries\
/redfish/v1/Systems/{computerSystemId}/LogServices/EventLog/Entries/\<str\>/attachment\
/redfish/v1/Systems/{computerSystemId}/LogServices/EventLog/Entries/Actions/LogService.ClearLog\

#### Crashdump

No internal design for multi-host handling. Includes following URIs:

/redfish/v1/Systems/{computerSystemId}/LogServices/Crashdump/Actions/LogService.ClearLog\
/redfish/v1/Systems/{computerSystemId}/LogServices/Crashdump/Actions/LogService.CollectDiagnosticData\
/redfish/v1/Systems/{computerSystemId}/LogServices/Crashdump/Entries\
/redfish/v1/Systems/{computerSystemId}/LogServices/Crashdump/Entries/\<str\>\
/redfish/v1/Systems/{computerSystemId}/LogServices/Crashdump/Entries/\<str\>/\<str\>

#### Dump

No internal design for multi-host handling. Includes following URIs:

/redfish/v1/Systems/{computerSystemId}/LogServices/Dump\
/redfish/v1/Systems/{computerSystemId}/LogServices/Dump/Actions/LogService.ClearLog\
/redfish/v1/Systems/{computerSystemId}/LogServices/Dump/Actions/LogService.CollectDiagnosticData\
/redfish/v1/Systems/{computerSystemId}/LogServices/Dump/Entries\
/redfish/v1/Systems/{computerSystemId}/LogServices/Dump/Entries/\<str\>

### Firmware

TBD

/redfish/v1/Systems/{computerSystemId}/Bios\
/redfish/v1/Systems/{computerSystemId}/Bios/Actions/Bios.ResetBios

### Inventory

Needs design for mapping between inventory item and correct host. This includes
following URIs:

/redfish/v1/Systems/{computerSystemId}/PCIeDevices\
/redfish/v1/Systems/{computerSystemId}/PCIeDevices/\<str\>\
/redfish/v1/Systems/{computerSystemId}/PCIeDevices/\<str\>/PCIeFunctions\
/redfish/v1/Systems/{computerSystemId}/PCIeDevices/\<str\>/PCIeFunctions/\<str\>\
/redfish/v1/Systems/{computerSystemId}/Processors/\<str\>/OperatingConfigs\
/redfish/v1/Systems/{computerSystemId}/Processors/\<str\>/OperatingConfigs/\<str\>\
/redfish/v1/Systems/{computerSystemId}/Storage/\
/redfish/v1/Systems/{computerSystemId}/Storage/1/Controllers/\
/redfish/v1/Systems/{computerSystemId}/Storage/1/Drives/\<str\>\
/redfish/v1/Systems/{computerSystemId}/Storage/\<str\>\
/redfish/v1/Systems/{computerSystemId}/FabricAdapters/\
/redfish/v1/Systems/{computerSystemId}/FabricAdapters/\<str\>
