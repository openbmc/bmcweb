# Redfish multi-host extension

This document shall provide an overview of how multi-host platforms handle
redfish, mostly specific to the **Systems** redfish resource and all
sub-resources aswell as the respective properties and which backends /
mechanisms we use to built them.

---

## Supported

### /redfish/v1/Systems

collection built from xyz.openbmc_project.Inventory.Decorator.ManagedHost dbus
interface

---

### /redfish/v1/Systems/{computerSystemId}

built from xyz.openbmc_project.Inventory.Decorator.ManagedHost

#### Properties

_Boot/BootSourceOverrideEnabled_ dbus: xyz.openbmc_project.Settings
/xyz/openbmc_project/control/hostN/boot/one_time

</br>

_Boot/BootSourceOverrideMode_ dbus: xyz.openbmc_project.Settings
/xyz/openbmc_project/control/hostN/boot

</br>

_Boot/BootSourceOverrideTarget_ dbus: xyz.openbmc_project.Settings
/xyz/openbmc_project/control/hostN/boot

</br>

_Boot/TrustedModuleRequiredToBoot_ dbus: xyz.openbmc_project.Settings
/xyz/openbmc_project/control/hostN/TPMEnable

</br>

_Boot/AutomaticRetryConfig_ dbus: xyz.openbmc_project.Settings
/xyz/openbmc_project/control/hostN/auto_reboot

</br>

_Boot/AutomaticRetryAttempts_ dbus: xyz.openbmc_project.State.HostN
/xyz/openbmc_project/state/hostN

</br>

_BootProgress/LastState_ dbus: xyz.openbmc_project.State.HostN
/xyz/openbmc_project/state/hostN

</br>

_BootProgress/LastStateTime_ dbus: xyz.openbmc_project.State.HostN
/xyz/openbmc_project/state/hostN

</br>

_PowerRestorePolicy_ dbus: xyz.openbmc_project.Settings
/xyz/openbmc_project/control/hostN/power_restore_policy

</br>

_PowerState_ dbus: xyz.openbmc_project.State.HostN
/xyz/openbmc_project/state/hostN

---

### /redfish/v1/Systems/\<str\>/ResetActionInfo

dbus: xyz.openbmc_project.State.HostN /xyz/openbmc_project/state/hostN

---

### /redfish/v1/Systems/{computerSystemId}/Actions/ComputerSystem.Reset

dbus: xyz.openbmc_project.State.HostN & /xyz/openbmc_project/state/hostN

Additional: NMI ResetType

dbus: xyz.openbmc_project.Control.Host.NMI &
/xyz/openbmc_project/control/hostN/nmi

---

### /redfish/v1/Systems/{computerSystemId}/LogServices

currently no mechanism in place. Fallback to hardcoded links.

---

### /redfish/v1/Systems/{computerSystemId}/LogServices/PostCodes/Entries

dbus: xyz.openbmc_project.State.Boot.PostCodeN
/xyz/openbmc_project/State/Boot/PostCodeN

---

### /redfish/v1/Systems/{computerSystemId}/LogServices/PostCodes/Entries/\<str\>

dbus: xyz.openbmc_project.State.Boot.PostCodeN
/xyz/openbmc_project/State/Boot/PostCodeN

---

### /redfish/v1/Systems/{computerSystemId}/LogServices/PostCodes/Entries/\<str\>/attachment

dbus: xyz.openbmc_project.State.Boot.PostCodeN
/xyz/openbmc_project/State/Boot/PostCodeN

---

### /redfish/v1/Systems/{computerSystemId}/LogServices/PostCodes/Actions/LogService.ClearLog

dbus: xyz.openbmc_project.State.Boot.PostCodeN
/xyz/openbmc_project/State/Boot/PostCodeN

---

### /redfish/v1/Systems/\<str\>/Memory

built from dbus interface xyz.openbmc_project.Inventory.Item.Dimm

---

### /redfish/v1/Systems/\<str\>/PCIeDevices

built from dbus interface xyz.openbmc_project.Inventory.Item.PCIeDevice

---

### /redfish/v1/Systems/\<str\>/Processors

built from dbus interface xyz.openbmc_project.Inventory.Item.Cpu

---

### /redfish/v1/Systems/\<str\>/Storage

built from dbus interface xyz.openbmc_project.Inventory.Item.Storage

---

### /redfish/v1/Systems/\<str\>/FabricAdapters

built from dbus interface xyz.openbmc_project.Inventory.Item.FabricAdapters

---

## Not implemented

The following is a list of the redfish resources currently not implemented for
multi-host platforms. This list is subject to change.

These are currently gated off by the
**BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM** constant.

---

## Logging

### EventLog

No internal design for multi-host handling

</br>

**/redfish/v1/Systems/\<str\>/LogServices/EventLog**

**/redfish/v1/Systems/\<str\>/LogServices/EventLog/Entries**

**/redfish/v1/Systems/\<str\>/LogServices/EventLog/Entries/\<str\>**

**/redfish/v1/Systems/\<str\>/LogServices/EventLog/Entries/\<str\>/attachment**

**/redfish/v1/Systems/\<str\>/LogServices/EventLog/Actions/LogService.ClearLog**

### Crashdump

See EventLog

</br>

**/redfish/v1/Systems/\<str\>/LogServices/Crashdump/Actions/LogService.ClearLog**

**/redfish/v1/Systems/\<str\>/LogServices/Crashdump/Actions/LogService.CollectDiagnosticData**

**/redfish/v1/Systems/\<str\>/LogServices/Crashdump/Entries**

**/redfish/v1/Systems/\<str\>/LogServices/Crashdump/Entries/\<str\>**

**/redfish/v1/Systems/\<str\>/LogServices/Crashdump/Entries/\<str\>/\<str\>**

### Dump

See EventLog

</br>

**/redfish/v1/Systems/\<str\>/LogServices/Dump**

**/redfish/v1/Systems/\<str\>/LogServices/Dump/Actions/LogService.ClearLog**

**/redfish/v1/Systems/\<str\>/LogServices/Dump/Actions/LogService.CollectDiagnosticData**

**/redfish/v1/Systems/\<str\>/LogServices/Dump/Entries**

**/redfish/v1/Systems/\<str\>/LogServices/Dump/Entries/\<str\>**

---

## Firmware

TBD

</br>

**/redfish/v1/Systems/\<str\>/Bios**

**/redfish/v1/Systems/\<str\>/Bios/Actions/Bios.ResetBios**

---

## Inventory

Needs design for mapping between inventory item and correct host.

**/redfish/v1/Systems/\<str\>/Memory/\<str\>**

**/redfish/v1/Systems/\<str\>/PCIeDevices/\<str\>**

**/redfish/v1/Systems/\<str\>/PCIeDevices/\<str\>/PCIeFunctions**

**/redfish/v1/Systems/\<str\>/PCIeDevices/\<str\>/PCIeFunctions/\<str\>**

**/redfish/v1/Systems/\<str\>/Processors/\<str\>**

**/redfish/v1/Systems/\<str\>/Processors/\<str\>/OperatingConfigs**

**/redfish/v1/Systems/\<str\>/Processors/\<str\>/OperatingConfigs/\<str\>**

**/redfish/v1/Systems/\<str\>/Storage/1/Controllers**

**/redfish/v1/Systems/\<str\>/Storage/1/Drives/\<str\>**

**/redfish/v1/Systems/\<str\>/Storage/\<str\>**

**/redfish/v1/Systems/\<str\>/FabricAdapters/\<str\>**

---
