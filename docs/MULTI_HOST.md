# Redfish multi-host extension

This document shall provide an overview of how multi-host platforms handle
redfish, mostly specific to the **Systems** redfish resource tree aswell as the
respective properties and which backends / mechanisms we use to built them.

---

## Table of contents

1. [Developing](#developing)
2. [Implemented](#implemented)
3. [Not implemented](#not-implemented)
4. [Work arounds](#work-arounds)

---

</br>

### Developing

To ease implementation & review, here are some of the current understandings and
design decisions, we currently follow to support multi-host in bmcweb.

The discussions took place on the patches and were talked about in depth in
multiple threads on discord in the redfish-and-bmcweb channel.

For a reference multi-host platform have a look at the
[meta-yosemite4 layer](https://github.com/openbmc/openbmc/tree/master/meta-facebook/meta-yosemite4).

---

#### Associations vs. xyz.openbmc_project.Inventory.Decorator.ManagedHost

In general, as per the
[physical-topology design document](https://github.com/openbmc/docs/blob/master/designs/physical-topology.md)
we would like to use the described dbus associations to associate a given
ComputerSystem in a multi-host setup with its specific resources. Recently, the
topology mechanism in entity-manager has been reworked in
[this patch series](https://gerrit.openbmc.org/c/openbmc/entity-manager/+/83027/8)
to allow for more complex descriptions of a server topology.

However, with the
[initial patch series](https://gerrit.openbmc.org/c/openbmc/bmcweb/+/73855) that
started off the effort to support multi-host platforms in bmcweb we had to make
some trade-offs. There were just too many associations needed, that were - and
still are - not in place, to implement the support. As a work-around, we
introduced the
[xyz.openbmc_project.Inventory.Decorator.ManagedHost](https://github.com/openbmc/phosphor-dbus-interfaces/blob/900af2c70e9c045508f60c029583b6cc30eb596a/yaml/xyz/openbmc_project/Inventory/Decorator/ManagedHost.interface.yaml)
dbus interface. With the interface configured in entity-manager we can add a
HostIndex to each ComputerSystem, which we then can use upon a ComputerSystem
request to create the necessary object paths and dbus service names, to form
the response. The interface first got proposed on the
[mailing list](https://lists.ozlabs.org/pipermail/openbmc/2024-June/035442.html).

Beyond that, the interface now also helps finding the right associations that
are already in place.

---

#### Indexing

Architecturally, index 0 is reserved for single-host systems. On multi-host
indexing of the hosts should start at index 1. More can be
[read here](https://github.com/openbmc/phosphor-dbus-interfaces/blob/900af2c70e9c045508f60c029583b6cc30eb596a/yaml/xyz/openbmc_project/State/README.md?plain=1#L75).

---

#### bmcweb variable `systemName`

`systemName` refers to the redfish identifier used for a specific ComputerSystem
upon request. Historically this id was hard-coded to `system`, before bmcweb
introduced the `redfish-system-uri-name` option to allow for customization of
the identifier. To retain backwards compatibility, the identifier is set to
`system` by default. This **does not** apply to multi-host platforms. Currently,
the redfish identifier for a given ComputerSystem on multi-host comes from
entity-manager. Specifically from the name property of the configuration, that
defines the ManagedHost interface.

---

#### Testing

Changes should be validated on both single- and multi-host.

---

</br>

## Implemented

### /redfish/v1/Systems

Collection built via DBus using
xyz.openbmc_project.Inventory.Decorator.ManagedHost interface

---

### /redfish/v1/Systems/{computerSystemId}

In general built via DBus from
xyz.openbmc_project.Inventory.Decorator.ManagedHost

### Properties under ComputerSystem resource

All built from DBus using xyz.openbmc_project.Inventory.Decorator.ManagedHost

#### _Boot/BootSourceOverrideEnabled_

**Service**: xyz.openbmc_project.Settings\
**Path**: /xyz/openbmc_project/control/hostN/boot/one_time

#### _Boot/BootSourceOverrideMode_

xyz.openbmc_project.Settings\
/xyz/openbmc_project/control/hostN/boot

#### _Boot/BootSourceOverrideTarget_

xyz.openbmc_project.Settings\
/xyz/openbmc_project/control/hostN/boot

#### _Boot/TrustedModuleRequiredToBoot_

xyz.openbmc_project.Settings\
/xyz/openbmc_project/control/hostN/TPMEnable

#### _Boot/AutomaticRetryConfig_

xyz.openbmc_project.Settings\
/xyz/openbmc_project/control/hostN/auto_reboot

#### _Boot/AutomaticRetryAttempts_

xyz.openbmc_project.State.HostN\
/xyz/openbmc_project/state/hostN

#### _BootProgress/LastState_

xyz.openbmc_project.State.HostN\
/xyz/openbmc_project/state/hostN

#### _BootProgress/LastStateTime_

xyz.openbmc_project.State.HostN\
/xyz/openbmc_project/state/hostN

#### _PowerRestorePolicy_

xyz.openbmc_project.Settings\
/xyz/openbmc_project/control/hostN/power_restore_policy

#### _PowerState_

xyz.openbmc_project.State.HostN\
/xyz/openbmc_project/state/hostN

---

#### /redfish/v1/Systems/{computerSystemId}/ResetActionInfo

Built from DBus using xyz.openbmc_project.Inventory.Decorator.ManagedHost\
xyz.openbmc_project.State.HostN\
/xyz/openbmc_project/state/hostN

---

#### /redfish/v1/Systems/{computerSystemId}/Actions/ComputerSystem.Reset

Built from DBus using xyz.openbmc_project.Inventory.Decorator.ManagedHost\
xyz.openbmc_project.State.HostN\
/xyz/openbmc_project/state/hostN

---

### Logging

#### /redfish/v1/Systems/{computerSystemId}/LogServices

Currently no mechanism in place. Fallback to hardcoded links.

---

#### /redfish/v1/Systems/{computerSystemId}/LogServices/PostCodes/Entries

Built from DBus using xyz.openbmc_project.Inventory.Decorator.ManagedHost\
xyz.openbmc_project.State.Boot.PostCodeN &
/xyz/openbmc_project/State/Boot/PostCodeN

---

#### /redfish/v1/Systems/{computerSystemId}/LogServices/PostCodes/Entries/\<str\>

Built from DBus using xyz.openbmc_project.Inventory.Decorator.ManagedHost\
xyz.openbmc_project.State.Boot.PostCodeN &
/xyz/openbmc_project/State/Boot/PostCodeN

---

#### /redfish/v1/Systems/{computerSystemId}/LogServices/PostCodes/Entries/\<str\>/attachment

Built from DBus using xyz.openbmc_project.Inventory.Decorator.ManagedHost\
xyz.openbmc_project.State.Boot.PostCodeN &
/xyz/openbmc_project/State/Boot/PostCodeN

---

#### /redfish/v1/Systems/{computerSystemId}/LogServices/PostCodes/Actions/LogService.ClearLog

Built from DBus using xyz.openbmc_project.Inventory.Decorator.ManagedHost\
xyz.openbmc_project.State.Boot.PostCodeN &
/xyz/openbmc_project/State/Boot/PostCodeN

---

### Inventory

#### /redfish/v1/Systems/{computerSystemId}/Memory

Built from DBus using associations &
xyz.openbmc_project.Inventory.Decorator.ManagedHost

#### /redfish/v1/Systems/{computerSystemId}/Memory/\<str\>\

Built from DBus using associations &
xyz.openbmc_project.Inventory.Decorator.ManagedHost

---

#### /redfish/v1/Systems/{ComputerSystem}/Processors/\

Built from DBus using associations &
xyz.openbmc_project.Inventory.Decorator.ManagedHost

#### /redfish/v1/Systems/{computerSystemId}/Processors/\<str\>\

Built from DBus using associations &
xyz.openbmc_project.Inventory.Decorator.ManagedHost

---

### Update Service

#### /redfish/v1/UpdateService/FirmwareInventory/\<str\>/

Built from DBus using associations &
xyz.openbmc_project.Inventory.Decorator.ManagedHost

---

</br>

## Not implemented

The following is a list of the redfish resources currently not implemented for
multi-host platforms. This list is subject to change. Further designs for the
respective backends need to be made, before any of these can be supported.

Excluding the PATCH properties, the resources are currently gated off by the
**BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM** constant.

---

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

### Power

Backend for setting **NMI** currently only supports single-host

### Logging

#### Crashdump

No internal design for multi-host handling. Includes following URIs:

/redfish/v1/Systems/{computerSystemId}/LogServices/Crashdump/Actions/LogService.ClearLog/\
/redfish/v1/Systems/{computerSystemId}/LogServices/Crashdump/Actions/LogService.CollectDiagnosticData/\
/redfish/v1/Systems/{computerSystemId}/LogServices/Crashdump/Entries/\
/redfish/v1/Systems/{computerSystemId}/LogServices/Crashdump/Entries/\<str\>\
/redfish/v1/Systems/{computerSystemId}/LogServices/Crashdump/Entries/\<str\>/\<str\>

#### Dump

No internal design for multi-host handling. Includes following URIs:

/redfish/v1/Systems/{computerSystemId}/LogServices/Dump/\
/redfish/v1/Systems/{computerSystemId}/LogServices/Dump/Actions/LogService.ClearLog/\
/redfish/v1/Systems/{computerSystemId}/LogServices/Dump/Actions/LogService.CollectDiagnosticData/\
/redfish/v1/Systems/{computerSystemId}/LogServices/Dump/Entries/\
/redfish/v1/Systems/{computerSystemId}/LogServices/Dump/Entries/\<str\>

---

## Firmware

TBD

/redfish/v1/Systems/{computerSystemId}/Bios/\
/redfish/v1/Systems/{computerSystemId}/Bios/Actions/Bios.ResetBios/

---

## Inventory

Needs design for mapping between inventory item and correct host. This includes
follwing URIs:

/redfish/v1/Systems/{computerSystemId}/PCIeDevices/\
/redfish/v1/Systems/{computerSystemId}/PCIeDevices/\<str\>\
/redfish/v1/Systems/{computerSystemId}/PCIeDevices/\<str\>/PCIeFunctions/\
/redfish/v1/Systems/{computerSystemId}/PCIeDevices/\<str\>/PCIeFunctions/\<str\>\
/redfish/v1/Systems/{computerSystemId}/Processors/\<str\>/OperatingConfigs/\
/redfish/v1/Systems/{computerSystemId}/Processors/\<str\>/OperatingConfigs/\<str\>\
/redfish/v1/Systems/{computerSystemId}/Storage/\
/redfish/v1/Systems/{computerSystemId}/Storage/1/Controllers/\
/redfish/v1/Systems/{computerSystemId}/Storage/1/Drives/\<str\>\
/redfish/v1/Systems/{computerSystemId}/Storage/\<str\>\
/redfish/v1/Systems/{computerSystemId}/FabricAdapters/\
/redfish/v1/Systems/{computerSystemId}/FabricAdapters/\<str\>

---

## Work-arounds

### EventLog

TBD
