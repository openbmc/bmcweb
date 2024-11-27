# Multi-host

This document shall provide an overview of how
multi-host platforms handle redfish, specific to the **Systems**
redfish resource and all sub-resources aswell as the respective properties
and which backends we use to built them.

The support is currently gated off by the
**BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM** constant.

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

# Not supported

**/redfish/v1/Systems/{computerSystemId}/**

- Memory
- PCIDevices

TBD
