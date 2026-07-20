// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include "hostlog_parser.hpp"

#include "bmcweb_config.h"

#include "generated/enums/log_entry.hpp"
#include "logging.hpp"

#include <sys/types.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <optional>
#include <regex>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace redfish
{
namespace log_parser
{
namespace hostlogger
{

using namespace log_services_utils;

const std::regex cFormatTimestamp(
    R"((Mon|Tue|Wed|Thu|Fri|Sat|Sun) )"
    R"((Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec) )"
    R"([ \d]\d \d{2}:\d{2}:\d{2} \d{4})");

const off_t tsOffset = 0;
const size_t tsLen = 24;
const off_t idOffset = 25;
const size_t idLen = 7;
const off_t entryOffset = 33;

static std::string stripId(std::string& idStr)
{
    return idStr.erase(0, std::min(idStr.find_first_not_of('0'),
                                   idStr.size() - 1));
}

void HostLogParser::getLogEntryCollection(nlohmann::json& logEntryArray,
                                          size_t skip, size_t top)
{
    const std::filesystem::path rotLogFile(
        std::format("{}.1", logFile.c_str()));

    uint32_t entryCount = 0;
    for (const auto& file : std::vector{rotLogFile, logFile})
    {
        std::ifstream f(file);
        if (!f.is_open())
        {
            // log file has not been rotated yet
            continue;
        }

        std::string line;

        while (std::getline(f, line) && entryCount < top)
        {
            ++entryCount;
            if (skip > entryCount)
            {
                continue;
            }

            if (std::regex_search(line, cFormatTimestamp))
            {
                if (line.size() <= entryOffset)
                {
                    BMCWEB_LOG_DEBUG(
                        "hostlog_parser: emtpy log entry message, discarding");
                    continue;
                }

                std::string createdTimestamp = line.substr(tsOffset, tsLen);
                std::string entryId = line.substr(idOffset, idLen);
                std::string entryMessage = line.substr(entryOffset);

                nlohmann::json::object_t logEntry = this->createLogEntry(
                    stripId(entryId), entryMessage,
                    log_entry::LogEntryType::Oem, log_entry::EventSeverity::OK,
                    createdTimestamp);
                logEntryArray.emplace_back(logEntry);
            }
            else
            {
                BMCWEB_LOG_DEBUG(
                    "hostlog_parser: incomplete log entry, discarding");
            }
        }
    }
}

nlohmann::json::object_t HostLogParser::getLogEntry(
    std::string_view targetLogEntryId)
{
    nlohmann::json::object_t logEntry;
    bool found = false;
    const std::filesystem::path rotLogFile(
        std::format("{}.1", logFile.c_str()));

    for (const auto& file : std::vector{rotLogFile, logFile})
    {
        std::ifstream f(file);
        if (!f.is_open())
        {
            // log file has not been rotated yet
            continue;
        }

        if (found)
        {
            break;
        }

        std::string line;

        while (std::getline(f, line))
        {
            if (std::regex_search(line, cFormatTimestamp))
            {
                if (line.size() <= entryOffset)
                {
                    BMCWEB_LOG_DEBUG(
                        "hostlog_parser: emtpy log entry message, discarding");
                    continue;
                }

                std::string entryId = line.substr(idOffset, idLen);
                std::string strippedEntryId = stripId(entryId);

                if (strippedEntryId == targetLogEntryId)
                {
                    std::string createdTimestamp = line.substr(tsOffset, tsLen);
                    std::string entryMessage = line.substr(entryOffset);
                    logEntry = this->createLogEntry(
                        strippedEntryId, entryMessage,
                        log_entry::LogEntryType::Oem,
                        log_entry::EventSeverity::OK, createdTimestamp);
                    found = true;
                    break;
                }
            }
        }
    }
    return logEntry;
}

std::optional<std::filesystem::path> getLogFile(
    const uint64_t computerSystemIndex)
{
    if constexpr (BMCWEB_REDFISH_HOST_LOG_PATHS_ARRAY.empty())
    {
        return std::nullopt;
    }

    std::filesystem::path logFile =
        (computerSystemIndex > 0)
            ? BMCWEB_REDFISH_HOST_LOG_PATHS_ARRAY.at(
                  static_cast<size_t>(computerSystemIndex - 1))
            : BMCWEB_REDFISH_HOST_LOG_PATHS_ARRAY.at(0);

    BMCWEB_LOG_DEBUG("log parser: Got path {}", logFile.string());

    std::error_code ec;
    if (!std::filesystem::is_regular_file(logFile, ec) || ec)
    {
        BMCWEB_LOG_ERROR("log parser: \"{}\" no such file", logFile.string());
        return std::nullopt;
    }

    return logFile;
}
} // namespace hostlogger
} // namespace log_parser
} // namespace redfish
