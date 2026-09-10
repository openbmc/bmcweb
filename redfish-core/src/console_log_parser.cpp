// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include "console_log_parser.hpp"

#include "bmcweb_config.h"

#include "async_resp.hpp"
#include "error_messages.hpp"
#include "generated/enums/log_entry.hpp"
#include "logging.hpp"
#include "utils/log_services_utils.hpp"
#include "utils/time_utils.hpp"

#include <boost/url/format.hpp>
#include <nlohmann/json.hpp>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <format>
#include <fstream>
#include <iomanip>
#include <locale>
#include <memory>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace redfish
{
namespace console
{
using namespace log_services_utils;

enum class ParseStatus
{
    Ok,
    NoMatch,
    Malformed,
};

struct ParsedLine
{
    std::string entryId;
    std::string entryMessage;
    std::string entryCreatedIso;
};

// regex match and capture:
// 1: cformat timestamp ("Mon Jan  2 15:04:05 2006")
// 2: minimum 7-digit entry id (zero-padded)
// 3: entry message text
const std::regex logLineRegex(
    R"(^((?:Mon|Tue|Wed|Thu|Fri|Sat|Sun)\s(?:Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)\s(?:\d{2}|\s\d)\s\d{2}:\d{2}:\d{2}\s\d{4})\s(\d{7,})\s(.*)$)");

static std::optional<std::filesystem::path> resolveHostLogFile(
    const uint64_t computerSystemIndex)
{
    const size_t idx = (computerSystemIndex == 0)
                           ? 0
                           : static_cast<size_t>(computerSystemIndex - 1);

    if (idx >= BMCWEB_REDFISH_HOST_LOG_PATHS_ARRAY.size())
    {
        BMCWEB_LOG_ERROR("computerSystemIndex {} out of bounds (size={}))",
                         computerSystemIndex,
                         BMCWEB_REDFISH_HOST_LOG_PATHS_ARRAY.size());
        return std::nullopt;
    }

    std::filesystem::path logFile = BMCWEB_REDFISH_HOST_LOG_PATHS_ARRAY.at(idx);

    BMCWEB_LOG_DEBUG("got log path {}", logFile.string());

    std::error_code ec;
    if (!std::filesystem::is_regular_file(logFile, ec) || ec)
    {
        BMCWEB_LOG_ERROR("\"{}\" not a regular file", logFile.string());
        return std::nullopt;
    }

    return logFile;
}

static std::string stripId(std::string_view idStr)
{
    if (idStr.empty())
    {
        return std::string{};
    }
    size_t firstNonZero = idStr.find_first_not_of('0');
    size_t start = (firstNonZero == std::string_view::npos) ? (idStr.size() - 1)
                                                            : firstNonZero;
    return std::string(idStr.substr(start));
}

static std::optional<std::string> ctimeToRfc3339(std::string_view ts)
{
    std::tm tm{};
    std::istringstream iss((std::string(ts)));
    iss.imbue(std::locale::classic());
    iss >> std::get_time(&tm, "%a %b %e %T %Y");
    if (iss.fail())
    {
        BMCWEB_LOG_WARNING("Failed to get time from {}", ts);
        return std::nullopt;
    }
    std::time_t tt = std::mktime(&tm);
    if (tt == static_cast<std::time_t>(-1))
    {
        BMCWEB_LOG_WARNING("Failed to convert time");
        return std::nullopt;
    }
    auto tp = std::chrono::system_clock::from_time_t(tt);
    std::string iso = time_utils::getDateTimeStdtime(tp);
    if (iso.empty())
    {
        BMCWEB_LOG_WARNING("Failed to transform into rfc3339");
        return std::nullopt;
    }

    return iso;
}

static ParseStatus parseLogLine(const std::string& line, ParsedLine& out)
{
    std::smatch m;
    if (std::regex_match(line, m, logLineRegex))
    {
        std::string entryId = stripId(m[2].str());
        if (entryId.empty())
        {
            BMCWEB_LOG_DEBUG("unable to strip entryId {}", m[2].str());
            return ParseStatus::Malformed;
        }

        std::optional<std::string> createdIso = ctimeToRfc3339(m[1].str());

        if (!createdIso)
        {
            BMCWEB_LOG_DEBUG("unable to transform timestamp to rfc3339");
            return ParseStatus::Malformed;
        }

        out.entryId = entryId;
        out.entryMessage = m[3].str();
        out.entryCreatedIso = *createdIso;
        return ParseStatus::Ok;
    }

    return ParseStatus::NoMatch;
}

static nlohmann::json::object_t createLogEntryJson(
    const std::string_view logService, const std::string_view parentCollection,
    const std::string_view rfResourceId, const std::string_view schemaVersion,
    const std::string_view entryId, const std::string_view entryMessage,
    log_entry::LogEntryType entryType, log_entry::EventSeverity eventSeverity,
    const std::string_view createdTimestamp)
{
    nlohmann::json::object_t entry;

    entry["@odata.type"] = std::format("#LogEntry.{}.LogEntry", schemaVersion);
    entry["@odata.id"] = boost::urls::format(
        "/redfish/v1/{}/{}/LogServices/{}/Entries/{}", parentCollection,
        rfResourceId, logService, entryId);
    entry["Created"] = createdTimestamp;
    entry["Name"] = std::format("{} Entry", logService);
    entry["Id"] = entryId;
    entry["Message"] = entryMessage;
    entry["EntryType"] = entryType;
    entry["Severity"] = eventSeverity;
    entry["OemRecordFormat"] = std::format("{} Entry", logService);

    return entry;
}

std::unique_ptr<LogParser> LogParser::requestParser(
    LogService logService, LogServiceParentCollection parentCollection,
    const std::string& resourceId, const uint64_t computerSystemIndex)
{
    std::optional<std::filesystem::path> file;
    std::optional<std::string> service;

    switch (logService)
    {
        case LogService::HostLogger:
            file = resolveHostLogFile(computerSystemIndex);
            service = logServiceToString(logService);
            break;
        default:
            return nullptr;
    }

    std::optional<std::string> collection =
        log_services_utils::logServiceParentCollectionToString(
            parentCollection);

    // validate everything at request time once
    if (!file || !service || !collection)
    {
        return nullptr;
    }

    return std::make_unique<LogParser>(
        std::move(*service), std::move(*collection), resourceId, *file);
}

void LogParser::setNextLink()
{
    LogParser::nextLink = !LogParser::nextLink;
}

bool LogParser::hasNextLink() const
{
    return LogParser::nextLink;
}

void LogParser::getLogEntryCollection(nlohmann::json& logEntryArray,
                                      size_t skip, size_t top)
{
    if (top == 0)
    {
        return;
    }

    const std::filesystem::path rotLogFile(
        std::format("{}.1", logFile.c_str()));

    uint64_t validSeen = 0;
    const uint64_t parseUntil = skip + top;

    auto emitEntry = [this, &logEntryArray](const ParsedLine& pl) {
        logEntryArray.emplace_back(createLogEntryJson(
            this->logService, this->parentCollection, this->rfResourceId,
            this->schemaVersion, pl.entryId, pl.entryMessage,
            log_entry::LogEntryType::Oem, log_entry::EventSeverity::OK,
            pl.entryCreatedIso));
    };

    for (const auto& file : std::vector{rotLogFile, logFile})
    {
        std::ifstream f(file);
        if (!f.is_open())
        {
            BMCWEB_LOG_DEBUG("log file has not been rotated");
            continue;
        }

        std::string line;

        while (std::getline(f, line))
        {
            ParsedLine pl;
            switch (parseLogLine(line, pl))
            {
                case ParseStatus::Malformed:
                    BMCWEB_LOG_WARNING("Log entry malformed, discarding");
                    break;
                case ParseStatus::NoMatch:
                    BMCWEB_LOG_WARNING(
                        "Failed to match logline regex, discarding");
                    break;
                case ParseStatus::Ok:
                    ++validSeen;
                    if (validSeen <= skip)
                    {
                        continue;
                    }
                    if (validSeen <= parseUntil)
                    {
                        emitEntry(pl);
                        continue;
                    }
                    // read one past skip+top, nextLink allowed
                    this->setNextLink();
                    return;
                default:
                    BMCWEB_LOG_ERROR("Unexpected ParseStatus: {}",
                                     static_cast<int>(parseLogLine(line, pl)));
                    break;
            }
        }
    }
}

void LogParser::getLogEntry(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            std::string_view targetLogEntryId)
{
    const std::filesystem::path rotLogFile(
        std::format("{}.1", logFile.c_str()));

    for (const auto& file : std::vector{rotLogFile, logFile})
    {
        std::ifstream f(file);
        if (!f.is_open())
        {
            BMCWEB_LOG_DEBUG("log file has not been rotated");
            continue;
        }

        std::string line;
        while (std::getline(f, line))
        {
            ParsedLine pl;
            if (parseLogLine(line, pl) == ParseStatus::Ok &&
                pl.entryId == targetLogEntryId)
            {
                asyncResp->res.jsonValue["@odata.type"] =
                    std::format("#LogEntry.{}.LogEntry", this->schemaVersion);
                asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
                    "/redfish/v1/{}/{}/LogServices/{}/Entries/{}",
                    this->parentCollection, this->rfResourceId,
                    this->logService, pl.entryId);
                asyncResp->res.jsonValue["Created"] = pl.entryCreatedIso;
                asyncResp->res.jsonValue["Name"] =
                    std::format("{} Entry", this->logService);
                asyncResp->res.jsonValue["Id"] = pl.entryId;
                asyncResp->res.jsonValue["Message"] = pl.entryMessage;
                asyncResp->res.jsonValue["EntryType"] =
                    log_entry::LogEntryType::Oem;
                asyncResp->res.jsonValue["Severity"] =
                    log_entry::EventSeverity::OK;
                asyncResp->res.jsonValue["OemRecordFormat"] =
                    std::format("{} Entry", this->logService);
                return;
            }
        }
    }

    messages::resourceNotFound(asyncResp->res, "LogEntry", targetLogEntryId);
}
} // namespace console
} // namespace redfish
