#include "utils/dbus_event_log_entry.hpp"

namespace redfish
{
namespace dbus_registries
{
std::optional<std::pair<std::string, std::vector<std::string>>>
    getRfMessageIdAndArgs(const DbusEventLogEntry& entry);
} // namespace dbus_registries
} // namespace redfish
