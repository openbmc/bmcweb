// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "bmcweb_config.h"

#include "app.hpp"
#include "async_resp.hpp"
#include "error_messages.hpp"
#include "generated/enums/log_entry.hpp"
#include "gzfile.hpp"
#include "http_request.hpp"
#include "human_sort.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/query_param.hpp"
#include "utils/systems_utils.hpp"

#include <boost/beast/http/verb.hpp>
#include <boost/url/format.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace redfish
{
constexpr const char* hostLoggerFolderPath = "/var/log/console";

// default output dir for phosphor-hostlogger in buffer mode
constexpr const char* multiHostLoggerFolderPath = "/var/lib/obmc/hostlogs";

inline bool getHostLoggerFiles(
    const std::string& hostLoggerFilePath,
    std::vector<std::filesystem::path>& hostLoggerFiles,
    const uint64_t computerSystemIndex)
{
    if constexpr (BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
        std::string logFilesPath = multiHostLoggerFolderPath;
        logFilesPath.append("/host" + std::to_string(computerSystemIndex));

        BMCWEB_LOG_DEBUG("LogFilesPath: {}", logFilesPath);

        std::error_code ec;
        std::filesystem::directory_iterator logPath(logFilesPath, ec);

        if (ec)
        {
            BMCWEB_LOG_WARNING("{}", ec.message());
            return false;
        }
        for (const std::filesystem::directory_entry& it : logPath)
        {
            BMCWEB_LOG_DEBUG("Logfile: {}", it.path().filename().string());
            hostLoggerFiles.emplace_back(it.path());
        }
        // TODO  07/13/25-19:58 olek: need to sort vector by timestamps
        return true;
    }

    std::error_code ec;
    std::filesystem::directory_iterator logPath(hostLoggerFilePath, ec);
    if (ec)
    {
        BMCWEB_LOG_WARNING("{}", ec.message());
        return false;
    }
    for (const std::filesystem::directory_entry& it : logPath)
    {
        std::string filename = it.path().filename();
        // Prefix of each log files is "log". Find the file and save the
        // path
        if (filename.starts_with("log"))
        {
            hostLoggerFiles.emplace_back(it.path());
        }
    }
    // As the log files rotate, they are appended with a ".#" that is higher for
    // the older logs. Since we start from oldest logs, sort the name in
    // descending order.
    std::sort(hostLoggerFiles.rbegin(), hostLoggerFiles.rend(),
              AlphanumLess<std::string>());

    return true;
}

inline bool getHostLoggerEntries(
    const std::vector<std::filesystem::path>& hostLoggerFiles, uint64_t skip,
    uint64_t top, std::vector<std::string>& logEntries, size_t& logCount)
{
    GzFileReader logFile;

    // Go though all log files and expose host logs.
    for (const std::filesystem::path& it : hostLoggerFiles)
    {
        if (!logFile.gzGetLines(it.string(), skip, top, logEntries, logCount))
        {
            BMCWEB_LOG_ERROR("fail to expose host logs");
            return false;
        }
    }
    // Get lastMessage from constructor by getter
    std::string lastMessage = logFile.getLastMessage();
    if (!lastMessage.empty())
    {
        logCount++;
        if (logCount > skip && logCount <= (skip + top))
        {
            logEntries.push_back(lastMessage);
        }
    }
    return true;
}

inline void fillHostLoggerEntryJson(
    const std::string& systemName, std::string_view logEntryID,
    std::string_view msg, nlohmann::json::object_t& logEntryJson)
{
    // Fill in the log entry with the gathered data.
    logEntryJson["@odata.type"] = "#LogEntry.v1_9_0.LogEntry";
    logEntryJson["@odata.id"] = boost::urls::format(
        "/redfish/v1/Systems/{}/LogServices/HostLogger/Entries/{}", systemName,
        logEntryID);
    logEntryJson["Name"] = "Host Logger Entry";
    logEntryJson["Id"] = logEntryID;
    logEntryJson["Message"] = msg;
    logEntryJson["EntryType"] = log_entry::LogEntryType::Oem;
    logEntryJson["Severity"] = log_entry::EventSeverity::OK;
    logEntryJson["OemRecordFormat"] = "Host Logger Entry";
}

inline void handleSystemsLogServicesHostloggerGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    if (!BMCWEB_REDFISH_SYSTEM_URI_NAME.empty())
    {
        if (systemName != BMCWEB_REDFISH_SYSTEM_URI_NAME)
        {
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }
    }
    asyncResp->res.jsonValue["@odata.id"] = std::format(
        "/redfish/v1/Systems/{}/LogServices/HostLogger", systemName);
    asyncResp->res.jsonValue["@odata.type"] = "#LogService.v1_2_0.LogService";
    asyncResp->res.jsonValue["Name"] = "Host Logger Service";
    asyncResp->res.jsonValue["Description"] = "Host Logger Service";
    asyncResp->res.jsonValue["Id"] = "HostLogger";
    asyncResp->res.jsonValue["Entries"]["@odata.id"] = std::format(
        "/redfish/v1/Systems/{}/LogServices/HostLogger/Entries", systemName);
}

inline void processSystemsLogServicesHostloggerEntriesGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, query_param::Query& delegatedQuery,
    const uint64_t computerSystemIndex)
{
    asyncResp->res.jsonValue["@odata.id"] = std::format(
        "/redfish/v1/Systems/{}/LogServices/HostLogger/Entries", systemName);
    asyncResp->res.jsonValue["@odata.type"] =
        "#LogEntryCollection.LogEntryCollection";
    asyncResp->res.jsonValue["Name"] = "HostLogger Entries";
    asyncResp->res.jsonValue["Description"] =
        "Collection of HostLogger Entries";
    nlohmann::json& logEntryArray = asyncResp->res.jsonValue["Members"];
    logEntryArray = nlohmann::json::array();
    asyncResp->res.jsonValue["Members@odata.count"] = 0;

    std::vector<std::filesystem::path> hostLoggerFiles;

    if (!getHostLoggerFiles(hostLoggerFolderPath, hostLoggerFiles,
                            computerSystemIndex))
    {
        BMCWEB_LOG_DEBUG("Failed to get host log file path");
        return;
    }
    // If we weren't provided top and skip limits, use the defaults.
    size_t skip = delegatedQuery.skip.value_or(0);
    size_t top = delegatedQuery.top.value_or(query_param::Query::maxTop);
    size_t logCount = 0;
    // This vector only store the entries we want to expose that
    // control by skip and top.
    std::vector<std::string> logEntries;
    if (!getHostLoggerEntries(hostLoggerFiles, skip, top, logEntries, logCount))
    {
        messages::internalError(asyncResp->res);
        return;
    }
    // If vector is empty, that means skip value larger than total
    // log count
    if (logEntries.empty())
    {
        asyncResp->res.jsonValue["Members@odata.count"] = logCount;
        return;
    }
    if (!logEntries.empty())
    {
        for (size_t i = 0; i < logEntries.size(); i++)
        {
            nlohmann::json::object_t hostLogEntry;
            fillHostLoggerEntryJson(systemName, std::to_string(skip + i),
                                    logEntries[i], hostLogEntry);
            logEntryArray.emplace_back(std::move(hostLogEntry));
        }

        asyncResp->res.jsonValue["Members@odata.count"] = logCount;
        if (skip + top < logCount)
        {
            asyncResp->res.jsonValue["Members@odata.nextLink"] =
                std::format(
                    "/redfish/v1/Systems/{}/LogServices/HostLogger/Entries?$skip=",
                    systemName) +
                std::to_string(skip + top);
        }
    }
}
inline void handleSystemsLogServicesHostloggerEntriesGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName)
{
    query_param::QueryCapabilities capabilities = {
        .canDelegateTop = true,
        .canDelegateSkip = true,
    };
    query_param::Query delegatedQuery;
    if (!redfish::setUpRedfishRouteWithDelegation(app, req, asyncResp,
                                                  delegatedQuery, capabilities))
    {
        return;
    }
    if (!BMCWEB_REDFISH_SYSTEM_URI_NAME.empty())
    {
        if (systemName != BMCWEB_REDFISH_SYSTEM_URI_NAME)
        {
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }
    }

    getComputerSystemIndex(
        asyncResp, systemName,
        std::bind_front(processSystemsLogServicesHostloggerEntriesGet,
                        asyncResp, systemName, delegatedQuery));
}

inline void processSystemsLogServicesHostloggerEntriesEntryGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& param,
    const uint64_t computerSystemIndex)
{
    std::string_view targetID = param;
    uint64_t idInt = 0;

    auto [ptr, ec] = std::from_chars(targetID.begin(), targetID.end(), idInt);
    if (ec != std::errc{} || ptr != targetID.end())
    {
        messages::resourceNotFound(asyncResp->res, "LogEntry", param);
        return;
    }

    std::vector<std::filesystem::path> hostLoggerFiles;
    if (!getHostLoggerFiles(hostLoggerFolderPath, hostLoggerFiles,
                            computerSystemIndex))
    {
        BMCWEB_LOG_DEBUG("Failed to get host log file path");
        return;
    }

    size_t logCount = 0;
    size_t top = 1;
    std::vector<std::string> logEntries;
    // We can get specific entry by skip and top. For example, if we
    // want to get nth entry, we can set skip = n-1 and top = 1 to
    // get that entry
    if (!getHostLoggerEntries(hostLoggerFiles, idInt, top, logEntries,
                              logCount))
    {
        messages::internalError(asyncResp->res);
        return;
    }

    if (!logEntries.empty())
    {
        nlohmann::json::object_t hostLogEntry;
        fillHostLoggerEntryJson(systemName, targetID, logEntries[0],
                                hostLogEntry);
        asyncResp->res.jsonValue.update(hostLogEntry);
        return;
    }

    // Requested ID was not found
    messages::resourceNotFound(asyncResp->res, "LogEntry", param);
}

inline void handleSystemsLogServicesHostloggerEntriesEntryGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& param)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    if (!BMCWEB_REDFISH_SYSTEM_URI_NAME.empty())
    {
        if (systemName != BMCWEB_REDFISH_SYSTEM_URI_NAME)
        {
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }
    }

    getComputerSystemIndex(
        asyncResp, systemName,
        std::bind_front(processSystemsLogServicesHostloggerEntriesEntryGet,
                        asyncResp, systemName, param));
}

inline void requestRoutesSystemsLogServiceHostlogger(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/LogServices/HostLogger/")
        .privileges(redfish::privileges::getLogService)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleSystemsLogServicesHostloggerGet, std::ref(app)));
    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/<str>/LogServices/HostLogger/Entries/")
        .privileges(redfish::privileges::getLogEntryCollection)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleSystemsLogServicesHostloggerEntriesGet, std::ref(app)));

    BMCWEB_ROUTE(
        app, "/redfish/v1/Systems/<str>/LogServices/HostLogger/Entries/<str>/")
        .privileges(redfish::privileges::getLogEntry)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleSystemsLogServicesHostloggerEntriesEntryGet, std::ref(app)));
}

} // namespace redfish
