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
#include "task.hpp"

#include <systemd/sd-journal.h>

#include <boost/algorithm/string/split.hpp>
#include <boost/beast/core/span.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/system/linux_error.hpp>
#include <dump_offload.hpp>
#include <error_messages.hpp>

#include <filesystem>
#include <string_view>
#include <variant>

namespace redfish
{

constexpr char const* crashdumpObject = "com.intel.crashdump";
constexpr char const* crashdumpPath = "/com/intel/crashdump";
constexpr char const* crashdumpInterface = "com.intel.crashdump";
constexpr char const* deleteAllInterface =
    "xyz.openbmc_project.Collection.DeleteAll";
constexpr char const* crashdumpOnDemandInterface =
    "com.intel.crashdump.OnDemand";
constexpr char const* crashdumpRawPECIInterface =
    "com.intel.crashdump.SendRawPeci";
constexpr char const* crashdumpTelemetryInterface =
    "com.intel.crashdump.Telemetry";

namespace message_registries
{
static const Message* getMessageFromRegistry(
    const std::string& messageKey,
    const boost::beast::span<const MessageEntry> registry)
{
    boost::beast::span<const MessageEntry>::const_iterator messageIt =
        std::find_if(registry.cbegin(), registry.cend(),
                     [&messageKey](const MessageEntry& messageEntry) {
                         return !std::strcmp(messageEntry.first,
                                             messageKey.c_str());
                     });
    if (messageIt != registry.cend())
    {
        return &messageIt->second;
    }

    return nullptr;
}

static const Message* getMessage(const std::string_view& messageID)
{
    // Redfish MessageIds are in the form
    // RegistryName.MajorVersion.MinorVersion.MessageKey, so parse it to find
    // the right Message
    std::vector<std::string> fields;
    fields.reserve(4);
    boost::split(fields, messageID, boost::is_any_of("."));
    std::string& registryName = fields[0];
    std::string& messageKey = fields[3];

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
    std::string, std::variant<std::string, bool, uint8_t, int16_t, uint16_t,
                              int32_t, uint32_t, int64_t, uint64_t, double>>;

using GetManagedObjectsType = boost::container::flat_map<
    sdbusplus::message::object_path,
    boost::container::flat_map<std::string, GetManagedPropertyType>>;

inline std::string translateSeverityDbusToRedfish(const std::string& s)
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

inline void deleteSystemDumpEntry(crow::Response& res,
                                  const std::string& entryID)
{
    std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);

    auto respHandler = [asyncResp](const boost::system::error_code ec) {
        BMCWEB_LOG_DEBUG << "System Dump Entry doDelete callback: Done";
        if (ec)
        {
            BMCWEB_LOG_ERROR
                << "System Dump (DBus) doDelete respHandler got error " << ec;
            asyncResp->res.result(
                boost::beast::http::status::internal_server_error);
            return;
        }
    };
    crow::connections::systemBus->async_method_call(
        respHandler, "xyz.openbmc_project.Dump.Manager",
        "/xyz/openbmc_project/dump/entry/" + entryID,
        "xyz.openbmc_project.Object.Delete", "Delete");
}

static int getJournalMetadata(sd_journal* journal,
                              const std::string_view& field,
                              std::string_view& contents)
{
    const char* data = nullptr;
    size_t length = 0;
    int ret = 0;
    // Get the metadata from the requested field of the journal entry
    ret = sd_journal_get_data(journal, field.data(),
                              reinterpret_cast<const void**>(&data), &length);
    if (ret < 0)
    {
        return ret;
    }
    contents = std::string_view(data, length);
    // Only use the content after the "=" character.
    contents.remove_prefix(std::min(contents.find("=") + 1, contents.size()));
    return ret;
}

static int getJournalMetadata(sd_journal* journal,
                              const std::string_view& field, const int& base,
                              long int& contents)
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

static bool getTimestampStr(const uint64_t usecSinceEpoch,
                            std::string& entryTimestamp)
{
    time_t t = static_cast<time_t>(usecSinceEpoch / 1000 / 1000);
    struct tm* loctime = localtime(&t);
    char entryTime[64] = {};
    if (nullptr != loctime)
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

static bool getEntryTimestamp(sd_journal* journal, std::string& entryTimestamp)
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
    return getTimestampStr(timestamp, entryTimestamp);
}

static bool getSkipParam(crow::Response& res, const crow::Request& req,
                         uint64_t& skip)
{
    char* skipParam = req.urlParams.get("$skip");
    if (skipParam != nullptr)
    {
        char* ptr = nullptr;
        skip = std::strtoul(skipParam, &ptr, 10);
        if (*skipParam == '\0' || *ptr != '\0')
        {

            messages::queryParameterValueTypeError(res, std::string(skipParam),
                                                   "$skip");
            return false;
        }
    }
    return true;
}

static constexpr const uint64_t maxEntriesPerPage = 1000;
static bool getTopParam(crow::Response& res, const crow::Request& req,
                        uint64_t& top)
{
    char* topParam = req.urlParams.get("$top");
    if (topParam != nullptr)
    {
        char* ptr = nullptr;
        top = std::strtoul(topParam, &ptr, 10);
        if (*topParam == '\0' || *ptr != '\0')
        {
            messages::queryParameterValueTypeError(res, std::string(topParam),
                                                   "$top");
            return false;
        }
        if (top < 1U || top > maxEntriesPerPage)
        {

            messages::queryParameterOutOfRange(
                res, std::to_string(top), "$top",
                "1-" + std::to_string(maxEntriesPerPage));
            return false;
        }
    }
    return true;
}

static bool getUniqueEntryID(sd_journal* journal, std::string& entryID,
                             const bool firstEntry = true)
{
    int ret = 0;
    static uint64_t prevTs = 0;
    static int index = 0;
    if (firstEntry)
    {
        prevTs = 0;
    }

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

static bool getUniqueEntryID(const std::string& logEntry, std::string& entryID,
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

static bool getTimestampFromID(crow::Response& res, const std::string& entryID,
                               uint64_t& timestamp, uint64_t& index)
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
        catch (std::invalid_argument&)
        {
            messages::resourceMissingAtURI(res, entryID);
            return false;
        }
        catch (std::out_of_range&)
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
    catch (std::invalid_argument&)
    {
        messages::resourceMissingAtURI(res, entryID);
        return false;
    }
    catch (std::out_of_range&)
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
    getRedfishLogFiles(std::vector<std::filesystem::path>& redfishLogFiles)
{
    static const std::filesystem::path redfishLogDir = "/var/log";
    static const std::string redfishLogFilename = "redfish";

    // Loop through the directory looking for redfish log files
    for (const std::filesystem::directory_entry& dirEnt :
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

#ifdef BMCWEB_ENABLE_REDFISH_MODULAR_LOG

boost::container::flat_map<std::string, uint8_t> mappingMode = {
    {"Standalone", 1}, {"FourSocket", 2}, {"EightSocket", 4}};

static uint8_t maxNodeCount = 1;
static constexpr const char* nodeLogPrefix = "/var/log/nodeLog/redfishNode";
static std::vector<uint8_t> getSlaveNodeId()
{
    std::vector<uint8_t> ret;
    crow::connections::systemBus->async_method_call(
        [](const boost::system::error_code ec,
           const std::variant<std::string>& socketStr) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                return;
            }
            auto pVal = std::get_if<std::string>(&socketStr);
            if (pVal)
            {
                // handle property Index
                auto propIter = mappingMode.find(*pVal);
                if (mappingMode.end() != propIter)
                {
                    maxNodeCount = propIter->second;
                }
                else
                {
                    return;
                }
            }
            else
            {
                BMCWEB_LOG_DEBUG << "Socket type failed.";
            }
        },
        "xyz.openbmc_project.modular", "/xyz/openbmc_project/modular",
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.modular.state", "BMCMode");
    for (uint8_t i = 1; i < maxNodeCount; i++)
    {
        std::string nodeLog = nodeLogPrefix + std::to_string(i);
        if (std::filesystem::is_regular_file(nodeLog))
        {
            ret.emplace_back(i);
        }
    }
    return ret;
}
#endif

static void ParseCrashdumpParameters(
    const std::vector<std::pair<std::string, VariantType>>& params,
    std::string& filename, std::string& timestamp, std::string& logfile)
{
    for (auto property : params)
    {
        if (property.first == "Timestamp")
        {
            const std::string* value =
                std::get_if<std::string>(&property.second);
            if (value != nullptr)
            {
                timestamp = *value;
            }
        }
        else if (property.first == "Filename")
        {
            const std::string* value =
                std::get_if<std::string>(&property.second);
            if (value != nullptr)
            {
                filename = *value;
            }
        }
        else if (property.first == "Log")
        {
            const std::string* value =
                std::get_if<std::string>(&property.second);
            if (value != nullptr)
            {
                logfile = *value;
            }
        }
    }
}

constexpr char const* postCodeIface = "xyz.openbmc_project.State.Boot.PostCode";
class SystemLogServiceCollection : public Node
{
  public:
    template <typename CrowApp>
    SystemLogServiceCollection(CrowApp& app) :
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
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        // Collections don't include the static data added by SubRoute because
        // it has a duplicate entry for members
        asyncResp->res.jsonValue["@odata.type"] =
            "#LogServiceCollection.LogServiceCollection";
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Systems/system/LogServices";
        asyncResp->res.jsonValue["Name"] = "System Log Services Collection";
        asyncResp->res.jsonValue["Description"] =
            "Collection of LogServices for this Computer System";
        nlohmann::json& logServiceArray = asyncResp->res.jsonValue["Members"];
        logServiceArray = nlohmann::json::array();
        logServiceArray.push_back(
            {{"@odata.id", "/redfish/v1/Systems/system/LogServices/EventLog"}});
#ifdef BMCWEB_ENABLE_REDFISH_SYSTEMDUMP_LOG
        logServiceArray.push_back(
            {{"@odata.id", "/redfish/v1/Systems/system/LogServices/System"}});
#endif

#ifdef BMCWEB_ENABLE_REDFISH_MODULAR_LOG
        std::vector<uint8_t> slaveId = getSlaveNodeId();
        for (const uint8_t id : slaveId)
        {
            logServiceArray.push_back(
                {{"@odata.id",
                  "/redfish/v1/Systems/system/LogServices/EventLogNode" +
                      std::to_string(id)}});
        }
#endif

#ifdef BMCWEB_ENABLE_REDFISH_CPU_LOG
        logServiceArray.push_back(
            {{"@odata.id",
              "/redfish/v1/Systems/system/LogServices/Crashdump"}});
#endif
        asyncResp->res.jsonValue["Members@odata.count"] =
            logServiceArray.size();

        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec,
                        const std::vector<std::string>& subtreePath) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << ec;
                    return;
                }

                for (auto& pathStr : subtreePath)
                {
                    if (pathStr.find("PostCode") != std::string::npos)
                    {
                        nlohmann::json& logServiceArray =
                            asyncResp->res.jsonValue["Members"];
                        logServiceArray.push_back(
                            {{"@odata.id", "/redfish/v1/Systems/system/"
                                           "LogServices/PostCodes"}});
                        asyncResp->res.jsonValue["Members@odata.count"] =
                            logServiceArray.size();
                        return;
                    }
                }
            },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths", "/", 0,
            std::array<const char*, 1>{postCodeIface});
    }
};

class EventLogService : public Node
{
  public:
    template <typename CrowApp>
    EventLogService(CrowApp& app) :
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
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);

        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Systems/system/LogServices/EventLog";
        asyncResp->res.jsonValue["@odata.type"] =
            "#LogService.v1_1_0.LogService";
        asyncResp->res.jsonValue["Name"] = "Event Log Service";
        asyncResp->res.jsonValue["Description"] = "System Event Log Service";
        asyncResp->res.jsonValue["Id"] = "EventLog";
        asyncResp->res.jsonValue["OverWritePolicy"] = "WrapsWhenFull";
        asyncResp->res.jsonValue["Entries"] = {
            {"@odata.id",
             "/redfish/v1/Systems/system/LogServices/EventLog/Entries"}};
        asyncResp->res.jsonValue["Actions"]["#LogService.ClearLog"] = {

            {"target", "/redfish/v1/Systems/system/LogServices/EventLog/"
                       "Actions/LogService.ClearLog"}};
    }
};

class JournalEventLogClear : public Node
{
  public:
    JournalEventLogClear(CrowApp& app) :
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
    void doPost(crow::Response& res, const crow::Request& req,
                const std::vector<std::string>& params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);

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

static int fillEventLogEntryJson(const std::string& logEntryID,
                                 const std::string logEntry,
                                 nlohmann::json& logEntryJson)
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
    std::string& messageID = logEntryFields[0];

    // Get the Message from the MessageRegistry
    const message_registries::Message* message =
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
        std::string& messageArgsStart = logEntryFields[1];
        // If the first string is empty, assume there are no MessageArgs
        std::size_t messageArgsSize = 0;
        if (!messageArgsStart.empty())
        {
            messageArgsSize = logEntryFields.size() - 1;
        }

        messageArgs = boost::beast::span(&messageArgsStart, messageArgsSize);

        // Fill the MessageArgs into the Message
        int i = 0;
        for (const std::string& messageArg : messageArgs)
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
        {"@odata.id",
         "/redfish/v1/Systems/system/LogServices/EventLog/Entries/" +
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
    JournalEventLogEntryCollection(CrowApp& app) :
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
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        uint64_t skip = 0;
        uint64_t top = maxEntriesPerPage; // Show max entries by default
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
            "/redfish/v1/Systems/system/LogServices/EventLog/Entries";
        asyncResp->res.jsonValue["Name"] = "System Event Log Entries";
        asyncResp->res.jsonValue["Description"] =
            "Collection of System Event Log Entries";

        nlohmann::json& logEntryArray = asyncResp->res.jsonValue["Members"];
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

            // Reset the unique ID on the first entry
            bool firstEntry = true;
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
                if (!getUniqueEntryID(logEntry, idStr, firstEntry))
                {
                    continue;
                }

                if (firstEntry)
                {
                    firstEntry = false;
                }

                logEntryArray.push_back({});
                nlohmann::json& bmcLogEntry = logEntryArray.back();
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

class JournalEventLogEntry : public Node
{
  public:
    JournalEventLogEntry(CrowApp& app) :
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
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);
            return;
        }
        const std::string& targetID = params[0];

        // Go through the log files and check the unique ID for each entry to
        // find the target entry
        std::vector<std::filesystem::path> redfishLogFiles;
        getRedfishLogFiles(redfishLogFiles);
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

            // Reset the unique ID on the first entry
            bool firstEntry = true;
            while (std::getline(logStream, logEntry))
            {
                std::string idStr;
                if (!getUniqueEntryID(logEntry, idStr, firstEntry))
                {
                    continue;
                }

                if (firstEntry)
                {
                    firstEntry = false;
                }

                if (idStr == targetID)
                {
                    if (fillEventLogEntryJson(idStr, logEntry,
                                              asyncResp->res.jsonValue) != 0)
                    {
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    return;
                }
            }
        }
        // Requested ID was not found
        messages::resourceMissingAtURI(asyncResp->res, targetID);
    }
};

class DBusEventLogEntryCollection : public Node
{
  public:
    template <typename CrowApp>
    DBusEventLogEntryCollection(CrowApp& app) :
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
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);

        // Collections don't include the static data added by SubRoute because
        // it has a duplicate entry for members
        asyncResp->res.jsonValue["@odata.type"] =
            "#LogEntryCollection.LogEntryCollection";
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Systems/system/LogServices/EventLog/Entries";
        asyncResp->res.jsonValue["Name"] = "System Event Log Entries";
        asyncResp->res.jsonValue["Description"] =
            "Collection of System Event Log Entries";

        // DBus implementation of EventLog/Entries
        // Make call to Logging Service to find all log entry objects
        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec,
                        GetManagedObjectsType& resp) {
                if (ec)
                {
                    // TODO Handle for specific error code
                    BMCWEB_LOG_ERROR
                        << "getLogEntriesIfaceData resp_handler got error "
                        << ec;
                    messages::internalError(asyncResp->res);
                    return;
                }
                nlohmann::json& entriesArray =
                    asyncResp->res.jsonValue["Members"];
                entriesArray = nlohmann::json::array();
                for (auto& objectPath : resp)
                {
                    for (auto& interfaceMap : objectPath.second)
                    {
                        if (interfaceMap.first !=
                            "xyz.openbmc_project.Logging.Entry")
                        {
                            BMCWEB_LOG_DEBUG << "Bailing early on "
                                             << interfaceMap.first;
                            continue;
                        }
                        entriesArray.push_back({});
                        nlohmann::json& thisEntry = entriesArray.back();
                        uint32_t* id = nullptr;
                        std::time_t timestamp{};
                        std::string* severity = nullptr;
                        std::string* message = nullptr;
                        for (auto& propertyMap : interfaceMap.second)
                        {
                            if (propertyMap.first == "Id")
                            {
                                id = std::get_if<uint32_t>(&propertyMap.second);
                                if (id == nullptr)
                                {
                                    messages::propertyMissing(asyncResp->res,
                                                              "Id");
                                }
                            }
                            else if (propertyMap.first == "Timestamp")
                            {
                                const uint64_t* millisTimeStamp =
                                    std::get_if<uint64_t>(&propertyMap.second);
                                if (millisTimeStamp == nullptr)
                                {
                                    messages::propertyMissing(asyncResp->res,
                                                              "Timestamp");
                                    continue;
                                }
                                // Retrieve Created property with format:
                                // yyyy-mm-ddThh:mm:ss
                                std::chrono::milliseconds chronoTimeStamp(
                                    *millisTimeStamp);
                                timestamp = std::chrono::duration_cast<
                                                std::chrono::duration<int>>(
                                                chronoTimeStamp)
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
                          [](const nlohmann::json& left,
                             const nlohmann::json& right) {
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
    DBusEventLogEntry(CrowApp& app) :
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
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);
            return;
        }
        const std::string& entryID = params[0];

        // DBus implementation of EventLog/Entries
        // Make call to Logging Service to find all log entry objects
        crow::connections::systemBus->async_method_call(
            [asyncResp, entryID](const boost::system::error_code ec,
                                 GetManagedPropertyType& resp) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR
                        << "EventLogEntry (DBus) resp_handler got error " << ec;
                    messages::internalError(asyncResp->res);
                    return;
                }
                uint32_t* id = nullptr;
                std::time_t timestamp{};
                std::string* severity = nullptr;
                std::string* message = nullptr;
                for (auto& propertyMap : resp)
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
                        const uint64_t* millisTimeStamp =
                            std::get_if<uint64_t>(&propertyMap.second);
                        if (millisTimeStamp == nullptr)
                        {
                            messages::propertyMissing(asyncResp->res,
                                                      "Timestamp");
                            continue;
                        }
                        // Retrieve Created property with format:
                        // yyyy-mm-ddThh:mm:ss
                        std::chrono::milliseconds chronoTimeStamp(
                            *millisTimeStamp);
                        timestamp =
                            std::chrono::duration_cast<
                                std::chrono::duration<int>>(chronoTimeStamp)
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
                if (id == nullptr || message == nullptr || severity == nullptr)
                {
                    return;
                }
                asyncResp->res.jsonValue = {
                    {"@odata.type", "#LogEntry.v1_4_0.LogEntry"},
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

    void doDelete(crow::Response& res, const crow::Request& req,
                  const std::vector<std::string>& params) override
    {

        BMCWEB_LOG_DEBUG << "Do delete single event entries.";

        auto asyncResp = std::make_shared<AsyncResp>(res);

        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);
            return;
        }
        std::string entryID = params[0];

        dbus::utility::escapePathForDbus(entryID);

        // Process response from Logging service.
        auto respHandler = [asyncResp](const boost::system::error_code ec) {
            BMCWEB_LOG_DEBUG << "EventLogEntry (DBus) doDelete callback: Done";
            if (ec)
            {
                // TODO Handle for specific error code
                BMCWEB_LOG_ERROR
                    << "EventLogEntry (DBus) doDelete respHandler got error "
                    << ec;
                asyncResp->res.result(
                    boost::beast::http::status::internal_server_error);
                return;
            }

            asyncResp->res.result(boost::beast::http::status::ok);
        };

        // Make call to Logging service to request Delete Log
        crow::connections::systemBus->async_method_call(
            respHandler, "xyz.openbmc_project.Logging",
            "/xyz/openbmc_project/logging/entry/" + entryID,
            "xyz.openbmc_project.Object.Delete", "Delete");
    }
};

class BMCLogServiceCollection : public Node
{
  public:
    template <typename CrowApp>
    BMCLogServiceCollection(CrowApp& app) :
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
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        // Collections don't include the static data added by SubRoute because
        // it has a duplicate entry for members
        asyncResp->res.jsonValue["@odata.type"] =
            "#LogServiceCollection.LogServiceCollection";
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Managers/bmc/LogServices";
        asyncResp->res.jsonValue["Name"] = "Open BMC Log Services Collection";
        asyncResp->res.jsonValue["Description"] =
            "Collection of LogServices for this Manager";
        nlohmann::json& logServiceArray = asyncResp->res.jsonValue["Members"];
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
    BMCJournalLogService(CrowApp& app) :
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
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        asyncResp->res.jsonValue["@odata.type"] =
            "#LogService.v1_1_0.LogService";
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Managers/bmc/LogServices/Journal";
        asyncResp->res.jsonValue["Name"] = "Open BMC Journal Log Service";
        asyncResp->res.jsonValue["Description"] = "BMC Journal Log Service";
        asyncResp->res.jsonValue["Id"] = "BMC Journal";
        asyncResp->res.jsonValue["OverWritePolicy"] = "WrapsWhenFull";
        asyncResp->res.jsonValue["Entries"] = {
            {"@odata.id",
             "/redfish/v1/Managers/bmc/LogServices/Journal/Entries"}};
    }
};

static int fillBMCJournalLogEntryJson(const std::string& bmcJournalLogEntryID,
                                      sd_journal* journal,
                                      nlohmann::json& bmcJournalLogEntryJson)
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
    long int severity = 8; // Default to an invalid priority
    ret = getJournalMetadata(journal, "PRIORITY", 10, severity);
    if (ret < 0)
    {
        BMCWEB_LOG_ERROR << "Failed to read PRIORITY field: " << strerror(-ret);
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
        {"@odata.id", "/redfish/v1/Managers/bmc/LogServices/Journal/Entries/" +
                          bmcJournalLogEntryID},
        {"Name", "BMC Journal Entry"},
        {"Id", bmcJournalLogEntryID},
        {"Message", msg},
        {"EntryType", "Oem"},
        {"Severity",
         severity <= 2 ? "Critical" : severity <= 4 ? "Warning" : "OK"},
        {"OemRecordFormat", "BMC Journal Entry"},
        {"Created", std::move(entryTimeStr)}};
    return 0;
}

class BMCJournalLogEntryCollection : public Node
{
  public:
    template <typename CrowApp>
    BMCJournalLogEntryCollection(CrowApp& app) :
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
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        static constexpr const long maxEntriesPerPage = 1000;
        uint64_t skip = 0;
        uint64_t top = maxEntriesPerPage; // Show max entries by default
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
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Managers/bmc/LogServices/Journal/Entries";
        asyncResp->res.jsonValue["Name"] = "Open BMC Journal Entries";
        asyncResp->res.jsonValue["Description"] =
            "Collection of BMC Journal Entries";
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Managers/bmc/LogServices/BmcLog/Entries";
        nlohmann::json& logEntryArray = asyncResp->res.jsonValue["Members"];
        logEntryArray = nlohmann::json::array();

        // Go through the journal and use the timestamp to create a unique ID
        // for each entry
        sd_journal* journalTmp = nullptr;
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
        // Reset the unique ID on the first entry
        bool firstEntry = true;
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
            if (!getUniqueEntryID(journal.get(), idStr, firstEntry))
            {
                continue;
            }

            if (firstEntry)
            {
                firstEntry = false;
            }

            logEntryArray.push_back({});
            nlohmann::json& bmcJournalLogEntry = logEntryArray.back();
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
    BMCJournalLogEntry(CrowApp& app) :
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
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);
            return;
        }
        const std::string& entryID = params[0];
        // Convert the unique ID back to a timestamp to find the entry
        uint64_t ts = 0;
        uint64_t index = 0;
        if (!getTimestampFromID(asyncResp->res, entryID, ts, index))
        {
            return;
        }

        sd_journal* journalTmp = nullptr;
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
        // tracking the unique ID
        std::string idStr;
        bool firstEntry = true;
        ret = sd_journal_seek_realtime_usec(journal.get(), ts);
        if (ret < 0)
        {
            BMCWEB_LOG_ERROR << "failed to seek to an entry in journal"
                             << strerror(-ret);
            messages::internalError(asyncResp->res);
            return;
        }
        for (uint64_t i = 0; i <= index; i++)
        {
            sd_journal_next(journal.get());
            if (!getUniqueEntryID(journal.get(), idStr, firstEntry))
            {
                messages::internalError(asyncResp->res);
                return;
            }
            if (firstEntry)
            {
                firstEntry = false;
            }
        }
        // Confirm that the entry ID matches what was requested
        if (idStr != entryID)
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

class SystemDumpService : public Node
{
  public:
    template <typename CrowApp>
    SystemDumpService(CrowApp& app) :
        Node(app, "/redfish/v1/Systems/system/LogServices/System/")
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
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);

        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Systems/system/LogServices/System";
        asyncResp->res.jsonValue["@odata.type"] =
            "#LogService.v1_1_0.LogService";
        asyncResp->res.jsonValue["Name"] = "Dump Log Service";
        asyncResp->res.jsonValue["Description"] = "System Dump Log Service";
        asyncResp->res.jsonValue["Id"] = "System";
        asyncResp->res.jsonValue["OverWritePolicy"] = "WrapsWhenFull";
        asyncResp->res.jsonValue["LogEntryTypes"] = "Dump";
        asyncResp->res.jsonValue["Oem"]["DumpType"] = "System";

        asyncResp->res.jsonValue["Entries"] = {
            {"@odata.id",
             "/redfish/v1/Systems/system/LogServices/System/Entries"}};
        asyncResp->res.jsonValue["Actions"]["#LogService.ClearLog"] = {
            {"target", "/redfish/v1/Systems/system/LogServices/System/"
                       "Actions/LogService.ClearLog"}};
        asyncResp->res.jsonValue["Actions"]["#LogService.CreateLog"] = {
            {"target", "/redfish/v1/Systems/system/LogServices/System/"
                       "Actions/LogService.CreateLog"}};
    }
};

class SystemDumpEntryCollection : public Node
{
  public:
    template <typename CrowApp>
    SystemDumpEntryCollection(CrowApp& app) :
        Node(app, "/redfish/v1/Systems/system/LogServices/System/Entries/")
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
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);

        asyncResp->res.jsonValue["@odata.type"] =
            "#LogEntryCollection.LogEntryCollection";
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Systems/system/LogServices/System/Entries";
        asyncResp->res.jsonValue["Name"] = "System Dump Entries";
        asyncResp->res.jsonValue["Description"] =
            "Collection of System Dump Entries";

        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec,
                        const crow::openbmc_mapper::GetSubTreeType& resp) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << " resp_handler got error " << ec;
                    messages::internalError(asyncResp->res);
                    return;
                }

                nlohmann::json& logArray = asyncResp->res.jsonValue["Members"];
                logArray = nlohmann::json::array();
                for (auto& object : resp)
                {
                    const std::string& path =
                        static_cast<const std::string&>(object.first);
                    std::size_t lastPos = path.rfind("/");
                    if (lastPos == std::string::npos)
                    {
                        continue;
                    }
                    std::string logID = path.substr(lastPos + 1);
                    logArray.push_back(
                        {{"@odata.id", "/redfish/v1/Systems/system/LogServices/"
                                       "System/Entries/" +
                                           logID}});
                }
                asyncResp->res.jsonValue["Members@odata.count"] =
                    logArray.size();
            },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetSubTree",
            "/xyz/openbmc_project/dump", 0,
            std::array<const char*, 1>{
                "xyz.openbmc_project.Dump.Entry.System"});
    }
};

class SystemDumpEntry : public Node
{
  public:
    SystemDumpEntry(CrowApp& app) :
        Node(app,
             "/redfish/v1/Systems/system/LogServices/System/Entries/<str>/",
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
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);
            return;
        }
        const std::string& entryID = params[0];
        crow::connections::systemBus->async_method_call(
            [asyncResp, entryID](const boost::system::error_code ec,
                                 GetManagedObjectsType& resp) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR
                        << "SystemDumpEntry resp_handler got error " << ec;
                    messages::internalError(asyncResp->res);
                    return;
                }

                for (auto& objectPath : resp)
                {
                    if (objectPath.first.str.find(
                            "/xyz/openbmc_project/dump/entry/" + entryID) ==
                        std::string::npos)
                    {
                        continue;
                    }

                    bool foundSystemDumpEntry = false;
                    for (auto& interfaceMap : objectPath.second)
                    {
                        if (interfaceMap.first ==
                            "xyz.openbmc_project.Dump.Entry.System")
                        {
                            foundSystemDumpEntry = true;
                            break;
                        }
                    }
                    if (foundSystemDumpEntry == false)
                    {
                        BMCWEB_LOG_DEBUG << "Can't find System Dump Entry";
                        messages::internalError(asyncResp->res);
                        return;
                    }

                    std::string timestamp{};
                    uint64_t size = 0;

                    for (auto& interfaceMap : objectPath.second)
                    {
                        if (interfaceMap.first ==
                            "xyz.openbmc_project.Dump.Entry")
                        {
                            for (auto& propertyMap : interfaceMap.second)
                            {
                                if (propertyMap.first == "Size")
                                {
                                    auto sizePtr = std::get_if<uint64_t>(
                                        &propertyMap.second);
                                    if (sizePtr == nullptr)
                                    {
                                        messages::propertyMissing(
                                            asyncResp->res, "Size");
                                        break;
                                    }
                                    size = *sizePtr;
                                    break;
                                }
                            }
                        }
                        else if (interfaceMap.first ==
                                 "xyz.openbmc_project.Time.EpochTime")
                        {
                            for (auto& propertyMap : interfaceMap.second)
                            {
                                if (propertyMap.first == "Elapsed")
                                {
                                    const uint64_t* usecsTimeStamp =
                                        std::get_if<uint64_t>(
                                            &propertyMap.second);
                                    if (usecsTimeStamp == nullptr)
                                    {
                                        messages::propertyMissing(
                                            asyncResp->res, "Elapsed");
                                        break;
                                    }
                                    getTimestampStr(*usecsTimeStamp, timestamp);
                                    break;
                                }
                            }
                        }
                    }
                    asyncResp->res.jsonValue = {
                        {"@odata.type", "#LogEntry.v1_4_0.LogEntry"},
                        {"@odata.id",
                         "/redfish/v1/Systems/system/LogServices/System/"
                         "Entries/" +
                             entryID},
                        {"Name", "System Dump Entry"},
                        {"Id", entryID},
                        {"SizeInB", size},
                        {"EntryType", "Dump"},
                        {"EntryCode", "User generated dump"},
                        {"Created", timestamp}};

                    asyncResp->res
                        .jsonValue["Actions"]["#LogEntry.DownloadLog"] = {
                        {"target",
                         "/redfish/v1/Systems/system/LogServices/System/"
                         "Entries/" +
                             entryID + "/Actions/LogEntry.DownloadLog"}};
                }
            },
            "xyz.openbmc_project.Dump.Manager", "/xyz/openbmc_project/dump",
            "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
    }

    void doDelete(crow::Response& res, const crow::Request& req,
                  const std::vector<std::string>& params) override
    {
        BMCWEB_LOG_DEBUG << "Do delete single dump entry";

        auto asyncResp = std::make_shared<AsyncResp>(res);

        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);
            return;
        }
        std::string entryID = params[0];

        crow::connections::systemBus->async_method_call(
            [asyncResp,
             entryID](const boost::system::error_code ec,
                      const crow::openbmc_mapper::GetSubTreeType& resp) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << " resp_handler got error " << ec;
                    messages::internalError(asyncResp->res);
                    return;
                }

                for (auto& object : resp)
                {
                    const std::string& path =
                        static_cast<const std::string&>(object.first);

                    std::size_t pos = path.rfind(
                        "/xyz/openbmc_project/dump/entry/" + entryID);
                    if (pos != std::string::npos)
                    {
                        deleteSystemDumpEntry(asyncResp->res, entryID);
                        return;
                    }
                }
            },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetSubTree",
            "/xyz/openbmc_project/dump", 0,
            std::array<const char*, 1>{
                "xyz.openbmc_project.Dump.Entry.System"});
    }
};

class SystemDumpEntryDownload : public Node
{
  public:
    SystemDumpEntryDownload(CrowApp& app) :
        Node(app,
             "/redfish/v1/Systems/system/LogServices/System/Entries/<str>/"
             "Actions/"
             "LogEntry.DownloadLog/",
             std::string())
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    void doPost(crow::Response& res, const crow::Request& req,
                const std::vector<std::string>& params) override
    {
        if (params.size() != 1)
        {
            messages::internalError(res);
            return;
        }
        const std::string& entryID = params[0];
        crow::obmc_dump::handleDumpOffloadUrl(req, res, entryID);
    }
};

class SystemDumpClear : public Node
{
  public:
    SystemDumpClear(CrowApp& app) :
        Node(app, "/redfish/v1/Systems/system/LogServices/System/"
                  "Actions/"
                  "LogService.ClearLog/")
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    void doPost(crow::Response& res, const crow::Request& req,
                const std::vector<std::string>& params) override
    {

        auto asyncResp = std::make_shared<AsyncResp>(res);
        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec,
                        const std::vector<std::string>& dumpList) {
                if (ec)
                {
                    messages::internalError(asyncResp->res);
                    return;
                }

                for (const std::string& objectPath : dumpList)
                {
                    std::size_t pos = objectPath.rfind("/");
                    if (pos != std::string::npos)
                    {
                        std::string logID = objectPath.substr(pos + 1);
                        deleteSystemDumpEntry(asyncResp->res, logID);
                    }
                }
            },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
            "/xyz/openbmc_project/dump", 0,
            std::array<const char*, 1>{
                "xyz.openbmc_project.Dump.Entry.System"});
    }
};

class CrashdumpService : public Node
{
  public:
    template <typename CrowApp>
    CrashdumpService(CrowApp& app) :
        Node(app, "/redfish/v1/Systems/system/LogServices/Crashdump/")
    {
        // Note: Deviated from redfish privilege registry for GET & HEAD
        // method for security reasons.
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::head, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    /**
     * Functions triggers appropriate requests on DBus
     */
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        // Copy over the static data to include the entries added by SubRoute
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Systems/system/LogServices/Crashdump";
        asyncResp->res.jsonValue["@odata.type"] =
            "#LogService.v1_1_0.LogService";
        asyncResp->res.jsonValue["Name"] = "Open BMC Oem Crashdump Service";
        asyncResp->res.jsonValue["Description"] = "Oem Crashdump Service";
        asyncResp->res.jsonValue["Id"] = "Oem Crashdump";
        asyncResp->res.jsonValue["OverWritePolicy"] = "WrapsWhenFull";
        asyncResp->res.jsonValue["MaxNumberOfRecords"] = 3;
        asyncResp->res.jsonValue["Entries"] = {
            {"@odata.id",
             "/redfish/v1/Systems/system/LogServices/Crashdump/Entries"}};
        asyncResp->res.jsonValue["Actions"] = {
            {"#LogService.ClearLog",
             {{"target", "/redfish/v1/Systems/system/LogServices/Crashdump/"
                         "Actions/LogService.ClearLog"}}},
            {"Oem",
             {{"#Crashdump.OnDemand",
               {{"target", "/redfish/v1/Systems/system/LogServices/Crashdump/"
                           "Actions/Oem/Crashdump.OnDemand"}}},
              {"#Crashdump.Telemetry",
               {{"target", "/redfish/v1/Systems/system/LogServices/Crashdump/"
                           "Actions/Oem/Crashdump.Telemetry"}}}}}};

#ifdef BMCWEB_ENABLE_REDFISH_RAW_PECI
        asyncResp->res.jsonValue["Actions"]["Oem"].push_back(
            {"#Crashdump.SendRawPeci",
             {{"target", "/redfish/v1/Systems/system/LogServices/Crashdump/"
                         "Actions/Oem/Crashdump.SendRawPeci"}}});
#endif
    }
};

class CrashdumpClear : public Node
{
  public:
    CrashdumpClear(CrowApp& app) :
        Node(app, "/redfish/v1/Systems/system/LogServices/Crashdump/Actions/"
                  "LogService.ClearLog/")
    {
        // Note: Deviated from redfish privilege registry for GET & HEAD
        // method for security reasons.
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::head, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::patch, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::put, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::post, {{"ConfigureComponents"}}}};
    }

  private:
    void doPost(crow::Response& res, const crow::Request& req,
                const std::vector<std::string>& params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);

        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec,
                        const std::string& resp) {
                if (ec)
                {
                    messages::internalError(asyncResp->res);
                    return;
                }
                messages::success(asyncResp->res);
            },
            crashdumpObject, crashdumpPath, deleteAllInterface, "DeleteAll");
    }
};

static void logCrashdumpEntry(std::shared_ptr<AsyncResp> asyncResp,
                              const std::string& logID,
                              nlohmann::json& logEntryJson)
{
    auto getStoredLogCallback =
        [asyncResp, logID, &logEntryJson](
            const boost::system::error_code ec,
            const std::vector<std::pair<std::string, VariantType>>& params) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "failed to get log ec: " << ec.message();
                if (ec.value() ==
                    boost::system::linux_error::bad_request_descriptor)
                {
                    messages::resourceNotFound(asyncResp->res, "LogEntry",
                                               logID);
                }
                else
                {
                    messages::internalError(asyncResp->res);
                }
                return;
            }

            std::string timestamp{};
            std::string filename{};
            std::string logfile{};
            ParseCrashdumpParameters(params, filename, timestamp, logfile);

            if (filename.empty() || timestamp.empty())
            {
                messages::resourceMissingAtURI(asyncResp->res, logID);
                return;
            }

            std::string crashdumpURI =
                "/redfish/v1/Systems/system/LogServices/Crashdump/Entries/" +
                logID + "/" + filename;
            logEntryJson = {{"@odata.type", "#LogEntry.v1_4_0.LogEntry"},
                            {"@odata.id", "/redfish/v1/Systems/system/"
                                          "LogServices/Crashdump/Entries/" +
                                              logID},
                            {"Name", "CPU Crashdump"},
                            {"Id", logID},
                            {"EntryType", "Oem"},
                            {"OemRecordFormat", "Crashdump URI"},
                            {"Message", std::move(crashdumpURI)},
                            {"Created", std::move(timestamp)}};
        };
    crow::connections::systemBus->async_method_call(
        std::move(getStoredLogCallback), crashdumpObject,
        crashdumpPath + std::string("/") + logID,
        "org.freedesktop.DBus.Properties", "GetAll", crashdumpInterface);
}

class CrashdumpEntryCollection : public Node
{
  public:
    template <typename CrowApp>
    CrashdumpEntryCollection(CrowApp& app) :
        Node(app, "/redfish/v1/Systems/system/LogServices/Crashdump/Entries/")
    {
        // Note: Deviated from redfish privilege registry for GET & HEAD
        // method for security reasons.
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::head, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    /**
     * Functions triggers appropriate requests on DBus
     */
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        // Collections don't include the static data added by SubRoute because
        // it has a duplicate entry for members
        auto getLogEntriesCallback = [asyncResp](
                                         const boost::system::error_code ec,
                                         const std::vector<std::string>& resp) {
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
            asyncResp->res.jsonValue["Name"] = "Open BMC Crashdump Entries";
            asyncResp->res.jsonValue["Description"] =
                "Collection of Crashdump Entries";
            nlohmann::json& logEntryArray = asyncResp->res.jsonValue["Members"];
            logEntryArray = nlohmann::json::array();
            std::vector<std::string> logIDs;
            // Get the list of log entries and build up an empty array big
            // enough to hold them
            for (const std::string& objpath : resp)
            {
                // Get the log ID
                std::size_t lastPos = objpath.rfind("/");
                if (lastPos == std::string::npos)
                {
                    continue;
                }
                logIDs.emplace_back(objpath.substr(lastPos + 1));

                // Add a space for the log entry to the array
                logEntryArray.push_back({});
            }
            // Now go through and set up async calls to fill in the entries
            size_t index = 0;
            for (const std::string& logID : logIDs)
            {
                // Add the log entry to the array
                logCrashdumpEntry(asyncResp, logID, logEntryArray[index++]);
            }
            asyncResp->res.jsonValue["Members@odata.count"] =
                logEntryArray.size();
        };
        crow::connections::systemBus->async_method_call(
            std::move(getLogEntriesCallback),
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths", "", 0,
            std::array<const char*, 1>{crashdumpInterface});
    }
};

class CrashdumpEntry : public Node
{
  public:
    CrashdumpEntry(CrowApp& app) :
        Node(app,
             "/redfish/v1/Systems/system/LogServices/Crashdump/Entries/<str>/",
             std::string())
    {
        // Note: Deviated from redfish privilege registry for GET & HEAD
        // method for security reasons.
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::head, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);
            return;
        }
        const std::string& logID = params[0];
        logCrashdumpEntry(asyncResp, logID, asyncResp->res.jsonValue);
    }
};

class CrashdumpFile : public Node
{
  public:
    CrashdumpFile(CrowApp& app) :
        Node(app,
             "/redfish/v1/Systems/system/LogServices/Crashdump/Entries/<str>/"
             "<str>/",
             std::string(), std::string())
    {
        // Note: Deviated from redfish privilege registry for GET & HEAD
        // method for security reasons.
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::head, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        if (params.size() != 2)
        {
            messages::internalError(asyncResp->res);
            return;
        }
        const std::string& logID = params[0];
        const std::string& fileName = params[1];

        auto getStoredLogCallback =
            [asyncResp, logID, fileName](
                const boost::system::error_code ec,
                const std::vector<std::pair<std::string, VariantType>>& resp) {
                if (ec)
                {
                    BMCWEB_LOG_DEBUG << "failed to get log ec: "
                                     << ec.message();
                    messages::internalError(asyncResp->res);
                    return;
                }

                std::string dbusFilename{};
                std::string dbusTimestamp{};
                std::string dbusFilepath{};

                ParseCrashdumpParameters(resp, dbusFilename, dbusTimestamp,
                                         dbusFilepath);

                if (dbusFilename.empty() || dbusTimestamp.empty() ||
                    dbusFilepath.empty())
                {
                    messages::resourceMissingAtURI(asyncResp->res, fileName);
                    return;
                }

                // Verify the file name parameter is correct
                if (fileName != dbusFilename)
                {
                    messages::resourceMissingAtURI(asyncResp->res, fileName);
                    return;
                }

                if (!std::filesystem::exists(dbusFilepath))
                {
                    messages::resourceMissingAtURI(asyncResp->res, fileName);
                    return;
                }
                std::ifstream ifs(dbusFilepath, std::ios::in |
                                                    std::ios::binary |
                                                    std::ios::ate);
                std::ifstream::pos_type fileSize = ifs.tellg();
                if (fileSize < 0)
                {
                    messages::generalError(asyncResp->res);
                    return;
                }
                ifs.seekg(0, std::ios::beg);

                auto crashData = std::make_unique<char[]>(
                    static_cast<unsigned int>(fileSize));

                ifs.read(crashData.get(), static_cast<int>(fileSize));

                // The cast to std::string is intentional in order to use the
                // assign() that applies move mechanics
                asyncResp->res.body().assign(
                    static_cast<std::string>(crashData.get()));

                // Configure this to be a file download when accessed from
                // a browser
                asyncResp->res.addHeader("Content-Disposition", "attachment");
            };
        crow::connections::systemBus->async_method_call(
            std::move(getStoredLogCallback), crashdumpObject,
            crashdumpPath + std::string("/") + logID,
            "org.freedesktop.DBus.Properties", "GetAll", crashdumpInterface);
    }
};

class OnDemandCrashdump : public Node
{
  public:
    OnDemandCrashdump(CrowApp& app) :
        Node(app,
             "/redfish/v1/Systems/system/LogServices/Crashdump/Actions/Oem/"
             "Crashdump.OnDemand/")
    {
        // Note: Deviated from redfish privilege registry for GET & HEAD
        // method for security reasons.
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::head, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::patch, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::put, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::post, {{"ConfigureComponents"}}}};
    }

  private:
    void doPost(crow::Response& res, const crow::Request& req,
                const std::vector<std::string>& params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);

        auto generateonDemandLogCallback = [asyncResp,
                                            req](const boost::system::error_code
                                                     ec,
                                                 const std::string& resp) {
            if (ec)
            {
                if (ec.value() == boost::system::errc::operation_not_supported)
                {
                    messages::resourceInStandby(asyncResp->res);
                }
                else if (ec.value() ==
                         boost::system::errc::device_or_resource_busy)
                {
                    messages::serviceTemporarilyUnavailable(asyncResp->res,
                                                            "60");
                }
                else
                {
                    messages::internalError(asyncResp->res);
                }
                return;
            }
            std::shared_ptr<task::TaskData> task = task::TaskData::createTask(
                [](boost::system::error_code err, sdbusplus::message::message&,
                   const std::shared_ptr<task::TaskData>& taskData) {
                    if (!err)
                    {
                        taskData->messages.emplace_back(
                            messages::taskCompletedOK(
                                std::to_string(taskData->index)));
                        taskData->state = "Completed";
                    }
                    return task::completed;
                },
                "type='signal',interface='org.freedesktop.DBus.Properties',"
                "member='PropertiesChanged',arg0namespace='com.intel."
                "crashdump'");
            task->startTimer(std::chrono::minutes(5));
            task->populateResp(asyncResp->res);
            task->payload.emplace(req);
        };
        crow::connections::systemBus->async_method_call(
            std::move(generateonDemandLogCallback), crashdumpObject,
            crashdumpPath, crashdumpOnDemandInterface, "GenerateOnDemandLog");
    }
};

class TelemetryCrashdump : public Node
{
  public:
    TelemetryCrashdump(CrowApp& app) :
        Node(app,
             "/redfish/v1/Systems/system/LogServices/Crashdump/Actions/Oem/"
             "Crashdump.Telemetry/")
    {
        // Note: Deviated from redfish privilege registry for GET & HEAD
        // method for security reasons.
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::head, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::patch, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::put, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::post, {{"ConfigureComponents"}}}};
    }

  private:
    void doPost(crow::Response& res, const crow::Request& req,
                const std::vector<std::string>& params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);

        auto generateTelemetryLogCallback = [asyncResp, req](
                                                const boost::system::error_code
                                                    ec,
                                                const std::string& resp) {
            if (ec)
            {
                if (ec.value() == boost::system::errc::operation_not_supported)
                {
                    messages::resourceInStandby(asyncResp->res);
                }
                else if (ec.value() ==
                         boost::system::errc::device_or_resource_busy)
                {
                    messages::serviceTemporarilyUnavailable(asyncResp->res,
                                                            "60");
                }
                else
                {
                    messages::internalError(asyncResp->res);
                }
                return;
            }
            std::shared_ptr<task::TaskData> task = task::TaskData::createTask(
                [](boost::system::error_code err, sdbusplus::message::message&,
                   const std::shared_ptr<task::TaskData>& taskData) {
                    if (!err)
                    {
                        taskData->messages.emplace_back(
                            messages::taskCompletedOK(
                                std::to_string(taskData->index)));
                        taskData->state = "Completed";
                    }
                    return task::completed;
                },
                "type='signal',interface='org.freedesktop.DBus.Properties',"
                "member='PropertiesChanged',arg0namespace='com.intel."
                "crashdump'");
            task->startTimer(std::chrono::minutes(5));
            task->populateResp(asyncResp->res);
            task->payload.emplace(req);
        };
        crow::connections::systemBus->async_method_call(
            std::move(generateTelemetryLogCallback), crashdumpObject,
            crashdumpPath, crashdumpTelemetryInterface, "GenerateTelemetryLog");
    }
};

class SendRawPECI : public Node
{
  public:
    SendRawPECI(CrowApp& app) :
        Node(app,
             "/redfish/v1/Systems/system/LogServices/Crashdump/Actions/Oem/"
             "Crashdump.SendRawPeci/")
    {
        // Note: Deviated from redfish privilege registry for GET & HEAD
        // method for security reasons.
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::head, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::patch, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::put, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::post, {{"ConfigureComponents"}}}};
    }

  private:
    void doPost(crow::Response& res, const crow::Request& req,
                const std::vector<std::string>& params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        std::vector<std::vector<uint8_t>> peciCommands;

        nlohmann::json reqJson =
            nlohmann::json::parse(req.body, nullptr, false);
        if (reqJson.find("PECICommands") != reqJson.end())
        {
            if (!json_util::readJson(req, res, "PECICommands", peciCommands))
            {
                return;
            }
            uint32_t idx = 0;
            for (auto const& cmd : peciCommands)
            {
                if (cmd.size() < 3)
                {
                    std::string s("[");
                    for (auto const& val : cmd)
                    {
                        if (val != *cmd.begin())
                        {
                            s += ",";
                        }
                        s += std::to_string(val);
                    }
                    s += "]";
                    messages::actionParameterValueFormatError(
                        res, s, "PECICommands[" + std::to_string(idx) + "]",
                        "SendRawPeci");
                    return;
                }
                idx++;
            }
        }
        else
        {
            /* This interface is deprecated */
            uint8_t clientAddress = 0;
            uint8_t readLength = 0;
            std::vector<uint8_t> peciCommand;
            if (!json_util::readJson(req, res, "ClientAddress", clientAddress,
                                     "ReadLength", readLength, "PECICommand",
                                     peciCommand))
            {
                return;
            }
            peciCommands.push_back({clientAddress, 0, readLength});
            peciCommands[0].insert(peciCommands[0].end(), peciCommand.begin(),
                                   peciCommand.end());
        }
        // Callback to return the Raw PECI response
        auto sendRawPECICallback =
            [asyncResp](const boost::system::error_code ec,
                        const std::vector<std::vector<uint8_t>>& resp) {
                if (ec)
                {
                    BMCWEB_LOG_DEBUG << "failed to process PECI commands ec: "
                                     << ec.message();
                    messages::internalError(asyncResp->res);
                    return;
                }
                asyncResp->res.jsonValue = {{"Name", "PECI Command Response"},
                                            {"PECIResponse", resp}};
            };
        // Call the SendRawPECI command with the provided data
        crow::connections::systemBus->async_method_call(
            std::move(sendRawPECICallback), crashdumpObject, crashdumpPath,
            crashdumpRawPECIInterface, "SendRawPeci", peciCommands);
    }
};

/**
 * DBusLogServiceActionsClear class supports POST method for ClearLog action.
 */
class DBusLogServiceActionsClear : public Node
{
  public:
    DBusLogServiceActionsClear(CrowApp& app) :
        Node(app, "/redfish/v1/Systems/system/LogServices/EventLog/Actions/"
                  "LogService.ClearLog/")
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
    void doPost(crow::Response& res, const crow::Request& req,
                const std::vector<std::string>& params) override
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

/****************************************************
 * Redfish PostCode interfaces
 * using DBUS interface: getPostCodesTS
 ******************************************************/
class PostCodesLogService : public Node
{
  public:
    PostCodesLogService(CrowApp& app) :
        Node(app, "/redfish/v1/Systems/system/LogServices/PostCodes/")
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
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);

        asyncResp->res.jsonValue = {
            {"@odata.id", "/redfish/v1/Systems/system/LogServices/PostCodes"},
            {"@odata.type", "#LogService.v1_1_0.LogService"},
            {"@odata.context", "/redfish/v1/$metadata#LogService.LogService"},
            {"Name", "POST Code Log Service"},
            {"Description", "POST Code Log Service"},
            {"Id", "BIOS POST Code Log"},
            {"OverWritePolicy", "WrapsWhenFull"},
            {"Entries",
             {{"@odata.id",
               "/redfish/v1/Systems/system/LogServices/PostCodes/Entries"}}}};
        asyncResp->res.jsonValue["Actions"]["#LogService.ClearLog"] = {
            {"target", "/redfish/v1/Systems/system/LogServices/PostCodes/"
                       "Actions/LogService.ClearLog"}};
    }
};

class PostCodesClear : public Node
{
  public:
    PostCodesClear(CrowApp& app) :
        Node(app, "/redfish/v1/Systems/system/LogServices/PostCodes/Actions/"
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
    void doPost(crow::Response& res, const crow::Request& req,
                const std::vector<std::string>& params) override
    {
        BMCWEB_LOG_DEBUG << "Do delete all postcodes entries.";

        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        // Make call to post-code service to request clear all
        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec) {
                if (ec)
                {
                    // TODO Handle for specific error code
                    BMCWEB_LOG_ERROR
                        << "doClearPostCodes resp_handler got error " << ec;
                    asyncResp->res.result(
                        boost::beast::http::status::internal_server_error);
                    messages::internalError(asyncResp->res);
                    return;
                }
            },
            "xyz.openbmc_project.State.Boot.PostCode",
            "/xyz/openbmc_project/State/Boot/PostCode",
            "xyz.openbmc_project.Collection.DeleteAll", "DeleteAll");
    }
};

static void fillPostCodeEntry(
    std::shared_ptr<AsyncResp> aResp,
    const boost::container::flat_map<uint64_t, uint64_t>& postcode,
    const uint16_t bootIndex, const uint64_t codeIndex = 0,
    const uint64_t skip = 0, const uint64_t top = 0)
{
    // Get the Message from the MessageRegistry
    const message_registries::Message* message =
        message_registries::getMessage("OpenBMC.0.1.BIOSPOSTCode");

    uint64_t currentCodeIndex = 0;
    nlohmann::json& logEntryArray = aResp->res.jsonValue["Members"];

    uint64_t firstCodeTimeUs = 0;
    for (const std::pair<uint64_t, uint64_t>& code : postcode)
    {
        currentCodeIndex++;
        std::string postcodeEntryID =
            "B" + std::to_string(bootIndex) + "-" +
            std::to_string(currentCodeIndex); // 1 based index in EntryID string

        uint64_t usecSinceEpoch = code.first;
        uint64_t usTimeOffset = 0;

        if (1 == currentCodeIndex)
        { // already incremented
            firstCodeTimeUs = code.first;
        }
        else
        {
            usTimeOffset = code.first - firstCodeTimeUs;
        }

        // skip if no specific codeIndex is specified and currentCodeIndex does
        // not fall between top and skip
        if ((codeIndex == 0) &&
            (currentCodeIndex <= skip || currentCodeIndex > top))
        {
            continue;
        }

        // skip if a specific codeIndex is specified and does not match the
        // currentIndex
        if ((codeIndex > 0) && (currentCodeIndex != codeIndex))
        {
            // This is done for simplicity. 1st entry is needed to calculate
            // time offset. To improve efficiency, one can get to the entry
            // directly (possibly with flatmap's nth method)
            continue;
        }

        // currentCodeIndex is within top and skip or equal to specified code
        // index

        // Get the Created time from the timestamp
        std::string entryTimeStr;
        if (!getTimestampStr(usecSinceEpoch, entryTimeStr))
        {
            continue;
        }

        // assemble messageArgs: BootIndex, TimeOffset(100us), PostCode(hex)
        std::ostringstream hexCode;
        hexCode << "0x" << std::setfill('0') << std::setw(2) << std::hex
                << code.second;
        std::ostringstream timeOffsetStr;
        // Set Fixed -Point Notation
        timeOffsetStr << std::fixed;
        // Set precision to 4 digits
        timeOffsetStr << std::setprecision(4);
        // Add double to stream
        timeOffsetStr << static_cast<double>(usTimeOffset) / 1000 / 1000;
        std::vector<std::string> messageArgs = {
            std::to_string(bootIndex), timeOffsetStr.str(), hexCode.str()};

        // Get MessageArgs template from message registry
        std::string msg;
        if (message != nullptr)
        {
            msg = message->message;

            // fill in this post code value
            int i = 0;
            for (const std::string& messageArg : messageArgs)
            {
                std::string argStr = "%" + std::to_string(++i);
                size_t argPos = msg.find(argStr);
                if (argPos != std::string::npos)
                {
                    msg.replace(argPos, argStr.length(), messageArg);
                }
            }
        }

        // Get Severity template from message registry
        std::string severity;
        if (message != nullptr)
        {
            severity = message->severity;
        }

        // add to AsyncResp
        logEntryArray.push_back({});
        nlohmann::json& bmcLogEntry = logEntryArray.back();
        bmcLogEntry = {
            {"@odata.type", "#LogEntry.v1_4_0.LogEntry"},
            {"@odata.context", "/redfish/v1/$metadata#LogEntry.LogEntry"},
            {"@odata.id", "/redfish/v1/Systems/system/LogServices/"
                          "PostCodes/Entries/" +
                              postcodeEntryID},
            {"Name", "POST Code Log Entry"},
            {"Id", postcodeEntryID},
            {"Message", std::move(msg)},
            {"MessageId", "OpenBMC.0.1.BIOSPOSTCode"},
            {"MessageArgs", std::move(messageArgs)},
            {"EntryType", "Event"},
            {"Severity", std::move(severity)},
            {"Created", std::move(entryTimeStr)}};
    }
}

static void getPostCodeForEntry(std::shared_ptr<AsyncResp> aResp,
                                const uint16_t bootIndex,
                                const uint64_t codeIndex)
{
    crow::connections::systemBus->async_method_call(
        [aResp, bootIndex, codeIndex](
            const boost::system::error_code ec,
            const boost::container::flat_map<uint64_t, uint64_t>& postcode) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS POST CODE PostCode response error";
                messages::internalError(aResp->res);
                return;
            }

            // skip the empty postcode boots
            if (postcode.empty())
            {
                return;
            }

            fillPostCodeEntry(aResp, postcode, bootIndex, codeIndex);

            aResp->res.jsonValue["Members@odata.count"] =
                aResp->res.jsonValue["Members"].size();
        },
        "xyz.openbmc_project.State.Boot.PostCode",
        "/xyz/openbmc_project/State/Boot/PostCode",
        "xyz.openbmc_project.State.Boot.PostCode", "GetPostCodesWithTimeStamp",
        bootIndex);
}

static void getPostCodeForBoot(std::shared_ptr<AsyncResp> aResp,
                               const uint16_t bootIndex,
                               const uint16_t bootCount,
                               const uint64_t entryCount, const uint64_t skip,
                               const uint64_t top)
{
    crow::connections::systemBus->async_method_call(
        [aResp, bootIndex, bootCount, entryCount, skip,
         top](const boost::system::error_code ec,
              const boost::container::flat_map<uint64_t, uint64_t>& postcode) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS POST CODE PostCode response error";
                messages::internalError(aResp->res);
                return;
            }

            uint64_t endCount = entryCount;
            if (!postcode.empty())
            {
                endCount = entryCount + postcode.size();

                if ((skip < endCount) && ((top + skip) > entryCount))
                {
                    uint64_t thisBootSkip =
                        std::max(skip, entryCount) - entryCount;
                    uint64_t thisBootTop =
                        std::min(top + skip, endCount) - entryCount;

                    fillPostCodeEntry(aResp, postcode, bootIndex, 0,
                                      thisBootSkip, thisBootTop);
                }
                aResp->res.jsonValue["Members@odata.count"] = endCount;
            }

            // continue to previous bootIndex
            if (bootIndex < bootCount)
            {
                getPostCodeForBoot(aResp, static_cast<uint16_t>(bootIndex + 1),
                                   bootCount, endCount, skip, top);
            }
            else
            {
                aResp->res.jsonValue["Members@odata.nextLink"] =
                    "/redfish/v1/Systems/system/LogServices/PostCodes/"
                    "Entries?$skip=" +
                    std::to_string(skip + top);
            }
        },
        "xyz.openbmc_project.State.Boot.PostCode",
        "/xyz/openbmc_project/State/Boot/PostCode",
        "xyz.openbmc_project.State.Boot.PostCode", "GetPostCodesWithTimeStamp",
        bootIndex);
}

static void getCurrentBootNumber(std::shared_ptr<AsyncResp> aResp,
                                 const uint64_t skip, const uint64_t top)
{
    uint64_t entryCount = 0;
    crow::connections::systemBus->async_method_call(
        [aResp, entryCount, skip,
         top](const boost::system::error_code ec,
              const std::variant<uint16_t>& bootCount) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                messages::internalError(aResp->res);
                return;
            }
            auto pVal = std::get_if<uint16_t>(&bootCount);
            if (pVal)
            {
                getPostCodeForBoot(aResp, 1, *pVal, entryCount, skip, top);
            }
            else
            {
                BMCWEB_LOG_DEBUG << "Post code boot index failed.";
            }
        },
        "xyz.openbmc_project.State.Boot.PostCode",
        "/xyz/openbmc_project/State/Boot/PostCode",
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.State.Boot.PostCode", "CurrentBootCycleCount");
}

class PostCodesEntryCollection : public Node
{
  public:
    template <typename CrowApp>
    PostCodesEntryCollection(CrowApp& app) :
        Node(app, "/redfish/v1/Systems/system/LogServices/PostCodes/Entries/")
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
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);

        asyncResp->res.jsonValue["@odata.type"] =
            "#LogEntryCollection.LogEntryCollection";
        asyncResp->res.jsonValue["@odata.context"] =
            "/redfish/v1/"
            "$metadata#LogEntryCollection.LogEntryCollection";
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Systems/system/LogServices/PostCodes/Entries";
        asyncResp->res.jsonValue["Name"] = "BIOS POST Code Log Entries";
        asyncResp->res.jsonValue["Description"] =
            "Collection of POST Code Log Entries";
        asyncResp->res.jsonValue["Members"] = nlohmann::json::array();
        asyncResp->res.jsonValue["Members@odata.count"] = 0;

        uint64_t skip = 0;
        uint64_t top = maxEntriesPerPage; // Show max entries by default
        if (!getSkipParam(asyncResp->res, req, skip))
        {
            return;
        }
        if (!getTopParam(asyncResp->res, req, top))
        {
            return;
        }
        getCurrentBootNumber(asyncResp, skip, top);
    }
};

class PostCodesEntry : public Node
{
  public:
    PostCodesEntry(CrowApp& app) :
        Node(app,
             "/redfish/v1/Systems/system/LogServices/PostCodes/Entries/<str>/",
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
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        const std::string& targetID = params[0];

        size_t bootPos = targetID.find('B');
        if (bootPos == std::string::npos)
        {
            // Requested ID was not found
            messages::resourceMissingAtURI(asyncResp->res, targetID);
            return;
        }
        std::string_view bootIndexStr(targetID);
        bootIndexStr.remove_prefix(bootPos + 1);
        uint16_t bootIndex = 0;
        uint64_t codeIndex = 0;
        size_t dashPos = bootIndexStr.find('-');

        if (dashPos == std::string::npos)
        {
            return;
        }
        std::string_view codeIndexStr(bootIndexStr);
        bootIndexStr.remove_suffix(dashPos);
        codeIndexStr.remove_prefix(dashPos + 1);

        bootIndex = static_cast<uint16_t>(
            strtoul(std::string(bootIndexStr).c_str(), NULL, 0));
        codeIndex = strtoul(std::string(codeIndexStr).c_str(), NULL, 0);
        if (bootIndex == 0 || codeIndex == 0)
        {
            BMCWEB_LOG_DEBUG << "Get Post Code invalid entry string "
                             << params[0];
        }

        asyncResp->res.jsonValue["@odata.type"] = "#LogEntry.v1_4_0.LogEntry";
        asyncResp->res.jsonValue["@odata.context"] =
            "/redfish/v1/$metadata#LogEntry.LogEntry";
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Systems/system/LogServices/PostCodes/"
            "Entries";
        asyncResp->res.jsonValue["Name"] = "BIOS POST Code Log Entries";
        asyncResp->res.jsonValue["Description"] =
            "Collection of POST Code Log Entries";
        asyncResp->res.jsonValue["Members"] = nlohmann::json::array();
        asyncResp->res.jsonValue["Members@odata.count"] = 0;

        getPostCodeForEntry(asyncResp, bootIndex, codeIndex);
    }
};

class SlvNodeLogService : public Node
{
  public:
    template <typename CrowApp>
    SlvNodeLogService(CrowApp& app) :
        Node(app, "/redfish/v1/Systems/system/LogServices/EventLogNode<str>/",
             std::string())
    {

        BMCWEB_LOG_ERROR << " 000 SlvNodeLogService: \n";
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);

        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);
            return;
        }
        const std::string& nodeId = params[0];

        asyncResp->res.jsonValue = {
            {"@odata.id",
             "/redfish/v1/Systems/system/LogServices/EventLogNode" + nodeId},
            {"@odata.type", "#LogService.v1_1_0.LogService"},
            {"@odata.context", "/redfish/v1/$metadata#LogService.LogService"},
            {"Name", "Slave Node Event Service"},
            {"Description", "Slave Node Event Service"},
            {"Id", "Slave Node Event Log"},
            {"OverWritePolicy", "WrapsWhenFull"},
            {"Entries",
             {{"@odata.id",
               "/redfish/v1/Systems/system/LogServices/EventLogNode" + nodeId +
                   "/Entries"}}}};
        asyncResp->res.jsonValue["Actions"]["#LogService.ClearLog"] = {
            {"target", "/redfish/v1/Systems/system/LogServices/EventLogNode" +
                           nodeId +
                           "/Actions"
                           "/LogService.ClearLog"}};
    }
};

class SlvNodeEventLogEntryCollection : public Node
{
  public:
    template <typename CrowApp>
    SlvNodeEventLogEntryCollection(CrowApp& app) :
        Node(
            app,
            "/redfish/v1/Systems/system/LogServices/EventLogNode<str>/Entries/",
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
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        uint64_t skip = 0;
        uint64_t top = maxEntriesPerPage; // Show max entries by default
        if (!getSkipParam(asyncResp->res, req, skip))
        {
            return;
        }
        if (!getTopParam(asyncResp->res, req, top))
        {
            return;
        }
        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);
            return;
        }
        const std::string& nodeId = params[0];

        // Collections don't include the static data added by SubRoute because
        // it has a duplicate entry for members
        asyncResp->res.jsonValue["@odata.type"] =
            "#LogEntryCollection.LogEntryCollection";
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Systems/system/LogServices/EventLogNode" + nodeId +
            "/Entries";
        asyncResp->res.jsonValue["Name"] = "System Event Log Entries";
        asyncResp->res.jsonValue["Description"] =
            "Collection of System Event Log Entries";

        nlohmann::json& logEntryArray = asyncResp->res.jsonValue["Members"];
        logEntryArray = nlohmann::json::array();
        // Go through the log files and create a unique ID for each entry
        uint64_t entryCount = 0;
        std::string logEntry;

        // Oldest logs are in the last file, so start there and loop backwards
        std::ifstream logStream(nodeLogPrefix + nodeId);
        if (!logStream.is_open())
        {
            return;
        }

        // Reset the unique ID on the first entry
        bool firstEntry = true;
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
            if (!getUniqueEntryID(logEntry, idStr, firstEntry))
            {
                continue;
            }

            if (firstEntry)
            {
                firstEntry = false;
            }

            logEntryArray.push_back({});
            nlohmann::json& bmcLogEntry = logEntryArray.back();
            if (fillEventLogEntryJson(idStr, logEntry, bmcLogEntry) != 0)
            {
                messages::internalError(asyncResp->res);
                return;
            }
        }
        asyncResp->res.jsonValue["Members@odata.count"] = entryCount;
        if (skip + top < entryCount)
        {
            asyncResp->res.jsonValue["Members@odata.nextLink"] =
                "/redfish/v1/Systems/system/LogServices/EventLogNode" + nodeId +
                "Entries?$skip=" + std::to_string(skip + top);
        }
    }
};

class SlvNodeEventLogEntry : public Node
{
  public:
    SlvNodeEventLogEntry(CrowApp& app) :
        Node(app,
             "/redfish/v1/Systems/system/LogServices/EventLogNode<str>/Entries/"
             "<str>/",
             std::string(), std::string())
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
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        if (params.size() != 2)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        const std::string& nodeId = params[0];
        const std::string& targetID = params[1];

        // Go through the log files and check the unique ID for each entry to
        // find the target entry
        std::string logEntry;

        std::ifstream logStream(nodeLogPrefix + nodeId);
        if (!logStream.is_open())
        {
            return;
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

            if (firstEntry)
            {
                firstEntry = false;
            }

            if (idStr == targetID)
            {
                if (fillEventLogEntryJson(idStr, logEntry,
                                          asyncResp->res.jsonValue) != 0)
                {
                    messages::internalError(asyncResp->res);
                    return;
                }
                return;
            }
        }
        // Requested ID was not found
        messages::resourceMissingAtURI(asyncResp->res, targetID);
    }
};

class SlvNodeEventLogClear : public Node
{
  public:
    SlvNodeEventLogClear(CrowApp& app) :
        Node(app,
             "/redfish/v1/Systems/system/LogServices/EventLogNode<str>/Actions/"
             "LogService.ClearLog/",
             std::string())
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
    void doPost(crow::Response& res, const crow::Request& req,
                const std::vector<std::string>& params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        const std::string& nodeId = params[0];
        if (std::stoi(nodeId) >= maxNodeCount)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        // Clear the EventLog by deleting the log files
        static const std::filesystem::path filename = nodeLogPrefix + nodeId;
        std::error_code ec;
        std::filesystem::remove(filename, ec);
    }
};

} // namespace redfish
