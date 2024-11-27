# Multi-host

This document shall provide an overview of how
multi-host platforms handle redfish, specific to the **Systems**
redfish resource and all sub-resources aswell as the respective properties
and which backends we use to built them.

---

</br>
</br>

# Redfish Resources

## /redfish/v1/Systems/

collection built from xyz.openbmc_project.Inventory.Decorator.ManagedHost dbus interface

---

</br>

## /redfish/v1/Systems/{computerSystemId}/

built from xyz.openbmc_project.Inventory.Decorator.ManagedHost

### Properties

*Boot/BootSourceOverrideEnabled*

dbus: xyz.openbmc_project.Settings /xyz/openbmc_project/control/hostN/boot/one_time

</br>

*Boot/BootSourceOverrideMode*

dbus: xyz.openbmc_project.Settings /xyz/openbmc_project/control/hostN/boot

</br>

*Boot/BootSourceOverrideTarget*

dbus: xyz.openbmc_project.Settings /xyz/openbmc_project/control/hostN/boot

</br>

*Boot/TrustedModuleRequiredToBoot*

dbus: xyz.openbmc_project.Settings /xyz/openbmc_project/control/hostN/TPMEnable

</br>

*Boot/AutomaticRetryConfig*

dbus: xyz.openbmc_project.Settings /xyz/openbmc_project/control/hostN/auto_reboot

</br>

*Boot/AutomaticRetryAttempts*

dbus: xyz.openbmc_project.State.HostN /xyz/openbmc_project/state/hostN

</br>

*BootProgress/LastState*

dbus: xyz.openbmc_project.State.HostN /xyz/openbmc_project/state/hostN

</br>

*BootProgress/LastStateTime*

dbus: xyz.openbmc_project.State.HostN /xyz/openbmc_project/state/hostN

</br>

*PowerRestorePolicy*

dbus: xyz.openbmc_project.Settings /xyz/openbmc_project/control/hostN/power_restore_policy

</br>

*PowerState*

dbus: xyz.openbmc_project.State.HostN /xyz/openbmc_project/state/hostN

---

</br>


## /redfish/v1/Systems/{computerSystemId}/Actions/ComputerSystem.Reset/

dbus: xyz.openbmc_project.State.HostN & /xyz/openbmc_project/state/hostN

Additional: NMI ResetType

dbus: xyz.openbmc_project.Control.Host.NMI & /xyz/openbmc_project/control/hostN/nmi

---

</br>

## /redfish/v1/Systems/{computerSystemId}/LogServices/

no backend needed

---

</br>

## /redfish/v1/Systems/{computerSystemId}/LogServices/PostCodes/Entries

dbus: xyz.openbmc_project.State.Boot.PostCodeN /xyz/openbmc_project/State/Boot/PostCodeN

---

</br>

## /redfish/v1/Systems/{computerSystemId}/LogServices/PostCodes/Entries/<str>

dbus: xyz.openbmc_project.State.Boot.PostCodeN /xyz/openbmc_project/State/Boot/PostCodeN

---

</br>

## /redfish/v1/Systems/{computerSystemId}/LogServices/PostCodes/Entries/\<str\>/attachment

dbus: xyz.openbmc_project.State.Boot.PostCodeN /xyz/openbmc_project/State/Boot/PostCodeN

---

</br>

## /redfish/v1/Systems/{computerSystemId}/LogServices/PostCodes/Actions/LogService.ClearLog/

dbus: xyz.openbmc_project.State.Boot.PostCodeN /xyz/openbmc_project/State/Boot/PostCodeN

---

</br>

# Not implemented
The following is a list of the redfish resources currently not implemented for multi-host
platforms. This list is subject to change.

These are currently gated off by the
**BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM** constant.

---

**/redfish/v1/Systems/{computerSystemId}/**

- Memory
- PCIDevices

**/redfish/v1/Systems/\<str\>/LogServices/Crashdump/Actions/LogService.ClearLog/**

**/redfish/v1/Systems/\<str\>/LogServices/Crashdump/Actions/LogService.CollectDiagnosticData/**

**/redfish/v1/Systems/\<str\>/LogServices/Crashdump/Entries/**

**/redfish/v1/Systems/\<str\>/LogServices/Crashdump/Entries/\<str\>/**

**/redfish/v1/Systems/\<str\>/LogServices/Crashdump/Entries/\<str\>/\<str\>/**

**/redfish/v1/Systems/\<str\>/LogServices/Dump/**

**/redfish/v1/Systems/\<str\>/LogServices/Dump/Actions/LogService.ClearLog/**

**/redfish/v1/Systems/\<str\>/LogServices/Dump/Actions/LogService.CollectDiagnosticData/**

**/redfish/v1/Systems/\<str\>/LogServices/Dump/Entries/**

**/redfish/v1/Systems/\<str\>/LogServices/Dump/Entries/\<str\>/**

**/redfish/v1/Systems/\<str\>/LogServices/EventLog/**

**/redfish/v1/Systems/\<str\>/LogServices/EventLog/Actions/LogService.ClearLog/**

**/redfish/v1/Systems/\<str\>/LogServices/EventLog/Entries/**

**/redfish/v1/Systems/\<str\>/LogServices/EventLog/Entries/\<str\>/**

**/redfish/v1/Systems/\<str\>/LogServices/EventLog/Entries/<str>/attachment/**

**/redfish/v1/Systems/\<str\>/LogServices/HostLogger/**

**/redfish/v1/Systems/\<str\>/LogServices/HostLogger/Entries/**

**/redfish/v1/Systems/\<str\>/LogServices/HostLogger/Entries/<str>/**

**/redfish/v1/Systems/\<str\>/Bios/**

**/redfish/v1/Systems/\<str\>/Bios/Actions/Bios.ResetBios/**

**/redfish/v1/Systems/\<str\>/Memory/**

**/redfish/v1/Systems/\<str\>/Memory/\<str\>/**

**/redfish/v1/Systems/\<str\>/PCIeDevices/**

**/redfish/v1/Systems/\<str\>/PCIeDevices/\<str\>/**

**/redfish/v1/Systems/\<str\>/PCIeDevices/\<str\>/PCIeFunctions/**

**/redfish/v1/Systems/\<str\>/PCIeDevices/\<str\>/PCIeFunctions/<str>/**

**/redfish/v1/Systems/\<str\>/Processors/**

**/redfish/v1/Systems/\<str\>/Processors/\<str\>/**

**/redfish/v1/Systems/\<str\>/Processors/\<str\>/OperatingConfigs/**

**/redfish/v1/Systems/\<str\>/Processors/\<str\>/OperatingConfigs/\<str\>/**

**/redfish/v1/Systems/\<str\>/ResetActionInfo/**

**/redfish/v1/Systems/\<str\>/Storage/**

**/redfish/v1/Systems/\<str\>/Storage/1/Controllers/**

**/redfish/v1/Systems/\<str\>/Storage/1/Drives/\<str\>/**

**/redfish/v1/Systems/\<str\>/Storage/\<str\>/**

**/redfish/v1/Systems/\<str\>/FabricAdapters/**

**/redfish/v1/Systems/\<str\>/FabricAdapters/\<str\>/**
