# Redfish multi-host extension

This document shall provide an overview of how multi-host platforms handle
redfish, mostly specific to the **Systems** redfish resource and all
sub-resources aswell as the respective properties and which backends /
mechanisms we use to built them.

---

## Table of contents

1. [Implemented](#implemented)
2. [Not implemented](#not-implemented)
3. [Work arounds](#work-arounds)

## Implemented

### /redfish/v1/Systems

Collection built via DBus from
xyz.openbmc_project.Inventory.Decorator.ManagedHost interface

---

### /redfish/v1/Systems/{computerSystemId}

In general built via DBus from
xyz.openbmc_project.Inventory.Decorator.ManagedHost

### Properties under ComputerSystem resource

All built from DBus

#### _Boot/BootSourceOverrideEnabled_

xyz.openbmc_project.Settings\
/xyz/openbmc_project/control/hostN/boot/one_time

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

Built from DBus\
Service: xyz.openbmc_project.State.HostN\
Path: /xyz/openbmc_project/state/hostN

---

#### /redfish/v1/Systems/{computerSystemId}/Actions/ComputerSystem.Reset

Built from DBus\
xyz.openbmc_project.State.HostN\
/xyz/openbmc_project/state/hostN

---

### Logging

#### /redfish/v1/Systems/{computerSystemId}/LogServices

Currently no mechanism in place. Fallback to hardcoded links.

---

#### /redfish/v1/Systems/{computerSystemId}/LogServices/PostCodes/Entries

Built from DBus\
xyz.openbmc_project.State.Boot.PostCodeN &
/xyz/openbmc_project/State/Boot/PostCodeN

---

#### /redfish/v1/Systems/{computerSystemId}/LogServices/PostCodes/Entries/\<str\>

Built from DBus\
xyz.openbmc_project.State.Boot.PostCodeN &
/xyz/openbmc_project/State/Boot/PostCodeN

---

#### /redfish/v1/Systems/{computerSystemId}/LogServices/PostCodes/Entries/\<str\>/attachment

Built from DBus\
xyz.openbmc_project.State.Boot.PostCodeN &
/xyz/openbmc_project/State/Boot/PostCodeN

---

#### /redfish/v1/Systems/{computerSystemId}/LogServices/PostCodes/Actions/LogService.ClearLog

Built from DBus\
xyz.openbmc_project.State.Boot.PostCodeN &
/xyz/openbmc_project/State/Boot/PostCodeN

---

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

/redfish/v1/Systems/{computerSystemId}/Memory/\
/redfish/v1/Systems/{computerSystemId}/Memory/\<str\>\
/redfish/v1/Systems/{computerSystemId}/PCIeDevices/\
/redfish/v1/Systems/{computerSystemId}/PCIeDevices/\<str\>\
/redfish/v1/Systems/{computerSystemId}/PCIeDevices/\<str\>/PCIeFunctions/\
/redfish/v1/Systems/{computerSystemId}/PCIeDevices/\<str\>/PCIeFunctions/\<str\>\
/redfish/v1/Systems/{ComputerSystem}/Processors/\
/redfish/v1/Systems/{computerSystemId}/Processors/\<str\>\
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
