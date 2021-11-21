#include "log_services.hpp"

// #including "task.hpp" adds 8s to build time of this file (12s -> 20s)
//#include "task.hpp"

#include "http_utility.hpp"
#include "../include/utils/json_utils.hpp"
#include <fstream>

namespace redfish
{

inline void createDump(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const crow::Request& req, const std::string& dumpType)
{

    std::string dumpPath;
    if (dumpType == "BMC")
    {
        dumpPath = "/redfish/v1/Managers/bmc/LogServices/Dump/Entries/";
    }
    else if (dumpType == "System")
    {
        dumpPath = "/redfish/v1/Systems/system/LogServices/Dump/Entries/";
    }
    else
    {
        BMCWEB_LOG_ERROR << "Invalid dump type: " << dumpType;
        messages::internalError(asyncResp->res);
        return;
    }

    std::optional<std::string> diagnosticDataType;
    std::optional<std::string> oemDiagnosticDataType;

    if (!redfish::json_util::readJson(
            req, asyncResp->res, "DiagnosticDataType", diagnosticDataType,
            "OEMDiagnosticDataType", oemDiagnosticDataType))
    {
        return;
    }

    if (dumpType == "System")
    {
        if (!oemDiagnosticDataType || !diagnosticDataType)
        {
            BMCWEB_LOG_ERROR << "CreateDump action parameter "
                                "'DiagnosticDataType'/"
                                "'OEMDiagnosticDataType' value not found!";
            messages::actionParameterMissing(
                asyncResp->res, "CollectDiagnosticData",
                "DiagnosticDataType & OEMDiagnosticDataType");
            return;
        }
        if ((*oemDiagnosticDataType != "System") ||
            (*diagnosticDataType != "OEM"))
        {
            BMCWEB_LOG_ERROR << "Wrong parameter values passed";
            messages::invalidObject(asyncResp->res,
                                    "System Dump creation parameters");
            return;
        }
    }
    else if (dumpType == "BMC")
    {
        if (!diagnosticDataType)
        {
            BMCWEB_LOG_ERROR << "CreateDump action parameter "
                                "'DiagnosticDataType' not found!";
            messages::actionParameterMissing(
                asyncResp->res, "CollectDiagnosticData", "DiagnosticDataType");
            return;
        }
        if (*diagnosticDataType != "Manager")
        {
            BMCWEB_LOG_ERROR
                << "Wrong parameter value passed for 'DiagnosticDataType'";
            messages::invalidObject(asyncResp->res,
                                    "BMC Dump creation parameters");
            return;
        }
    }

    crow::connections::systemBus->async_method_call(
        [asyncResp, req, dumpPath, dumpType](const boost::system::error_code ec,
                                             const uint32_t& dumpId) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "CreateDump resp_handler got error " << ec;
                messages::internalError(asyncResp->res);
                return;
            }
            BMCWEB_LOG_DEBUG << "Dump Created. Id: " << dumpId;

            createDumpTaskCallback(req, asyncResp, dumpId, dumpPath, dumpType);
        },
        "xyz.openbmc_project.Dump.Manager",
        "/xyz/openbmc_project/dump/" +
            std::string(boost::algorithm::to_lower_copy(dumpType)),
        "xyz.openbmc_project.Dump.Create", "CreateDump");
}

void requestRoutesSystemLogServiceCollection(App& app)
{
    /**
     * Functions triggers appropriate requests on DBus
     */
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/LogServices/")
        .privileges(redfish::privileges::getLogServiceCollection)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)

            {
                // Collections don't include the static data added by SubRoute
                // because it has a duplicate entry for members
                asyncResp->res.jsonValue["@odata.type"] =
                    "#LogServiceCollection.LogServiceCollection";
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/Systems/system/LogServices";
                asyncResp->res.jsonValue["Name"] =
                    "System Log Services Collection";
                asyncResp->res.jsonValue["Description"] =
                    "Collection of LogServices for this Computer System";
                nlohmann::json& logServiceArray =
                    asyncResp->res.jsonValue["Members"];
                logServiceArray = nlohmann::json::array();
                logServiceArray.push_back(
                    {{"@odata.id",
                      "/redfish/v1/Systems/system/LogServices/EventLog"}});
#ifdef BMCWEB_ENABLE_REDFISH_DUMP_LOG
                logServiceArray.push_back(
                    {{"@odata.id",
                      "/redfish/v1/Systems/system/LogServices/Dump"}});
#endif

#ifdef BMCWEB_ENABLE_REDFISH_CPU_LOG
                logServiceArray.push_back(
                    {{"@odata.id",
                      "/redfish/v1/Systems/system/LogServices/Crashdump"}});
#endif
                asyncResp->res.jsonValue["Members@odata.count"] =
                    logServiceArray.size();

                crow::connections::systemBus->async_method_call(
                    [asyncResp](const boost::system::error_code ec,
                                const std::vector<std::string>& subtreePath) {
                        if (ec)
                        {
                            BMCWEB_LOG_ERROR << ec;
                            return;
                        }

                        for (auto& pathStr : subtreePath)
                        {
                            if (pathStr.find("PostCode") != std::string::npos)
                            {
                                nlohmann::json& logServiceArrayLocal =
                                    asyncResp->res.jsonValue["Members"];
                                logServiceArrayLocal.push_back(
                                    {{"@odata.id", "/redfish/v1/Systems/system/"
                                                   "LogServices/PostCodes"}});
                                asyncResp->res
                                    .jsonValue["Members@odata.count"] =
                                    logServiceArrayLocal.size();
                                return;
                            }
                        }
                    },
                    "xyz.openbmc_project.ObjectMapper",
                    "/xyz/openbmc_project/object_mapper",
                    "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths", "/",
                    0, std::array<const char*, 1>{postCodeIface});
            });
}

void requestRoutesEventLogService(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/LogServices/EventLog/")
        .privileges(redfish::privileges::getLogService)
        .methods(
            boost::beast::http::verb::
                get)([](const crow::Request&,
                        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
            asyncResp->res.jsonValue["@odata.id"] =
                "/redfish/v1/Systems/system/LogServices/EventLog";
            asyncResp->res.jsonValue["@odata.type"] =
                "#LogService.v1_1_0.LogService";
            asyncResp->res.jsonValue["Name"] = "Event Log Service";
            asyncResp->res.jsonValue["Description"] =
                "System Event Log Service";
            asyncResp->res.jsonValue["Id"] = "EventLog";
            asyncResp->res.jsonValue["OverWritePolicy"] = "WrapsWhenFull";

            std::pair<std::string, std::string> redfishDateTimeOffset =
                crow::utility::getDateTimeOffsetNow();

            asyncResp->res.jsonValue["DateTime"] = redfishDateTimeOffset.first;
            asyncResp->res.jsonValue["DateTimeLocalOffset"] =
                redfishDateTimeOffset.second;

            asyncResp->res.jsonValue["Entries"] = {
                {"@odata.id",
                 "/redfish/v1/Systems/system/LogServices/EventLog/Entries"}};
            asyncResp->res.jsonValue["Actions"]["#LogService.ClearLog"] = {

                {"target", "/redfish/v1/Systems/system/LogServices/EventLog/"
                           "Actions/LogService.ClearLog"}};
        });
}

void requestRoutesJournalEventLogClear(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/LogServices/EventLog/Actions/"
                      "LogService.ClearLog/")
        .privileges({{"ConfigureComponents"}})
        .methods(boost::beast::http::verb::post)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                // Clear the EventLog by deleting the log files
                std::vector<std::filesystem::path> redfishLogFiles;
                if (getRedfishLogFiles(redfishLogFiles))
                {
                    for (const std::filesystem::path& file : redfishLogFiles)
                    {
                        std::error_code ec;
                        std::filesystem::remove(file, ec);
                    }
                }

                // Reload rsyslog so it knows to start new log files
                crow::connections::systemBus->async_method_call(
                    [asyncResp](const boost::system::error_code ec) {
                        if (ec)
                        {
                            BMCWEB_LOG_ERROR << "Failed to reload rsyslog: "
                                             << ec;
                            messages::internalError(asyncResp->res);
                            return;
                        }

                        messages::success(asyncResp->res);
                    },
                    "org.freedesktop.systemd1", "/org/freedesktop/systemd1",
                    "org.freedesktop.systemd1.Manager", "ReloadUnit",
                    "rsyslog.service", "replace");
            });
}

void requestRoutesJournalEventLogEntryCollection(App& app)
{
    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/system/LogServices/EventLog/Entries/")
        .privileges(redfish::privileges::getLogEntryCollection)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                uint64_t skip = 0;
                uint64_t top = maxEntriesPerPage; // Show max entries by default
                if (!getSkipParam(asyncResp, req, skip))
                {
                    return;
                }
                if (!getTopParam(asyncResp, req, top))
                {
                    return;
                }
                // Collections don't include the static data added by SubRoute
                // because it has a duplicate entry for members
                asyncResp->res.jsonValue["@odata.type"] =
                    "#LogEntryCollection.LogEntryCollection";
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/Systems/system/LogServices/EventLog/Entries";
                asyncResp->res.jsonValue["Name"] = "System Event Log Entries";
                asyncResp->res.jsonValue["Description"] =
                    "Collection of System Event Log Entries";

                nlohmann::json& logEntryArray =
                    asyncResp->res.jsonValue["Members"];
                logEntryArray = nlohmann::json::array();
                // Go through the log files and create a unique ID for each
                // entry
                std::vector<std::filesystem::path> redfishLogFiles;
                getRedfishLogFiles(redfishLogFiles);
                uint64_t entryCount = 0;
                std::string logEntry;

                // Oldest logs are in the last file, so start there and loop
                // backwards
                for (auto it = redfishLogFiles.rbegin();
                     it < redfishLogFiles.rend(); it++)
                {
                    std::ifstream logStream(*it);
                    if (!logStream.is_open())
                    {
                        continue;
                    }

                    // Reset the unique ID on the first entry
                    bool firstEntry = true;
                    while (std::getline(logStream, logEntry))
                    {
                        entryCount++;
                        // Handle paging using skip (number of entries to skip
                        // from the start) and top (number of entries to
                        // display)
                        if (entryCount <= skip || entryCount > skip + top)
                        {
                            continue;
                        }

                        std::string idStr;
                        if (!getUniqueEntryID(logEntry, idStr, firstEntry))
                        {
                            continue;
                        }

                        if (firstEntry)
                        {
                            firstEntry = false;
                        }

                        logEntryArray.push_back({});
                        nlohmann::json& bmcLogEntry = logEntryArray.back();
                        if (fillEventLogEntryJson(idStr, logEntry,
                                                  bmcLogEntry) != 0)
                        {
                            messages::internalError(asyncResp->res);
                            return;
                        }
                    }
                }
                asyncResp->res.jsonValue["Members@odata.count"] = entryCount;
                if (skip + top < entryCount)
                {
                    asyncResp->res.jsonValue["Members@odata.nextLink"] =
                        "/redfish/v1/Systems/system/LogServices/EventLog/"
                        "Entries?$skip=" +
                        std::to_string(skip + top);
                }
            });
}

void requestRoutesJournalEventLogEntry(App& app)
{
    BMCWEB_ROUTE(
        app, "/redfish/v1/Systems/system/LogServices/EventLog/Entries/<str>/")
        .privileges(redfish::privileges::getLogEntry)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& param) {
                const std::string& targetID = param;

                // Go through the log files and check the unique ID for each
                // entry to find the target entry
                std::vector<std::filesystem::path> redfishLogFiles;
                getRedfishLogFiles(redfishLogFiles);
                std::string logEntry;

                // Oldest logs are in the last file, so start there and loop
                // backwards
                for (auto it = redfishLogFiles.rbegin();
                     it < redfishLogFiles.rend(); it++)
                {
                    std::ifstream logStream(*it);
                    if (!logStream.is_open())
                    {
                        continue;
                    }

                    // Reset the unique ID on the first entry
                    bool firstEntry = true;
                    while (std::getline(logStream, logEntry))
                    {
                        std::string idStr;
                        if (!getUniqueEntryID(logEntry, idStr, firstEntry))
                        {
                            continue;
                        }

                        if (firstEntry)
                        {
                            firstEntry = false;
                        }

                        if (idStr == targetID)
                        {
                            if (fillEventLogEntryJson(
                                    idStr, logEntry,
                                    asyncResp->res.jsonValue) != 0)
                            {
                                messages::internalError(asyncResp->res);
                                return;
                            }
                            return;
                        }
                    }
                }
                // Requested ID was not found
                messages::resourceMissingAtURI(asyncResp->res, targetID);
            });
}

void requestRoutesDBusEventLogEntryCollection(App& app)
{
    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/system/LogServices/EventLog/Entries/")
        .privileges(redfish::privileges::getLogEntryCollection)
        .methods(
            boost::beast::http::verb::
                get)([](const crow::Request&,
                        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
            // Collections don't include the static data added by SubRoute
            // because it has a duplicate entry for members
            asyncResp->res.jsonValue["@odata.type"] =
                "#LogEntryCollection.LogEntryCollection";
            asyncResp->res.jsonValue["@odata.id"] =
                "/redfish/v1/Systems/system/LogServices/EventLog/Entries";
            asyncResp->res.jsonValue["Name"] = "System Event Log Entries";
            asyncResp->res.jsonValue["Description"] =
                "Collection of System Event Log Entries";

            // DBus implementation of EventLog/Entries
            // Make call to Logging Service to find all log entry objects
            crow::connections::systemBus->async_method_call(
                [asyncResp](const boost::system::error_code ec,
                            GetManagedObjectsType& resp) {
                    if (ec)
                    {
                        // TODO Handle for specific error code
                        BMCWEB_LOG_ERROR
                            << "getLogEntriesIfaceData resp_handler got error "
                            << ec;
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    nlohmann::json& entriesArray =
                        asyncResp->res.jsonValue["Members"];
                    entriesArray = nlohmann::json::array();
                    for (auto& objectPath : resp)
                    {
                        uint32_t* id = nullptr;
                        std::time_t timestamp{};
                        std::time_t updateTimestamp{};
                        std::string* severity = nullptr;
                        std::string* message = nullptr;
                        std::string* filePath = nullptr;
                        bool resolved = false;
                        for (auto& interfaceMap : objectPath.second)
                        {
                            if (interfaceMap.first ==
                                "xyz.openbmc_project.Logging.Entry")
                            {
                                for (auto& propertyMap : interfaceMap.second)
                                {
                                    if (propertyMap.first == "Id")
                                    {
                                        id = std::get_if<uint32_t>(
                                            &propertyMap.second);
                                    }
                                    else if (propertyMap.first == "Timestamp")
                                    {
                                        const uint64_t* millisTimeStamp =
                                            std::get_if<uint64_t>(
                                                &propertyMap.second);
                                        if (millisTimeStamp != nullptr)
                                        {
                                            timestamp =
                                                crow::utility::getTimestamp(
                                                    *millisTimeStamp);
                                        }
                                    }
                                    else if (propertyMap.first ==
                                             "UpdateTimestamp")
                                    {
                                        const uint64_t* millisTimeStamp =
                                            std::get_if<uint64_t>(
                                                &propertyMap.second);
                                        if (millisTimeStamp != nullptr)
                                        {
                                            updateTimestamp =
                                                crow::utility::getTimestamp(
                                                    *millisTimeStamp);
                                        }
                                    }
                                    else if (propertyMap.first == "Severity")
                                    {
                                        severity = std::get_if<std::string>(
                                            &propertyMap.second);
                                    }
                                    else if (propertyMap.first == "Message")
                                    {
                                        message = std::get_if<std::string>(
                                            &propertyMap.second);
                                    }
                                    else if (propertyMap.first == "Resolved")
                                    {
                                        bool* resolveptr = std::get_if<bool>(
                                            &propertyMap.second);
                                        if (resolveptr == nullptr)
                                        {
                                            messages::internalError(
                                                asyncResp->res);
                                            return;
                                        }
                                        resolved = *resolveptr;
                                    }
                                }
                                if (id == nullptr || message == nullptr ||
                                    severity == nullptr)
                                {
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                            }
                            else if (interfaceMap.first ==
                                     "xyz.openbmc_project.Common.FilePath")
                            {
                                for (auto& propertyMap : interfaceMap.second)
                                {
                                    if (propertyMap.first == "Path")
                                    {
                                        filePath = std::get_if<std::string>(
                                            &propertyMap.second);
                                    }
                                }
                            }
                        }
                        // Object path without the
                        // xyz.openbmc_project.Logging.Entry interface, ignore
                        // and continue.
                        if (id == nullptr || message == nullptr ||
                            severity == nullptr)
                        {
                            continue;
                        }
                        entriesArray.push_back({});
                        nlohmann::json& thisEntry = entriesArray.back();
                        thisEntry["@odata.type"] = "#LogEntry.v1_8_0.LogEntry";
                        thisEntry["@odata.id"] =
                            "/redfish/v1/Systems/system/"
                            "LogServices/EventLog/Entries/" +
                            std::to_string(*id);
                        thisEntry["Name"] = "System Event Log Entry";
                        thisEntry["Id"] = std::to_string(*id);
                        thisEntry["Message"] = *message;
                        thisEntry["Resolved"] = resolved;
                        thisEntry["EntryType"] = "Event";
                        thisEntry["Severity"] =
                            translateSeverityDbusToRedfish(*severity);
                        thisEntry["Created"] =
                            crow::utility::getDateTime(timestamp);
                        thisEntry["Modified"] =
                            crow::utility::getDateTime(updateTimestamp);
                        if (filePath != nullptr)
                        {
                            thisEntry["AdditionalDataURI"] =
                                "/redfish/v1/Systems/system/LogServices/"
                                "EventLog/"
                                "Entries/" +
                                std::to_string(*id) + "/attachment";
                        }
                    }
                    std::sort(entriesArray.begin(), entriesArray.end(),
                              [](const nlohmann::json& left,
                                 const nlohmann::json& right) {
                                  return (left["Id"] <= right["Id"]);
                              });
                    asyncResp->res.jsonValue["Members@odata.count"] =
                        entriesArray.size();
                },
                "xyz.openbmc_project.Logging", "/xyz/openbmc_project/logging",
                "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
        });
}

void requestRoutesDBusEventLogEntry(App& app)
{
    BMCWEB_ROUTE(
        app, "/redfish/v1/Systems/system/LogServices/EventLog/Entries/<str>/")
        .privileges(redfish::privileges::getLogEntry)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& param)

            {
                std::string entryID = param;
                dbus::utility::escapePathForDbus(entryID);

                // DBus implementation of EventLog/Entries
                // Make call to Logging Service to find all log entry objects
                crow::connections::systemBus->async_method_call(
                    [asyncResp, entryID](const boost::system::error_code ec,
                                         GetManagedPropertyType& resp) {
                        if (ec.value() == EBADR)
                        {
                            messages::resourceNotFound(
                                asyncResp->res, "EventLogEntry", entryID);
                            return;
                        }
                        if (ec)
                        {
                            BMCWEB_LOG_ERROR << "EventLogEntry (DBus) "
                                                "resp_handler got error "
                                             << ec;
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        uint32_t* id = nullptr;
                        std::time_t timestamp{};
                        std::time_t updateTimestamp{};
                        std::string* severity = nullptr;
                        std::string* message = nullptr;
                        std::string* filePath = nullptr;
                        bool resolved = false;

                        for (auto& propertyMap : resp)
                        {
                            if (propertyMap.first == "Id")
                            {
                                id = std::get_if<uint32_t>(&propertyMap.second);
                            }
                            else if (propertyMap.first == "Timestamp")
                            {
                                const uint64_t* millisTimeStamp =
                                    std::get_if<uint64_t>(&propertyMap.second);
                                if (millisTimeStamp != nullptr)
                                {
                                    timestamp = crow::utility::getTimestamp(
                                        *millisTimeStamp);
                                }
                            }
                            else if (propertyMap.first == "UpdateTimestamp")
                            {
                                const uint64_t* millisTimeStamp =
                                    std::get_if<uint64_t>(&propertyMap.second);
                                if (millisTimeStamp != nullptr)
                                {
                                    updateTimestamp =
                                        crow::utility::getTimestamp(
                                            *millisTimeStamp);
                                }
                            }
                            else if (propertyMap.first == "Severity")
                            {
                                severity = std::get_if<std::string>(
                                    &propertyMap.second);
                            }
                            else if (propertyMap.first == "Message")
                            {
                                message = std::get_if<std::string>(
                                    &propertyMap.second);
                            }
                            else if (propertyMap.first == "Resolved")
                            {
                                bool* resolveptr =
                                    std::get_if<bool>(&propertyMap.second);
                                if (resolveptr == nullptr)
                                {
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                                resolved = *resolveptr;
                            }
                            else if (propertyMap.first == "Path")
                            {
                                filePath = std::get_if<std::string>(
                                    &propertyMap.second);
                            }
                        }
                        if (id == nullptr || message == nullptr ||
                            severity == nullptr)
                        {
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        asyncResp->res.jsonValue["@odata.type"] =
                            "#LogEntry.v1_8_0.LogEntry";
                        asyncResp->res.jsonValue["@odata.id"] =
                            "/redfish/v1/Systems/system/LogServices/EventLog/"
                            "Entries/" +
                            std::to_string(*id);
                        asyncResp->res.jsonValue["Name"] =
                            "System Event Log Entry";
                        asyncResp->res.jsonValue["Id"] = std::to_string(*id);
                        asyncResp->res.jsonValue["Message"] = *message;
                        asyncResp->res.jsonValue["Resolved"] = resolved;
                        asyncResp->res.jsonValue["EntryType"] = "Event";
                        asyncResp->res.jsonValue["Severity"] =
                            translateSeverityDbusToRedfish(*severity);
                        asyncResp->res.jsonValue["Created"] =
                            crow::utility::getDateTime(timestamp);
                        asyncResp->res.jsonValue["Modified"] =
                            crow::utility::getDateTime(updateTimestamp);
                        if (filePath != nullptr)
                        {
                            asyncResp->res.jsonValue["AdditionalDataURI"] =
                                "/redfish/v1/Systems/system/LogServices/"
                                "EventLog/"
                                "attachment/" +
                                std::to_string(*id);
                        }
                    },
                    "xyz.openbmc_project.Logging",
                    "/xyz/openbmc_project/logging/entry/" + entryID,
                    "org.freedesktop.DBus.Properties", "GetAll", "");
            });

    BMCWEB_ROUTE(
        app, "/redfish/v1/Systems/system/LogServices/EventLog/Entries/<str>/")
        .privileges(redfish::privileges::patchLogEntry)
        .methods(boost::beast::http::verb::patch)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& entryId) {
                std::optional<bool> resolved;

                if (!json_util::readJson(req, asyncResp->res, "Resolved",
                                         resolved))
                {
                    return;
                }
                BMCWEB_LOG_DEBUG << "Set Resolved";

                crow::connections::systemBus->async_method_call(
                    [asyncResp, entryId](const boost::system::error_code ec) {
                        if (ec)
                        {
                            BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                            messages::internalError(asyncResp->res);
                            return;
                        }
                    },
                    "xyz.openbmc_project.Logging",
                    "/xyz/openbmc_project/logging/entry/" + entryId,
                    "org.freedesktop.DBus.Properties", "Set",
                    "xyz.openbmc_project.Logging.Entry", "Resolved",
                    std::variant<bool>(*resolved));
            });

    BMCWEB_ROUTE(
        app, "/redfish/v1/Systems/system/LogServices/EventLog/Entries/<str>/")
        .privileges(redfish::privileges::deleteLogEntry)

        .methods(boost::beast::http::verb::delete_)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& param)

            {
                BMCWEB_LOG_DEBUG << "Do delete single event entries.";

                std::string entryID = param;

                dbus::utility::escapePathForDbus(entryID);

                // Process response from Logging service.
                auto respHandler = [asyncResp, entryID](
                                       const boost::system::error_code ec) {
                    BMCWEB_LOG_DEBUG
                        << "EventLogEntry (DBus) doDelete callback: Done";
                    if (ec)
                    {
                        if (ec.value() == EBADR)
                        {
                            messages::resourceNotFound(asyncResp->res,
                                                       "LogEntry", entryID);
                            return;
                        }
                        // TODO Handle for specific error code
                        BMCWEB_LOG_ERROR << "EventLogEntry (DBus) doDelete "
                                            "respHandler got error "
                                         << ec;
                        asyncResp->res.result(
                            boost::beast::http::status::internal_server_error);
                        return;
                    }

                    asyncResp->res.result(boost::beast::http::status::ok);
                };

                // Make call to Logging service to request Delete Log
                crow::connections::systemBus->async_method_call(
                    respHandler, "xyz.openbmc_project.Logging",
                    "/xyz/openbmc_project/logging/entry/" + entryID,
                    "xyz.openbmc_project.Object.Delete", "Delete");
            });
}

void requestRoutesDBusEventLogEntryDownload(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/LogServices/EventLog/Entries/"
                      "<str>/attachment")
        .privileges(redfish::privileges::getLogEntry)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& param)

            {
                if (!http_helpers::isOctetAccepted(
                        req.getHeaderValue("Accept")))
                {
                    asyncResp->res.result(
                        boost::beast::http::status::bad_request);
                    return;
                }

                std::string entryID = param;
                dbus::utility::escapePathForDbus(entryID);

                crow::connections::systemBus->async_method_call(
                    [asyncResp,
                     entryID](const boost::system::error_code ec,
                              const sdbusplus::message::unix_fd& unixfd) {
                        if (ec.value() == EBADR)
                        {
                            messages::resourceNotFound(
                                asyncResp->res, "EventLogAttachment", entryID);
                            return;
                        }
                        if (ec)
                        {
                            BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                            messages::internalError(asyncResp->res);
                            return;
                        }

                        int fd = -1;
                        fd = dup(unixfd);
                        if (fd == -1)
                        {
                            messages::internalError(asyncResp->res);
                            return;
                        }

                        long long int size = lseek(fd, 0, SEEK_END);
                        if (size == -1)
                        {
                            messages::internalError(asyncResp->res);
                            return;
                        }

                        // Arbitrary max size of 64kb
                        constexpr int maxFileSize = 65536;
                        if (size > maxFileSize)
                        {
                            BMCWEB_LOG_ERROR
                                << "File size exceeds maximum allowed size of "
                                << maxFileSize;
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        std::vector<char> data(static_cast<size_t>(size));
                        long long int rc = lseek(fd, 0, SEEK_SET);
                        if (rc == -1)
                        {
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        rc = read(fd, data.data(), data.size());
                        if ((rc == -1) || (rc != size))
                        {
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        close(fd);

                        std::string_view strData(data.data(), data.size());
                        std::string output =
                            crow::utility::base64encode(strData);

                        asyncResp->res.addHeader("Content-Type",
                                                 "application/octet-stream");
                        asyncResp->res.addHeader("Content-Transfer-Encoding",
                                                 "Base64");
                        asyncResp->res.body() = std::move(output);
                    },
                    "xyz.openbmc_project.Logging",
                    "/xyz/openbmc_project/logging/entry/" + entryID,
                    "xyz.openbmc_project.Logging.Entry", "GetEntry");
            });
}

void requestRoutesBMCLogServiceCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Managers/bmc/LogServices/")
        .privileges(redfish::privileges::getLogServiceCollection)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                // Collections don't include the static data added by SubRoute
                // because it has a duplicate entry for members
                asyncResp->res.jsonValue["@odata.type"] =
                    "#LogServiceCollection.LogServiceCollection";
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/Managers/bmc/LogServices";
                asyncResp->res.jsonValue["Name"] =
                    "Open BMC Log Services Collection";
                asyncResp->res.jsonValue["Description"] =
                    "Collection of LogServices for this Manager";
                nlohmann::json& logServiceArray =
                    asyncResp->res.jsonValue["Members"];
                logServiceArray = nlohmann::json::array();
#ifdef BMCWEB_ENABLE_REDFISH_DUMP_LOG
                logServiceArray.push_back(
                    {{"@odata.id",
                      "/redfish/v1/Managers/bmc/LogServices/Dump"}});
#endif
#ifdef BMCWEB_ENABLE_REDFISH_BMC_JOURNAL
                logServiceArray.push_back(
                    {{"@odata.id",
                      "/redfish/v1/Managers/bmc/LogServices/Journal"}});
#endif
                asyncResp->res.jsonValue["Members@odata.count"] =
                    logServiceArray.size();
            });
}

void requestRoutesBMCJournalLogService(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Managers/bmc/LogServices/Journal/")
        .privileges(redfish::privileges::getLogService)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)

            {
                asyncResp->res.jsonValue["@odata.type"] =
                    "#LogService.v1_1_0.LogService";
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/Managers/bmc/LogServices/Journal";
                asyncResp->res.jsonValue["Name"] =
                    "Open BMC Journal Log Service";
                asyncResp->res.jsonValue["Description"] =
                    "BMC Journal Log Service";
                asyncResp->res.jsonValue["Id"] = "BMC Journal";
                asyncResp->res.jsonValue["OverWritePolicy"] = "WrapsWhenFull";

                std::pair<std::string, std::string> redfishDateTimeOffset =
                    crow::utility::getDateTimeOffsetNow();
                asyncResp->res.jsonValue["DateTime"] =
                    redfishDateTimeOffset.first;
                asyncResp->res.jsonValue["DateTimeLocalOffset"] =
                    redfishDateTimeOffset.second;

                asyncResp->res.jsonValue["Entries"] = {
                    {"@odata.id",
                     "/redfish/v1/Managers/bmc/LogServices/Journal/Entries"}};
            });
}

static int fillBMCJournalLogEntryJson(const std::string& bmcJournalLogEntryID,
                                      sd_journal* journal,
                                      nlohmann::json& bmcJournalLogEntryJson)
{
    // Get the Log Entry contents
    int ret = 0;

    std::string message;
    std::string_view syslogID;
    ret = getJournalMetadata(journal, "SYSLOG_IDENTIFIER", syslogID);
    if (ret < 0)
    {
        BMCWEB_LOG_ERROR << "Failed to read SYSLOG_IDENTIFIER field: "
                         << strerror(-ret);
    }
    if (!syslogID.empty())
    {
        message += std::string(syslogID) + ": ";
    }

    std::string_view msg;
    ret = getJournalMetadata(journal, "MESSAGE", msg);
    if (ret < 0)
    {
        BMCWEB_LOG_ERROR << "Failed to read MESSAGE field: " << strerror(-ret);
        return 1;
    }
    message += std::string(msg);

    // Get the severity from the PRIORITY field
    long int severity = 8; // Default to an invalid priority
    ret = getJournalMetadata(journal, "PRIORITY", 10, severity);
    if (ret < 0)
    {
        BMCWEB_LOG_ERROR << "Failed to read PRIORITY field: " << strerror(-ret);
    }

    // Get the Created time from the timestamp
    std::string entryTimeStr;
    if (!getEntryTimestamp(journal, entryTimeStr))
    {
        return 1;
    }

    // Fill in the log entry with the gathered data
    bmcJournalLogEntryJson = {
        {"@odata.type", "#LogEntry.v1_8_0.LogEntry"},
        {"@odata.id", "/redfish/v1/Managers/bmc/LogServices/Journal/Entries/" +
                          bmcJournalLogEntryID},
        {"Name", "BMC Journal Entry"},
        {"Id", bmcJournalLogEntryID},
        {"Message", std::move(message)},
        {"EntryType", "Oem"},
        {"Severity", severity <= 2   ? "Critical"
                     : severity <= 4 ? "Warning"
                                     : "OK"},
        {"OemRecordFormat", "BMC Journal Entry"},
        {"Created", std::move(entryTimeStr)}};
    return 0;
}

void requestRoutesBMCJournalLogEntryCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Managers/bmc/LogServices/Journal/Entries/")
        .privileges(redfish::privileges::getLogEntryCollection)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                static constexpr const long maxEntriesPerPage = 1000;
                uint64_t skip = 0;
                uint64_t top = maxEntriesPerPage; // Show max entries by default
                if (!getSkipParam(asyncResp, req, skip))
                {
                    return;
                }
                if (!getTopParam(asyncResp, req, top))
                {
                    return;
                }
                // Collections don't include the static data added by SubRoute
                // because it has a duplicate entry for members
                asyncResp->res.jsonValue["@odata.type"] =
                    "#LogEntryCollection.LogEntryCollection";
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/Managers/bmc/LogServices/Journal/Entries";
                asyncResp->res.jsonValue["Name"] = "Open BMC Journal Entries";
                asyncResp->res.jsonValue["Description"] =
                    "Collection of BMC Journal Entries";
                nlohmann::json& logEntryArray =
                    asyncResp->res.jsonValue["Members"];
                logEntryArray = nlohmann::json::array();

                // Go through the journal and use the timestamp to create a
                // unique ID for each entry
                sd_journal* journalTmp = nullptr;
                int ret = sd_journal_open(&journalTmp, SD_JOURNAL_LOCAL_ONLY);
                if (ret < 0)
                {
                    BMCWEB_LOG_ERROR << "failed to open journal: "
                                     << strerror(-ret);
                    messages::internalError(asyncResp->res);
                    return;
                }
                std::unique_ptr<sd_journal, decltype(&sd_journal_close)>
                    journal(journalTmp, sd_journal_close);
                journalTmp = nullptr;
                uint64_t entryCount = 0;
                // Reset the unique ID on the first entry
                bool firstEntry = true;
                SD_JOURNAL_FOREACH(journal.get())
                {
                    entryCount++;
                    // Handle paging using skip (number of entries to skip from
                    // the start) and top (number of entries to display)
                    if (entryCount <= skip || entryCount > skip + top)
                    {
                        continue;
                    }

                    std::string idStr;
                    if (!getUniqueEntryID(journal.get(), idStr, firstEntry))
                    {
                        continue;
                    }

                    if (firstEntry)
                    {
                        firstEntry = false;
                    }

                    logEntryArray.push_back({});
                    nlohmann::json& bmcJournalLogEntry = logEntryArray.back();
                    if (fillBMCJournalLogEntryJson(idStr, journal.get(),
                                                   bmcJournalLogEntry) != 0)
                    {
                        messages::internalError(asyncResp->res);
                        return;
                    }
                }
                asyncResp->res.jsonValue["Members@odata.count"] = entryCount;
                if (skip + top < entryCount)
                {
                    asyncResp->res.jsonValue["Members@odata.nextLink"] =
                        "/redfish/v1/Managers/bmc/LogServices/Journal/"
                        "Entries?$skip=" +
                        std::to_string(skip + top);
                }
            });
}

void requestRoutesBMCJournalLogEntry(App& app)
{
    BMCWEB_ROUTE(app,
                 "/redfish/v1/Managers/bmc/LogServices/Journal/Entries/<str>/")
        .privileges(redfish::privileges::getLogEntry)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& entryID) {
                // Convert the unique ID back to a timestamp to find the entry
                uint64_t ts = 0;
                uint64_t index = 0;
                if (!getTimestampFromID(asyncResp, entryID, ts, index))
                {
                    return;
                }

                sd_journal* journalTmp = nullptr;
                int ret = sd_journal_open(&journalTmp, SD_JOURNAL_LOCAL_ONLY);
                if (ret < 0)
                {
                    BMCWEB_LOG_ERROR << "failed to open journal: "
                                     << strerror(-ret);
                    messages::internalError(asyncResp->res);
                    return;
                }
                std::unique_ptr<sd_journal, decltype(&sd_journal_close)>
                    journal(journalTmp, sd_journal_close);
                journalTmp = nullptr;
                // Go to the timestamp in the log and move to the entry at the
                // index tracking the unique ID
                std::string idStr;
                bool firstEntry = true;
                ret = sd_journal_seek_realtime_usec(journal.get(), ts);
                if (ret < 0)
                {
                    BMCWEB_LOG_ERROR << "failed to seek to an entry in journal"
                                     << strerror(-ret);
                    messages::internalError(asyncResp->res);
                    return;
                }
                for (uint64_t i = 0; i <= index; i++)
                {
                    sd_journal_next(journal.get());
                    if (!getUniqueEntryID(journal.get(), idStr, firstEntry))
                    {
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    if (firstEntry)
                    {
                        firstEntry = false;
                    }
                }
                // Confirm that the entry ID matches what was requested
                if (idStr != entryID)
                {
                    messages::resourceMissingAtURI(asyncResp->res, entryID);
                    return;
                }

                if (fillBMCJournalLogEntryJson(entryID, journal.get(),
                                               asyncResp->res.jsonValue) != 0)
                {
                    messages::internalError(asyncResp->res);
                    return;
                }
            });
}

void requestRoutesBMCDumpCreate(App& app)
{

    BMCWEB_ROUTE(app, "/redfish/v1/Managers/bmc/LogServices/Dump/"
                      "Actions/"
                      "LogService.CollectDiagnosticData/")
        .privileges(redfish::privileges::postLogService)
        .methods(boost::beast::http::verb::post)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                createDump(asyncResp, req, "BMC");
            });
}

void requestRoutesBMCDumpClear(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Managers/bmc/LogServices/Dump/"
                      "Actions/"
                      "LogService.ClearLog/")
        .privileges(redfish::privileges::postLogService)
        .methods(boost::beast::http::verb::post)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                clearDump(asyncResp, "BMC");
            });
}

void requestRoutesSystemDumpService(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/LogServices/Dump/")
        .privileges(redfish::privileges::getLogService)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)

            {
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/Systems/system/LogServices/Dump";
                asyncResp->res.jsonValue["@odata.type"] =
                    "#LogService.v1_2_0.LogService";
                asyncResp->res.jsonValue["Name"] = "Dump LogService";
                asyncResp->res.jsonValue["Description"] =
                    "System Dump LogService";
                asyncResp->res.jsonValue["Id"] = "Dump";
                asyncResp->res.jsonValue["OverWritePolicy"] = "WrapsWhenFull";

                std::pair<std::string, std::string> redfishDateTimeOffset =
                    crow::utility::getDateTimeOffsetNow();
                asyncResp->res.jsonValue["DateTime"] =
                    redfishDateTimeOffset.first;
                asyncResp->res.jsonValue["DateTimeLocalOffset"] =
                    redfishDateTimeOffset.second;

                asyncResp->res.jsonValue["Entries"] = {
                    {"@odata.id",
                     "/redfish/v1/Systems/system/LogServices/Dump/Entries"}};
                asyncResp->res.jsonValue["Actions"] = {
                    {"#LogService.ClearLog",
                     {{"target",
                       "/redfish/v1/Systems/system/LogServices/Dump/Actions/"
                       "LogService.ClearLog"}}},
                    {"#LogService.CollectDiagnosticData",
                     {{"target",
                       "/redfish/v1/Systems/system/LogServices/Dump/Actions/"
                       "LogService.CollectDiagnosticData"}}}};
            });
}

void requestRoutesBMCDumpService(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Managers/bmc/LogServices/Dump/")
        .privileges(redfish::privileges::getLogService)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/Managers/bmc/LogServices/Dump";
                asyncResp->res.jsonValue["@odata.type"] =
                    "#LogService.v1_2_0.LogService";
                asyncResp->res.jsonValue["Name"] = "Dump LogService";
                asyncResp->res.jsonValue["Description"] = "BMC Dump LogService";
                asyncResp->res.jsonValue["Id"] = "Dump";
                asyncResp->res.jsonValue["OverWritePolicy"] = "WrapsWhenFull";

                std::pair<std::string, std::string> redfishDateTimeOffset =
                    crow::utility::getDateTimeOffsetNow();
                asyncResp->res.jsonValue["DateTime"] =
                    redfishDateTimeOffset.first;
                asyncResp->res.jsonValue["DateTimeLocalOffset"] =
                    redfishDateTimeOffset.second;

                asyncResp->res.jsonValue["Entries"] = {
                    {"@odata.id",
                     "/redfish/v1/Managers/bmc/LogServices/Dump/Entries"}};
                asyncResp->res.jsonValue["Actions"] = {
                    {"#LogService.ClearLog",
                     {{"target", "/redfish/v1/Managers/bmc/LogServices/Dump/"
                                 "Actions/LogService.ClearLog"}}},
                    {"#LogService.CollectDiagnosticData",
                     {{"target", "/redfish/v1/Managers/bmc/LogServices/Dump/"
                                 "Actions/LogService.CollectDiagnosticData"}}}};
            });
}

void requestRoutesBMCDumpEntryCollection(App& app)
{

    /**
     * Functions triggers appropriate requests on DBus
     */
    BMCWEB_ROUTE(app, "/redfish/v1/Managers/bmc/LogServices/Dump/Entries/")
        .privileges(redfish::privileges::getLogEntryCollection)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                asyncResp->res.jsonValue["@odata.type"] =
                    "#LogEntryCollection.LogEntryCollection";
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/Managers/bmc/LogServices/Dump/Entries";
                asyncResp->res.jsonValue["Name"] = "BMC Dump Entries";
                asyncResp->res.jsonValue["Description"] =
                    "Collection of BMC Dump Entries";

                getDumpEntryCollection(asyncResp, "BMC");
            });
}

void requestRoutesBMCDumpEntry(App& app)
{
    BMCWEB_ROUTE(app,
                 "/redfish/v1/Managers/bmc/LogServices/Dump/Entries/<str>/")
        .privileges(redfish::privileges::getLogEntry)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& param) {
                getDumpEntryById(asyncResp, param, "BMC");
            });
    BMCWEB_ROUTE(app,
                 "/redfish/v1/Managers/bmc/LogServices/Dump/Entries/<str>/")
        .privileges(redfish::privileges::deleteLogEntry)
        .methods(boost::beast::http::verb::delete_)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& param) {
                deleteDumpEntry(asyncResp, param, "bmc");
            });
}

void requestRoutesCrashdumpFile(App& app)
{
    // Note: Deviated from redfish privilege registry for GET & HEAD
    // method for security reasons.
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Systems/system/LogServices/Crashdump/Entries/<str>/<str>/")
        .privileges(redfish::privileges::getLogEntry)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& logID, const std::string& fileName) {
                auto getStoredLogCallback =
                    [asyncResp, logID, fileName](
                        const boost::system::error_code ec,
                        const std::vector<std::pair<std::string, VariantType>>&
                            resp) {
                        if (ec)
                        {
                            BMCWEB_LOG_DEBUG << "failed to get log ec: "
                                             << ec.message();
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
                            messages::resourceMissingAtURI(asyncResp->res,
                                                           fileName);
                            return;
                        }

                        // Verify the file name parameter is correct
                        if (fileName != dbusFilename)
                        {
                            messages::resourceMissingAtURI(asyncResp->res,
                                                           fileName);
                            return;
                        }

                        if (!std::filesystem::exists(dbusFilepath))
                        {
                            messages::resourceMissingAtURI(asyncResp->res,
                                                           fileName);
                            return;
                        }
                        std::ifstream ifs(dbusFilepath, std::ios::in |
                                                            std::ios::binary |
                                                            std::ios::ate);
                        std::ifstream::pos_type fileSize = ifs.tellg();
                        if (fileSize < 0)
                        {
                            messages::generalError(asyncResp->res);
                            return;
                        }
                        ifs.seekg(0, std::ios::beg);

                        auto crashData = std::make_unique<char[]>(
                            static_cast<unsigned int>(fileSize));

                        ifs.read(crashData.get(), static_cast<int>(fileSize));

                        // The cast to std::string is intentional in order to
                        // use the assign() that applies move mechanics
                        asyncResp->res.body().assign(
                            static_cast<std::string>(crashData.get()));

                        // Configure this to be a file download when accessed
                        // from a browser
                        asyncResp->res.addHeader("Content-Disposition",
                                                 "attachment");
                    };
                crow::connections::systemBus->async_method_call(
                    std::move(getStoredLogCallback), crashdumpObject,
                    crashdumpPath + std::string("/") + logID,
                    "org.freedesktop.DBus.Properties", "GetAll",
                    crashdumpInterface);
            });
}

// moved to task.cpp
//void requestRoutesCrashdumpCollect(App& app)

void requestRoutesDBusLogServiceActionsClear(App& app)
{
    /**
     * Function handles POST method request.
     * The Clear Log actions does not require any parameter.The action deletes
     * all entries found in the Entries collection for this Log Service.
     */

    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/LogServices/EventLog/Actions/"
                      "LogService.ClearLog/")
        .privileges(redfish::privileges::postLogService)
        .methods(boost::beast::http::verb::post)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                BMCWEB_LOG_DEBUG << "Do delete all entries.";

                // Process response from Logging service.
                auto respHandler = [asyncResp](
                                       const boost::system::error_code ec) {
                    BMCWEB_LOG_DEBUG
                        << "doClearLog resp_handler callback: Done";
                    if (ec)
                    {
                        // TODO Handle for specific error code
                        BMCWEB_LOG_ERROR << "doClearLog resp_handler got error "
                                         << ec;
                        asyncResp->res.result(
                            boost::beast::http::status::internal_server_error);
                        return;
                    }

                    asyncResp->res.result(
                        boost::beast::http::status::no_content);
                };

                // Make call to Logging service to request Clear Log
                crow::connections::systemBus->async_method_call(
                    respHandler, "xyz.openbmc_project.Logging",
                    "/xyz/openbmc_project/logging",
                    "xyz.openbmc_project.Collection.DeleteAll", "DeleteAll");
            });
}

void requestRoutesPostCodesLogService(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/LogServices/PostCodes/")
        .privileges(redfish::privileges::getLogService)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                asyncResp->res.jsonValue = {
                    {"@odata.id",
                     "/redfish/v1/Systems/system/LogServices/PostCodes"},
                    {"@odata.type", "#LogService.v1_1_0.LogService"},
                    {"Name", "POST Code Log Service"},
                    {"Description", "POST Code Log Service"},
                    {"Id", "BIOS POST Code Log"},
                    {"OverWritePolicy", "WrapsWhenFull"},
                    {"Entries",
                     {{"@odata.id", "/redfish/v1/Systems/system/LogServices/"
                                    "PostCodes/Entries"}}}};

                std::pair<std::string, std::string> redfishDateTimeOffset =
                    crow::utility::getDateTimeOffsetNow();
                asyncResp->res.jsonValue["DateTime"] =
                    redfishDateTimeOffset.first;
                asyncResp->res.jsonValue["DateTimeLocalOffset"] =
                    redfishDateTimeOffset.second;

                asyncResp->res.jsonValue["Actions"]["#LogService.ClearLog"] = {
                    {"target",
                     "/redfish/v1/Systems/system/LogServices/PostCodes/"
                     "Actions/LogService.ClearLog"}};
            });
}

void requestRoutesPostCodesClear(App& app)
{
    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/system/LogServices/PostCodes/Actions/"
                 "LogService.ClearLog/")
        // The following privilege is incorrect;  It should be ConfigureManager
        //.privileges(redfish::privileges::postLogService)
        .privileges({{"ConfigureComponents"}})
        .methods(boost::beast::http::verb::post)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                BMCWEB_LOG_DEBUG << "Do delete all postcodes entries.";

                // Make call to post-code service to request clear all
                crow::connections::systemBus->async_method_call(
                    [asyncResp](const boost::system::error_code ec) {
                        if (ec)
                        {
                            // TODO Handle for specific error code
                            BMCWEB_LOG_ERROR
                                << "doClearPostCodes resp_handler got error "
                                << ec;
                            asyncResp->res.result(boost::beast::http::status::
                                                      internal_server_error);
                            messages::internalError(asyncResp->res);
                            return;
                        }
                    },
                    "xyz.openbmc_project.State.Boot.PostCode0",
                    "/xyz/openbmc_project/State/Boot/PostCode0",
                    "xyz.openbmc_project.Collection.DeleteAll", "DeleteAll");
            });
}

static void fillPostCodeEntry(
    const std::shared_ptr<bmcweb::AsyncResp>& aResp,
    const boost::container::flat_map<
        uint64_t, std::tuple<uint64_t, std::vector<uint8_t>>>& postcode,
    const uint16_t bootIndex, const uint64_t codeIndex = 0,
    const uint64_t skip = 0, const uint64_t top = 0)
{
    // Get the Message from the MessageRegistry
    const message_registries::Message* message =
        message_registries::getMessage("OpenBMC.0.2.BIOSPOSTCode");

    uint64_t currentCodeIndex = 0;
    nlohmann::json& logEntryArray = aResp->res.jsonValue["Members"];

    uint64_t firstCodeTimeUs = 0;
    for (const std::pair<uint64_t, std::tuple<uint64_t, std::vector<uint8_t>>>&
             code : postcode)
    {
        currentCodeIndex++;
        std::string postcodeEntryID =
            "B" + std::to_string(bootIndex) + "-" +
            std::to_string(currentCodeIndex); // 1 based index in EntryID string

        uint64_t usecSinceEpoch = code.first;
        uint64_t usTimeOffset = 0;

        if (1 == currentCodeIndex)
        { // already incremented
            firstCodeTimeUs = code.first;
        }
        else
        {
            usTimeOffset = code.first - firstCodeTimeUs;
        }

        // skip if no specific codeIndex is specified and currentCodeIndex does
        // not fall between top and skip
        if ((codeIndex == 0) &&
            (currentCodeIndex <= skip || currentCodeIndex > top))
        {
            continue;
        }

        // skip if a specific codeIndex is specified and does not match the
        // currentIndex
        if ((codeIndex > 0) && (currentCodeIndex != codeIndex))
        {
            // This is done for simplicity. 1st entry is needed to calculate
            // time offset. To improve efficiency, one can get to the entry
            // directly (possibly with flatmap's nth method)
            continue;
        }

        // currentCodeIndex is within top and skip or equal to specified code
        // index

        // Get the Created time from the timestamp
        std::string entryTimeStr;
        entryTimeStr = crow::utility::getDateTime(
            static_cast<std::time_t>(usecSinceEpoch / 1000 / 1000));

        // assemble messageArgs: BootIndex, TimeOffset(100us), PostCode(hex)
        std::ostringstream hexCode;
        hexCode << "0x" << std::setfill('0') << std::setw(2) << std::hex
                << std::get<0>(code.second);
        std::ostringstream timeOffsetStr;
        // Set Fixed -Point Notation
        timeOffsetStr << std::fixed;
        // Set precision to 4 digits
        timeOffsetStr << std::setprecision(4);
        // Add double to stream
        timeOffsetStr << static_cast<double>(usTimeOffset) / 1000 / 1000;
        std::vector<std::string> messageArgs = {
            std::to_string(bootIndex), timeOffsetStr.str(), hexCode.str()};

        // Get MessageArgs template from message registry
        std::string msg;
        if (message != nullptr)
        {
            msg = message->message;

            // fill in this post code value
            int i = 0;
            for (const std::string& messageArg : messageArgs)
            {
                std::string argStr = "%" + std::to_string(++i);
                size_t argPos = msg.find(argStr);
                if (argPos != std::string::npos)
                {
                    msg.replace(argPos, argStr.length(), messageArg);
                }
            }
        }

        // Get Severity template from message registry
        std::string severity;
        if (message != nullptr)
        {
            severity = message->severity;
        }

        // add to AsyncResp
        logEntryArray.push_back({});
        nlohmann::json& bmcLogEntry = logEntryArray.back();
        bmcLogEntry = {{"@odata.type", "#LogEntry.v1_8_0.LogEntry"},
                       {"@odata.id", "/redfish/v1/Systems/system/LogServices/"
                                     "PostCodes/Entries/" +
                                         postcodeEntryID},
                       {"Name", "POST Code Log Entry"},
                       {"Id", postcodeEntryID},
                       {"Message", std::move(msg)},
                       {"MessageId", "OpenBMC.0.2.BIOSPOSTCode"},
                       {"MessageArgs", std::move(messageArgs)},
                       {"EntryType", "Event"},
                       {"Severity", std::move(severity)},
                       {"Created", entryTimeStr}};
        if (!std::get<std::vector<uint8_t>>(code.second).empty())
        {
            bmcLogEntry["AdditionalDataURI"] =
                "/redfish/v1/Systems/system/LogServices/PostCodes/Entries/" +
                postcodeEntryID + "/attachment";
        }
    }
}

static void getPostCodeForEntry(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                                const uint16_t bootIndex,
                                const uint64_t codeIndex)
{
    crow::connections::systemBus->async_method_call(
        [aResp, bootIndex,
         codeIndex](const boost::system::error_code ec,
                    const boost::container::flat_map<
                        uint64_t, std::tuple<uint64_t, std::vector<uint8_t>>>&
                        postcode) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS POST CODE PostCode response error";
                messages::internalError(aResp->res);
                return;
            }

            // skip the empty postcode boots
            if (postcode.empty())
            {
                return;
            }

            fillPostCodeEntry(aResp, postcode, bootIndex, codeIndex);

            aResp->res.jsonValue["Members@odata.count"] =
                aResp->res.jsonValue["Members"].size();
        },
        "xyz.openbmc_project.State.Boot.PostCode0",
        "/xyz/openbmc_project/State/Boot/PostCode0",
        "xyz.openbmc_project.State.Boot.PostCode", "GetPostCodesWithTimeStamp",
        bootIndex);
}

static void getPostCodeForBoot(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                               const uint16_t bootIndex,
                               const uint16_t bootCount,
                               const uint64_t entryCount, const uint64_t skip,
                               const uint64_t top)
{
    crow::connections::systemBus->async_method_call(
        [aResp, bootIndex, bootCount, entryCount, skip,
         top](const boost::system::error_code ec,
              const boost::container::flat_map<
                  uint64_t, std::tuple<uint64_t, std::vector<uint8_t>>>&
                  postcode) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS POST CODE PostCode response error";
                messages::internalError(aResp->res);
                return;
            }

            uint64_t endCount = entryCount;
            if (!postcode.empty())
            {
                endCount = entryCount + postcode.size();

                if ((skip < endCount) && ((top + skip) > entryCount))
                {
                    uint64_t thisBootSkip =
                        std::max(skip, entryCount) - entryCount;
                    uint64_t thisBootTop =
                        std::min(top + skip, endCount) - entryCount;

                    fillPostCodeEntry(aResp, postcode, bootIndex, 0,
                                      thisBootSkip, thisBootTop);
                }
                aResp->res.jsonValue["Members@odata.count"] = endCount;
            }

            // continue to previous bootIndex
            if (bootIndex < bootCount)
            {
                getPostCodeForBoot(aResp, static_cast<uint16_t>(bootIndex + 1),
                                   bootCount, endCount, skip, top);
            }
            else
            {
                aResp->res.jsonValue["Members@odata.nextLink"] =
                    "/redfish/v1/Systems/system/LogServices/PostCodes/"
                    "Entries?$skip=" +
                    std::to_string(skip + top);
            }
        },
        "xyz.openbmc_project.State.Boot.PostCode0",
        "/xyz/openbmc_project/State/Boot/PostCode0",
        "xyz.openbmc_project.State.Boot.PostCode", "GetPostCodesWithTimeStamp",
        bootIndex);
}

static void
    getCurrentBootNumber(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                         const uint64_t skip, const uint64_t top)
{
    uint64_t entryCount = 0;
    crow::connections::systemBus->async_method_call(
        [aResp, entryCount, skip,
         top](const boost::system::error_code ec,
              const std::variant<uint16_t>& bootCount) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                messages::internalError(aResp->res);
                return;
            }
            auto pVal = std::get_if<uint16_t>(&bootCount);
            if (pVal)
            {
                getPostCodeForBoot(aResp, 1, *pVal, entryCount, skip, top);
            }
            else
            {
                BMCWEB_LOG_DEBUG << "Post code boot index failed.";
            }
        },
        "xyz.openbmc_project.State.Boot.PostCode0",
        "/xyz/openbmc_project/State/Boot/PostCode0",
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.State.Boot.PostCode", "CurrentBootCycleCount");
}

void requestRoutesPostCodesEntryCollection(App& app)
{
    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/system/LogServices/PostCodes/Entries/")
        .privileges(redfish::privileges::getLogEntryCollection)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                asyncResp->res.jsonValue["@odata.type"] =
                    "#LogEntryCollection.LogEntryCollection";
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/Systems/system/LogServices/PostCodes/Entries";
                asyncResp->res.jsonValue["Name"] = "BIOS POST Code Log Entries";
                asyncResp->res.jsonValue["Description"] =
                    "Collection of POST Code Log Entries";
                asyncResp->res.jsonValue["Members"] = nlohmann::json::array();
                asyncResp->res.jsonValue["Members@odata.count"] = 0;

                uint64_t skip = 0;
                uint64_t top = maxEntriesPerPage; // Show max entries by default
                if (!getSkipParam(asyncResp, req, skip))
                {
                    return;
                }
                if (!getTopParam(asyncResp, req, top))
                {
                    return;
                }
                getCurrentBootNumber(asyncResp, skip, top);
            });
}

inline static bool parsePostCode(const std::string& postCodeID,
                                 uint64_t& currentValue, uint16_t& index)
{
    std::vector<std::string> split;
    boost::algorithm::split(split, postCodeID, boost::is_any_of("-"));
    if (split.size() != 2 || split[0].length() < 2 || split[0].front() != 'B')
    {
        return false;
    }

    const char* start = split[0].data() + 1;
    const char* end = split[0].data() + split[0].size();
    auto [ptrIndex, ecIndex] = std::from_chars(start, end, index);

    if (ptrIndex != end || ecIndex != std::errc())
    {
        return false;
    }

    start = split[1].data();
    end = split[1].data() + split[1].size();
    auto [ptrValue, ecValue] = std::from_chars(start, end, currentValue);
    if (ptrValue != end || ecValue != std::errc())
    {
        return false;
    }

    return true;
}

void requestRoutesPostCodesEntryAdditionalData(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/LogServices/PostCodes/"
                      "Entries/<str>/attachment/")
        .privileges(redfish::privileges::getLogEntry)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& postCodeID) {
                if (!http_helpers::isOctetAccepted(
                        req.getHeaderValue("Accept")))
                {
                    asyncResp->res.result(
                        boost::beast::http::status::bad_request);
                    return;
                }

                uint64_t currentValue = 0;
                uint16_t index = 0;
                if (!parsePostCode(postCodeID, currentValue, index))
                {
                    messages::resourceNotFound(asyncResp->res, "LogEntry",
                                               postCodeID);
                    return;
                }

                crow::connections::systemBus->async_method_call(
                    [asyncResp, postCodeID, currentValue](
                        const boost::system::error_code ec,
                        const std::vector<std::tuple<
                            uint64_t, std::vector<uint8_t>>>& postcodes) {
                        if (ec.value() == EBADR)
                        {
                            messages::resourceNotFound(asyncResp->res,
                                                       "LogEntry", postCodeID);
                            return;
                        }
                        if (ec)
                        {
                            BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                            messages::internalError(asyncResp->res);
                            return;
                        }

                        size_t value = static_cast<size_t>(currentValue) - 1;
                        if (value == std::string::npos ||
                            postcodes.size() < currentValue)
                        {
                            BMCWEB_LOG_ERROR << "Wrong currentValue value";
                            messages::resourceNotFound(asyncResp->res,
                                                       "LogEntry", postCodeID);
                            return;
                        }

                        auto& [tID, code] = postcodes[value];
                        if (code.empty())
                        {
                            BMCWEB_LOG_INFO << "No found post code data";
                            messages::resourceNotFound(asyncResp->res,
                                                       "LogEntry", postCodeID);
                            return;
                        }

                        std::string_view strData(
                            reinterpret_cast<const char*>(code.data()),
                            code.size());

                        asyncResp->res.addHeader("Content-Type",
                                                 "application/octet-stream");
                        asyncResp->res.addHeader("Content-Transfer-Encoding",
                                                 "Base64");
                        asyncResp->res.body() =
                            crow::utility::base64encode(strData);
                    },
                    "xyz.openbmc_project.State.Boot.PostCode0",
                    "/xyz/openbmc_project/State/Boot/PostCode0",
                    "xyz.openbmc_project.State.Boot.PostCode", "GetPostCodes",
                    index);
            });
}

void requestRoutesPostCodesEntry(App& app)
{
    BMCWEB_ROUTE(
        app, "/redfish/v1/Systems/system/LogServices/PostCodes/Entries/<str>/")
        .privileges(redfish::privileges::getLogEntry)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& targetID) {
                uint16_t bootIndex = 0;
                uint64_t codeIndex = 0;
                if (!parsePostCode(targetID, codeIndex, bootIndex))
                {
                    // Requested ID was not found
                    messages::resourceMissingAtURI(asyncResp->res, targetID);
                    return;
                }
                if (bootIndex == 0 || codeIndex == 0)
                {
                    BMCWEB_LOG_DEBUG << "Get Post Code invalid entry string "
                                     << targetID;
                }

                asyncResp->res.jsonValue["@odata.type"] =
                    "#LogEntry.v1_4_0.LogEntry";
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/Systems/system/LogServices/PostCodes/"
                    "Entries";
                asyncResp->res.jsonValue["Name"] = "BIOS POST Code Log Entries";
                asyncResp->res.jsonValue["Description"] =
                    "Collection of POST Code Log Entries";
                asyncResp->res.jsonValue["Members"] = nlohmann::json::array();
                asyncResp->res.jsonValue["Members@odata.count"] = 0;

                getPostCodeForEntry(asyncResp, bootIndex, codeIndex);
            });
}

}