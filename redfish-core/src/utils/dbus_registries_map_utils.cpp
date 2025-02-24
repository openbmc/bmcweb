#include "utils/dbus_registries_map_utils.hpp"

#include "event_log.hpp"
#include "external_registries_map.hpp"
#include "logging.hpp"
#include "utils/dbus_event_log_entry.hpp"
#include "utils/time_utils.hpp"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <format>
#include <optional>
#include <string_view>
#include <vector>

namespace redfish
{
namespace dbus_registries_map
{
std::optional<MappedInfo> getMappedInfo(const DbusEventLogEntry& entry)
{
    const auto* it = std::find_if(
        dbusToRedfishMessageId.begin(), dbusToRedfishMessageId.end(),
        [&entry](const DbusMessageEntry& element) {
            return entry.Message == element.first;
        });

    if (it == dbusToRedfishMessageId.end())
    {
        return std::nullopt;
    }

    auto dbusToRfInfo = it->second;
    std::vector<std::string> args;

    for (size_t index = 0; index < dbusToRfInfo.numberOfArgs; index++)
    {
        auto additionalDataElement = entry.AdditionalData.find(
            std::string(dbusToRfInfo.argsInfo[index].name));

        if (additionalDataElement == entry.AdditionalData.end())
        {
            BMCWEB_LOG_WARNING(
                "Could not find expected arg '{}' in additional data",
                dbusToRfInfo.argsInfo[index].name);
            continue;
        }
        args.push_back(additionalDataElement->second);
    }
    return MappedInfo{dbusToRfInfo.registryName, dbusToRfInfo.redfishMessageId,
                      args};
}

bool DbusEventLogEntryToEventLogObjectsType(const DbusEventLogEntry& entry,
                                            EventLogObjectsType& event)
{
    auto optDBusToRfInfo = getMappedInfo(entry);
    if (!optDBusToRfInfo.has_value())
    {
        BMCWEB_LOG_ERROR(
            "Could not find entry for the '{}' message in the DBusToRedfish map",
            entry.Message);
        return false;
    }

    // Get the Message from the MessageRegistry
    const auto registry =
        registries::getRegistryFromPrefix(optDBusToRfInfo->registryName);
    if (!registry)
    {
        BMCWEB_LOG_WARNING("Could not find registry '{}' for log entry {}",
                           optDBusToRfInfo->registryName, entry.Id);
        return false;
    }

    const auto registryEntry = registry->get();
    const registries::Message* message = registries::getMessageFromRegistry(
        optDBusToRfInfo->registryEntryName, registryEntry.entries);
    if (message == nullptr)
    {
        BMCWEB_LOG_WARNING(
            "Could not find message '{}' for log entry {} in registry '{}'",
            optDBusToRfInfo->registryEntryName, entry.Id,
            optDBusToRfInfo->registryName);
        return false;
    }

    std::string messageId = std::format(
        "{}.{}.{}.{}", std::string_view(optDBusToRfInfo->registryName),
        registryEntry.header.versionMajor, registryEntry.header.versionMinor,
        std::string_view(optDBusToRfInfo->registryEntryName));

    std::vector<std::string_view> messageArgsView(
        optDBusToRfInfo->messageArgs.begin(),
        optDBusToRfInfo->messageArgs.end());
    std::string msg =
        redfish::registries::fillMessageArgs(messageArgsView, message->message);
    if (msg.empty())
    {
        BMCWEB_LOG_WARNING(
            "Message is empty after filling fillMessageArgs for log entry {}",
            entry.Id);
        return false;
    }

    event.id = std::to_string(entry.Id);
    event.message = std::move(msg);
    event.messageId = std::move(messageId);
    event.messageArgs = optDBusToRfInfo->messageArgs;
    event.resolution = message->resolution;
    event.severity = message->messageSeverity;
    event.timestamp = redfish::time_utils::getDateTimeUintMs(entry.Timestamp);

    return true;
}
} // namespace dbus_registries_map
} // namespace redfish
