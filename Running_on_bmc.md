# Enabling and Running Bmcweb #
This document has some pointers to enable and run bmcweb as part of BMC image.

## Enabling Bmcweb
Bmcweb and phosphor-bmcweb-cert-config packages are added to BMC build
by default. Make sure these 2 packages are not removed for your machine
configuration.

If Avahi or SLP is used then the bmcweb service may have to be added
to the service directory. To add bmcweb to registered services,
follow the example at,
https://github.com/openbmc/openbmc/blob/master/meta-ibm/recipes-phosphor/bmcweb/bmcweb_%25.bbappend

## Dependencies
Bmcweb depends on user manager and phosphor certificate manager.
User manager(IMAGE_FEATURE obmc-user-mgmt) is enabled by default
just make sure it is not removed.
The user that will be used to execute the redfish should be part of
priv-admin group.


## Running Bmcweb
Bmcweb service comes up in port 443. If you need to listen to different
port, change the "ListenStream" parameter in bmcweb.socket

If bmcweb is started as standalone executable, it will come up in port
18080

## Inventory Manager
Bmcweb uses Phosphor inventory manager. So to enable sensor and other
machine details inventory manager needs to be populated.

For sensors, sensor associations JSON file needs to be created.
Example can be found at,
https://github.com/openbmc/meta-ibm/blob/master/recipes-phosphor/inventory/phosphor-inventory-manager/associations.json

More details on sensor associations can be found at,
https://github.com/openbmc/docs/blob/master/architecture/sensor-architecture.md
