// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "generated/enums/log_entry.hpp"
#include "utils/log_services_utils.hpp"

#include <nlohmann/json.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace redfish
{
namespace log_parser
{

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
    std::optional<std::string> entryCreatedIso;
};

class Parser
{
  public:
    /* @brief return a valid parser upon log file validation
     *
     * @param logService
     * @param parentCollection
     * @param rfResourceId
     * @param computerSystemIndex
     *
     * @return std::unique_ptr<Parser>
     * */
    static std::unique_ptr<Parser> requestParser(
        log_services_utils::LogService logService,
        log_services_utils::LogServiceParentCollection parentCollection,
        const std::string& rfResourceId, uint64_t computerSystemIndex);

    Parser() = delete;
    virtual ~Parser() = default;
    Parser(const Parser&) = delete;
    Parser& operator=(const Parser&) = delete;
    Parser(Parser&&) = delete;
    Parser& operator=(Parser&&) = delete;

    /* @brief parse all log files and fill the log entry array
     *
     * @param logEntryArray
     * @param skip
     * @param top
     *
     * @return
     * */
    virtual void getLogEntryCollection(nlohmann::json& logEntryArray,
                                       size_t skip, size_t top) = 0;

    /* @brief get the specified log_entry
     *
     * @param targetLogEntryId
     *
     * @return nlohmann::json::object_t logEntry
     * */
    virtual nlohmann::json::object_t getLogEntry(
        std::string_view targetLogEntryId) = 0;

    bool hasNextLink() const;

  protected:
    Parser(log_services_utils::LogService service,
           log_services_utils::LogServiceParentCollection collection,
           std::string resourceId, std::filesystem::path file) :
        logService(service), parentCollection(collection),
        rfResourceId(std::move(resourceId)), logFile(std::move(file))
    {}

    virtual ParseStatus parseLogLine(const std::string& line,
                                     ParsedLine& out) = 0;

    /* @brief create a logEntry json object as described in logEntry schema
     *
     * @param entryId
     * @param entryMessage
     * @param entryType
     * @param severity
     * @param createdTimestamp
     *
     * @return nlohmann::json::object_t logEntry
     * */
    nlohmann::json::object_t createLogEntry(
        std::string_view entryId, std::string_view entryMessage,
        log_entry::LogEntryType entryType, log_entry::EventSeverity severity,
        const std::optional<std::string>& createdTimestamp = std::nullopt);

    void setNextLink();

    log_services_utils::LogService logService;
    log_services_utils::LogServiceParentCollection parentCollection;
    const std::string rfResourceId;
    const std::filesystem::path logFile;
    const std::string_view schemaVersion = "v1_21_0";
    bool nextLink = false;
};
} // namespace log_parser
} // namespace redfish
