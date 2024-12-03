#pragma once

#include "dbus_utility.hpp"
#include "event_logs_object_type.hpp"

#include <sdbusplus/bus/match.hpp>
namespace redfish
{
class DbusEventLogMonitor
{
  public:
    DbusEventLogMonitor();
    sdbusplus::bus::match_t dbusEventLogMonitor;

    static bool
        eventLogObjectFromDBus(const dbus::utility::DBusPropertiesMap& map,
                               EventLogObjectsType& event);
};

class DbusTelemetryMonitor
{
  public:
    DbusTelemetryMonitor();

    sdbusplus::bus::match_t matchTelemetryMonitor;
};
} // namespace redfish
