// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
// SPDX-FileCopyrightText: Copyright 2018 Intel Corporation
#pragma once

#include "bmcweb_config.h"

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_singleton.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/log_entry.hpp"
#include "generated/enums/log_service.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "task.hpp"
#include "task_messages.hpp"
#include "utils/dbus_utils.hpp"
#include "utils/etag_utils.hpp"
#include "utils/eventlog_utils.hpp"
#include "utils/json_utils.hpp"
#include "utils/time_utils.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/system/linux_error.hpp>
#include <boost/url/format.hpp>
#include <boost/url/url.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/message/native_types.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <array>
#include <chrono>
#include <format>
#include <functional>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace redfish
{

constexpr const char* crashdumpObject = "com.intel.crashdump";
constexpr const char* crashdumpPath = "/com/intel/crashdump";
constexpr const char* crashdumpInterface = "com.intel.crashdump";
constexpr const char* deleteAllInterface =
    "xyz.openbmc_project.Collection.DeleteAll";
constexpr const char* crashdumpOnDemandInterface =
    "com.intel.crashdump.OnDemand";
constexpr const char* crashdumpTelemetryInterface =
    "com.intel.crashdump.Telemetry";

inline void parseCrashdumpParameters(
    const dbus::utility::DBusPropertiesMap& params, std::string& filename,
    std::string& timestamp, std::string& logfile)
{
    const std::string* filenamePtr = nullptr;
    const std::string* timestampPtr = nullptr;
    const std::string* logfilePtr = nullptr;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), params, "Timestamp", timestampPtr,
        "Filename", filenamePtr, "Log", logfilePtr);

    if (!success)
    {
        return;
    }

    if (filenamePtr != nullptr)
    {
        filename = *filenamePtr;
    }

    if (timestampPtr != nullptr)
    {
        timestamp = *timestampPtr;
    }

    if (logfilePtr != nullptr)
    {
        logfile = *logfilePtr;
    }
}

inline void handleSystemsLogServiceCollectionGet(
    crow::App& app, const crow::Request& req,
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

    // Collections don't include the static data added by SubRoute
    // because it has a duplicate entry for members
    asyncResp->res.jsonValue["@odata.type"] =
        "#LogServiceCollection.LogServiceCollection";
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Systems/{}/LogServices", BMCWEB_REDFISH_SYSTEM_URI_NAME);
    asyncResp->res.jsonValue["Name"] = "System Log Services Collection";
    asyncResp->res.jsonValue["Description"] =
        "Collection of LogServices for this Computer System";
    nlohmann::json& logServiceArray = asyncResp->res.jsonValue["Members"];
    logServiceArray = nlohmann::json::array();

    if constexpr (BMCWEB_REDFISH_EVENTLOG_LOCATION == "systems" &&
                  !BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
        nlohmann::json::object_t eventLog;
        eventLog["@odata.id"] =
            boost::urls::format("/redfish/v1/Systems/{}/LogServices/EventLog",
                                BMCWEB_REDFISH_SYSTEM_URI_NAME);
        logServiceArray.emplace_back(std::move(eventLog));
    }

    if constexpr (BMCWEB_REDFISH_DUMP_LOG)
    {
        nlohmann::json::object_t dumpLog;
        dumpLog["@odata.id"] =
            boost::urls::format("/redfish/v1/Systems/{}/LogServices/Dump",
                                BMCWEB_REDFISH_SYSTEM_URI_NAME);
        logServiceArray.emplace_back(std::move(dumpLog));
    }

    if constexpr (BMCWEB_REDFISH_CPU_LOG)
    {
        nlohmann::json::object_t crashdump;
        crashdump["@odata.id"] =
            boost::urls::format("/redfish/v1/Systems/{}/LogServices/Crashdump",
                                BMCWEB_REDFISH_SYSTEM_URI_NAME);
        logServiceArray.emplace_back(std::move(crashdump));
    }

    if constexpr (BMCWEB_REDFISH_HOST_LOGGER)
    {
        nlohmann::json::object_t hostlogger;
        hostlogger["@odata.id"] =
            boost::urls::format("/redfish/v1/Systems/{}/LogServices/HostLogger",
                                BMCWEB_REDFISH_SYSTEM_URI_NAME);
        logServiceArray.emplace_back(std::move(hostlogger));
    }
    asyncResp->res.jsonValue["Members@odata.count"] = logServiceArray.size();

    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.State.Boot.PostCode"};
    dbus::utility::getSubTreePaths(
        "/", 0, interfaces,
        // ast-grep-ignore: long-lambda
        [asyncResp](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreePathsResponse& subtreePath) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("{}", ec);
                return;
            }

            for (const auto& pathStr : subtreePath)
            {
                if (pathStr.contains("PostCode"))
                {
                    nlohmann::json& logServiceArrayLocal =
                        asyncResp->res.jsonValue["Members"];
                    nlohmann::json::object_t member;
                    member["@odata.id"] = boost::urls::format(
                        "/redfish/v1/Systems/{}/LogServices/PostCodes",
                        BMCWEB_REDFISH_SYSTEM_URI_NAME);

                    logServiceArrayLocal.emplace_back(std::move(member));

                    asyncResp->res.jsonValue["Members@odata.count"] =
                        logServiceArrayLocal.size();
                    return;
                }
            }
        });
}

inline void handleManagersLogServicesCollectionGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& managerId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    if (managerId != BMCWEB_REDFISH_MANAGER_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "Manager", managerId);
        return;
    }

    // Collections don't include the static data added by SubRoute
    // because it has a duplicate entry for members
    asyncResp->res.jsonValue["@odata.type"] =
        "#LogServiceCollection.LogServiceCollection";
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Managers/{}/LogServices", BMCWEB_REDFISH_MANAGER_URI_NAME);
    asyncResp->res.jsonValue["Name"] = "Open BMC Log Services Collection";
    asyncResp->res.jsonValue["Description"] =
        "Collection of LogServices for this Manager";
    nlohmann::json& logServiceArray = asyncResp->res.jsonValue["Members"];
    logServiceArray = nlohmann::json::array();

    if constexpr (BMCWEB_REDFISH_BMC_JOURNAL)
    {
        nlohmann::json::object_t journal;
        journal["@odata.id"] =
            boost::urls::format("/redfish/v1/Managers/{}/LogServices/Journal",
                                BMCWEB_REDFISH_MANAGER_URI_NAME);
        logServiceArray.emplace_back(std::move(journal));
    }

    if constexpr (BMCWEB_REDFISH_EVENTLOG_LOCATION == "managers")
    {
        nlohmann::json::object_t eventLog;
        eventLog["@odata.id"] =
            boost::urls::format("/redfish/v1/Managers/{}/LogServices/EventLog",
                                BMCWEB_REDFISH_MANAGER_URI_NAME);
        logServiceArray.emplace_back(std::move(eventLog));
    }

    asyncResp->res.jsonValue["Members@odata.count"] = logServiceArray.size();

    if constexpr (BMCWEB_REDFISH_DUMP_LOG)
    {
        constexpr std::array<std::string_view, 1> interfaces = {
            "xyz.openbmc_project.Collection.DeleteAll"};
        dbus::utility::getSubTreePaths(
            "/xyz/openbmc_project/dump", 0, interfaces,
            // ast-grep-ignore: long-lambda
            [asyncResp](const boost::system::error_code& ec,
                        const dbus::utility::MapperGetSubTreePathsResponse&
                            subTreePaths) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR(
                        "handleManagersLogServicesCollectionGet respHandler got error {}",
                        ec);
                    // Assume that getting an error simply means there are no
                    // dump LogServices. Return without adding any error
                    // response.
                    return;
                }

                nlohmann::json& logServiceArrayLocal =
                    asyncResp->res.jsonValue["Members"];

                for (const std::string& path : subTreePaths)
                {
                    if (path == "/xyz/openbmc_project/dump/bmc")
                    {
                        nlohmann::json::object_t member;
                        member["@odata.id"] = boost::urls::format(
                            "/redfish/v1/Managers/{}/LogServices/Dump",
                            BMCWEB_REDFISH_MANAGER_URI_NAME);
                        logServiceArrayLocal.emplace_back(std::move(member));
                    }
                    else if (path == "/xyz/openbmc_project/dump/faultlog")
                    {
                        nlohmann::json::object_t member;
                        member["@odata.id"] = boost::urls::format(
                            "/redfish/v1/Managers/{}/LogServices/FaultLog",
                            BMCWEB_REDFISH_MANAGER_URI_NAME);
                        logServiceArrayLocal.emplace_back(std::move(member));
                    }
                }

                asyncResp->res.jsonValue["Members@odata.count"] =
                    logServiceArrayLocal.size();
            });
    }
}

inline void handleSystemsEventLogServiceGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    if constexpr (BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
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
    eventlog_utils::handleSystemsAndManagersEventLogServiceGet(
        asyncResp, eventlog_utils::LogServiceParentCollection::Systems);
}

inline void handleManagersEventLogServiceGet(

    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& managerId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    if (managerId != BMCWEB_REDFISH_MANAGER_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "Manager", managerId);
        return;
    }
    eventlog_utils::handleSystemsAndManagersEventLogServiceGet(
        asyncResp, eventlog_utils::LogServiceParentCollection::Managers);
}

inline void requestRoutesCrashdumpService(App& app)
{
    /**
     * Functions triggers appropriate requests on DBus
     */
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/LogServices/Crashdump/")
        .privileges(redfish::privileges::getLogService)
        .methods(boost::beast::http::verb::get)
        // ast-grep-ignore: long-lambda
        ([&app](const crow::Request& req,
                const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                const std::string& systemName) {
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

            // Copy over the static data to include the entries added by
            // SubRoute
            asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
                "/redfish/v1/Systems/{}/LogServices/Crashdump",
                BMCWEB_REDFISH_SYSTEM_URI_NAME);
            asyncResp->res.jsonValue["@odata.type"] =
                "#LogService.v1_2_0.LogService";
            asyncResp->res.jsonValue["Name"] = "Open BMC Oem Crashdump Service";
            asyncResp->res.jsonValue["Description"] = "Oem Crashdump Service";
            asyncResp->res.jsonValue["Id"] = "Crashdump";
            asyncResp->res.jsonValue["OverWritePolicy"] =
                log_service::OverWritePolicy::WrapsWhenFull;
            asyncResp->res.jsonValue["MaxNumberOfRecords"] = 3;

            std::pair<std::string, std::string> redfishDateTimeOffset =
                redfish::time_utils::getDateTimeOffsetNow();
            asyncResp->res.jsonValue["DateTime"] = redfishDateTimeOffset.first;
            asyncResp->res.jsonValue["DateTimeLocalOffset"] =
                redfishDateTimeOffset.second;

            asyncResp->res.jsonValue["Entries"]["@odata.id"] =
                boost::urls::format(
                    "/redfish/v1/Systems/{}/LogServices/Crashdump/Entries",
                    BMCWEB_REDFISH_SYSTEM_URI_NAME);
            asyncResp->res.jsonValue["Actions"]["#LogService.ClearLog"]
                                    ["target"] = boost::urls::format(
                "/redfish/v1/Systems/{}/LogServices/Crashdump/Actions/LogService.ClearLog",
                BMCWEB_REDFISH_SYSTEM_URI_NAME);
            asyncResp->res
                .jsonValue["Actions"]["#LogService.CollectDiagnosticData"]
                          ["target"] = boost::urls::format(
                "/redfish/v1/Systems/{}/LogServices/Crashdump/Actions/LogService.CollectDiagnosticData",
                BMCWEB_REDFISH_SYSTEM_URI_NAME);

            etag_utils::setEtagOmitDateTimeHandler(asyncResp);
        });
}

inline void requestRoutesCrashdumpClear(App& app)
{
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Systems/<str>/LogServices/Crashdump/Actions/LogService.ClearLog/")
        .privileges(redfish::privileges::
                        postLogServiceSubOverComputerSystemLogServiceCollection)
        .methods(boost::beast::http::verb::post)(
            // ast-grep-ignore: long-lambda
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemName) {
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
                dbus::utility::async_method_call(
                    asyncResp,
                    [asyncResp](const boost::system::error_code& ec,
                                const std::string&) {
                        if (ec)
                        {
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        messages::success(asyncResp->res);
                    },
                    crashdumpObject, crashdumpPath, deleteAllInterface,
                    "DeleteAll");
            });
}

inline void logCrashdumpEntry(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& logID, nlohmann::json& logEntryJson)
{
    auto getStoredLogCallback =
        // ast-grep-ignore: long-lambda
        [asyncResp, logID,
         &logEntryJson](const boost::system::error_code& ec,
                        const dbus::utility::DBusPropertiesMap& params) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG("failed to get log ec: {}", ec.message());
                if (ec.value() ==
                    boost::system::linux_error::bad_request_descriptor)
                {
                    messages::resourceNotFound(asyncResp->res, "LogEntry",
                                               logID);
                }
                else
                {
                    messages::internalError(asyncResp->res);
                }
                return;
            }

            std::string timestamp{};
            std::string filename{};
            std::string logfile{};
            parseCrashdumpParameters(params, filename, timestamp, logfile);

            if (filename.empty() || timestamp.empty())
            {
                messages::resourceNotFound(asyncResp->res, "LogEntry", logID);
                return;
            }

            nlohmann::json::object_t logEntry;
            logEntry["@odata.type"] = "#LogEntry.v1_9_0.LogEntry";
            logEntry["@odata.id"] = boost::urls::format(
                "/redfish/v1/Systems/{}/LogServices/Crashdump/Entries/{}",
                BMCWEB_REDFISH_SYSTEM_URI_NAME, logID);
            logEntry["Name"] = "CPU Crashdump";
            logEntry["Id"] = logID;
            logEntry["EntryType"] = log_entry::LogEntryType::Oem;
            logEntry["AdditionalDataURI"] = boost::urls::format(
                "/redfish/v1/Systems/{}/LogServices/Crashdump/Entries/{}/{}",
                BMCWEB_REDFISH_SYSTEM_URI_NAME, logID, filename);
            logEntry["DiagnosticDataType"] = "OEM";
            logEntry["OEMDiagnosticDataType"] = "PECICrashdump";
            logEntry["Created"] = std::move(timestamp);

            // If logEntryJson references an array of LogEntry resources
            // ('Members' list), then push this as a new entry, otherwise set it
            // directly
            if (logEntryJson.is_array())
            {
                logEntryJson.push_back(logEntry);
                asyncResp->res.jsonValue["Members@odata.count"] =
                    logEntryJson.size();
            }
            else
            {
                logEntryJson.update(logEntry);
            }
        };
    dbus::utility::getAllProperties(
        crashdumpObject, crashdumpPath + std::string("/") + logID,
        crashdumpInterface, std::move(getStoredLogCallback));
}

inline void requestRoutesCrashdumpEntryCollection(App& app)
{
    /**
     * Functions triggers appropriate requests on DBus
     */
    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/<str>/LogServices/Crashdump/Entries/")
        .privileges(redfish::privileges::getLogEntryCollection)
        .methods(boost::beast::http::verb::get)
        // ast-grep-ignore: long-lambda
        ([&app](const crow::Request& req,
                const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                const std::string& systemName) {
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

            constexpr std::array<std::string_view, 1> interfaces = {
                crashdumpInterface};
            dbus::utility::getSubTreePaths(
                "/", 0, interfaces,
                // ast-grep-ignore: long-lambda
                [asyncResp](const boost::system::error_code& ec,
                            const std::vector<std::string>& resp) {
                    if (ec)
                    {
                        if (ec.value() !=
                            boost::system::errc::no_such_file_or_directory)
                        {
                            BMCWEB_LOG_DEBUG("failed to get entries ec: {}",
                                             ec.message());
                            messages::internalError(asyncResp->res);
                            return;
                        }
                    }
                    asyncResp->res.jsonValue["@odata.type"] =
                        "#LogEntryCollection.LogEntryCollection";
                    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
                        "/redfish/v1/Systems/{}/LogServices/Crashdump/Entries",
                        BMCWEB_REDFISH_SYSTEM_URI_NAME);
                    asyncResp->res.jsonValue["Name"] =
                        "Open BMC Crashdump Entries";
                    asyncResp->res.jsonValue["Description"] =
                        "Collection of Crashdump Entries";
                    asyncResp->res.jsonValue["Members"] =
                        nlohmann::json::array();
                    asyncResp->res.jsonValue["Members@odata.count"] = 0;

                    for (const std::string& path : resp)
                    {
                        const sdbusplus::object_path objPath(path);
                        // Get the log ID
                        std::string logID = objPath.filename();
                        if (logID.empty())
                        {
                            continue;
                        }
                        // Add the log entry to the array
                        logCrashdumpEntry(asyncResp, logID,
                                          asyncResp->res.jsonValue["Members"]);
                    }
                });
        });
}

inline void requestRoutesCrashdumpEntry(App& app)
{
    BMCWEB_ROUTE(
        app, "/redfish/v1/Systems/<str>/LogServices/Crashdump/Entries/<str>/")
        .privileges(redfish::privileges::getLogEntry)
        .methods(boost::beast::http::verb::get)(
            // ast-grep-ignore: long-lambda
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemName, const std::string& param) {
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
                const std::string& logID = param;
                logCrashdumpEntry(asyncResp, logID, asyncResp->res.jsonValue);
            });
}

inline void requestRoutesCrashdumpFile(App& app)
{
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Systems/<str>/LogServices/Crashdump/Entries/<str>/<str>/")
        .privileges(redfish::privileges::getLogEntry)
        .methods(boost::beast::http::verb::get)(
            // ast-grep-ignore: long-lambda
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& systemName, const std::string& logID,
               const std::string& fileName) {
                // Do not call getRedfishRoute here since the crashdump file is
                // not a Redfish resource.

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

                auto getStoredLogCallback =
                    [asyncResp, logID, fileName,
                     url(boost::urls::url(req.url()))](
                        const boost::system::error_code& ec,
                        const std::vector<std::pair<
                            std::string, dbus::utility::DbusVariantType>>&
                            resp) {
                        if (ec)
                        {
                            BMCWEB_LOG_DEBUG("failed to get log ec: {}",
                                             ec.message());
                            messages::internalError(asyncResp->res);
                            return;
                        }

                        std::string dbusFilename{};
                        std::string dbusTimestamp{};
                        std::string dbusFilepath{};

                        parseCrashdumpParameters(resp, dbusFilename,
                                                 dbusTimestamp, dbusFilepath);

                        if (dbusFilename.empty() || dbusTimestamp.empty() ||
                            dbusFilepath.empty())
                        {
                            messages::resourceNotFound(asyncResp->res,
                                                       "LogEntry", logID);
                            return;
                        }

                        // Verify the file name parameter is correct
                        if (fileName != dbusFilename)
                        {
                            messages::resourceNotFound(asyncResp->res,
                                                       "LogEntry", logID);
                            return;
                        }

                        if (asyncResp->res.openFile(dbusFilepath) !=
                            crow::OpenCode::Success)
                        {
                            messages::resourceNotFound(asyncResp->res,
                                                       "LogEntry", logID);
                            return;
                        }

                        // Configure this to be a file download when accessed
                        // from a browser
                        asyncResp->res.addHeader(
                            boost::beast::http::field::content_disposition,
                            "attachment");
                    };
                dbus::utility::getAllProperties(
                    *crow::connections::systemBus, crashdumpObject,
                    crashdumpPath + std::string("/") + logID,
                    crashdumpInterface, std::move(getStoredLogCallback));
            });
}

enum class OEMDiagnosticType
{
    onDemand,
    telemetry,
    invalid,
};

inline OEMDiagnosticType getOEMDiagnosticType(std::string_view oemDiagStr)
{
    if (oemDiagStr == "OnDemand")
    {
        return OEMDiagnosticType::onDemand;
    }
    if (oemDiagStr == "Telemetry")
    {
        return OEMDiagnosticType::telemetry;
    }

    return OEMDiagnosticType::invalid;
}

inline void requestRoutesCrashdumpCollect(App& app)
{
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Systems/<str>/LogServices/Crashdump/Actions/LogService.CollectDiagnosticData/")
        .privileges(redfish::privileges::
                        postLogServiceSubOverComputerSystemLogServiceCollection)
        .methods(boost::beast::http::verb::post)(
            // ast-grep-ignore: long-lambda
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemName) {
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

                std::string diagnosticDataType;
                std::string oemDiagnosticDataType;
                if (!redfish::json_util::readJsonAction(               //
                        req, asyncResp->res,                           //
                        "DiagnosticDataType", diagnosticDataType,      //
                        "OEMDiagnosticDataType", oemDiagnosticDataType //
                        ))
                {
                    return;
                }

                if (diagnosticDataType != "OEM")
                {
                    BMCWEB_LOG_ERROR(
                        "Only OEM DiagnosticDataType supported for Crashdump");
                    messages::actionParameterValueFormatError(
                        asyncResp->res, diagnosticDataType,
                        "DiagnosticDataType", "CollectDiagnosticData");
                    return;
                }

                OEMDiagnosticType oemDiagType =
                    getOEMDiagnosticType(oemDiagnosticDataType);

                std::string iface;
                std::string method;
                std::string taskMatchStr;
                if (oemDiagType == OEMDiagnosticType::onDemand)
                {
                    iface = crashdumpOnDemandInterface;
                    method = "GenerateOnDemandLog";
                    taskMatchStr =
                        "type='signal',"
                        "interface='org.freedesktop.DBus.Properties',"
                        "member='PropertiesChanged',"
                        "arg0namespace='com.intel.crashdump'";
                }
                else if (oemDiagType == OEMDiagnosticType::telemetry)
                {
                    iface = crashdumpTelemetryInterface;
                    method = "GenerateTelemetryLog";
                    taskMatchStr =
                        "type='signal',"
                        "interface='org.freedesktop.DBus.Properties',"
                        "member='PropertiesChanged',"
                        "arg0namespace='com.intel.crashdump'";
                }
                else
                {
                    BMCWEB_LOG_ERROR("Unsupported OEMDiagnosticDataType: {}",
                                     oemDiagnosticDataType);
                    messages::actionParameterValueFormatError(
                        asyncResp->res, oemDiagnosticDataType,
                        "OEMDiagnosticDataType", "CollectDiagnosticData");
                    return;
                }

                auto collectCrashdumpCallback =
                    [asyncResp, payload(task::Payload(req)),
                     taskMatchStr](const boost::system::error_code& ec,
                                   const std::string&) mutable {
                        if (ec)
                        {
                            if (ec.value() ==
                                boost::system::errc::operation_not_supported)
                            {
                                messages::resourceInStandby(asyncResp->res);
                            }
                            else if (ec.value() == boost::system::errc::
                                                       device_or_resource_busy)
                            {
                                messages::serviceTemporarilyUnavailable(
                                    asyncResp->res, "60");
                            }
                            else
                            {
                                messages::internalError(asyncResp->res);
                            }
                            return;
                        }
                        std::shared_ptr<task::TaskData> task =
                            task::TaskData::createTask(
                                // ast-grep-ignore: long-lambda
                                [](const boost::system::error_code& ec2,
                                   sdbusplus::message_t&,
                                   const std::shared_ptr<task::TaskData>&
                                       taskData) {
                                    if (!ec2)
                                    {
                                        taskData->messages.emplace_back(
                                            messages::taskCompletedOK(
                                                std::to_string(
                                                    taskData->index)));
                                        taskData->state = "Completed";
                                    }
                                    return task::completed;
                                },
                                taskMatchStr);

                        task->startTimer(std::chrono::minutes(5));
                        task->payload.emplace(std::move(payload));
                        task->populateResp(asyncResp->res);
                    };

                dbus::utility::async_method_call(
                    asyncResp, std::move(collectCrashdumpCallback),
                    crashdumpObject, crashdumpPath, iface, method);
            });
}

inline void requestRoutesSystemsLogServiceCollection(App& app)
{
    /**
     * Functions triggers appropriate requests on DBus
     */
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/LogServices/")
        .privileges(redfish::privileges::getLogServiceCollection)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleSystemsLogServiceCollectionGet, std::ref(app)));
}

inline void requestRoutesManagersLogServiceCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Managers/<str>/LogServices/")
        .privileges(redfish::privileges::getLogServiceCollection)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleManagersLogServicesCollectionGet, std::ref(app)));
}

inline void requestRoutesSystemsEventLogService(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/LogServices/EventLog/")
        .privileges(redfish::privileges::getLogService)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleSystemsEventLogServiceGet, std::ref(app)));
}

inline void requestRoutesManagersEventLogService(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Managers/<str>/LogServices/EventLog/")
        .privileges(redfish::privileges::getLogService)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleManagersEventLogServiceGet, std::ref(app)));
}
} // namespace redfish
