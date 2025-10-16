// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
// SPDX-FileCopyrightText: Copyright 2018 Intel Corporation
#pragma once

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "http_response.hpp"
#include "logging.hpp"
#include "registries.hpp"
#include "str_utility.hpp"
#include "utils/query_param.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/system/linux_error.hpp>
#include <boost/url/format.hpp>
#include <boost/url/url.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/message/native_types.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>

namespace redfish
{
namespace eventlog_utils
{

/*
 * Journal EventLog utilities
 * */

inline bool getRedfishLogFiles(
    std::vector<std::filesystem::path>& redfishLogFiles)
{
    static const std::filesystem::path redfishLogDir = "/var/log";
    static const std::string redfishLogFilename = "redfish";

    // Loop through the directory looking for redfish log files
    for (const std::filesystem::directory_entry& dirEnt :
         std::filesystem::directory_iterator(redfishLogDir))
    {
        // If we find a redfish log file, save the path
        std::string filename = dirEnt.path().filename();
        if (filename.starts_with(redfishLogFilename))
        {
            redfishLogFiles.emplace_back(redfishLogDir / filename);
        }
    }
    // As the log files rotate, they are appended with a ".#" that is higher for
    // the older logs. Since we don't expect more than 10 log files, we
    // can just sort the list to get them in order from newest to oldest
    std::ranges::sort(redfishLogFiles);

    return !redfishLogFiles.empty();
}

inline bool getUniqueEntryID(const std::string& logEntry, std::string& entryID,
                             const bool firstEntry = true)
{
    static time_t prevTs = 0;
    static int index = 0;
    if (firstEntry)
    {
        prevTs = 0;
    }

    // Get the entry timestamp
    std::time_t curTs = 0;
    std::tm timeStruct = {};
    std::istringstream entryStream(logEntry);
    if (entryStream >> std::get_time(&timeStruct, "%Y-%m-%dT%H:%M:%S"))
    {
        curTs = std::mktime(&timeStruct);
    }
    // If the timestamp isn't unique, increment the index
    if (curTs == prevTs)
    {
        index++;
    }
    else
    {
        // Otherwise, reset it
        index = 0;
    }
    // Save the timestamp
    prevTs = curTs;

    entryID = std::to_string(curTs);
    if (index > 0)
    {
        entryID += "_" + std::to_string(index);
    }
    return true;
}

enum class LogParseError
{
    success,
    parseFailed,
    messageIdNotInRegistry,
};

static LogParseError fillEventLogEntryJson(
    const std::string& logEntryID, const std::string& logEntry,
    nlohmann::json::object_t& logEntryJson)
{
    // The redfish log format is "<Timestamp> <MessageId>,<MessageArgs>"
    // First get the Timestamp
    size_t space = logEntry.find_first_of(' ');
    if (space == std::string::npos)
    {
        return LogParseError::parseFailed;
    }
    std::string timestamp = logEntry.substr(0, space);
    // Then get the log contents
    size_t entryStart = logEntry.find_first_not_of(' ', space);
    if (entryStart == std::string::npos)
    {
        return LogParseError::parseFailed;
    }
    std::string_view entry(logEntry);
    entry.remove_prefix(entryStart);
    // Use split to separate the entry into its fields
    std::vector<std::string> logEntryFields;
    bmcweb::split(logEntryFields, entry, ',');
    // We need at least a MessageId to be valid
    auto logEntryIter = logEntryFields.begin();
    if (logEntryIter == logEntryFields.end())
    {
        return LogParseError::parseFailed;
    }
    std::string& messageID = *logEntryIter;

    std::optional<registries::MessageId> msgComponents =
        registries::getMessageComponents(messageID);
    if (!msgComponents)
    {
        return LogParseError::parseFailed;
    }

    std::optional<registries::RegistryEntryRef> registry =
        registries::getRegistryFromPrefix(msgComponents->registryName);
    if (!registry)
    {
        return LogParseError::messageIdNotInRegistry;
    }

    // Get the Message from the MessageKey and RegistryEntries
    const registries::Message* message = registries::getMessageFromRegistry(
        msgComponents->messageKey, registry->get().entries);

    logEntryIter++;
    if (message == nullptr)
    {
        BMCWEB_LOG_WARNING("Log entry not found in registry: {}", logEntry);
        return LogParseError::messageIdNotInRegistry;
    }

    const unsigned int& versionMajor = registry->get().header.versionMajor;
    const unsigned int& versionMinor = registry->get().header.versionMinor;

    std::vector<std::string_view> messageArgs(logEntryIter,
                                              logEntryFields.end());
    messageArgs.resize(message->numberOfArgs);

    std::string msg =
        redfish::registries::fillMessageArgs(messageArgs, message->message);
    if (msg.empty())
    {
        return LogParseError::parseFailed;
    }

    // Get the Created time from the timestamp. The log timestamp is in RFC3339
    // format which matches the Redfish format except for the fractional seconds
    // between the '.' and the '+', so just remove them.
    std::size_t dot = timestamp.find_first_of('.');
    std::size_t plus = timestamp.find_first_of('+');
    if (dot != std::string::npos && plus != std::string::npos)
    {
        timestamp.erase(dot, plus - dot);
    }

    // Fill in the log entry with the gathered data
    logEntryJson["@odata.type"] = "#LogEntry.v1_9_0.LogEntry";
    logEntryJson["@odata.id"] = boost::urls::format(
        "/redfish/v1/Systems/{}/LogServices/EventLog/Entries/{}",
        BMCWEB_REDFISH_SYSTEM_URI_NAME, logEntryID);
    logEntryJson["Name"] = "System Event Log Entry";
    logEntryJson["Id"] = logEntryID;
    logEntryJson["Message"] = std::move(msg);
    logEntryJson["MessageId"] =
        std::format("{}.{}.{}.{}", msgComponents->registryName, versionMajor,
                    versionMinor, msgComponents->messageKey);
    logEntryJson["MessageArgs"] = messageArgs;
    logEntryJson["EntryType"] = "Event";
    logEntryJson["Severity"] = message->messageSeverity;
    logEntryJson["Created"] = std::move(timestamp);
    return LogParseError::success;
}

inline void handleRequestSystemsLogServiceEventLogLogEntryCollection(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    query_param::Query& delegatedQuery)
{
    size_t top = delegatedQuery.top.value_or(query_param::Query::maxTop);
    size_t skip = delegatedQuery.skip.value_or(0);

    // Collections don't include the static data added by SubRoute
    // because it has a duplicate entry for members
    asyncResp->res.jsonValue["@odata.type"] =
        "#LogEntryCollection.LogEntryCollection";
    asyncResp->res.jsonValue["@odata.id"] =
        std::format("/redfish/v1/Systems/{}/LogServices/EventLog/Entries",
                    BMCWEB_REDFISH_SYSTEM_URI_NAME);
    asyncResp->res.jsonValue["Name"] = "System Event Log Entries";
    asyncResp->res.jsonValue["Description"] =
        "Collection of System Event Log Entries";

    nlohmann::json& logEntryArray = asyncResp->res.jsonValue["Members"];
    logEntryArray = nlohmann::json::array();
    // Go through the log files and create a unique ID for each
    // entry
    std::vector<std::filesystem::path> redfishLogFiles;
    getRedfishLogFiles(redfishLogFiles);
    uint64_t entryCount = 0;
    std::string logEntry;

    // Oldest logs are in the last file, so start there and loop
    // backwards
    for (auto it = redfishLogFiles.rbegin(); it < redfishLogFiles.rend(); it++)
    {
        std::ifstream logStream(*it);
        if (!logStream.is_open())
        {
            continue;
        }

        // Reset the unique ID on the first entry
        bool firstEntry = true;
        while (std::getline(logStream, logEntry))
        {
            std::string idStr;
            if (!getUniqueEntryID(logEntry, idStr, firstEntry))
            {
                continue;
            }
            firstEntry = false;

            nlohmann::json::object_t bmcLogEntry;
            LogParseError status =
                fillEventLogEntryJson(idStr, logEntry, bmcLogEntry);
            if (status == LogParseError::messageIdNotInRegistry)
            {
                continue;
            }
            if (status != LogParseError::success)
            {
                messages::internalError(asyncResp->res);
                return;
            }

            entryCount++;
            // Handle paging using skip (number of entries to skip from the
            // start) and top (number of entries to display)
            if (entryCount <= skip || entryCount > skip + top)
            {
                continue;
            }

            logEntryArray.emplace_back(std::move(bmcLogEntry));
        }
    }
    asyncResp->res.jsonValue["Members@odata.count"] = entryCount;
    if (skip + top < entryCount)
    {
        asyncResp->res.jsonValue["Members@odata.nextLink"] =
            boost::urls::format(
                "/redfish/v1/Systems/{}/LogServices/EventLog/Entries?$skip={}",
                BMCWEB_REDFISH_SYSTEM_URI_NAME, std::to_string(skip + top));
    }
}

inline void handleRequestSystemsLogServiceEventLogEntriesGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& param)
{
    const std::string& targetID = param;

    // Go through the log files and check the unique ID for each
    // entry to find the target entry
    std::vector<std::filesystem::path> redfishLogFiles;
    getRedfishLogFiles(redfishLogFiles);
    std::string logEntry;

    // Oldest logs are in the last file, so start there and loop
    // backwards
    for (auto it = redfishLogFiles.rbegin(); it < redfishLogFiles.rend(); it++)
    {
        std::ifstream logStream(*it);
        if (!logStream.is_open())
        {
            continue;
        }

        // Reset the unique ID on the first entry
        bool firstEntry = true;
        while (std::getline(logStream, logEntry))
        {
            std::string idStr;
            if (!getUniqueEntryID(logEntry, idStr, firstEntry))
            {
                continue;
            }
            firstEntry = false;

            if (idStr == targetID)
            {
                nlohmann::json::object_t bmcLogEntry;
                LogParseError status =
                    fillEventLogEntryJson(idStr, logEntry, bmcLogEntry);
                if (status != LogParseError::success)
                {
                    messages::internalError(asyncResp->res);
                    return;
                }
                asyncResp->res.jsonValue.update(bmcLogEntry);
                return;
            }
        }
    }
    // Requested ID was not found
    messages::resourceNotFound(asyncResp->res, "LogEntry", targetID);
}

inline void handleRequestSystemsLogServicesEventLogActionsClearPost(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    // Clear the EventLog by deleting the log files
    std::vector<std::filesystem::path> redfishLogFiles;
    if (getRedfishLogFiles(redfishLogFiles))
    {
        for (const std::filesystem::path& file : redfishLogFiles)
        {
            std::error_code ec;
            std::filesystem::remove(file, ec);
        }
    }

    // Reload rsyslog so it knows to start new log files
    dbus::utility::async_method_call(
        asyncResp,
        [asyncResp](const boost::system::error_code& ec) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("Failed to reload rsyslog: {}", ec);
                messages::internalError(asyncResp->res);
                return;
            }

            messages::success(asyncResp->res);
        },
        "org.freedesktop.systemd1", "/org/freedesktop/systemd1",
        "org.freedesktop.systemd1.Manager", "ReloadUnit", "rsyslog.service",
        "replace");
}
} // namespace eventlog_utils
} // namespace redfish
