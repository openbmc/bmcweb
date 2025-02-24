#pragma once

#include "dbus_utility.hpp"
#include "utils/dbus_utils.hpp"

#include <sdbusplus/unpack_properties.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>

namespace redfish
{

struct DbusEventLogEntry
{
    // represents a subset of an instance of dbus interface
    // xyz.openbmc_project.Logging.Entry

    uint32_t Id = 0;
    std::string Message;
    const std::string* Path = nullptr;
    const std::string* Resolution = nullptr;
    bool Resolved = false;
    std::string ServiceProviderNotify;
    std::string Severity;
    uint64_t Timestamp = 0;
    uint64_t UpdateTimestamp = 0;
    std::unordered_map<std::string, std::string> AdditionalData = {};
};

inline std::optional<DbusEventLogEntry> fillDbusEventLogEntryFromPropertyMap(
    const dbus::utility::DBusPropertiesMap& resp)
{
    DbusEventLogEntry entry;

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
        "UpdateTimestamp", entry.UpdateTimestamp,
        "AdditionalData", entry.AdditionalData
    );
    // clang-format on
    if (!success)
    {
        return std::nullopt;
    }
    return entry;
}
} // namespace redfish
