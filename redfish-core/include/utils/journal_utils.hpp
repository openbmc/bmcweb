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

inline std::optional<long int> getJournalMetadataInt(JournalReadState& journal,
                                                     const char* field)
{
    // Get the metadata from the requested field of the journal entry
    std::string metadata = journal.getData(field);
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
    std::optional<uint64_t> timestamp = journal.getRealtimeUsec();
    if (!timestamp)
    {
        return std::nullopt;
    }
    return redfish::time_utils::getDateTimeUintUs(*timestamp);
}

inline bool fillBMCJournalLogEntryJson(
    JournalReadState& journal, nlohmann::json::object_t& bmcJournalLogEntryJson)
{
    std::string bmcJournalLogEntryID = journal.getCursor();
    if (bmcJournalLogEntryID.empty())
    {
        return false;
    }

    // Get the Log Entry contents
    std::string message;
    std::string syslogID = journal.getData("SYSLOG_IDENTIFIER");
    if (!syslogID.empty())
    {
        message += std::string(syslogID) + ": ";
    }

    std::string msg = journal.getData("MESSAGE");
    if (msg.empty())
    {
        BMCWEB_LOG_ERROR("Failed to read MESSAGE field");
        return false;
    }
    message += std::string(msg);

    // Get the severity from the PRIORITY field
    std::optional<long int> severity =
        getJournalMetadataInt(journal, "PRIORITY");
    if (!severity)
    {
        BMCWEB_LOG_DEBUG("Failed to read PRIORITY field");
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
