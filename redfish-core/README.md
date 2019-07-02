# Redfish #

bmcweb provides an implementation of the [Redfish][1] API. This README discusses
some of the details of that implementation and different implementations
available for certain areas.

## LogServices

The [LogService][2] resource provides properties for monitoring and configuring
events for the service or resource to which it is associated.

Within bmcweb, the LogService object resides under the System resource. It
tracks all events for the system.

The LogService supports multiple log entry types. bmcweb has support for
the `Event` type. This is the new Redfish-defined type.

bmcweb supports two different implementations of the
`LogService/EventLog/Entries` URI.

The default implementation uses rsyslog to write Redfish events from the journal
to the persistent /var/log/ filesystem. The bmcweb software then looks for these
files in /var/log/ and returns the appropriate Redfish EventLog Entries for
these. More details on adding events can be found [here][3]

The other implementation of EventLog Entries can be enabled by compiling bmcweb
with the `-DBMCWEB_ENABLE_REDFISH_DBUS_LOG_ENTRIES=ON` option. This will cause
bmcweb to look to [phosphor-logging][4] for any D-Bus log entries. These will
then be translated to Redfish EventLog Entries.

These two implementations do not work together, so choosing one will disable
the other.


[1]: https://www.dmtf.org/standards/redfish
[2]: https://redfish.dmtf.org/schemas/v1/LogService.json
[3]: https://github.com/openbmc/docs/blob/master/redfish-logging-in-bmcweb.md
[4]: https://github.com/openbmc/phosphor-logging
