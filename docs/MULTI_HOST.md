# Redfish multi-host implementation

This document shall provide an overview of handling a multi-host platform via
Redfish requests. A multi-host platform is one where a single BMC directly
manages and interfaces with multiple hosts or computing nodes. This should not
be confused with bmcweb's [aggregation feature][1], which aggregates resources from
multiple hosts or computing nodes managed by satellite BMCs.

## Design

### Associations

In accordance with the physical-topology
[design document][2], bmcweb uses dbus associations to link a given
ComputerSystem or Chassis with its specific resources.

### xyz.openbmc_project.Inventory.Decorator.ManagedHost

In places, where associations do not exist yet, bmcweb utilizes
[xyz.openbmc_project.Inventory.Decorator.ManagedHost][3] to create dbus object
paths. The HostIndex assigns a unique index to each host, which is expected to
remain consistent across boots. Note, this is only temporary for as long as the
neccessary associations do not exist and will be fixed once they are in place.

### EventLog

On single-host, the EventLog is provided through the ComputerSystem resource.
However, on multi-host systems, immediate implementation was not feasible
because it necessitated backend support for filtering EventLog entries by host
or compute node - a capability that is not available. As a more straightforward
solution, EventLog delivery was transitioned to the Managers resource and is
enabled by default and can be configured with the `redfish-eventlog-managers`
package config. According to the Redfish specification, both the ComputerSystem
and Managers resources are authorized to serve the EventLog.

[1]: https://github.com/openbmc/bmcweb/blob/master/docs/AGGREGATION.md
[2]: https://github.com/openbmc/docs/blob/master/designs/physical-topology.md
[3]: https://github.com/openbmc/phosphor-dbus-interfaces/blob/1730647ae60100bbbaac1f5d010f459d9eb91331/yaml/xyz/openbmc_project/Inventory/Decorator/ManagedHost.interface.yaml
