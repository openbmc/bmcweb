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
// Entry is formed like "BootID_timestamp" or "BootID_timestamp_index"
inline bool
    getTimestampFromID(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       std::string_view entryIDStrView, sd_id128_t& bootID,
                       uint64_t& timestamp, uint64_t& index)
{
    // Convert the unique ID back to a bootID + timestamp to find the entry
    auto underscore1Pos = entryIDStrView.find('_');
    if (underscore1Pos == std::string_view::npos)
    {
        // EntryID has no bootID or timestamp
        messages::resourceNotFound(asyncResp->res, "LogEntry", entryIDStrView);
        return false;
    }

    // EntryID has bootID + timestamp

    // Convert entryIDViewString to BootID
    // NOTE: bootID string which needs to be null-terminated for
    // sd_id128_from_string()
    std::string bootIDStr(entryIDStrView.substr(0, underscore1Pos));
    if (sd_id128_from_string(bootIDStr.c_str(), &bootID) < 0)
    {
        messages::resourceNotFound(asyncResp->res, "LogEntry", entryIDStrView);
        return false;
    }

    // Get the timestamp from entryID
    entryIDStrView.remove_prefix(underscore1Pos + 1);

    auto [timestampEnd, tstampEc] = std::from_chars(
        entryIDStrView.begin(), entryIDStrView.end(), timestamp);
    if (tstampEc != std::errc())
    {
        messages::resourceNotFound(asyncResp->res, "LogEntry", entryIDStrView);
        return false;
    }
    entryIDStrView = std::string_view(
        timestampEnd,
        static_cast<size_t>(std::distance(timestampEnd, entryIDStrView.end())));
    if (entryIDStrView.empty())
    {
        index = 0U;
        return true;
    }
    // Timestamp might include optional index, if two events happened at the
    // same "time".
    if (entryIDStrView[0] != '_')
    {
        messages::resourceNotFound(asyncResp->res, "LogEntry", entryIDStrView);
        return false;
    }
    entryIDStrView.remove_prefix(1);
    auto [ptr, indexEc] =
        std::from_chars(entryIDStrView.begin(), entryIDStrView.end(), index);
    if (indexEc != std::errc() || ptr != entryIDStrView.end())
    {
        messages::resourceNotFound(asyncResp->res, "LogEntry", entryIDStrView);
        return false;
    }
    if (index <= 1)
    {
        // Indexes go directly from no postfix (handled above) to _2
        // so if we ever see _0 or _1, it's incorrect
        messages::resourceNotFound(asyncResp->res, "LogEntry", entryIDStrView);
        return false;
    }

    // URI indexes are one based, journald is zero based
    index -= 1;
    return true;
}

inline std::string getUniqueEntryID(uint64_t index, uint64_t curTs,
                                    sd_id128_t& curBootID)
{
    // make entryID as <bootID>_<timestamp>[_<index>]
    std::array<char, SD_ID128_STRING_MAX> bootIDStr{};
    sd_id128_to_string(curBootID, bootIDStr.data());
    std::string postfix;
    if (index > 0)
    {
        postfix = std::format("_{}", index + 1);
    }
    return std::format("{}_{}{}", bootIDStr.data(), curTs, postfix);
}

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
    entryTimestamp = time_utils::getDateTimeUintUs(timestamp);
    return true;
}

inline bool fillBMCJournalLogEntryJson(
    const std::string& bmcJournalLogEntryID, sd_journal* journal,
    nlohmann::json::object_t& bmcJournalLogEntryJson)
{
    // Get the Log Entry contents
    std::string message;
    std::string_view syslogID;
    int ret = getJournalMetadata(journal, "SYSLOG_IDENTIFIER", syslogID);
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
    bmcJournalLogEntryJson["@odata.id"] = boost::urls::format(
        "/redfish/v1/Managers/{}/LogServices/Journal/Entries/{}",
        BMCWEB_REDFISH_MANAGER_URI_NAME, bmcJournalLogEntryID);
    bmcJournalLogEntryJson["Name"] = "BMC Journal Entry";
    bmcJournalLogEntryJson["Id"] = bmcJournalLogEntryID;
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
    if (!setUpRedfishRoute(app, req, asyncResp))
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
        time_utils::getDateTimeOffsetNow();
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
    uint64_t index = 0;
    sd_id128_t prevBootID{};
    uint64_t prevTs = 0;
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

        // Get the entry timestamp
        sd_id128_t curBootID{};
        uint64_t curTs = 0;
        int ret = sd_journal_get_monotonic_usec(readState.journal.get(), &curTs,
                                                &curBootID);
        if (ret < 0)
        {
            BMCWEB_LOG_ERROR("Failed to read entry timestamp: {}",
                             strerror(-ret));
            messages::internalError(asyncResp->res);
            return;
        }

        // If the timestamp isn't unique on the same boot, increment the index
        bool sameBootIDs = sd_id128_equal(curBootID, readState.prevBootID) != 0;
        if (sameBootIDs && (curTs == readState.prevTs))
        {
            readState.index++;
        }
        else
        {
            // Otherwise, reset it
            readState.index = 0;
        }

        // Save the bootID
        readState.prevBootID = curBootID;

        // Save the timestamp
        readState.prevTs = curTs;

        std::string idStr = getUniqueEntryID(readState.index, curTs, curBootID);

        nlohmann::json::object_t bmcJournalLogEntry;
        if (!fillBMCJournalLogEntryJson(idStr, readState.journal.get(),
                                        bmcJournalLogEntry))
        {
            messages::internalError(asyncResp->res);
            return;
        }
        logEntryArray->emplace_back(std::move(bmcJournalLogEntry));

        ret = sd_journal_next(readState.journal.get());
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
    if (!setUpRedfishRouteWithDelegation(app, req, asyncResp, delegatedQuery,
                                         capabilities))
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
    sd_id128_t curBootID{};
    uint64_t curTs = 0;
    if (skip > 0)
    {
        if (sd_journal_next_skip(journal.get(), skip) < 0)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        // Get the entry timestamp
        ret = sd_journal_get_monotonic_usec(journal.get(), &curTs, &curBootID);
        if (ret < 0)
        {
            BMCWEB_LOG_ERROR("Failed to read entry timestamp: {}",
                             strerror(-ret));
            messages::internalError(asyncResp->res);
            return;
        }

        uint64_t endChunkSeqNum = 0;
#if LIBSYSTEMD_VERSION >= 254
        {
            if (sd_journal_get_seqnum(journal.get(), &endChunkSeqNum, nullptr) <
                0)
            {
                messages::internalError(asyncResp->res);
                return;
            }
        }
#endif

        // Seek to the first entry with the same timestamp and boot
        ret = sd_journal_seek_monotonic_usec(journal.get(), curBootID, curTs);
        if (ret < 0)
        {
            BMCWEB_LOG_ERROR("Failed to seek: {}", strerror(-ret));
            messages::internalError(asyncResp->res);
            return;
        }
        if (sd_journal_next(journal.get()) < 0)
        {
            messages::internalError(asyncResp->res);
            return;
        }
        uint64_t startChunkSeqNum = 0;
#if LIBSYSTEMD_VERSION >= 254
        {
            if (sd_journal_get_seqnum(journal.get(), &startChunkSeqNum,
                                      nullptr) < 0)
            {
                messages::internalError(asyncResp->res);
                return;
            }
        }
#endif

        // Get the difference between the start and end.  Most of the time this
        // will be 0
        BMCWEB_LOG_DEBUG("start={} end={}", startChunkSeqNum, endChunkSeqNum);
        index = endChunkSeqNum - startChunkSeqNum;
        if (index > endChunkSeqNum)
        {
            // Detect underflows.  Should never happen.
            messages::internalError(asyncResp->res);
            return;
        }
        if (index > 0)
        {
            BMCWEB_LOG_DEBUG("index = {}", index);
            if (sd_journal_next_skip(journal.get(), index) < 0)
            {
                messages::internalError(asyncResp->res);
                return;
            }
        }
    }
    // If this is the first entry of this series, reset the timestamps so the
    // Index doesn't increment
    if (index == 0)
    {
        curBootID = {};
        curTs = 0;
    }
    else
    {
        index -= 1;
    }
    BMCWEB_LOG_DEBUG("Index was {}", index);
    readJournalEntries(top, asyncResp,
                       {std::move(journal), index, curBootID, curTs});
}

inline void handleManagersJournalEntriesLogEntryGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& managerId, const std::string& entryID)
{
    if (!setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    if (managerId != BMCWEB_REDFISH_MANAGER_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "Manager", managerId);
        return;
    }

    // Convert the unique ID back to a timestamp to find the entry
    sd_id128_t bootID{};
    uint64_t ts = 0;
    uint64_t index = 0;
    if (!getTimestampFromID(asyncResp, entryID, bootID, ts, index))
    {
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
    // Go to the timestamp in the log and move to the entry at the
    // index tracking the unique ID
    ret = sd_journal_seek_monotonic_usec(journal.get(), bootID, ts);
    if (ret < 0)
    {
        BMCWEB_LOG_ERROR("failed to seek to an entry in journal{}",
                         strerror(-ret));
        messages::internalError(asyncResp->res);
        return;
    }

    if (sd_journal_next_skip(journal.get(), index + 1) < 0)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    nlohmann::json::object_t bmcJournalLogEntry;
    if (!fillBMCJournalLogEntryJson(entryID, journal.get(), bmcJournalLogEntry))
    {
        messages::internalError(asyncResp->res);
        return;
    }
    asyncResp->res.jsonValue.update(bmcJournalLogEntry);
}

inline void requestRoutesBMCJournalLogService(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Managers/<str>/LogServices/Journal/")
        .privileges(privileges::getLogService)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleManagersLogServiceJournalGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Managers/<str>/LogServices/Journal/Entries/")
        .privileges(privileges::getLogEntryCollection)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleManagersJournalLogEntryCollectionGet, std::ref(app)));

    BMCWEB_ROUTE(
        app, "/redfish/v1/Managers/<str>/LogServices/Journal/Entries/<str>/")
        .privileges(privileges::getLogEntry)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleManagersJournalEntriesLogEntryGet, std::ref(app)));
}
} // namespace redfish
