// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
// SPDX-FileCopyrightText: Copyright 2020 Intel Corporation
#include "event_log.hpp"

#include "logging.hpp"
#include "registries.hpp"
#include "str_utility.hpp"

#include <nlohmann/json.hpp>

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace redfish
{

namespace event_log
{

bool getUniqueEntryID(const std::string& logEntry, std::string& entryID)
{
    static time_t prevTs = 0;
    static int index = 0;

    // Get the entry timestamp
    std::time_t curTs = 0;
    std::tm timeStruct = {};
    std::istringstream entryStream(logEntry);
    if (entryStream >> std::get_time(&timeStruct, "%Y-%m-%dT%H:%M:%S"))
    {
        curTs = std::mktime(&timeStruct);
        if (curTs == -1)
        {
            return false;
        }
    }
    // If the timestamp isn't unique, increment the index
    index = (curTs == prevTs) ? index + 1 : 0;

    // Save the timestamp
    prevTs = curTs;

    entryID = std::to_string(curTs);
    if (index > 0)
    {
        entryID += "_" + std::to_string(index);
    }
    return true;
}

int getEventLogParams(const std::string& logEntry, std::string& timestamp,
                      std::string& messageID,
                      std::vector<std::string>& messageArgs)
{
    // The redfish log format is "<Timestamp> <MessageId>,<MessageArgs>"
    // First get the Timestamp
    size_t space = logEntry.find_first_of(' ');
    if (space == std::string::npos)
    {
        BMCWEB_LOG_ERROR("EventLog Params: could not find first space: {}",
                         logEntry);
        return -EINVAL;
    }
    timestamp = logEntry.substr(0, space);
    // Then get the log contents
    size_t entryStart = logEntry.find_first_not_of(' ', space);
    if (entryStart == std::string::npos)
    {
        BMCWEB_LOG_ERROR("EventLog Params: could not find log contents: {}",
                         logEntry);
        return -EINVAL;
    }
    std::string_view entry(logEntry);
    entry.remove_prefix(entryStart);
    // Use split to separate the entry into its fields
    std::vector<std::string> logEntryFields;
    bmcweb::split(logEntryFields, entry, ',');
    // We need at least a MessageId to be valid
    if (logEntryFields.empty())
    {
        BMCWEB_LOG_ERROR("EventLog Params: could not find entry fields: {}",
                         logEntry);
        return -EINVAL;
    }
    messageID = logEntryFields[0];

    // Get the MessageArgs from the log if there are any
    if (logEntryFields.size() > 1)
    {
        const std::string& messageArgsStart = logEntryFields[1];
        // If the first string is empty, assume there are no MessageArgs
        if (!messageArgsStart.empty())
        {
            messageArgs.assign(logEntryFields.begin() + 1,
                               logEntryFields.end());
        }
    }

    return 0;
}

int formatEventLogEntry(uint64_t eventId, const std::string& logEntryID,
                        const std::string& messageID,
                        const std::span<std::string_view> messageArgs,
                        std::string timestamp, const std::string& customText,
                        nlohmann::json::object_t& logEntryJson)
{
    std::optional<registries::MessageId> msgComponents =
        registries::getMessageComponents(messageID);
    if (msgComponents == std::nullopt)
    {
        BMCWEB_LOG_DEBUG("Could not get Message components");
        return -1;
    }

    std::optional<registries::RegistryEntryRef> registry =
        registries::getRegistryFromPrefix(msgComponents->registryName);
    if (!registry)
    {
        BMCWEB_LOG_DEBUG("Could not get registry from prefix");
        return -1;
    }

    // Get the Message from the MessageKey and RegistryEntries
    const registries::Message* message = registries::getMessageFromRegistry(
        msgComponents->messageKey, registry->get().entries);

    if (message == nullptr)
    {
        BMCWEB_LOG_DEBUG(
            "Could not find MessageKey '{}' for log entry {} in registry",
            msgComponents->messageKey, logEntryID);
        return -1;
    }

    std::string msg =
        redfish::registries::fillMessageArgs(messageArgs, message->message);
    if (msg.empty())
    {
        BMCWEB_LOG_DEBUG("Message is empty after filling fillMessageArgs");
        return -1;
    }

    // Get the Created time from the timestamp. The log timestamp is in
    // RFC3339 format which matches the Redfish format except for the
    // fractional seconds between the '.' and the '+', so just remove them.
    std::size_t dot = timestamp.find_first_of('.');
    std::size_t plus = timestamp.find_first_of('+', dot);
    if (dot != std::string::npos && plus != std::string::npos)
    {
        timestamp.erase(dot, plus - dot);
    }

    const unsigned int& versionMajor = registry->get().header.versionMajor;
    const unsigned int& versionMinor = registry->get().header.versionMinor;

    // Fill in the log entry with the gathered data
    logEntryJson["EventId"] = std::to_string(eventId);

    logEntryJson["Severity"] = message->messageSeverity;
    logEntryJson["Message"] = std::move(msg);
    logEntryJson["MessageId"] =
        std::format("{}.{}.{}.{}", msgComponents->registryName, versionMajor,
                    versionMinor, msgComponents->messageKey);
    logEntryJson["MessageArgs"] = messageArgs;
    logEntryJson["EventTimestamp"] = std::move(timestamp);
    logEntryJson["Context"] = customText;
    return 0;
}

} // namespace event_log

} // namespace redfish
