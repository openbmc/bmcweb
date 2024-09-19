#pragma once

#include "dbus_utility.hpp"
#include "utils/dbus_utils.hpp"

#include <string>

namespace redfish
{
struct DbusEventLogEntry
{
    // represents a subset of an instance of dbus interface
    // xyz.openbmc_project.Logging.Entry

    uint32_t Id{};
    std::string Message;
    const std::string* Path{};
    const std::string* Resolution{};
    bool Resolved{};
    std::string ServiceProviderNotify;
    std::string Severity;
    uint64_t Timestamp{};
    uint64_t UpdateTimestamp{};
};

inline bool fillDbusEventLogEntryFromPropertyMap(
    const dbus::utility::DBusPropertiesMap& resp, DbusEventLogEntry& entry)
{
    entry.Path = nullptr;
    entry.Resolution = nullptr;
    entry.Resolved = false;

    // clang-format off
    bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), resp,
        "Id", entry.Id,
        "Message", entry.Message,
        "Path", entry.Path,
        "Resolution", entry.Resolution,
        "Resolved", entry.Resolved,
        "ServiceProviderNotify", entry.ServiceProviderNotify,
        "Severity", entry.Severity,
        "Timestamp", entry.Timestamp,
        "UpdateTimestamp", entry.UpdateTimestamp
    );
    // clang-format on
    return success;
}
} // namespace redfish
