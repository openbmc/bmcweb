/*
// Copyright (c) 2018 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
#pragma once

#include "node.hpp"
#include "registries.hpp"
#include "registries/base_message_registry.hpp"
#include "registries/openbmc_message_registry.hpp"

#include <systemd/sd-journal.h>

#include <boost/algorithm/string/split.hpp>
#include <boost/beast/core/span.hpp>
#include <boost/container/flat_map.hpp>
#include <error_messages.hpp>
#include <filesystem>
#include <string_view>
#include <variant>

namespace redfish
{

constexpr char const *CrashdumpObject = "com.intel.crashdump";
constexpr char const *CrashdumpPath = "/com/intel/crashdump";
constexpr char const *CrashdumpOnDemandPath = "/com/intel/crashdump/OnDemand";
constexpr char const *CrashdumpInterface = "com.intel.crashdump";
constexpr char const *CrashdumpOnDemandInterface =
    "com.intel.crashdump.OnDemand";
constexpr char const *CrashdumpRawPECIInterface =
    "com.intel.crashdump.SendRawPeci";

namespace message_registries
{
static const Message *getMessageFromRegistry(
    const std::string &messageKey,
    const boost::beast::span<const MessageEntry> registry)
{
    boost::beast::span<const MessageEntry>::const_iterator messageIt =
        std::find_if(registry.cbegin(), registry.cend(),
                     [&messageKey](const MessageEntry &messageEntry) {
                         return !std::strcmp(messageEntry.first,
                                             messageKey.c_str());
                     });
    if (messageIt != registry.cend())
    {
        return &messageIt->second;
    }

    return nullptr;
}

static const Message *getMessage(const std::string_view &messageID)
{
    // Redfish MessageIds are in the form
    // RegistryName.MajorVersion.MinorVersion.MessageKey, so parse it to find
    // the right Message
    std::vector<std::string> fields;
    fields.reserve(4);
    boost::split(fields, messageID, boost::is_any_of("."));
    std::string &registryName = fields[0];
    std::string &messageKey = fields[3];

    // Find the right registry and check it for the MessageKey
    if (std::string(base::header.registryPrefix) == registryName)
    {
        return getMessageFromRegistry(
            messageKey, boost::beast::span<const MessageEntry>(base::registry));
    }
    if (std::string(openbmc::header.registryPrefix) == registryName)
    {
        return getMessageFromRegistry(
            messageKey,
            boost::beast::span<const MessageEntry>(openbmc::registry));
    }
    return nullptr;
}
} // namespace message_registries

namespace fs = std::filesystem;

using GetManagedPropertyType = boost::container::flat_map<
    std::string,
    sdbusplus::message::variant<std::string, bool, uint8_t, int16_t, uint16_t,
                                int32_t, uint32_t, int64_t, uint64_t, double>>;

using GetManagedObjectsType = boost::container::flat_map<
    sdbusplus::message::object_path,
    boost::container::flat_map<std::string, GetManagedPropertyType>>;

inline std::string translateSeverityDbusToRedfish(const std::string &s)
{
    if (s == "xyz.openbmc_project.Logging.Entry.Level.Alert")
    {
        return "Critical";
    }
    else if (s == "xyz.openbmc_project.Logging.Entry.Level.Critical")
    {
        return "Critical";
    }
    else if (s == "xyz.openbmc_project.Logging.Entry.Level.Debug")
    {
        return "OK";
    }
    else if (s == "xyz.openbmc_project.Logging.Entry.Level.Emergency")
    {
        return "Critical";
    }
    else if (s == "xyz.openbmc_project.Logging.Entry.Level.Error")
    {
        return "Critical";
    }
    else if (s == "xyz.openbmc_project.Logging.Entry.Level.Informational")
    {
        return "OK";
    }
    else if (s == "xyz.openbmc_project.Logging.Entry.Level.Notice")
    {
        return "OK";
    }
    else if (s == "xyz.openbmc_project.Logging.Entry.Level.Warning")
    {
        return "Warning";
    }
    return "";
}

static int getJournalMetadata(sd_journal *journal,
                              const std::string_view &field,
                              std::string_view &contents)
{
    const char *data = nullptr;
    size_t length = 0;
    int ret = 0;
    // Get the metadata from the requested field of the journal entry
    ret = sd_journal_get_data(journal, field.data(), (const void **)&data,
                              &length);
    if (ret < 0)
    {
        return ret;
    }
    contents = std::string_view(data, length);
    // Only use the content after the "=" character.
    contents.remove_prefix(std::min(contents.find("=") + 1, contents.size()));
    return ret;
}

static int getJournalMetadata(sd_journal *journal,
                              const std::string_view &field, const int &base,
                              int &contents)
{
    int ret = 0;
    std::string_view metadata;
    // Get the metadata from the requested field of the journal entry
    ret = getJournalMetadata(journal, field, metadata);
    if (ret < 0)
    {
        return ret;
    }
    contents = strtol(metadata.data(), nullptr, base);
    return ret;
}

static bool getEntryTimestamp(sd_journal *journal, std::string &entryTimestamp)
{
    int ret = 0;
    uint64_t timestamp = 0;
    ret = sd_journal_get_realtime_usec(journal, &timestamp);
    if (ret < 0)
    {
        BMCWEB_LOG_ERROR << "Failed to read entry timestamp: "
                         << strerror(-ret);
        return false;
    }
    time_t t =
        static_cast<time_t>(timestamp / 1000 / 1000); // Convert from us to s
    struct tm *loctime = localtime(&t);
    char entryTime[64] = {};
    if (NULL != loctime)
    {
        strftime(entryTime, sizeof(entryTime), "%FT%T%z", loctime);
    }
    // Insert the ':' into the timezone
    std::string_view t1(entryTime);
    std::string_view t2(entryTime);
    if (t1.size() > 2 && t2.size() > 2)
    {
        t1.remove_suffix(2);
        t2.remove_prefix(t2.size() - 2);
    }
    entryTimestamp = std::string(t1) + ":" + std::string(t2);
    return true;
}

static bool getSkipParam(crow::Response &res, const crow::Request &req,
                         long &skip)
{
    char *skipParam = req.urlParams.get("$skip");
    if (skipParam != nullptr)
    {
        char *ptr = nullptr;
        skip = std::strtol(skipParam, &ptr, 10);
        if (*skipParam == '\0' || *ptr != '\0')
        {

            messages::queryParameterValueTypeError(res, std::string(skipParam),
                                                   "$skip");
            return false;
        }
        if (skip < 0)
        {

            messages::queryParameterOutOfRange(res, std::to_string(skip),
                                               "$skip", "greater than 0");
            return false;
        }
    }
    return true;
}

static constexpr const long maxEntriesPerPage = 1000;
static bool getTopParam(crow::Response &res, const crow::Request &req,
                        long &top)
{
    char *topParam = req.urlParams.get("$top");
    if (topParam != nullptr)
    {
        char *ptr = nullptr;
        top = std::strtol(topParam, &ptr, 10);
        if (*topParam == '\0' || *ptr != '\0')
        {
            messages::queryParameterValueTypeError(res, std::string(topParam),
                                                   "$top");
            return false;
        }
        if (top < 1 || top > maxEntriesPerPage)
        {

            messages::queryParameterOutOfRange(
                res, std::to_string(top), "$top",
                "1-" + std::to_string(maxEntriesPerPage));
            return false;
        }
    }
    return true;
}

static bool getUniqueEntryID(sd_journal *journal, std::string &entryID)
{
    int ret = 0;
    static uint64_t prevTs = 0;
    static int index = 0;
    // Get the entry timestamp
    uint64_t curTs = 0;
    ret = sd_journal_get_realtime_usec(journal, &curTs);
    if (ret < 0)
    {
        BMCWEB_LOG_ERROR << "Failed to read entry timestamp: "
                         << strerror(-ret);
        return false;
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

static bool getUniqueEntryID(const std::string &logEntry, std::string &entryID)
{
    static uint64_t prevTs = 0;
    static int index = 0;
    // Get the entry timestamp
    uint64_t curTs = 0;
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

static bool getTimestampFromID(crow::Response &res, const std::string &entryID,
                               uint64_t &timestamp, uint16_t &index)
{
    if (entryID.empty())
    {
        return false;
    }
    // Convert the unique ID back to a timestamp to find the entry
    std::string_view tsStr(entryID);

    auto underscorePos = tsStr.find("_");
    if (underscorePos != tsStr.npos)
    {
        // Timestamp has an index
        tsStr.remove_suffix(tsStr.size() - underscorePos);
        std::string_view indexStr(entryID);
        indexStr.remove_prefix(underscorePos + 1);
        std::size_t pos;
        try
        {
            index = std::stoul(std::string(indexStr), &pos);
        }
        catch (std::invalid_argument)
        {
            messages::resourceMissingAtURI(res, entryID);
            return false;
        }
        catch (std::out_of_range)
        {
            messages::resourceMissingAtURI(res, entryID);
            return false;
        }
        if (pos != indexStr.size())
        {
            messages::resourceMissingAtURI(res, entryID);
            return false;
        }
    }
    // Timestamp has no index
    std::size_t pos;
    try
    {
        timestamp = std::stoull(std::string(tsStr), &pos);
    }
    catch (std::invalid_argument)
    {
        messages::resourceMissingAtURI(res, entryID);
        return false;
    }
    catch (std::out_of_range)
    {
        messages::resourceMissingAtURI(res, entryID);
        return false;
    }
    if (pos != tsStr.size())
    {
        messages::resourceMissingAtURI(res, entryID);
        return false;
    }
    return true;
}

static bool
    getRedfishLogFiles(std::vector<std::filesystem::path> &redfishLogFiles)
{
    static const std::filesystem::path redfishLogDir = "/var/log";
    static const std::string redfishLogFilename = "redfish";

    // Loop through the directory looking for redfish log files
    for (const std::filesystem::directory_entry &dirEnt :
         std::filesystem::directory_iterator(redfishLogDir))
    {
        // If we find a redfish log file, save the path
        std::string filename = dirEnt.path().filename();
        if (boost::starts_with(filename, redfishLogFilename))
        {
            redfishLogFiles.emplace_back(redfishLogDir / filename);
        }
    }
    // As the log files rotate, they are appended with a ".#" that is higher for
    // the older logs. Since we don't expect more than 10 log files, we
    // can just sort the list to get them in order from newest to oldest
    std::sort(redfishLogFiles.begin(), redfishLogFiles.end());

    return !redfishLogFiles.empty();
}

class SystemLogServiceCollection : public Node
{
  public:
    template <typename CrowApp>
    SystemLogServiceCollection(CrowApp &app) :
        Node(app, "/redfish/v1/Systems/system/LogServices/")
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    /**
     * Functions triggers appropriate requests on DBus
     */
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        // Collections don't include the static data added by SubRoute because
        // it has a duplicate entry for members
        asyncResp->res.jsonValue["@odata.type"] =
            "#LogServiceCollection.LogServiceCollection";
        asyncResp->res.jsonValue["@odata.context"] =
            "/redfish/v1/$metadata#LogServiceCollection.LogServiceCollection";
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Systems/system/LogServices";
        asyncResp->res.jsonValue["Name"] = "System Log Services Collection";
        asyncResp->res.jsonValue["Description"] =
            "Collection of LogServices for this Computer System";
        nlohmann::json &logServiceArray = asyncResp->res.jsonValue["Members"];
        logServiceArray = nlohmann::json::array();
        logServiceArray.push_back(
            {{"@odata.id", "/redfish/v1/Systems/system/LogServices/EventLog"}});
#ifdef BMCWEB_ENABLE_REDFISH_CPU_LOG
        logServiceArray.push_back(
            {{"@odata.id",
              "/redfish/v1/Systems/system/LogServices/Crashdump"}});
#endif
        asyncResp->res.jsonValue["Members@odata.count"] =
            logServiceArray.size();
    }
};

class EventLogService : public Node
{
  public:
    template <typename CrowApp>
    EventLogService(CrowApp &app) :
        Node(app, "/redfish/v1/Systems/system/LogServices/EventLog/")
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);

        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Systems/system/LogServices/EventLog";
        asyncResp->res.jsonValue["@odata.type"] =
            "#LogService.v1_1_0.LogService";
        asyncResp->res.jsonValue["@odata.context"] =
            "/redfish/v1/$metadata#LogService.LogService";
        asyncResp->res.jsonValue["Name"] = "Event Log Service";
        asyncResp->res.jsonValue["Description"] = "System Event Log Service";
        asyncResp->res.jsonValue["Id"] = "Event Log";
        asyncResp->res.jsonValue["OverWritePolicy"] = "WrapsWhenFull";
        asyncResp->res.jsonValue["Entries"] = {
            {"@odata.id",
             "/redfish/v1/Systems/system/LogServices/EventLog/Entries"}};
        asyncResp->res.jsonValue["Actions"] = {
            {{"#LogService.ClearLog",
              {{"target", "/redfish/v1/Systems/system/LogServices/EventLog/"
                          "Actions/LogService.ClearLog"}}}}};
    }
};

class EventLogClear : public Node
{
  public:
    EventLogClear(CrowApp &app) :
        Node(app, "/redfish/v1/Systems/system/LogServices/EventLog/Actions/"
                  "LogService.ClearLog/")
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::put, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::post, {{"ConfigureComponents"}}}};
    }

  private:
    void doPost(crow::Response &res, const crow::Request &req,
                const std::vector<std::string> &params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);

        // Clear the EventLog by deleting the log files
        std::vector<std::filesystem::path> redfishLogFiles;
        if (getRedfishLogFiles(redfishLogFiles))
        {
            for (const std::filesystem::path &file : redfishLogFiles)
            {
                std::error_code ec;
                std::filesystem::remove(file, ec);
            }
        }

        // Reload rsyslog so it knows to start new log files
        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "Failed to reload rsyslog: " << ec;
                    messages::internalError(asyncResp->res);
                    return;
                }

                messages::success(asyncResp->res);
            },
            "org.freedesktop.systemd1", "/org/freedesktop/systemd1",
            "org.freedesktop.systemd1.Manager", "ReloadUnit", "rsyslog.service",
            "replace");
    }
};

static int fillEventLogEntryJson(const std::string &logEntryID,
                                 const std::string logEntry,
                                 nlohmann::json &logEntryJson)
{
    // The redfish log format is "<Timestamp> <MessageId>,<MessageArgs>"
    // First get the Timestamp
    size_t space = logEntry.find_first_of(" ");
    if (space == std::string::npos)
    {
        return 1;
    }
    std::string timestamp = logEntry.substr(0, space);
    // Then get the log contents
    size_t entryStart = logEntry.find_first_not_of(" ", space);
    if (entryStart == std::string::npos)
    {
        return 1;
    }
    std::string_view entry(logEntry);
    entry.remove_prefix(entryStart);
    // Use split to separate the entry into its fields
    std::vector<std::string> logEntryFields;
    boost::split(logEntryFields, entry, boost::is_any_of(","),
                 boost::token_compress_on);
    // We need at least a MessageId to be valid
    if (logEntryFields.size() < 1)
    {
        return 1;
    }
    std::string &messageID = logEntryFields[0];

    // Get the Message from the MessageRegistry
    const message_registries::Message *message =
        message_registries::getMessage(messageID);

    std::string msg;
    std::string severity;
    if (message != nullptr)
    {
        msg = message->message;
        severity = message->severity;
    }

    // Get the MessageArgs from the log if there are any
    boost::beast::span<std::string> messageArgs;
    if (logEntryFields.size() > 1)
    {
        std::string &messageArgsStart = logEntryFields[1];
        // If the first string is empty, assume there are no MessageArgs
        std::size_t messageArgsSize = 0;
        if (!messageArgsStart.empty())
        {
            messageArgsSize = logEntryFields.size() - 1;
        }

        messageArgs = boost::beast::span(&messageArgsStart, messageArgsSize);

        // Fill the MessageArgs into the Message
        int i = 0;
        for (const std::string &messageArg : messageArgs)
        {
            std::string argStr = "%" + std::to_string(++i);
            size_t argPos = msg.find(argStr);
            if (argPos != std::string::npos)
            {
                msg.replace(argPos, argStr.length(), messageArg);
            }
        }
    }

    // Get the Created time from the timestamp. The log timestamp is in RFC3339
    // format which matches the Redfish format except for the fractional seconds
    // between the '.' and the '+', so just remove them.
    std::size_t dot = timestamp.find_first_of(".");
    std::size_t plus = timestamp.find_first_of("+");
    if (dot != std::string::npos && plus != std::string::npos)
    {
        timestamp.erase(dot, plus - dot);
    }

    // Fill in the log entry with the gathered data
    logEntryJson = {
        {"@odata.type", "#LogEntry.v1_4_0.LogEntry"},
        {"@odata.context", "/redfish/v1/$metadata#LogEntry.LogEntry"},
        {"@odata.id",
         "/redfish/v1/Systems/system/LogServices/EventLog/Entries/#" +
             logEntryID},
        {"Name", "System Event Log Entry"},
        {"Id", logEntryID},
        {"Message", std::move(msg)},
        {"MessageId", std::move(messageID)},
        {"MessageArgs", std::move(messageArgs)},
        {"EntryType", "Event"},
        {"Severity", std::move(severity)},
        {"Created", std::move(timestamp)}};
    return 0;
}

class JournalEventLogEntryCollection : public Node
{
  public:
    template <typename CrowApp>
    JournalEventLogEntryCollection(CrowApp &app) :
        Node(app, "/redfish/v1/Systems/system/LogServices/EventLog/Entries/")
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        long skip = 0;
        long top = maxEntriesPerPage; // Show max entries by default
        if (!getSkipParam(asyncResp->res, req, skip))
        {
            return;
        }
        if (!getTopParam(asyncResp->res, req, top))
        {
            return;
        }
        // Collections don't include the static data added by SubRoute because
        // it has a duplicate entry for members
        asyncResp->res.jsonValue["@odata.type"] =
            "#LogEntryCollection.LogEntryCollection";
        asyncResp->res.jsonValue["@odata.context"] =
            "/redfish/v1/$metadata#LogEntryCollection.LogEntryCollection";
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Systems/system/LogServices/EventLog/Entries";
        asyncResp->res.jsonValue["Name"] = "System Event Log Entries";
        asyncResp->res.jsonValue["Description"] =
            "Collection of System Event Log Entries";

        nlohmann::json &logEntryArray = asyncResp->res.jsonValue["Members"];
        logEntryArray = nlohmann::json::array();
        // Go through the log files and create a unique ID for each entry
        std::vector<std::filesystem::path> redfishLogFiles;
        getRedfishLogFiles(redfishLogFiles);
        uint64_t entryCount = 0;
        std::string logEntry;

        // Oldest logs are in the last file, so start there and loop backwards
        for (auto it = redfishLogFiles.rbegin(); it < redfishLogFiles.rend();
             it++)
        {
            std::ifstream logStream(*it);
            if (!logStream.is_open())
            {
                continue;
            }

            while (std::getline(logStream, logEntry))
            {
                entryCount++;
                // Handle paging using skip (number of entries to skip from the
                // start) and top (number of entries to display)
                if (entryCount <= skip || entryCount > skip + top)
                {
                    continue;
                }

                std::string idStr;
                if (!getUniqueEntryID(logEntry, idStr))
                {
                    continue;
                }

                logEntryArray.push_back({});
                nlohmann::json &bmcLogEntry = logEntryArray.back();
                if (fillEventLogEntryJson(idStr, logEntry, bmcLogEntry) != 0)
                {
                    messages::internalError(asyncResp->res);
                    return;
                }
            }
        }
        asyncResp->res.jsonValue["Members@odata.count"] = entryCount;
        if (skip + top < entryCount)
        {
            asyncResp->res.jsonValue["Members@odata.nextLink"] =
                "/redfish/v1/Systems/system/LogServices/EventLog/"
                "Entries?$skip=" +
                std::to_string(skip + top);
        }
    }
};

class DBusEventLogEntryCollection : public Node
{
  public:
    template <typename CrowApp>
    DBusEventLogEntryCollection(CrowApp &app) :
        Node(app, "/redfish/v1/Systems/system/LogServices/EventLog/Entries/")
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);

        // Collections don't include the static data added by SubRoute because
        // it has a duplicate entry for members
        asyncResp->res.jsonValue["@odata.type"] =
            "#LogEntryCollection.LogEntryCollection";
        asyncResp->res.jsonValue["@odata.context"] =
            "/redfish/v1/$metadata#LogEntryCollection.LogEntryCollection";
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Systems/system/LogServices/EventLog/Entries";
        asyncResp->res.jsonValue["Name"] = "System Event Log Entries";
        asyncResp->res.jsonValue["Description"] =
            "Collection of System Event Log Entries";

        // DBus implementation of EventLog/Entries
        // Make call to Logging Service to find all log entry objects
        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec,
                        GetManagedObjectsType &resp) {
                if (ec)
                {
                    // TODO Handle for specific error code
                    BMCWEB_LOG_ERROR
                        << "getLogEntriesIfaceData resp_handler got error "
                        << ec;
                    messages::internalError(asyncResp->res);
                    return;
                }
                nlohmann::json &entriesArray =
                    asyncResp->res.jsonValue["Members"];
                entriesArray = nlohmann::json::array();
                for (auto &objectPath : resp)
                {
                    for (auto &interfaceMap : objectPath.second)
                    {
                        if (interfaceMap.first !=
                            "xyz.openbmc_project.Logging.Entry")
                        {
                            BMCWEB_LOG_DEBUG << "Bailing early on "
                                             << interfaceMap.first;
                            continue;
                        }
                        entriesArray.push_back({});
                        nlohmann::json &thisEntry = entriesArray.back();
                        uint32_t *id;
                        std::time_t timestamp;
                        std::string *severity, *message;
                        bool *resolved;
                        for (auto &propertyMap : interfaceMap.second)
                        {
                            if (propertyMap.first == "Id")
                            {
                                id = sdbusplus::message::variant_ns::get_if<
                                    uint32_t>(&propertyMap.second);
                                if (id == nullptr)
                                {
                                    messages::propertyMissing(asyncResp->res,
                                                              "Id");
                                }
                            }
                            else if (propertyMap.first == "Timestamp")
                            {
                                const uint64_t *millisTimeStamp =
                                    std::get_if<uint64_t>(&propertyMap.second);
                                if (millisTimeStamp == nullptr)
                                {
                                    messages::propertyMissing(asyncResp->res,
                                                              "Timestamp");
                                }
                                // Retrieve Created property with format:
                                // yyyy-mm-ddThh:mm:ss
                                std::chrono::milliseconds chronoTimeStamp(
                                    *millisTimeStamp);
                                timestamp =
                                    std::chrono::duration_cast<
                                        std::chrono::seconds>(chronoTimeStamp)
                                        .count();
                            }
                            else if (propertyMap.first == "Severity")
                            {
                                severity = std::get_if<std::string>(
                                    &propertyMap.second);
                                if (severity == nullptr)
                                {
                                    messages::propertyMissing(asyncResp->res,
                                                              "Severity");
                                }
                            }
                            else if (propertyMap.first == "Message")
                            {
                                message = std::get_if<std::string>(
                                    &propertyMap.second);
                                if (message == nullptr)
                                {
                                    messages::propertyMissing(asyncResp->res,
                                                              "Message");
                                }
                            }
                        }
                        thisEntry = {
                            {"@odata.type", "#LogEntry.v1_4_0.LogEntry"},
                            {"@odata.context", "/redfish/v1/"
                                               "$metadata#LogEntry.LogEntry"},
                            {"@odata.id",
                             "/redfish/v1/Systems/system/LogServices/EventLog/"
                             "Entries/" +
                                 std::to_string(*id)},
                            {"Name", "System Event Log Entry"},
                            {"Id", std::to_string(*id)},
                            {"Message", *message},
                            {"EntryType", "Event"},
                            {"Severity",
                             translateSeverityDbusToRedfish(*severity)},
                            {"Created", crow::utility::getDateTime(timestamp)}};
                    }
                }
                std::sort(entriesArray.begin(), entriesArray.end(),
                          [](const nlohmann::json &left,
                             const nlohmann::json &right) {
                              return (left["Id"] <= right["Id"]);
                          });
                asyncResp->res.jsonValue["Members@odata.count"] =
                    entriesArray.size();
            },
            "xyz.openbmc_project.Logging", "/xyz/openbmc_project/logging",
            "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
    }
};

class DBusEventLogEntry : public Node
{
  public:
    DBusEventLogEntry(CrowApp &app) :
        Node(app,
             "/redfish/v1/Systems/system/LogServices/EventLog/Entries/<str>/",
             std::string())
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);
            return;
        }
        const std::string &entryID = params[0];

        // DBus implementation of EventLog/Entries
        // Make call to Logging Service to find all log entry objects
        crow::connections::systemBus->async_method_call(
            [asyncResp, entryID](const boost::system::error_code ec,
                                 GetManagedPropertyType &resp) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR
                        << "EventLogEntry (DBus) resp_handler got error " << ec;
                    messages::internalError(asyncResp->res);
                    return;
                }
                uint32_t *id;
                std::time_t timestamp;
                std::string *severity, *message;
                bool *resolved;
                for (auto &propertyMap : resp)
                {
                    if (propertyMap.first == "Id")
                    {
                        id = std::get_if<uint32_t>(&propertyMap.second);
                        if (id == nullptr)
                        {
                            messages::propertyMissing(asyncResp->res, "Id");
                        }
                    }
                    else if (propertyMap.first == "Timestamp")
                    {
                        const uint64_t *millisTimeStamp =
                            std::get_if<uint64_t>(&propertyMap.second);
                        if (millisTimeStamp == nullptr)
                        {
                            messages::propertyMissing(asyncResp->res,
                                                      "Timestamp");
                        }
                        // Retrieve Created property with format:
                        // yyyy-mm-ddThh:mm:ss
                        std::chrono::milliseconds chronoTimeStamp(
                            *millisTimeStamp);
                        timestamp =
                            std::chrono::duration_cast<std::chrono::seconds>(
                                chronoTimeStamp)
                                .count();
                    }
                    else if (propertyMap.first == "Severity")
                    {
                        severity =
                            std::get_if<std::string>(&propertyMap.second);
                        if (severity == nullptr)
                        {
                            messages::propertyMissing(asyncResp->res,
                                                      "Severity");
                        }
                    }
                    else if (propertyMap.first == "Message")
                    {
                        message = std::get_if<std::string>(&propertyMap.second);
                        if (message == nullptr)
                        {
                            messages::propertyMissing(asyncResp->res,
                                                      "Message");
                        }
                    }
                }
                asyncResp->res.jsonValue = {
                    {"@odata.type", "#LogEntry.v1_4_0.LogEntry"},
                    {"@odata.context", "/redfish/v1/"
                                       "$metadata#LogEntry.LogEntry"},
                    {"@odata.id",
                     "/redfish/v1/Systems/system/LogServices/EventLog/"
                     "Entries/" +
                         std::to_string(*id)},
                    {"Name", "System Event Log Entry"},
                    {"Id", std::to_string(*id)},
                    {"Message", *message},
                    {"EntryType", "Event"},
                    {"Severity", translateSeverityDbusToRedfish(*severity)},
                    {"Created", crow::utility::getDateTime(timestamp)}};
            },
            "xyz.openbmc_project.Logging",
            "/xyz/openbmc_project/logging/entry/" + entryID,
            "org.freedesktop.DBus.Properties", "GetAll",
            "xyz.openbmc_project.Logging.Entry");
    }
};

class BMCLogServiceCollection : public Node
{
  public:
    template <typename CrowApp>
    BMCLogServiceCollection(CrowApp &app) :
        Node(app, "/redfish/v1/Managers/bmc/LogServices/")
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    /**
     * Functions triggers appropriate requests on DBus
     */
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        // Collections don't include the static data added by SubRoute because
        // it has a duplicate entry for members
        asyncResp->res.jsonValue["@odata.type"] =
            "#LogServiceCollection.LogServiceCollection";
        asyncResp->res.jsonValue["@odata.context"] =
            "/redfish/v1/$metadata#LogServiceCollection.LogServiceCollection";
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Managers/bmc/LogServices";
        asyncResp->res.jsonValue["Name"] = "Open BMC Log Services Collection";
        asyncResp->res.jsonValue["Description"] =
            "Collection of LogServices for this Manager";
        nlohmann::json &logServiceArray = asyncResp->res.jsonValue["Members"];
        logServiceArray = nlohmann::json::array();
#ifdef BMCWEB_ENABLE_REDFISH_BMC_JOURNAL
        logServiceArray.push_back(
            {{"@odata.id", "/redfish/v1/Managers/bmc/LogServices/Journal"}});
#endif
        asyncResp->res.jsonValue["Members@odata.count"] =
            logServiceArray.size();
    }
};

class BMCJournalLogService : public Node
{
  public:
    template <typename CrowApp>
    BMCJournalLogService(CrowApp &app) :
        Node(app, "/redfish/v1/Managers/bmc/LogServices/Journal/")
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        asyncResp->res.jsonValue["@odata.type"] =
            "#LogService.v1_1_0.LogService";
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Managers/bmc/LogServices/Journal";
        asyncResp->res.jsonValue["@odata.context"] =
            "/redfish/v1/$metadata#LogService.LogService";
        asyncResp->res.jsonValue["Name"] = "Open BMC Journal Log Service";
        asyncResp->res.jsonValue["Description"] = "BMC Journal Log Service";
        asyncResp->res.jsonValue["Id"] = "BMC Journal";
        asyncResp->res.jsonValue["OverWritePolicy"] = "WrapsWhenFull";
        asyncResp->res.jsonValue["Entries"] = {
            {"@odata.id",
             "/redfish/v1/Managers/bmc/LogServices/Journal/Entries"}};
    }
};

static int fillBMCJournalLogEntryJson(const std::string &bmcJournalLogEntryID,
                                      sd_journal *journal,
                                      nlohmann::json &bmcJournalLogEntryJson)
{
    // Get the Log Entry contents
    int ret = 0;

    std::string_view msg;
    ret = getJournalMetadata(journal, "MESSAGE", msg);
    if (ret < 0)
    {
        BMCWEB_LOG_ERROR << "Failed to read MESSAGE field: " << strerror(-ret);
        return 1;
    }

    // Get the severity from the PRIORITY field
    int severity = 8; // Default to an invalid priority
    ret = getJournalMetadata(journal, "PRIORITY", 10, severity);
    if (ret < 0)
    {
        BMCWEB_LOG_ERROR << "Failed to read PRIORITY field: " << strerror(-ret);
        return 1;
    }

    // Get the Created time from the timestamp
    std::string entryTimeStr;
    if (!getEntryTimestamp(journal, entryTimeStr))
    {
        return 1;
    }

    // Fill in the log entry with the gathered data
    bmcJournalLogEntryJson = {
        {"@odata.type", "#LogEntry.v1_4_0.LogEntry"},
        {"@odata.context", "/redfish/v1/$metadata#LogEntry.LogEntry"},
        {"@odata.id", "/redfish/v1/Managers/bmc/LogServices/Journal/Entries/" +
                          bmcJournalLogEntryID},
        {"Name", "BMC Journal Entry"},
        {"Id", bmcJournalLogEntryID},
        {"Message", msg},
        {"EntryType", "Oem"},
        {"Severity",
         severity <= 2 ? "Critical"
                       : severity <= 4 ? "Warning" : severity <= 7 ? "OK" : ""},
        {"OemRecordFormat", "BMC Journal Entry"},
        {"Created", std::move(entryTimeStr)}};
    return 0;
}

class BMCJournalLogEntryCollection : public Node
{
  public:
    template <typename CrowApp>
    BMCJournalLogEntryCollection(CrowApp &app) :
        Node(app, "/redfish/v1/Managers/bmc/LogServices/Journal/Entries/")
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        static constexpr const long maxEntriesPerPage = 1000;
        long skip = 0;
        long top = maxEntriesPerPage; // Show max entries by default
        if (!getSkipParam(asyncResp->res, req, skip))
        {
            return;
        }
        if (!getTopParam(asyncResp->res, req, top))
        {
            return;
        }
        // Collections don't include the static data added by SubRoute because
        // it has a duplicate entry for members
        asyncResp->res.jsonValue["@odata.type"] =
            "#LogEntryCollection.LogEntryCollection";
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Managers/bmc/LogServices/Journal/Entries";
        asyncResp->res.jsonValue["@odata.context"] =
            "/redfish/v1/$metadata#LogEntryCollection.LogEntryCollection";
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Managers/bmc/LogServices/Journal/Entries";
        asyncResp->res.jsonValue["Name"] = "Open BMC Journal Entries";
        asyncResp->res.jsonValue["Description"] =
            "Collection of BMC Journal Entries";
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Managers/bmc/LogServices/BmcLog/Entries";
        nlohmann::json &logEntryArray = asyncResp->res.jsonValue["Members"];
        logEntryArray = nlohmann::json::array();

        // Go through the journal and use the timestamp to create a unique ID
        // for each entry
        sd_journal *journalTmp = nullptr;
        int ret = sd_journal_open(&journalTmp, SD_JOURNAL_LOCAL_ONLY);
        if (ret < 0)
        {
            BMCWEB_LOG_ERROR << "failed to open journal: " << strerror(-ret);
            messages::internalError(asyncResp->res);
            return;
        }
        std::unique_ptr<sd_journal, decltype(&sd_journal_close)> journal(
            journalTmp, sd_journal_close);
        journalTmp = nullptr;
        uint64_t entryCount = 0;
        SD_JOURNAL_FOREACH(journal.get())
        {
            entryCount++;
            // Handle paging using skip (number of entries to skip from the
            // start) and top (number of entries to display)
            if (entryCount <= skip || entryCount > skip + top)
            {
                continue;
            }

            std::string idStr;
            if (!getUniqueEntryID(journal.get(), idStr))
            {
                continue;
            }

            logEntryArray.push_back({});
            nlohmann::json &bmcJournalLogEntry = logEntryArray.back();
            if (fillBMCJournalLogEntryJson(idStr, journal.get(),
                                           bmcJournalLogEntry) != 0)
            {
                messages::internalError(asyncResp->res);
                return;
            }
        }
        asyncResp->res.jsonValue["Members@odata.count"] = entryCount;
        if (skip + top < entryCount)
        {
            asyncResp->res.jsonValue["Members@odata.nextLink"] =
                "/redfish/v1/Managers/bmc/LogServices/Journal/Entries?$skip=" +
                std::to_string(skip + top);
        }
    }
};

class BMCJournalLogEntry : public Node
{
  public:
    BMCJournalLogEntry(CrowApp &app) :
        Node(app, "/redfish/v1/Managers/bmc/LogServices/Journal/Entries/<str>/",
             std::string())
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);
            return;
        }
        const std::string &entryID = params[0];
        // Convert the unique ID back to a timestamp to find the entry
        uint64_t ts = 0;
        uint16_t index = 0;
        if (!getTimestampFromID(asyncResp->res, entryID, ts, index))
        {
            return;
        }

        sd_journal *journalTmp = nullptr;
        int ret = sd_journal_open(&journalTmp, SD_JOURNAL_LOCAL_ONLY);
        if (ret < 0)
        {
            BMCWEB_LOG_ERROR << "failed to open journal: " << strerror(-ret);
            messages::internalError(asyncResp->res);
            return;
        }
        std::unique_ptr<sd_journal, decltype(&sd_journal_close)> journal(
            journalTmp, sd_journal_close);
        journalTmp = nullptr;
        // Go to the timestamp in the log and move to the entry at the index
        ret = sd_journal_seek_realtime_usec(journal.get(), ts);
        for (int i = 0; i <= index; i++)
        {
            sd_journal_next(journal.get());
        }
        // Confirm that the entry ID matches what was requested
        std::string idStr;
        if (!getUniqueEntryID(journal.get(), idStr) || idStr != entryID)
        {
            messages::resourceMissingAtURI(asyncResp->res, entryID);
            return;
        }

        if (fillBMCJournalLogEntryJson(entryID, journal.get(),
                                       asyncResp->res.jsonValue) != 0)
        {
            messages::internalError(asyncResp->res);
            return;
        }
    }
};

class CrashdumpService : public Node
{
  public:
    template <typename CrowApp>
    CrashdumpService(CrowApp &app) :
        Node(app, "/redfish/v1/Systems/system/LogServices/Crashdump/")
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    /**
     * Functions triggers appropriate requests on DBus
     */
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        // Copy over the static data to include the entries added by SubRoute
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Systems/system/LogServices/Crashdump";
        asyncResp->res.jsonValue["@odata.type"] =
            "#LogService.v1_1_0.LogService";
        asyncResp->res.jsonValue["@odata.context"] =
            "/redfish/v1/$metadata#LogService.LogService";
        asyncResp->res.jsonValue["Name"] = "Open BMC Crashdump Service";
        asyncResp->res.jsonValue["Description"] = "Crashdump Service";
        asyncResp->res.jsonValue["Id"] = "Crashdump";
        asyncResp->res.jsonValue["OverWritePolicy"] = "WrapsWhenFull";
        asyncResp->res.jsonValue["MaxNumberOfRecords"] = 3;
        asyncResp->res.jsonValue["Entries"] = {
            {"@odata.id",
             "/redfish/v1/Systems/system/LogServices/Crashdump/Entries"}};
        asyncResp->res.jsonValue["Actions"] = {
            {"Oem",
             {{"#Crashdump.OnDemand",
               {{"target", "/redfish/v1/Systems/system/LogServices/Crashdump/"
                           "Actions/Oem/Crashdump.OnDemand"}}}}}};

#ifdef BMCWEB_ENABLE_REDFISH_RAW_PECI
        asyncResp->res.jsonValue["Actions"]["Oem"].push_back(
            {"#Crashdump.SendRawPeci",
             {{"target", "/redfish/v1/Systems/system/LogServices/Crashdump/"
                         "Actions/Oem/Crashdump.SendRawPeci"}}});
#endif
    }
};

class CrashdumpEntryCollection : public Node
{
  public:
    template <typename CrowApp>
    CrashdumpEntryCollection(CrowApp &app) :
        Node(app, "/redfish/v1/Systems/system/LogServices/Crashdump/Entries/")
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    /**
     * Functions triggers appropriate requests on DBus
     */
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        // Collections don't include the static data added by SubRoute because
        // it has a duplicate entry for members
        auto getLogEntriesCallback = [asyncResp](
                                         const boost::system::error_code ec,
                                         const std::vector<std::string> &resp) {
            if (ec)
            {
                if (ec.value() !=
                    boost::system::errc::no_such_file_or_directory)
                {
                    BMCWEB_LOG_DEBUG << "failed to get entries ec: "
                                     << ec.message();
                    messages::internalError(asyncResp->res);
                    return;
                }
            }
            asyncResp->res.jsonValue["@odata.type"] =
                "#LogEntryCollection.LogEntryCollection";
            asyncResp->res.jsonValue["@odata.id"] =
                "/redfish/v1/Systems/system/LogServices/Crashdump/Entries";
            asyncResp->res.jsonValue["@odata.context"] =
                "/redfish/v1/"
                "$metadata#LogEntryCollection.LogEntryCollection";
            asyncResp->res.jsonValue["Name"] = "Open BMC Crashdump Entries";
            asyncResp->res.jsonValue["Description"] =
                "Collection of Crashdump Entries";
            nlohmann::json &logEntryArray = asyncResp->res.jsonValue["Members"];
            logEntryArray = nlohmann::json::array();
            for (const std::string &objpath : resp)
            {
                // Don't list the on-demand log
                if (objpath.compare(CrashdumpOnDemandPath) == 0)
                {
                    continue;
                }
                std::size_t lastPos = objpath.rfind("/");
                if (lastPos != std::string::npos)
                {
                    logEntryArray.push_back(
                        {{"@odata.id", "/redfish/v1/Systems/system/LogServices/"
                                       "Crashdump/Entries/" +
                                           objpath.substr(lastPos + 1)}});
                }
            }
            asyncResp->res.jsonValue["Members@odata.count"] =
                logEntryArray.size();
        };
        crow::connections::systemBus->async_method_call(
            std::move(getLogEntriesCallback),
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths", "", 0,
            std::array<const char *, 1>{CrashdumpInterface});
    }
};

std::string getLogCreatedTime(const nlohmann::json &Crashdump)
{
    nlohmann::json::const_iterator cdIt = Crashdump.find("crashlog_data");
    if (cdIt != Crashdump.end())
    {
        nlohmann::json::const_iterator siIt = cdIt->find("SYSTEM_INFO");
        if (siIt != cdIt->end())
        {
            nlohmann::json::const_iterator tsIt = siIt->find("timestamp");
            if (tsIt != siIt->end())
            {
                const std::string *logTime =
                    tsIt->get_ptr<const std::string *>();
                if (logTime != nullptr)
                {
                    return *logTime;
                }
            }
        }
    }
    BMCWEB_LOG_DEBUG << "failed to find log timestamp";

    return std::string();
}

class CrashdumpEntry : public Node
{
  public:
    CrashdumpEntry(CrowApp &app) :
        Node(app,
             "/redfish/v1/Systems/system/LogServices/Crashdump/Entries/<str>/",
             std::string())
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);
            return;
        }
        const uint8_t logId = std::atoi(params[0].c_str());
        auto getStoredLogCallback = [asyncResp, logId](
                                        const boost::system::error_code ec,
                                        const std::variant<std::string> &resp) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "failed to get log ec: " << ec.message();
                messages::internalError(asyncResp->res);
                return;
            }
            const std::string *log = std::get_if<std::string>(&resp);
            if (log == nullptr)
            {
                messages::internalError(asyncResp->res);
                return;
            }
            nlohmann::json j = nlohmann::json::parse(*log, nullptr, false);
            if (j.is_discarded())
            {
                messages::internalError(asyncResp->res);
                return;
            }
            std::string t = getLogCreatedTime(j);
            asyncResp->res.jsonValue = {
                {"@odata.type", "#LogEntry.v1_4_0.LogEntry"},
                {"@odata.context", "/redfish/v1/$metadata#LogEntry.LogEntry"},
                {"@odata.id",
                 "/redfish/v1/Systems/system/LogServices/Crashdump/Entries/" +
                     std::to_string(logId)},
                {"Name", "CPU Crashdump"},
                {"Id", logId},
                {"EntryType", "Oem"},
                {"OemRecordFormat", "Intel Crashdump"},
                {"Oem", {{"Intel", std::move(j)}}},
                {"Created", std::move(t)}};
        };
        crow::connections::systemBus->async_method_call(
            std::move(getStoredLogCallback), CrashdumpObject,
            CrashdumpPath + std::string("/") + std::to_string(logId),
            "org.freedesktop.DBus.Properties", "Get", CrashdumpInterface,
            "Log");
    }
};

class OnDemandCrashdump : public Node
{
  public:
    OnDemandCrashdump(CrowApp &app) :
        Node(app,
             "/redfish/v1/Systems/system/LogServices/Crashdump/Actions/Oem/"
             "Crashdump.OnDemand/")
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    void doPost(crow::Response &res, const crow::Request &req,
                const std::vector<std::string> &params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        static std::unique_ptr<sdbusplus::bus::match::match> onDemandLogMatcher;

        // Only allow one OnDemand Log request at a time
        if (onDemandLogMatcher != nullptr)
        {
            asyncResp->res.addHeader("Retry-After", "30");
            messages::serviceTemporarilyUnavailable(asyncResp->res, "30");
            return;
        }
        // Make this static so it survives outside this method
        static boost::asio::deadline_timer timeout(*req.ioService);

        timeout.expires_from_now(boost::posix_time::seconds(30));
        timeout.async_wait([asyncResp](const boost::system::error_code &ec) {
            onDemandLogMatcher = nullptr;
            if (ec)
            {
                // operation_aborted is expected if timer is canceled before
                // completion.
                if (ec != boost::asio::error::operation_aborted)
                {
                    BMCWEB_LOG_ERROR << "Async_wait failed " << ec;
                }
                return;
            }
            BMCWEB_LOG_ERROR << "Timed out waiting for on-demand log";

            messages::internalError(asyncResp->res);
        });

        auto onDemandLogMatcherCallback = [asyncResp](
                                              sdbusplus::message::message &m) {
            BMCWEB_LOG_DEBUG << "OnDemand log available match fired";
            boost::system::error_code ec;
            timeout.cancel(ec);
            if (ec)
            {
                BMCWEB_LOG_ERROR << "error canceling timer " << ec;
            }
            sdbusplus::message::object_path objPath;
            boost::container::flat_map<
                std::string, boost::container::flat_map<
                                 std::string, std::variant<std::string>>>
                interfacesAdded;
            m.read(objPath, interfacesAdded);
            const std::string *log = std::get_if<std::string>(
                &interfacesAdded[CrashdumpInterface]["Log"]);
            if (log == nullptr)
            {
                messages::internalError(asyncResp->res);
                // Careful with onDemandLogMatcher.  It is a unique_ptr to the
                // match object inside which this lambda is executing.  Once it
                // is set to nullptr, the match object will be destroyed and the
                // lambda will lose its context, including res, so it needs to
                // be the last thing done.
                onDemandLogMatcher = nullptr;
                return;
            }
            nlohmann::json j = nlohmann::json::parse(*log, nullptr, false);
            if (j.is_discarded())
            {
                messages::internalError(asyncResp->res);
                // Careful with onDemandLogMatcher.  It is a unique_ptr to the
                // match object inside which this lambda is executing.  Once it
                // is set to nullptr, the match object will be destroyed and the
                // lambda will lose its context, including res, so it needs to
                // be the last thing done.
                onDemandLogMatcher = nullptr;
                return;
            }
            std::string t = getLogCreatedTime(j);
            asyncResp->res.jsonValue = {
                {"@odata.type", "#LogEntry.v1_4_0.LogEntry"},
                {"@odata.context", "/redfish/v1/$metadata#LogEntry.LogEntry"},
                {"Name", "CPU Crashdump"},
                {"EntryType", "Oem"},
                {"OemRecordFormat", "Intel Crashdump"},
                {"Oem", {{"Intel", std::move(j)}}},
                {"Created", std::move(t)}};
            // Careful with onDemandLogMatcher.  It is a unique_ptr to the
            // match object inside which this lambda is executing.  Once it is
            // set to nullptr, the match object will be destroyed and the lambda
            // will lose its context, including res, so it needs to be the last
            // thing done.
            onDemandLogMatcher = nullptr;
        };
        onDemandLogMatcher = std::make_unique<sdbusplus::bus::match::match>(
            *crow::connections::systemBus,
            sdbusplus::bus::match::rules::interfacesAdded() +
                sdbusplus::bus::match::rules::argNpath(0,
                                                       CrashdumpOnDemandPath),
            std::move(onDemandLogMatcherCallback));

        auto generateonDemandLogCallback =
            [asyncResp](const boost::system::error_code ec,
                        const std::string &resp) {
                if (ec)
                {
                    if (ec.value() ==
                        boost::system::errc::operation_not_supported)
                    {
                        messages::resourceInStandby(asyncResp->res);
                    }
                    else
                    {
                        messages::internalError(asyncResp->res);
                    }
                    boost::system::error_code timeoutec;
                    timeout.cancel(timeoutec);
                    if (timeoutec)
                    {
                        BMCWEB_LOG_ERROR << "error canceling timer "
                                         << timeoutec;
                    }
                    onDemandLogMatcher = nullptr;
                    return;
                }
            };
        crow::connections::systemBus->async_method_call(
            std::move(generateonDemandLogCallback), CrashdumpObject,
            CrashdumpPath, CrashdumpOnDemandInterface, "GenerateOnDemandLog");
    }
};

class SendRawPECI : public Node
{
  public:
    SendRawPECI(CrowApp &app) :
        Node(app,
             "/redfish/v1/Systems/system/LogServices/Crashdump/Actions/Oem/"
             "Crashdump.SendRawPeci/")
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::head, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::patch, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::put, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::post, {{"ConfigureComponents"}}}};
    }

  private:
    void doPost(crow::Response &res, const crow::Request &req,
                const std::vector<std::string> &params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        uint8_t clientAddress = 0;
        uint8_t readLength = 0;
        std::vector<uint8_t> peciCommand;
        if (!json_util::readJson(req, res, "ClientAddress", clientAddress,
                                 "ReadLength", readLength, "PECICommand",
                                 peciCommand))
        {
            return;
        }

        // Callback to return the Raw PECI response
        auto sendRawPECICallback =
            [asyncResp](const boost::system::error_code ec,
                        const std::vector<uint8_t> &resp) {
                if (ec)
                {
                    BMCWEB_LOG_DEBUG << "failed to send PECI command ec: "
                                     << ec.message();
                    messages::internalError(asyncResp->res);
                    return;
                }
                asyncResp->res.jsonValue = {{"Name", "PECI Command Response"},
                                            {"PECIResponse", resp}};
            };
        // Call the SendRawPECI command with the provided data
        crow::connections::systemBus->async_method_call(
            std::move(sendRawPECICallback), CrashdumpObject, CrashdumpPath,
            CrashdumpRawPECIInterface, "SendRawPeci", clientAddress, readLength,
            peciCommand);
    }
};

/**
 * DBusLogServiceActionsClear class supports POST method for ClearLog action.
 */
class DBusLogServiceActionsClear : public Node
{
  public:
    DBusLogServiceActionsClear(CrowApp &app) :
        Node(app, "/redfish/v1/Systems/system/LogServices/EventLog/Actions/"
                  "LogService.Reset")
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    /**
     * Function handles POST method request.
     * The Clear Log actions does not require any parameter.The action deletes
     * all entries found in the Entries collection for this Log Service.
     */
    void doPost(crow::Response &res, const crow::Request &req,
                const std::vector<std::string> &params) override
    {
        BMCWEB_LOG_DEBUG << "Do delete all entries.";

        auto asyncResp = std::make_shared<AsyncResp>(res);
        // Process response from Logging service.
        auto resp_handler = [asyncResp](const boost::system::error_code ec) {
            BMCWEB_LOG_DEBUG << "doClearLog resp_handler callback: Done";
            if (ec)
            {
                // TODO Handle for specific error code
                BMCWEB_LOG_ERROR << "doClearLog resp_handler got error " << ec;
                asyncResp->res.result(
                    boost::beast::http::status::internal_server_error);
                return;
            }

            asyncResp->res.result(boost::beast::http::status::no_content);
        };

        // Make call to Logging service to request Clear Log
        crow::connections::systemBus->async_method_call(
            resp_handler, "xyz.openbmc_project.Logging",
            "/xyz/openbmc_project/logging",
            "xyz.openbmc_project.Collection.DeleteAll", "DeleteAll");
    }
};
} // namespace redfish
