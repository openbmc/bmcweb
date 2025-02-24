#pragma once

#include "event_logs_object_type.hpp"
#include "utils/dbus_event_log_entry.hpp"

#include <optional>
#include <string>
#include <vector>

namespace redfish
{
namespace dbus_registries_map
{
struct MappedInfo
{
    std::string registryName;
    std::string registryEntryName;
    std::vector<std::string> messageArgs;
};

std::optional<MappedInfo> getMappedInfo(const DbusEventLogEntry& entry);

bool dbusEventLogEntryToEventLogObjectsType(const DbusEventLogEntry& entry,
                                            EventLogObjectsType& event);

} // namespace dbus_registries_map
} // namespace redfish
