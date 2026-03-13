#pragma once

#include "bmcweb_config.h"

#include "generated/enums/log_entry.hpp"
#include "logging.hpp"
#include "utility.hpp"
#include "utils/journal_read_state.hpp"
#include "utils/time_utils.hpp"

#include <boost/url/format.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace redfish
{

inline std::string_view getJournalMetadata(JournalReadState& journal,
                                           const char* field)
{
    std::string_view contents = journal.get_data(field);
    // Only use the content after the "=" character.  (note, may be empty)
    contents.remove_prefix(std::min(contents.find('=') + 1, contents.size()));
    return contents;
}

inline std::optional<long int> getJournalMetadataInt(JournalReadState& journal,
                                                     const char* field)
{
    // Get the metadata from the requested field of the journal entry
    std::string_view metadata = getJournalMetadata(journal, field);
    if (metadata.empty())
    {
        return std::nullopt;
    }
    long int contents = 0;
    std::from_chars_result res =
        std::from_chars(&*metadata.begin(), &*metadata.end(), contents);
    if (res.ec != std::error_code{} || res.ptr != &*metadata.end())
    {
        return std::nullopt;
    }
    return contents;
}

inline std::optional<std::string> getEntryTimestamp(JournalReadState& journal)
{
    std::optional<uint64_t> timestamp = journal.get_realtime_usec();
    if (!timestamp)
    {
        return std::nullopt;
    }
    return redfish::time_utils::getDateTimeUintUs(*timestamp);
}

inline bool fillBMCJournalLogEntryJson(
    JournalReadState& journal, nlohmann::json::object_t& bmcJournalLogEntryJson)
{
    char* cursor = nullptr;
    int ret = journal.get_cursor(&cursor);
    if (ret < 0)
    {
        return false;
    }
    // sd_journal_get_cursor() uses malloc() to allocate cursor memory, so set
    // the deleter to std::free to deallocate it on delete
    std::unique_ptr<char, decltype(&std::free)> cursorPtr(cursor, &std::free);
    std::string bmcJournalLogEntryID(cursor);

    // Get the Log Entry contents
    std::string message;
    std::string_view syslogID =
        getJournalMetadata(journal, "SYSLOG_IDENTIFIER");
    if (!syslogID.empty())
    {
        message += std::string(syslogID) + ": ";
    }

    std::string_view msg = getJournalMetadata(journal, "MESSAGE");
    if (ret < 0)
    {
        BMCWEB_LOG_ERROR("Failed to read MESSAGE field: {}", ret);
        return false;
    }
    message += std::string(msg);

    // Get the severity from the PRIORITY field
    std::optional<long int> severity =
        getJournalMetadataInt(journal, "PRIORITY");
    if (!severity)
    {
        BMCWEB_LOG_DEBUG("Failed to read PRIORITY field: {}", ret);
        severity = 8; // Default to an invalid priority
    }

    // Get the Created time from the timestamp
    std::optional<std::string> entryTimeStr = getEntryTimestamp(journal);
    if (entryTimeStr)
    {
        bmcJournalLogEntryJson["Created"] = std::move(*entryTimeStr);
    }

    // Fill in the log entry with the gathered data
    bmcJournalLogEntryJson["@odata.type"] = "#LogEntry.v1_9_0.LogEntry";

    std::string entryIdBase64 =
        crow::utility::base64encode(bmcJournalLogEntryID);

    bmcJournalLogEntryJson["@odata.id"] = boost::urls::format(
        "/redfish/v1/Managers/{}/LogServices/Journal/Entries/{}",
        BMCWEB_REDFISH_MANAGER_URI_NAME, entryIdBase64);
    bmcJournalLogEntryJson["Name"] = "BMC Journal Entry";
    bmcJournalLogEntryJson["Id"] = entryIdBase64;
    bmcJournalLogEntryJson["Message"] = std::move(message);
    bmcJournalLogEntryJson["EntryType"] = log_entry::LogEntryType::Oem;
    log_entry::EventSeverity rfseverity = log_entry::EventSeverity::OK;
    if (severity <= 2)
    {
        rfseverity = log_entry::EventSeverity::Critical;
    }
    else if (severity <= 4)
    {
        rfseverity = log_entry::EventSeverity::Warning;
    }

    bmcJournalLogEntryJson["Severity"] = rfseverity;
    bmcJournalLogEntryJson["OemRecordFormat"] = "BMC Journal Entry";

    return true;
}

} // namespace redfish
