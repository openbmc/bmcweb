# Multi-host

This document lists all dbus resources needed to support multi-host systems in
the future. In code these are gated off by the
**BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM** constant.

```plaintext


                           ┌────────┐
                     ┌────>│ Host 1 │
                     │     └────────┘
┌─────┐     DBUS     │     ┌────────┐
│ BMC │──────────────┼────>│ Host 2 │
└─────┘              .     └────────┘
                     .
                     .
                     │     ┌────────┐
                     └────>│ Host N │
                           └────────┘
```

When looking at dbus, the main difference is that we need to map do some some
kind of mapping from dbus resources to the correct host.
This is true for both object paths and in some cases services.

As of now, the agreement stands to construct dbus resource names manually in bmcweb.
In the future we consider using associations to
realise the correct mapping of resources to a host. This document shall help
with that, as it lists all dbus resources that we might want to associate with
a host in the future.

## Example dbus resource

Example task: Get the power state of host system in slot 1:

- Dbus object: /xyz/openbmc_project/state/host1
- Dbus service: xyz.openbmc_project.State.Host1

Generelized for a given index N:
- Dbus object: /xyz/openbmc_project/state/hostN
- Dbus service: xyz.openbmc_project.State.HostN

## Dbus resources

Following are all DBus resources needed per header, that are currently gated off
by the **BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM** constant.

| header                          | dbus object path                                          | dbus service name                        |
| ------------                    | --------------------------------------------------------- | --------------------------------------   |
| systems.hpp                     | /xyz/openbmc_project/state/hostN                          | xyz.openbmc_project.State.HostN          |
|                                 | /xyz/openbmc_project/control/hostN/boot                   | xyz.openbmc_project.Settings             |
|                                 | /xyz/openbmc_project/control/hostN/boot/one_time          | xyz.openbmc_project.Settings                                        |
|                                 | /xyz/openbmc_project/control/hostN/power_restore_policy   | xyz.openbmc_project.Settings                                        |
|                                 | /xyz/openbmc_project/control/hostN/auto_reboot            | xyz.openbmc_project.Settings                                        |
|                                 | /xyz/openbmc_project/control/hostN/TPMEnable              | xyz.openbmc_project.Settings                                        |
|                                 | /xyz/openbmc_project/control/hostN/nmi                    | xyz.openbmc_project.Settings                                        |
|                                 | /xyz/openbmc_project/watchdog/hostN                       | Not implemented                          |
|                                 | /xyz/openbmc_project/inventory/path/to/chassis_object     | xyz.openbmc_project.EntityManager
| log_service.hpp                 | /xyz/openbmc_project/State/Boot/PostCodeN                 | xyz.openbmc_project.State.Boot.PostCodeN |
| systems_logservice_postcode.hpp | /xyz/openbmc_project/State/Boot/PostCodeN                 | xyz.openbmc_project.State.Boot.PostCodeN
| power.hpp                       | /xyz/openbmc_project/control/hostN/power_cap              | xyz.openbmc_project.Settings
