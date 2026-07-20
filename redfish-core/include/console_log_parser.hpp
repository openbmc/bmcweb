// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "log_parser.hpp"
#include "utils/log_services_utils.hpp"

#include <nlohmann/json.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

namespace redfish
{
namespace log_parser
{
namespace console
{
class ConsoleLogParser : public Parser
{
  public:
    ConsoleLogParser(log_services_utils::LogService service,
                     log_services_utils::LogServiceParentCollection collection,
                     const std::string& resourceId,
                     const std::filesystem::path& file) :
        Parser(service, collection, resourceId, file)
    {}
    ~ConsoleLogParser() override = default;
    ConsoleLogParser(const ConsoleLogParser&) = delete;
    ConsoleLogParser& operator=(const ConsoleLogParser&) = delete;
    ConsoleLogParser(ConsoleLogParser&&) = delete;
    ConsoleLogParser& operator=(ConsoleLogParser&&) = delete;

    static std::unique_ptr<ConsoleLogParser> make(
        log_services_utils::LogService service,
        log_services_utils::LogServiceParentCollection collection,
        const std::string& resourceId, uint64_t computerSystemIndex);

    void getLogEntryCollection(nlohmann::json& logEntryArray, size_t skip,
                               size_t top) override;

    nlohmann::json::object_t getLogEntry(
        std::string_view targetLogEntryId) override;

  protected:
    ParseStatus parseLogLine(const std::string& line, ParsedLine& out) override;
};
} // namespace console
} // namespace log_parser
} // namespace redfish
