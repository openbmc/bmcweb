// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "async_resp.hpp"
#include "utils/log_services_utils.hpp"

#include <nlohmann/json.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace redfish
{
namespace console
{

class LogParser
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
    static std::unique_ptr<LogParser> requestParser(
        log_services_utils::LogService logService,
        log_services_utils::LogServiceParentCollection parentCollection,
        const std::string& rfResourceId, uint64_t computerSystemIndex);

    LogParser(std::string service, std::string collection,
              std::string resourceId, std::filesystem::path file) :
        logService(std::move(service)), parentCollection(std::move(collection)),
        rfResourceId(std::move(resourceId)), logFile(std::move(file))
    {}

    LogParser() = delete;
    ~LogParser() = default;
    LogParser(const LogParser&) = delete;
    LogParser& operator=(const LogParser&) = delete;
    LogParser(LogParser&&) = delete;
    LogParser& operator=(LogParser&&) = delete;

    /* @brief parse all log files and fill the log entry array
     *
     * @param logEntryArray
     * @param skip
     * @param top
     *
     * @return
     * */
    void getLogEntryCollection(nlohmann::json& logEntryArray, size_t skip,
                               size_t top);

    /* @brief get the specified log_entry
     *
     * @param targetLogEntryId
     *
     * @return true on success
     * */
    void getLogEntry(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     std::string_view targetLogEntryId);

    bool hasNextLink() const;

  private:
    void setNextLink();

    const std::string logService;
    const std::string parentCollection;
    const std::string rfResourceId;
    const std::string_view schemaVersion = "v1_21_0";
    const std::filesystem::path logFile;
    bool nextLink = false;
};
} // namespace console
} // namespace redfish
