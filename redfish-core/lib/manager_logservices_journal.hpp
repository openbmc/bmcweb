#pragma once

#include "app.hpp"
#include "error_messages.hpp"
#include "generated/enums/log_entry.hpp"
#include "query.hpp"
#include "registries/base_message_registry.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/time_utils.hpp"

#include <systemd/sd-journal.h>

#include <boost/beast/http/verb.hpp>

#include <array>
#include <memory>
#include <string>
#include <string_view>

namespace redfish
{

inline int getJournalMetadata(sd_journal* journal, std::string_view field,
                              std::string_view& contents)
{
    const char* data = nullptr;
    size_t length = 0;
    int ret = 0;
    // Get the metadata from the requested field of the journal entry
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    const void** dataVoid = reinterpret_cast<const void**>(&data);

    ret = sd_journal_get_data(journal, field.data(), dataVoid, &length);
    if (ret < 0)
    {
        return ret;
    }
    contents = std::string_view(data, length);
    // Only use the content after the "=" character.
    contents.remove_prefix(std::min(contents.find('=') + 1, contents.size()));
    return ret;
}

inline int getJournalMetadataInt(sd_journal* journal, std::string_view field,
                                 const int& base, long int& contents)
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

inline bool getEntryTimestamp(sd_journal* journal, std::string& entryTimestamp)
{
    int ret = 0;
    uint64_t timestamp = 0;
    ret = sd_journal_get_realtime_usec(journal, &timestamp);
    if (ret < 0)
    {
        BMCWEB_LOG_ERROR("Failed to read entry timestamp: {}", strerror(-ret));
        return false;
    }
    entryTimestamp = redfish::time_utils::getDateTimeUintUs(timestamp);
    return true;
}

inline bool fillBMCJournalLogEntryJson(
    sd_journal* journal, nlohmann::json::object_t& bmcJournalLogEntryJson)
{
    char* cursor = nullptr;
    int ret = sd_journal_get_cursor(journal, &cursor);
    if (ret < 0)
    {
        return false;
    }
    std::unique_ptr<char*> cursorptr(cursor);
    std::string bmcJournalLogEntryID(cursor);

    // Get the Log Entry contents
    std::string message;
    std::string_view syslogID;
    ret = getJournalMetadata(journal, "SYSLOG_IDENTIFIER", syslogID);
    if (ret < 0)
    {
        BMCWEB_LOG_DEBUG("Failed to read SYSLOG_IDENTIFIER field: {}",
                         strerror(-ret));
    }
    if (!syslogID.empty())
    {
        message += std::string(syslogID) + ": ";
    }

    std::string_view msg;
    ret = getJournalMetadata(journal, "MESSAGE", msg);
    if (ret < 0)
    {
        BMCWEB_LOG_ERROR("Failed to read MESSAGE field: {}", strerror(-ret));
        return false;
    }
    message += std::string(msg);

    // Get the severity from the PRIORITY field
    long int severity = 8; // Default to an invalid priority
    ret = getJournalMetadataInt(journal, "PRIORITY", 10, severity);
    if (ret < 0)
    {
        BMCWEB_LOG_DEBUG("Failed to read PRIORITY field: {}", strerror(-ret));
    }

    // Get the Created time from the timestamp
    std::string entryTimeStr;
    if (!getEntryTimestamp(journal, entryTimeStr))
    {
        return false;
    }

    // Fill in the log entry with the gathered data
    bmcJournalLogEntryJson["@odata.type"] = "#LogEntry.v1_9_0.LogEntry";

    std::string entryIdBase64 =
        crow::utility::base64encodeUrlSafe(bmcJournalLogEntryID);

    bmcJournalLogEntryJson["@odata.id"] = boost::urls::format(
        "/redfish/v1/Managers/{}/LogServices/Journal/Entries/{}",
        BMCWEB_REDFISH_MANAGER_URI_NAME, entryIdBase64);
    bmcJournalLogEntryJson["Name"] = "BMC Journal Entry";
    bmcJournalLogEntryJson["Id"] = entryIdBase64;
    bmcJournalLogEntryJson["Message"] = std::move(message);
    bmcJournalLogEntryJson["EntryType"] = log_entry::LogEntryType::Oem;
    log_entry::EventSeverity severityEnum = log_entry::EventSeverity::OK;
    if (severity <= 2)
    {
        severityEnum = log_entry::EventSeverity::Critical;
    }
    else if (severity <= 4)
    {
        severityEnum = log_entry::EventSeverity::Warning;
    }

    bmcJournalLogEntryJson["Severity"] = severityEnum;
    bmcJournalLogEntryJson["OemRecordFormat"] = "BMC Journal Entry";
    bmcJournalLogEntryJson["Created"] = std::move(entryTimeStr);
    return true;
}

inline void handleManagersLogServiceJournalGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& managerId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    if (managerId != BMCWEB_REDFISH_MANAGER_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "Manager", managerId);
        return;
    }

    asyncResp->res.jsonValue["@odata.type"] = "#LogService.v1_2_0.LogService";
    asyncResp->res.jsonValue["@odata.id"] =
        boost::urls::format("/redfish/v1/Managers/{}/LogServices/Journal",
                            BMCWEB_REDFISH_MANAGER_URI_NAME);
    asyncResp->res.jsonValue["Name"] = "Open BMC Journal Log Service";
    asyncResp->res.jsonValue["Description"] = "BMC Journal Log Service";
    asyncResp->res.jsonValue["Id"] = "Journal";
    asyncResp->res.jsonValue["OverWritePolicy"] = "WrapsWhenFull";

    std::pair<std::string, std::string> redfishDateTimeOffset =
        redfish::time_utils::getDateTimeOffsetNow();
    asyncResp->res.jsonValue["DateTime"] = redfishDateTimeOffset.first;
    asyncResp->res.jsonValue["DateTimeLocalOffset"] =
        redfishDateTimeOffset.second;

    asyncResp->res.jsonValue["Entries"]["@odata.id"] = boost::urls::format(
        "/redfish/v1/Managers/{}/LogServices/Journal/Entries",
        BMCWEB_REDFISH_MANAGER_URI_NAME);
}

struct JournalReadState
{
    std::unique_ptr<sd_journal, decltype(&sd_journal_close)> journal;
};

inline void readJournalEntries(
    uint64_t topEntryCount, const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    JournalReadState&& readState)
{
    nlohmann::json& logEntry = asyncResp->res.jsonValue["Members"];
    nlohmann::json::array_t* logEntryArray =
        logEntry.get_ptr<nlohmann::json::array_t*>();
    if (logEntryArray == nullptr)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    // The Journal APIs unfortunately do blocking calls to the filesystem, and
    // can be somewhat expensive.  Short of creating our own io_uring based
    // implementation of sd-journal, which would be difficult, the best thing we
    // can do is to only parse a certain number of entries at a time.  The
    // current chunk size is selected arbitrarily to ensure that we're not
    // trying to process thousands of entries at the same time.
    // The implementation will process the number of entries, then return
    // control to the io_context to let other operations continue.
    size_t segmentCountRemaining = 10;

    // Reset the unique ID on the first entry
    for (uint64_t entryCount = logEntryArray->size();
         entryCount < topEntryCount; entryCount++)
    {
        if (segmentCountRemaining == 0)
        {
            boost::asio::post(crow::connections::systemBus->get_io_context(),
                              [asyncResp, topEntryCount,
                               readState = std::move(readState)]() mutable {
                                  readJournalEntries(topEntryCount, asyncResp,
                                                     std::move(readState));
                              });
            return;
        }

        nlohmann::json::object_t bmcJournalLogEntry;
        if (!fillBMCJournalLogEntryJson(readState.journal.get(),
                                        bmcJournalLogEntry))
        {
            messages::internalError(asyncResp->res);
            return;
        }
        logEntryArray->emplace_back(std::move(bmcJournalLogEntry));

        int ret = sd_journal_next(readState.journal.get());
        if (ret < 0)
        {
            messages::internalError(asyncResp->res);
            return;
        }
        if (ret == 0)
        {
            break;
        }
        segmentCountRemaining--;
    }
}

inline void handleManagersJournalLogEntryCollectionGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& managerId)
{
    query_param::QueryCapabilities capabilities = {
        .canDelegateTop = true,
        .canDelegateSkip = true,
    };
    query_param::Query delegatedQuery;
    if (!redfish::setUpRedfishRouteWithDelegation(app, req, asyncResp,
                                                  delegatedQuery, capabilities))
    {
        return;
    }

    if (managerId != BMCWEB_REDFISH_MANAGER_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "Manager", managerId);
        return;
    }

    size_t skip = delegatedQuery.skip.value_or(0);
    size_t top = delegatedQuery.top.value_or(query_param::Query::maxTop);

    // Collections don't include the static data added by SubRoute
    // because it has a duplicate entry for members
    asyncResp->res.jsonValue["@odata.type"] =
        "#LogEntryCollection.LogEntryCollection";
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Managers/{}/LogServices/Journal/Entries",
        BMCWEB_REDFISH_MANAGER_URI_NAME);
    asyncResp->res.jsonValue["Name"] = "Open BMC Journal Entries";
    asyncResp->res.jsonValue["Description"] =
        "Collection of BMC Journal Entries";
    asyncResp->res.jsonValue["Members"] = nlohmann::json::array_t();

    // Go through the journal and use the timestamp to create a
    // unique ID for each entry
    sd_journal* journalTmp = nullptr;
    int ret = sd_journal_open(&journalTmp, SD_JOURNAL_LOCAL_ONLY);
    if (ret < 0)
    {
        BMCWEB_LOG_ERROR("failed to open journal: {}", strerror(-ret));
        messages::internalError(asyncResp->res);
        return;
    }

    std::unique_ptr<sd_journal, decltype(&sd_journal_close)> journal(
        journalTmp, sd_journal_close);
    journalTmp = nullptr;

    // Seek to the end
    if (sd_journal_seek_tail(journal.get()) < 0)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    // Get the last entry
    if (sd_journal_previous(journal.get()) < 0)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    // Get the last sequence number
    uint64_t endSeqNum = 0;
#if LIBSYSTEMD_VERSION >= 254
    {
        if (sd_journal_get_seqnum(journal.get(), &endSeqNum, nullptr) < 0)
        {
            messages::internalError(asyncResp->res);
            return;
        }
    }
#endif

    // Seek to the beginning
    if (sd_journal_seek_head(journal.get()) < 0)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    // Get the first entry
    if (sd_journal_next(journal.get()) < 0)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    // Get the first sequence number
    uint64_t startSeqNum = 0;
#if LIBSYSTEMD_VERSION >= 254
    {
        if (sd_journal_get_seqnum(journal.get(), &startSeqNum, nullptr) < 0)
        {
            messages::internalError(asyncResp->res);
            return;
        }
    }
#endif

    BMCWEB_LOG_DEBUG("journal Sequence IDs start:{} end:{}", startSeqNum,
                     endSeqNum);

    // Add 1 to account for the last entry
    uint64_t totalEntries = endSeqNum - startSeqNum + 1;
    asyncResp->res.jsonValue["Members@odata.count"] = totalEntries;
    if (skip + top < totalEntries)
    {
        asyncResp->res.jsonValue["Members@odata.nextLink"] =
            boost::urls::format(
                "/redfish/v1/Managers/{}/LogServices/Journal/Entries?$skip={}",
                BMCWEB_REDFISH_MANAGER_URI_NAME, std::to_string(skip + top));
    }
    uint64_t index = 0;
    if (skip > 0)
    {
        if (sd_journal_next_skip(journal.get(), skip) < 0)
        {
            messages::internalError(asyncResp->res);
            return;
        }
    }
    BMCWEB_LOG_DEBUG("Index was {}", index);
    readJournalEntries(top, asyncResp, {std::move(journal)});
}

inline void handleManagersJournalEntriesLogEntryGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& managerId, const std::string& entryID)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    if (managerId != BMCWEB_REDFISH_MANAGER_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "Manager", managerId);
        return;
    }

    sd_journal* journalTmp = nullptr;
    int ret = sd_journal_open(&journalTmp, SD_JOURNAL_LOCAL_ONLY);
    if (ret < 0)
    {
        BMCWEB_LOG_ERROR("failed to open journal: {}", strerror(-ret));
        messages::internalError(asyncResp->res);
        return;
    }
    std::unique_ptr<sd_journal, decltype(&sd_journal_close)> journal(
        journalTmp, sd_journal_close);
    journalTmp = nullptr;

    std::string cursor;
    if (!crow::utility::base64Decode(entryID, cursor,
                                     crow::utility::decodingDataUrlSafe))
    {
        messages::resourceNotFound(asyncResp->res, "LogEntry", entryID);
        return;
    }

    // Go to the cursor in the log
    ret = sd_journal_seek_cursor(journal.get(), cursor.c_str());
    if (ret < 0)
    {
        messages::resourceNotFound(asyncResp->res, "LogEntry", entryID);
        return;
    }

    if (sd_journal_next(journal.get()) < 0)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    ret = sd_journal_test_cursor(journal.get(), cursor.c_str());
    if (ret == 0)
    {
        messages::resourceNotFound(asyncResp->res, "LogEntry", entryID);
        return;
    }
    if (ret < 0)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    nlohmann::json::object_t bmcJournalLogEntry;
    if (!fillBMCJournalLogEntryJson(journal.get(), bmcJournalLogEntry))
    {
        messages::internalError(asyncResp->res);
        return;
    }
    asyncResp->res.jsonValue.update(bmcJournalLogEntry);
}

inline void requestRoutesBMCJournalLogService(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Managers/<str>/LogServices/Journal/")
        .privileges(redfish::privileges::getLogService)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleManagersLogServiceJournalGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Managers/<str>/LogServices/Journal/Entries/")
        .privileges(redfish::privileges::getLogEntryCollection)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleManagersJournalLogEntryCollectionGet, std::ref(app)));

    BMCWEB_ROUTE(
        app, "/redfish/v1/Managers/<str>/LogServices/Journal/Entries/<str>/")
        .privileges(redfish::privileges::getLogEntry)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleManagersJournalEntriesLogEntryGet, std::ref(app)));
}
} // namespace redfish
