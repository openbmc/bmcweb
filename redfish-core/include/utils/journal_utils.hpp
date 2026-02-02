#pragma once

#include "bmcweb_config.h"

#include "generated/enums/log_entry.hpp"
#include "logging.hpp"
#include "utility.hpp"
#include "utils/time_utils.hpp"

#include <systemd/sd-journal.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace redfish
{

inline int getJournalMetadata(sd_journal* journal, const char* field,
                              std::string_view& contents)
{
    const char* data = nullptr;
    size_t length = 0;
    int ret = 0;
    // Get the metadata from the requested field of the journal entry
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    const void** dataVoid = reinterpret_cast<const void**>(&data);

    ret = sd_journal_get_data(journal, field, dataVoid, &length);
    if (ret < 0)
    {
        return ret;
    }
    contents = std::string_view(data, length);
    // Only use the content after the "=" character.
    contents.remove_prefix(std::min(contents.find('=') + 1, contents.size()));
    return ret;
}

inline int getJournalMetadataInt(sd_journal* journal, const char* field,
                                 long& contents)
{
    std::string_view metadata;
    // Get the metadata from the requested field of the journal entry
    int ret = getJournalMetadata(journal, field, metadata);
    if (ret < 0)
    {
        return ret;
    }
    std::from_chars_result res =
        std::from_chars(&*metadata.begin(), &*metadata.end(), contents);
    if (res.ec != std::error_code{} || res.ptr != &*metadata.end())
    {
        return -1;
    }
    return 0;
}

inline bool getEntryTimestamp(sd_journal* journal, std::string& entryTimestamp)
{
    int ret = 0;
    uint64_t timestamp = 0;
    ret = sd_journal_get_realtime_usec(journal, &timestamp);
    if (ret < 0)
    {
        BMCWEB_LOG_ERROR("Failed to read entry timestamp: {}", ret);
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
    std::unique_ptr<char*> cursorptr = std::make_unique<char*>(cursor);
    std::string bmcJournalLogEntryID(cursor);

    // Get the Log Entry contents
    std::string message;
    std::string_view syslogID;
    ret = getJournalMetadata(journal, "SYSLOG_IDENTIFIER", syslogID);
    if (ret < 0)
    {
        BMCWEB_LOG_DEBUG("Failed to read SYSLOG_IDENTIFIER field: {}", ret);
    }
    if (!syslogID.empty())
    {
        message += std::string(syslogID) + ": ";
    }

    std::string_view msg;
    ret = getJournalMetadata(journal, "MESSAGE", msg);
    if (ret < 0)
    {
        BMCWEB_LOG_ERROR("Failed to read MESSAGE field: {}", ret);
        return false;
    }
    message += std::string(msg);

    // Get the severity from the PRIORITY field
    long severity = 8; // Default to an invalid priority
    ret = getJournalMetadataInt(journal, "PRIORITY", severity);
    if (ret < 0)
    {
        BMCWEB_LOG_DEBUG("Failed to read PRIORITY field: {}", ret);
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
        crow::utility::base64encode(bmcJournalLogEntryID);

    bmcJournalLogEntryJson["@odata.id"] = boost_swap_impl::format(
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
    bmcJournalLogEntryJson["Created"] = std::move(entryTimeStr);
    return true;
}

} // namespace redfish
