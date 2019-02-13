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

#include "filesystem.hpp"
#include "node.hpp"

#include <systemd/sd-journal.h>

#include <boost/container/flat_map.hpp>
#include <boost/utility/string_view.hpp>
#include <variant>

namespace redfish
{

constexpr char const *cpuLogObject = "com.intel.CpuDebugLog";
constexpr char const *cpuLogPath = "/com/intel/CpuDebugLog";
constexpr char const *cpuLogImmediatePath = "/com/intel/CpuDebugLog/Immediate";
constexpr char const *cpuLogInterface = "com.intel.CpuDebugLog";
constexpr char const *cpuLogImmediateInterface =
    "com.intel.CpuDebugLog.Immediate";
constexpr char const *cpuLogRawPECIInterface =
    "com.intel.CpuDebugLog.SendRawPeci";

namespace fs = std::filesystem;

static int getJournalMetadata(sd_journal *journal,
                              const boost::string_view &field,
                              boost::string_view &contents)
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
    contents = boost::string_view(data, length);
    // Only use the content after the "=" character.
    contents.remove_prefix(std::min(contents.find("=") + 1, contents.size()));
    return ret;
}

static int getJournalMetadata(sd_journal *journal,
                              const boost::string_view &field, const int &base,
                              int &contents)
{
    int ret = 0;
    boost::string_view metadata;
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
    boost::string_view t1(entryTime);
    boost::string_view t2(entryTime);
    if (t1.size() > 2 && t2.size() > 2)
    {
        t1.remove_suffix(2);
        t2.remove_prefix(t2.size() - 2);
    }
    entryTimestamp = t1.to_string() + ":" + t2.to_string();
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

static bool getTimestampFromID(crow::Response &res, const std::string &entryID,
                               uint64_t &timestamp, uint16_t &index)
{
    if (entryID.empty())
    {
        return false;
    }
    // Convert the unique ID back to a timestamp to find the entry
    boost::string_view tsStr(entryID);

    auto underscorePos = tsStr.find("_");
    if (underscorePos != tsStr.npos)
    {
        // Timestamp has an index
        tsStr.remove_suffix(tsStr.size() - underscorePos);
        boost::string_view indexStr(entryID);
        indexStr.remove_prefix(underscorePos + 1);
        std::size_t pos;
        try
        {
            index = std::stoul(indexStr.to_string(), &pos);
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
        timestamp = std::stoull(tsStr.to_string(), &pos);
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
            {{"@odata.id", "/redfish/v1/Systems/system/LogServices/CpuLog"}});
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
    }
};

static int fillEventLogEntryJson(const std::string &bmcLogEntryID,
                                 const boost::string_view &messageID,
                                 sd_journal *journal,
                                 nlohmann::json &bmcLogEntryJson)
{
    // Get the Log Entry contents
    int ret = 0;

    boost::string_view msg;
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

    // Get the MessageArgs from the journal entry by finding all of the
    // REDFISH_MESSAGE_ARG_x fields
    const void *data;
    size_t length;
    std::vector<std::string> messageArgs;
    SD_JOURNAL_FOREACH_DATA(journal, data, length)
    {
        boost::string_view field(static_cast<const char *>(data), length);
        if (field.starts_with("REDFISH_MESSAGE_ARG_"))
        {
            // Get the Arg number from the field name
            field.remove_prefix(sizeof("REDFISH_MESSAGE_ARG_") - 1);
            if (field.empty())
            {
                continue;
            }
            int argNum = std::strtoul(field.data(), nullptr, 10);
            if (argNum == 0)
            {
                continue;
            }
            // Get the Arg value after the "=" character.
            field.remove_prefix(std::min(field.find("=") + 1, field.size()));
            // Make sure we have enough space in messageArgs
            if (argNum > messageArgs.size())
            {
                messageArgs.resize(argNum);
            }
            messageArgs[argNum - 1] = field.to_string();
        }
    }

    // Get the Created time from the timestamp
    std::string entryTimeStr;
    if (!getEntryTimestamp(journal, entryTimeStr))
    {
        return 1;
    }

    // Fill in the log entry with the gathered data
    bmcLogEntryJson = {
        {"@odata.type", "#LogEntry.v1_3_0.LogEntry"},
        {"@odata.context", "/redfish/v1/$metadata#LogEntry.LogEntry"},
        {"@odata.id",
         "/redfish/v1/Systems/system/LogServices/EventLog/Entries/" +
             bmcLogEntryID},
        {"Name", "System Event Log Entry"},
        {"Id", bmcLogEntryID},
        {"Message", msg},
        {"MessageId", messageID},
        {"MessageArgs", std::move(messageArgs)},
        {"EntryType", "Event"},
        {"Severity",
         severity <= 2 ? "Critical"
                       : severity <= 4 ? "Warning" : severity <= 7 ? "OK" : ""},
        {"Created", std::move(entryTimeStr)}};
    return 0;
}

class EventLogEntryCollection : public Node
{
  public:
    template <typename CrowApp>
    EventLogEntryCollection(CrowApp &app) :
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

        // Go through the journal and create a unique ID for each entry
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
            // Look for only journal entries that contain a REDFISH_MESSAGE_ID
            // field
            boost::string_view messageID;
            ret = getJournalMetadata(journal.get(), "REDFISH_MESSAGE_ID",
                                     messageID);
            if (ret < 0)
            {
                continue;
            }

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
            nlohmann::json &bmcLogEntry = logEntryArray.back();
            if (fillEventLogEntryJson(idStr, messageID, journal.get(),
                                      bmcLogEntry) != 0)
            {
                messages::internalError(asyncResp->res);
                return;
            }
        }
        asyncResp->res.jsonValue["Members@odata.count"] = entryCount;
        if (skip + top < entryCount)
        {
            asyncResp->res.jsonValue["Members@odata.nextLink"] =
                "/redfish/v1/Managers/bmc/LogServices/BmcLog/Entries?$skip=" +
                std::to_string(skip + top);
        }
    }
};

class EventLogEntry : public Node
{
  public:
    EventLogEntry(CrowApp &app) :
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

        // only use journal entries that contain a REDFISH_MESSAGE_ID field
        boost::string_view messageID;
        ret =
            getJournalMetadata(journal.get(), "REDFISH_MESSAGE_ID", messageID);
        if (ret < 0)
        {
            messages::resourceNotFound(asyncResp->res, "LogEntry", "system");
            return;
        }

        if (fillEventLogEntryJson(entryID, messageID, journal.get(),
                                  asyncResp->res.jsonValue) != 0)
        {
            messages::internalError(asyncResp->res);
            return;
        }
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
             "/redfish/v1/Managers/bmc/LogServices/Journal/Entries/"}};
    }
};

static int fillBMCJournalLogEntryJson(const std::string &bmcJournalLogEntryID,
                                      sd_journal *journal,
                                      nlohmann::json &bmcJournalLogEntryJson)
{
    // Get the Log Entry contents
    int ret = 0;

    boost::string_view msg;
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
        {"@odata.type", "#LogEntry.v1_3_0.LogEntry"},
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
        {"OemRecordFormat", "Intel BMC Journal Entry"},
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

class CPULogService : public Node
{
  public:
    template <typename CrowApp>
    CPULogService(CrowApp &app) :
        Node(app, "/redfish/v1/Systems/system/LogServices/CpuLog/")
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
            "/redfish/v1/Systems/system/LogServices/CpuLog";
        asyncResp->res.jsonValue["@odata.type"] =
            "#LogService.v1_1_0.LogService";
        asyncResp->res.jsonValue["@odata.context"] =
            "/redfish/v1/$metadata#LogService.LogService";
        asyncResp->res.jsonValue["Name"] = "Open BMC CPU Log Service";
        asyncResp->res.jsonValue["Description"] = "CPU Log Service";
        asyncResp->res.jsonValue["Id"] = "CPU Log";
        asyncResp->res.jsonValue["OverWritePolicy"] = "WrapsWhenFull";
        asyncResp->res.jsonValue["MaxNumberOfRecords"] = 3;
        asyncResp->res.jsonValue["Entries"] = {
            {"@odata.id",
             "/redfish/v1/Managers/bmc/LogServices/CpuLog/Entries"}};
        asyncResp->res.jsonValue["Actions"] = {
            {"Oem",
             {{"#CpuLog.Immediate",
               {{"target", "/redfish/v1/Systems/system/LogServices/CpuLog/"
                           "Actions/Oem/CpuLog.Immediate"}}}}}};

#ifdef BMCWEB_ENABLE_REDFISH_RAW_PECI
        asyncResp->res.jsonValue["Actions"]["Oem"].push_back(
            {"#CpuLog.SendRawPeci",
             {{"target", "/redfish/v1/Systems/system/LogServices/CpuLog/"
                         "Actions/Oem/CpuLog.SendRawPeci"}}});
#endif
    }
};

class CPULogEntryCollection : public Node
{
  public:
    template <typename CrowApp>
    CPULogEntryCollection(CrowApp &app) :
        Node(app, "/redfish/v1/Systems/system/LogServices/CpuLog/Entries/")
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
                "/redfish/v1/Systems/system/LogServices/CpuLog/Entries";
            asyncResp->res.jsonValue["@odata.context"] =
                "/redfish/v1/"
                "$metadata#LogEntryCollection.LogEntryCollection";
            asyncResp->res.jsonValue["Name"] = "Open BMC CPU Log Entries";
            asyncResp->res.jsonValue["Description"] =
                "Collection of CPU Log Entries";
            nlohmann::json &logEntryArray = asyncResp->res.jsonValue["Members"];
            logEntryArray = nlohmann::json::array();
            for (const std::string &objpath : resp)
            {
                // Don't list the immediate log
                if (objpath.compare(cpuLogImmediatePath) == 0)
                {
                    continue;
                }
                std::size_t lastPos = objpath.rfind("/");
                if (lastPos != std::string::npos)
                {
                    logEntryArray.push_back(
                        {{"@odata.id", "/redfish/v1/Systems/system/LogServices/"
                                       "CpuLog/Entries/" +
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
            std::array<const char *, 1>{cpuLogInterface});
    }
};

std::string getLogCreatedTime(const nlohmann::json &cpuLog)
{
    nlohmann::json::const_iterator cdIt = cpuLog.find("crashlog_data");
    if (cdIt != cpuLog.end())
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

class CPULogEntry : public Node
{
  public:
    CPULogEntry(CrowApp &app) :
        Node(app,
             "/redfish/v1/Systems/system/LogServices/CpuLog/Entries/<str>/",
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
                {"@odata.type", "#LogEntry.v1_3_0.LogEntry"},
                {"@odata.context", "/redfish/v1/$metadata#LogEntry.LogEntry"},
                {"@odata.id",
                 "/redfish/v1/Systems/system/LogServices/CpuLog/Entries/" +
                     std::to_string(logId)},
                {"Name", "CPU Debug Log"},
                {"Id", logId},
                {"EntryType", "Oem"},
                {"OemRecordFormat", "Intel CPU Log"},
                {"Oem", {{"Intel", std::move(j)}}},
                {"Created", std::move(t)}};
        };
        crow::connections::systemBus->async_method_call(
            std::move(getStoredLogCallback), cpuLogObject,
            cpuLogPath + std::string("/") + std::to_string(logId),
            "org.freedesktop.DBus.Properties", "Get", cpuLogInterface, "Log");
    }
};

class ImmediateCPULog : public Node
{
  public:
    ImmediateCPULog(CrowApp &app) :
        Node(app, "/redfish/v1/Systems/system/LogServices/CpuLog/Actions/Oem/"
                  "CpuLog.Immediate/")
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
        static std::unique_ptr<sdbusplus::bus::match::match>
            immediateLogMatcher;

        // Only allow one Immediate Log request at a time
        if (immediateLogMatcher != nullptr)
        {
            asyncResp->res.addHeader("Retry-After", "30");
            messages::serviceTemporarilyUnavailable(asyncResp->res, "30");
            return;
        }
        // Make this static so it survives outside this method
        static boost::asio::deadline_timer timeout(*req.ioService);

        timeout.expires_from_now(boost::posix_time::seconds(30));
        timeout.async_wait([asyncResp](const boost::system::error_code &ec) {
            immediateLogMatcher = nullptr;
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
            BMCWEB_LOG_ERROR << "Timed out waiting for immediate log";

            messages::internalError(asyncResp->res);
        });

        auto immediateLogMatcherCallback = [asyncResp](
                                               sdbusplus::message::message &m) {
            BMCWEB_LOG_DEBUG << "Immediate log available match fired";
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
                &interfacesAdded[cpuLogInterface]["Log"]);
            if (log == nullptr)
            {
                messages::internalError(asyncResp->res);
                // Careful with immediateLogMatcher.  It is a unique_ptr to the
                // match object inside which this lambda is executing.  Once it
                // is set to nullptr, the match object will be destroyed and the
                // lambda will lose its context, including res, so it needs to
                // be the last thing done.
                immediateLogMatcher = nullptr;
                return;
            }
            nlohmann::json j = nlohmann::json::parse(*log, nullptr, false);
            if (j.is_discarded())
            {
                messages::internalError(asyncResp->res);
                // Careful with immediateLogMatcher.  It is a unique_ptr to the
                // match object inside which this lambda is executing.  Once it
                // is set to nullptr, the match object will be destroyed and the
                // lambda will lose its context, including res, so it needs to
                // be the last thing done.
                immediateLogMatcher = nullptr;
                return;
            }
            std::string t = getLogCreatedTime(j);
            asyncResp->res.jsonValue = {
                {"@odata.type", "#LogEntry.v1_3_0.LogEntry"},
                {"@odata.context", "/redfish/v1/$metadata#LogEntry.LogEntry"},
                {"Name", "CPU Debug Log"},
                {"EntryType", "Oem"},
                {"OemRecordFormat", "Intel CPU Log"},
                {"Oem", {{"Intel", std::move(j)}}},
                {"Created", std::move(t)}};
            // Careful with immediateLogMatcher.  It is a unique_ptr to the
            // match object inside which this lambda is executing.  Once it is
            // set to nullptr, the match object will be destroyed and the lambda
            // will lose its context, including res, so it needs to be the last
            // thing done.
            immediateLogMatcher = nullptr;
        };
        immediateLogMatcher = std::make_unique<sdbusplus::bus::match::match>(
            *crow::connections::systemBus,
            sdbusplus::bus::match::rules::interfacesAdded() +
                sdbusplus::bus::match::rules::argNpath(0, cpuLogImmediatePath),
            std::move(immediateLogMatcherCallback));

        auto generateImmediateLogCallback =
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
                    immediateLogMatcher = nullptr;
                    return;
                }
            };
        crow::connections::systemBus->async_method_call(
            std::move(generateImmediateLogCallback), cpuLogObject, cpuLogPath,
            cpuLogImmediateInterface, "GenerateImmediateLog");
    }
};

class SendRawPECI : public Node
{
  public:
    SendRawPECI(CrowApp &app) :
        Node(app, "/redfish/v1/Systems/system/LogServices/CpuLog/Actions/Oem/"
                  "CpuLog.SendRawPeci/")
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
            std::move(sendRawPECICallback), cpuLogObject, cpuLogPath,
            cpuLogRawPECIInterface, "SendRawPeci", clientAddress, readLength,
            peciCommand);
    }
};

} // namespace redfish
