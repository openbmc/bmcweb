#pragma once

#include <sdbusplus/bus/match.hpp>
namespace redfish
{
class DbusTelemetryMonitor
{
  public:
    DbusTelemetryMonitor();

    sdbusplus::bus::match_t matchTelemetryMonitor;
};
} // namespace redfish
