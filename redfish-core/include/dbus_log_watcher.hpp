#pragma once

#include <sdbusplus/bus/match.hpp>
namespace redfish
{
class DbusEventLogMonitor
{
  public:
    DbusEventLogMonitor();
    sdbusplus::bus::match_t dbusEventLogMonitor;
};

class DbusTelemetryMonitor
{
  public:
    DbusTelemetryMonitor();

    sdbusplus::bus::match_t matchTelemetryMonitor;
};
} // namespace redfish
