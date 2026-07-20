// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include "log_parser.hpp"

#include "generated/enums/log_entry.hpp"
#include "hostlog_parser.hpp"
#include "utils/log_services_utils.hpp"

#include <boost/url/format.hpp>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <filesystem>
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace redfish
{
namespace log_parser
{
using namespace log_services_utils;

static std::optional<std::filesystem::path> discoverLogFile(
    LogService logService, const uint64_t computerSystemIndex)
{
    switch (logService)
    {
        case LogService::HostLogger:
        {
            return hostlogger::getLogFile(computerSystemIndex);
        }
        default:
            return std::nullopt;
    }
}

std::unique_ptr<Parser> Parser::requestParser(
    LogService service, LogServiceParentCollection collection,
    const std::string& resourceId, const uint64_t computerSystemIndex)
{
    std::optional<const std::filesystem::path> file =
        discoverLogFile(service, computerSystemIndex);

    switch (service)
    {
        case LogService::HostLogger:
        {
            if (file.has_value())
            {
                return std::make_unique<hostlogger::HostLogParser>(
                    service, collection, resourceId, file.value());
            }
            return nullptr;
        }
        default:
            return nullptr;
    }
}

nlohmann::json::object_t Parser::createLogEntry(
    std::string_view entryId, std::string_view entryMessage,
    log_entry::LogEntryType entryType, log_entry::EventSeverity eventSeverity,
    std::optional<std::string_view> createdTimestamp)
{
    nlohmann::json::object_t entry;

    entry["@odata.type"] =
        std::format("#LogEntry.{}.LogEntry", this->schemaVersion);
    entry["@odata.id"] = boost::urls::format(
        "/redfish/v1/{}/{}/LogServices/HostLogger/Entries/{}",
        logServiceParentCollectionToString(this->parentCollection),
        rfResourceId, entryId);

    if (createdTimestamp.has_value())
    {
        entry["Created"] = createdTimestamp;
    }

    entry["Name"] =
        std::format("{} Entry", logServiceToString(this->logService));
    entry["Id"] = entryId;
    entry["Message"] = entryMessage;
    entry["EntryType"] = entryType;
    entry["Severity"] = eventSeverity;
    entry["OemRecordFormat"] =
        std::format("{} Entry", logServiceToString(this->logService));

    return entry;
}
} // namespace log_parser
} // namespace redfish
