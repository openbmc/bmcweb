// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include "log_parser.hpp"

#include "console_log_parser.hpp"
#include "generated/enums/log_entry.hpp"
#include "utils/log_services_utils.hpp"

#include <boost/url/format.hpp>
#include <nlohmann/json.hpp>

#include <cstdint>
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

std::unique_ptr<Parser> Parser::requestParser(
    LogService service, LogServiceParentCollection collection,
    const std::string& resourceId, const uint64_t computerSystemIndex)
{
    switch (service)
    {
        case LogService::HostLogger:
            return console::ConsoleLogParser::make(
                service, collection, resourceId, computerSystemIndex);
        default:
            return nullptr;
    }
}

nlohmann::json::object_t Parser::createLogEntry(
    std::string_view entryId, std::string_view entryMessage,
    log_entry::LogEntryType entryType, log_entry::EventSeverity eventSeverity,
    const std::optional<std::string>& createdTimestamp)
{
    nlohmann::json::object_t entry;

    entry["@odata.type"] =
        std::format("#LogEntry.{}.LogEntry", this->schemaVersion);
    entry["@odata.id"] = boost::urls::format(
        "/redfish/v1/{}/{}/LogServices/{}/Entries/{}",
        logServiceParentCollectionToString(this->parentCollection),
        rfResourceId, logServiceToString(this->logService), entryId);

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

void Parser::setNextLink()
{
    Parser::nextLink = !Parser::nextLink;
}

bool Parser::hasNextLink() const
{
    return Parser::nextLink;
}
} // namespace log_parser
} // namespace redfish
