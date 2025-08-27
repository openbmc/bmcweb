// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
// SPDX-FileCopyrightText: Copyright 2018 Intel Corporation
#pragma once

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/log_service.hpp"
#include "http_response.hpp"
#include "logging.hpp"
#include "registries.hpp"
#include "str_utility.hpp"
#include "utils/dbus_event_log_entry.hpp"
#include "utils/etag_utils.hpp"
#include "utils/log_services_utils.hpp"
#include "utils/query_param.hpp"
#include "utils/time_utils.hpp"

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
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace redfish
{
namespace eventlog_utils
{

constexpr const char* rfSystemsStr = "Systems";
constexpr const char* rfManagersStr = "Managers";

enum class LogServiceParent
{
    Systems,
    Managers
};

inline std::string logServiceParentToString(LogServiceParent parent)
{
    std::string parentStr;
    switch (parent)
    {
        case LogServiceParent::Managers:
            parentStr = rfManagersStr;
            break;
        case LogServiceParent::Systems:
            parentStr = rfSystemsStr;
            break;
        default:
            BMCWEB_LOG_ERROR("Unable to stringify bmcweb eventlog location");
            break;
    }
    return parentStr;
}

inline std::string_view getChildIdFromParent(LogServiceParent parent)
{
    std::string_view childId;

    switch (parent)
    {
        case LogServiceParent::Managers:
            childId = BMCWEB_REDFISH_MANAGER_URI_NAME;
            break;
        case LogServiceParent::Systems:
            childId = BMCWEB_REDFISH_SYSTEM_URI_NAME;
            break;
        default:
            BMCWEB_LOG_ERROR(
                "Unable to stringify bmcweb eventlog location childId");
            break;
    }
    return childId;
}

inline std::string getLogEntryDescriptor(LogServiceParent parent)
{
    std::string descriptor;
    switch (parent)
    {
        case LogServiceParent::Managers:
            descriptor = "Manager";
            break;
        case LogServiceParent::Systems:
            descriptor = "System";
            break;
        default:
            BMCWEB_LOG_ERROR("Unable to get Log Entry descriptor");
            break;
    }
    return descriptor;
}

inline void handleSystemsAndManagersEventLogServiceGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    LogServiceParent parent)
{
    const std::string parentStr = logServiceParentToString(parent);
    const std::string_view childId = getChildIdFromParent(parent);
    const std::string logEntryDescriptor = getLogEntryDescriptor(parent);

    if (parentStr.empty() || childId.empty() || logEntryDescriptor.empty())
    {
        messages::internalError(asyncResp->res);
        return;
    }

    asyncResp->res.jsonValue["@odata.id"] = std::format(
        "/redfish/v1/{}/{}/LogServices/EventLog", parentStr, childId);
    asyncResp->res.jsonValue["@odata.type"] = "#LogService.v1_2_0.LogService";
    asyncResp->res.jsonValue["Name"] = "Event Log Service";
    asyncResp->res.jsonValue["Description"] =
        std::format("{} Event Log Service", logEntryDescriptor);
    asyncResp->res.jsonValue["Id"] = "EventLog";
    asyncResp->res.jsonValue["OverWritePolicy"] =
        log_service::OverWritePolicy::WrapsWhenFull;

    std::pair<std::string, std::string> redfishDateTimeOffset =
        redfish::time_utils::getDateTimeOffsetNow();

    asyncResp->res.jsonValue["DateTime"] = redfishDateTimeOffset.first;
    asyncResp->res.jsonValue["DateTimeLocalOffset"] =
        redfishDateTimeOffset.second;

    asyncResp->res.jsonValue["Entries"]["@odata.id"] = std::format(
        "/redfish/v1/{}/{}/LogServices/EventLog/Entries", parentStr, childId);
    asyncResp->res.jsonValue["Actions"]["#LogService.ClearLog"]["target"]

        = std::format(
            "/redfish/v1/{}/{}/LogServices/EventLog/Actions/LogService.ClearLog",
            parentStr, childId);
    etag_utils::setEtagOmitDateTimeHandler(asyncResp);
}

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
    nlohmann::json::object_t& logEntryJson, const std::string& parentStr,
    const std::string_view childId, const std::string& logEntryDescriptor)
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
    // format which matches the Redfish format except for the
    // fractional seconds between the '.' and the '+', so just remove them.
    std::size_t dot = timestamp.find_first_of('.');
    std::size_t plus = timestamp.find_first_of('+');
    if (dot != std::string::npos && plus != std::string::npos)
    {
        timestamp.erase(dot, plus - dot);
    }

    // Fill in the log entry with the gathered data
    logEntryJson["@odata.type"] = "#LogEntry.v1_9_0.LogEntry";
    logEntryJson["@odata.id"] =
        boost::urls::format("/redfish/v1/{}/{}/LogServices/EventLog/Entries/{}",
                            parentStr, childId, logEntryID);
    logEntryJson["Name"] =
        std::format("{} Event Log Entry", logEntryDescriptor);
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

inline void handleSystemsAndManagersLogServiceEventLogLogEntryCollection(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    query_param::Query& delegatedQuery, LogServiceParent parent)
{
    size_t top = delegatedQuery.top.value_or(query_param::Query::maxTop);
    size_t skip = delegatedQuery.skip.value_or(0);

    const std::string parentStr = logServiceParentToString(parent);
    const std::string_view childId = getChildIdFromParent(parent);
    const std::string logEntryDescriptor = getLogEntryDescriptor(parent);

    if (parentStr.empty() || childId.empty() || logEntryDescriptor.empty())
    {
        messages::internalError(asyncResp->res);
        return;
    }

    // Collections don't include the static data added by SubRoute
    // because it has a duplicate entry for members
    asyncResp->res.jsonValue["@odata.type"] =
        "#LogEntryCollection.LogEntryCollection";
    asyncResp->res.jsonValue["@odata.id"] = std::format(
        "/redfish/v1/{}/{}/LogServices/EventLog/Entries", parentStr, childId);
    asyncResp->res.jsonValue["Name"] =
        std::format("{} Event Log Entries", logEntryDescriptor);
    asyncResp->res.jsonValue["Description"] =
        std::format("Collection of {} Event Log Entries", logEntryDescriptor);

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
                fillEventLogEntryJson(idStr, logEntry, bmcLogEntry, parentStr,
                                      childId, logEntryDescriptor);
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
                "/redfish/v1/{}/{}/LogServices/EventLog/Entries?$skip={}",
                parentStr, childId, std::to_string(skip + top));
    }
}

inline void handleSystemsAndManagersLogServiceEventLogEntriesGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& param, LogServiceParent parent)
{
    const std::string& targetID = param;

    const std::string parentStr = logServiceParentToString(parent);
    const std::string_view childId = getChildIdFromParent(parent);
    const std::string logEntryDescriptor = getLogEntryDescriptor(parent);

    if (parentStr.empty() || childId.empty() || logEntryDescriptor.empty())
    {
        messages::internalError(asyncResp->res);
        return;
    }

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
                LogParseError status = fillEventLogEntryJson(
                    idStr, logEntry, bmcLogEntry, parentStr, childId,
                    logEntryDescriptor);
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

inline void handleSystemsAndManagersLogServicesEventLogActionsClearPost(
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

/*
 * DBus EventLog utilities
 * */

inline std::optional<bool> getProviderNotifyAction(const std::string& notify)
{
    std::optional<bool> notifyAction;
    if (notify == "xyz.openbmc_project.Logging.Entry.Notify.Notify")
    {
        notifyAction = true;
    }
    else if (notify == "xyz.openbmc_project.Logging.Entry.Notify.Inhibit")
    {
        notifyAction = false;
    }

    return notifyAction;
}

inline std::string translateSeverityDbusToRedfish(const std::string& s)
{
    if ((s == "xyz.openbmc_project.Logging.Entry.Level.Alert") ||
        (s == "xyz.openbmc_project.Logging.Entry.Level.Critical") ||
        (s == "xyz.openbmc_project.Logging.Entry.Level.Emergency") ||
        (s == "xyz.openbmc_project.Logging.Entry.Level.Error"))
    {
        return "Critical";
    }
    if ((s == "xyz.openbmc_project.Logging.Entry.Level.Debug") ||
        (s == "xyz.openbmc_project.Logging.Entry.Level.Informational") ||
        (s == "xyz.openbmc_project.Logging.Entry.Level.Notice"))
    {
        return "OK";
    }
    if (s == "xyz.openbmc_project.Logging.Entry.Level.Warning")
    {
        return "Warning";
    }
    return "";
}

inline void fillEventLogLogEntryFromDbusLogEntry(
    const DbusEventLogEntry& entry, nlohmann::json& objectToFillOut,
    const std::string& parentStr, const std::string_view childId,
    const std::string& logEntryDescriptor)
{
    objectToFillOut["@odata.type"] = "#LogEntry.v1_9_0.LogEntry";
    objectToFillOut["@odata.id"] =
        boost::urls::format("/redfish/v1/{}/{}/LogServices/EventLog/Entries/{}",
                            parentStr, childId, std::to_string(entry.Id));
    objectToFillOut["Name"] =
        std::format("{} Event Log Entry", logEntryDescriptor);
    objectToFillOut["Id"] = std::to_string(entry.Id);
    objectToFillOut["Message"] = entry.Message;
    objectToFillOut["Resolved"] = entry.Resolved;
    std::optional<bool> notifyAction =
        getProviderNotifyAction(entry.ServiceProviderNotify);
    if (notifyAction)
    {
        objectToFillOut["ServiceProviderNotified"] = *notifyAction;
    }
    if ((entry.Resolution != nullptr) && !entry.Resolution->empty())
    {
        objectToFillOut["Resolution"] = *entry.Resolution;
    }
    objectToFillOut["EntryType"] = "Event";
    objectToFillOut["Severity"] =
        translateSeverityDbusToRedfish(entry.Severity);
    objectToFillOut["Created"] =
        redfish::time_utils::getDateTimeUintMs(entry.Timestamp);
    objectToFillOut["Modified"] =
        redfish::time_utils::getDateTimeUintMs(entry.UpdateTimestamp);
    if (entry.Path != nullptr)
    {
        objectToFillOut["AdditionalDataURI"] = boost::urls::format(
            "/redfish/v1/{}/{}/LogServices/EventLog/Entries/{}/attachment",
            parentStr, childId, std::to_string(entry.Id));
    }
}

inline void afterLogEntriesGetManagedObjects(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& parentStr, const std::string_view childId,
    const std::string& logEntryDescriptor, const boost::system::error_code& ec,
    const dbus::utility::ManagedObjectType& resp)
{
    if (ec)
    {
        // TODO Handle for specific error code
        BMCWEB_LOG_ERROR("getLogEntriesIfaceData resp_handler got error {}",
                         ec);
        messages::internalError(asyncResp->res);
        return;
    }
    nlohmann::json::array_t entriesArray;
    for (const auto& objectPath : resp)
    {
        dbus::utility::DBusPropertiesMap propsFlattened;
        auto isEntry =
            std::ranges::find_if(objectPath.second, [](const auto& object) {
                return object.first == "xyz.openbmc_project.Logging.Entry";
            });
        if (isEntry == objectPath.second.end())
        {
            continue;
        }

        for (const auto& interfaceMap : objectPath.second)
        {
            for (const auto& propertyMap : interfaceMap.second)
            {
                propsFlattened.emplace_back(propertyMap.first,
                                            propertyMap.second);
            }
        }
        std::optional<DbusEventLogEntry> optEntry =
            fillDbusEventLogEntryFromPropertyMap(propsFlattened);

        if (!optEntry.has_value())
        {
            messages::internalError(asyncResp->res);
            return;
        }
        fillEventLogLogEntryFromDbusLogEntry(
            *optEntry, entriesArray.emplace_back(), parentStr, childId,
            logEntryDescriptor);
    }

    redfish::json_util::sortJsonArrayByKey(entriesArray, "Id");
    asyncResp->res.jsonValue["Members@odata.count"] = entriesArray.size();
    asyncResp->res.jsonValue["Members"] = std::move(entriesArray);
}

inline void dBusEventLogEntryCollection(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    LogServiceParent parent)
{
    const std::string_view childId = getChildIdFromParent(parent);
    const std::string parentStr = logServiceParentToString(parent);
    const std::string logEntryDescriptor = getLogEntryDescriptor(parent);

    if (parentStr.empty() || childId.empty() || logEntryDescriptor.empty())
    {
        messages::internalError(asyncResp->res);
        return;
    }

    // Collections don't include the static data added by SubRoute
    // because it has a duplicate entry for members
    asyncResp->res.jsonValue["@odata.type"] =
        "#LogEntryCollection.LogEntryCollection";
    asyncResp->res.jsonValue["@odata.id"] = std::format(
        "/redfish/v1/{}/{}/LogServices/EventLog/Entries", parentStr, childId);
    asyncResp->res.jsonValue["Name"] =
        std::format("{} Event Log Entries", logEntryDescriptor);
    asyncResp->res.jsonValue["Description"] =
        std::format("Collection of {} Event Log Entries", logEntryDescriptor);

    // DBus implementation of EventLog/Entries
    // Make call to Logging Service to find all log entry objects
    sdbusplus::message::object_path path("/xyz/openbmc_project/logging");
    dbus::utility::getManagedObjects(
        "xyz.openbmc_project.Logging", path,
        [asyncResp, parentStr, childId,
         logEntryDescriptor](const boost::system::error_code& ec,
                             const dbus::utility::ManagedObjectType& resp) {
            afterLogEntriesGetManagedObjects(asyncResp, parentStr, childId,
                                             logEntryDescriptor, ec, resp);
        });
}

inline void afterDBusEventLogEntryGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& parentStr, const std::string_view childId,
    const std::string& logEntryDescriptor, const std::string& entryID,
    const boost::system::error_code& ec,
    const dbus::utility::DBusPropertiesMap& resp)
{
    if (ec.value() == EBADR)
    {
        messages::resourceNotFound(asyncResp->res, "EventLogEntry", entryID);
        return;
    }
    if (ec)
    {
        BMCWEB_LOG_ERROR("EventLogEntry (DBus) resp_handler got error {}", ec);
        messages::internalError(asyncResp->res);
        return;
    }

    std::optional<DbusEventLogEntry> optEntry =
        fillDbusEventLogEntryFromPropertyMap(resp);

    if (!optEntry.has_value())
    {
        messages::internalError(asyncResp->res);
        return;
    }

    fillEventLogLogEntryFromDbusLogEntry(
        *optEntry, asyncResp->res.jsonValue, parentStr, childId,
        logEntryDescriptor);
}

inline void dBusEventLogEntryGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    LogServiceParent parent, std::string entryID)
{
    const std::string parentStr = logServiceParentToString(parent);
    const std::string_view childId = getChildIdFromParent(parent);
    const std::string logEntryDescriptor = getLogEntryDescriptor(parent);

    if (parentStr.empty() || childId.empty() || logEntryDescriptor.empty())
    {
        messages::internalError(asyncResp->res);
        return;
    }

    dbus::utility::escapePathForDbus(entryID);

    // DBus implementation of EventLog/Entries
    // Make call to Logging Service to find all log entry objects
    dbus::utility::getAllProperties(
        "xyz.openbmc_project.Logging",
        "/xyz/openbmc_project/logging/entry/" + entryID, "",
        std::bind_front(afterDBusEventLogEntryGet, asyncResp, parentStr,
                        childId, logEntryDescriptor, entryID));
}

inline void dBusEventLogEntryPatch(
    const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& entryId)
{
    std::optional<bool> resolved;

    if (!json_util::readJsonPatch(req, asyncResp->res, "Resolved", resolved))
    {
        return;
    }
    BMCWEB_LOG_DEBUG("Set Resolved");

    setDbusProperty(asyncResp, "Resolved", "xyz.openbmc_project.Logging",
                    "/xyz/openbmc_project/logging/entry/" + entryId,
                    "xyz.openbmc_project.Logging.Entry", "Resolved",
                    resolved.value_or(false));
}

inline void dBusEventLogEntryDelete(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp, std::string entryID)
{
    BMCWEB_LOG_DEBUG("Do delete single event entries.");

    dbus::utility::escapePathForDbus(entryID);

    // Process response from Logging service.
    auto respHandler = [asyncResp,
                        entryID](const boost::system::error_code& ec) {
        BMCWEB_LOG_DEBUG("EventLogEntry (DBus) doDelete callback: Done");
        if (ec)
        {
            if (ec.value() == EBADR)
            {
                messages::resourceNotFound(asyncResp->res, "LogEntry", entryID);
                return;
            }
            // TODO Handle for specific error code
            BMCWEB_LOG_ERROR(
                "EventLogEntry (DBus) doDelete respHandler got error {}", ec);
            asyncResp->res.result(
                boost::beast::http::status::internal_server_error);
            return;
        }

        messages::success(asyncResp->res);
    };

    // Make call to Logging service to request Delete Log
    dbus::utility::async_method_call(
        asyncResp, respHandler, "xyz.openbmc_project.Logging",
        "/xyz/openbmc_project/logging/entry/" + entryID,
        "xyz.openbmc_project.Object.Delete", "Delete");
}

inline void dBusLogServiceActionsClear(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    BMCWEB_LOG_DEBUG("Do delete all entries.");

    // Process response from Logging service.
    auto respHandler = [asyncResp](const boost::system::error_code& ec) {
        BMCWEB_LOG_DEBUG("doClearLog resp_handler callback: Done");
        if (ec)
        {
            // TODO Handle for specific error code
            BMCWEB_LOG_ERROR("doClearLog resp_handler got error {}", ec);
            asyncResp->res.result(
                boost::beast::http::status::internal_server_error);
            return;
        }

        messages::success(asyncResp->res);
    };

    // Make call to Logging service to request Clear Log
    dbus::utility::async_method_call(
        asyncResp, respHandler, "xyz.openbmc_project.Logging",
        "/xyz/openbmc_project/logging",
        "xyz.openbmc_project.Collection.DeleteAll", "DeleteAll");
}

inline void downloadEventLogEntry(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& entryID, const std::string& downloadEntryType)
{
    std::string entryPath =
        sdbusplus::message::object_path("/xyz/openbmc_project/logging/entry") /
        entryID;

    auto downloadEventLogEntryHandler =
        [asyncResp, entryID,
         downloadEntryType](const boost::system::error_code& ec,
                            const sdbusplus::message::unix_fd& unixfd) {
            log_services_utils::downloadEntryCallback(
                asyncResp, entryID, downloadEntryType, ec, unixfd);
        };

    dbus::utility::async_method_call(
        asyncResp, std::move(downloadEventLogEntryHandler),
        "xyz.openbmc_project.Logging", entryPath,
        "xyz.openbmc_project.Logging.Entry", "GetEntry");
}
} // namespace eventlog_utils
} // namespace redfish
