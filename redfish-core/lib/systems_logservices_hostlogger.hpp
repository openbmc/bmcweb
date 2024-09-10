// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "app.hpp"
#include "generated/enums/log_entry.hpp"
#include "query.hpp"
#include "registries/openbmc_message_registry.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/time_utils.hpp"

#include <cstdint>
#include <memory>
#include <string_view>
#include <utility>
#include <vector>

namespace redfish
{

inline void fillHostLoggerEntryJson(std::string_view logEntryID,
                                    std::string_view msg,
                                    nlohmann::json::object_t& logEntryJson)
{
    // Fill in the log entry with the gathered data.
    logEntryJson["@odata.type"] = "#LogEntry.v1_9_0.LogEntry";
    logEntryJson["@odata.id"] = boost::urls::format(
        "/redfish/v1/Systems/{}/LogServices/HostLogger/Entries/{}",
        BMCWEB_REDFISH_SYSTEM_URI_NAME, logEntryID);
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
    if constexpr (BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
        // Option currently returns no systems.  TBD
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    if (systemName != BMCWEB_REDFISH_SYSTEM_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    asyncResp->res.jsonValue["@odata.id"] =
        std::format("/redfish/v1/Systems/{}/LogServices/HostLogger",
                    BMCWEB_REDFISH_SYSTEM_URI_NAME);
    asyncResp->res.jsonValue["@odata.type"] = "#LogService.v1_2_0.LogService";
    asyncResp->res.jsonValue["Name"] = "Host Logger Service";
    asyncResp->res.jsonValue["Description"] = "Host Logger Service";
    asyncResp->res.jsonValue["Id"] = "HostLogger";
    asyncResp->res.jsonValue["Entries"]["@odata.id"] =
        std::format("/redfish/v1/Systems/{}/LogServices/HostLogger/Entries",
                    BMCWEB_REDFISH_SYSTEM_URI_NAME);
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
    if constexpr (BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
        // Option currently returns no systems.  TBD
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    if (systemName != BMCWEB_REDFISH_SYSTEM_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    asyncResp->res.jsonValue["@odata.id"] =
        std::format("/redfish/v1/Systems/{}/LogServices/HostLogger/Entries",
                    BMCWEB_REDFISH_SYSTEM_URI_NAME);
    asyncResp->res.jsonValue["@odata.type"] =
        "#LogEntryCollection.LogEntryCollection";
    asyncResp->res.jsonValue["Name"] = "HostLogger Entries";
    asyncResp->res.jsonValue["Description"] =
        "Collection of HostLogger Entries";
    nlohmann::json& logEntryArray = asyncResp->res.jsonValue["Members"];
    logEntryArray = nlohmann::json::array();
    asyncResp->res.jsonValue["Members@odata.count"] = 0;

    std::vector<std::filesystem::path> hostLoggerFiles;
    if (!getHostLoggerFiles(hostLoggerFolderPath, hostLoggerFiles))
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
            fillHostLoggerEntryJson(std::to_string(skip + i), logEntries[i],
                                    hostLogEntry);
            logEntryArray.emplace_back(std::move(hostLogEntry));
        }

        asyncResp->res.jsonValue["Members@odata.count"] = logCount;
        if (skip + top < logCount)
        {
            asyncResp->res.jsonValue["Members@odata.nextLink"] =
                std::format(
                    "/redfish/v1/Systems/{}/LogServices/HostLogger/Entries?$skip=",
                    BMCWEB_REDFISH_SYSTEM_URI_NAME) +
                std::to_string(skip + top);
        }
    }
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
    if constexpr (BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
        // Option currently returns no systems.  TBD
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    if (systemName != BMCWEB_REDFISH_SYSTEM_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    std::string_view targetID = param;

    uint64_t idInt = 0;

    auto [ptr, ec] = std::from_chars(targetID.begin(), targetID.end(), idInt);
    if (ec != std::errc{} || ptr != targetID.end())
    {
        messages::resourceNotFound(asyncResp->res, "LogEntry", param);
        return;
    }

    std::vector<std::filesystem::path> hostLoggerFiles;
    if (!getHostLoggerFiles(hostLoggerFolderPath, hostLoggerFiles))
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
        fillHostLoggerEntryJson(targetID, logEntries[0], hostLogEntry);
        asyncResp->res.jsonValue.update(hostLogEntry);
        return;
    }

    // Requested ID was not found
    messages::resourceNotFound(asyncResp->res, "LogEntry", param);
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
