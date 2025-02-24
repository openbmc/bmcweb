#pragma once

#include "utils/dbus_event_log_entry.hpp"

#include <string>
#include <vector>

namespace redfish
{
namespace dbus_registries_map
{
struct MappedInfo
{
    std::string messageId;
    std::vector<std::string> dbusArgNames = {};
};

std::optional<MappedInfo> getMappedInfo(const DbusEventLogEntry& entry);
    
} // namespace dbus_registries_map
} // namespace redfish
