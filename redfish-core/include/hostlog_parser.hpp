// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "log_parser.hpp"
#include "utils/log_services_utils.hpp"

#include <nlohmann/json.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace redfish
{
namespace log_parser
{
namespace hostlogger
{
using namespace log_services_utils;
class HostLogParser : public log_parser::Parser
{
  public:
    HostLogParser(LogService service, LogServiceParentCollection collection,
                  const std::string& resourceId,
                  const std::filesystem::path& file) :
        Parser(service, collection, resourceId, file)
    {}
    ~HostLogParser() override = default;
    HostLogParser(const HostLogParser&) = delete;
    HostLogParser& operator=(const HostLogParser&) = delete;
    HostLogParser(HostLogParser&&) = delete;
    HostLogParser& operator=(HostLogParser&&) = delete;

    void getLogEntryCollection(nlohmann::json& logEntryArray, size_t skip,
                               size_t top) override;

    nlohmann::json::object_t getLogEntry(
        std::string_view targetLogEntryId) override;
};

std::optional<std::filesystem::path> getLogFile(uint64_t computerSystemIndex);
} // namespace hostlogger
} // namespace log_parser
} // namespace redfish
