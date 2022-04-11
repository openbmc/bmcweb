/*
// Copyright (c) 2018 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
#pragma once

#include "app.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "gzfile.hpp"
#include "http_utility.hpp"
#include "human_sort.hpp"
#include "query.hpp"
#include "registries.hpp"
#include "registries/base_message_registry.hpp"
#include "registries/openbmc_message_registry.hpp"
#include "registries/privilege_registry.hpp"
#include "task.hpp"
#include "utils/dbus_utils.hpp"
#include "utils/time_utils.hpp"

#include <systemd/sd-journal.h>
#include <tinyxml2.h>
#include <unistd.h>

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/system/linux_error.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <array>
#include <charconv>
#include <filesystem>
#include <optional>
#include <span>
#include <string_view>
#include <variant>

namespace redfish
{

constexpr char const* crashdumpObject = "com.intel.crashdump";
constexpr char const* crashdumpPath = "/com/intel/crashdump";
constexpr char const* crashdumpInterface = "com.intel.crashdump";
constexpr char const* deleteAllInterface =
    "xyz.openbmc_project.Collection.DeleteAll";
constexpr char const* crashdumpOnDemandInterface =
    "com.intel.crashdump.OnDemand";
constexpr char const* crashdumpTelemetryInterface =
    "com.intel.crashdump.Telemetry";

enum class DumpCreationProgress
{
    DUMP_CREATE_SUCCESS,
    DUMP_CREATE_FAILED,
    DUMP_CREATE_INPROGRESS
};

namespace fs = std::filesystem;

inline std::string translateSeverityDbusToRedfish(const std::string& s)
{
    if ((s == "xyz.openbmc_project.Logging.Entry.Level.Alert") ||
        (s == "xyz.openbmc_project.Logging.Entry.Level.Critical") ||
        (s == "xyz.openbmc_project.Logging.Entry.Level.Emergency") ||
        (s == "xyz.openbmc_project.Logging.Entry.Level.Error"))
    {
        return "Critical";
    }
    if ((s == "xyz.openbmc_project.Logging.Entry.Level.Debug") ||
        (s == "xyz.openbmc_project.Logging.Entry.Level.Informational") ||
        (s == "xyz.openbmc_project.Logging.Entry.Level.Notice"))
    {
        return "OK";
    }
    if (s == "xyz.openbmc_project.Logging.Entry.Level.Warning")
    {
        return "Warning";
    }
    return "";
}

inline std::optional<bool> getProviderNotifyAction(const std::string& notify)
{
    std::optional<bool> notifyAction;
    if (notify == "xyz.openbmc_project.Logging.Entry.Notify.Notify")
    {
        notifyAction = true;
    }
    else if (notify == "xyz.openbmc_project.Logging.Entry.Notify.Inhibit")
    {
        notifyAction = false;
    }

    return notifyAction;
}

inline static int getJournalMetadata(sd_journal* journal,
                                     std::string_view field,
                                     std::string_view& contents)
{
    const char* data = nullptr;
    size_t length = 0;
    int ret = 0;
    // Get the metadata from the requested field of the journal entry
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    const void** dataVoid = reinterpret_cast<const void**>(&data);

    ret = sd_journal_get_data(journal, field.data(), dataVoid, &length);
    if (ret < 0)
    {
        return ret;
    }
    contents = std::string_view(data, length);
    // Only use the content after the "=" character.
    contents.remove_prefix(std::min(contents.find('=') + 1, contents.size()));
    return ret;
}

inline static int getJournalMetadata(sd_journal* journal,
                                     std::string_view field, const int& base,
                                     long int& contents)
{
    int ret = 0;
    std::string_view metadata;
    // Get the metadata from the requested field of the journal entry
    ret = getJournalMetadata(journal, field, metadata);
    if (ret < 0)
    {
        return ret;
    }
    contents = strtol(metadata.data(), nullptr, base);
    return ret;
}

inline static bool getEntryTimestamp(sd_journal* journal,
                                     std::string& entryTimestamp)
{
    int ret = 0;
    uint64_t timestamp = 0;
    ret = sd_journal_get_realtime_usec(journal, &timestamp);
    if (ret < 0)
    {
        BMCWEB_LOG_ERROR << "Failed to read entry timestamp: "
                         << strerror(-ret);
        return false;
    }
    entryTimestamp = redfish::time_utils::getDateTimeUintUs(timestamp);
    return true;
}

inline static bool getUniqueEntryID(sd_journal* journal, std::string& entryID,
                                    const bool firstEntry = true)
{
    int ret = 0;
    static uint64_t prevTs = 0;
    static int index = 0;
    if (firstEntry)
    {
        prevTs = 0;
    }

    // Get the entry timestamp
    uint64_t curTs = 0;
    ret = sd_journal_get_realtime_usec(journal, &curTs);
    if (ret < 0)
    {
        BMCWEB_LOG_ERROR << "Failed to read entry timestamp: "
                         << strerror(-ret);
        return false;
    }
    // If the timestamp isn't unique, increment the index
    if (curTs == prevTs)
    {
        index++;
    }
    else
    {
        // Otherwise, reset it
        index = 0;
    }
    // Save the timestamp
    prevTs = curTs;

    entryID = std::to_string(curTs);
    if (index > 0)
    {
        entryID += "_" + std::to_string(index);
    }
    return true;
}

static bool getUniqueEntryID(const std::string& logEntry, std::string& entryID,
                             const bool firstEntry = true)
{
    static time_t prevTs = 0;
    static int index = 0;
    if (firstEntry)
    {
        prevTs = 0;
    }

    // Get the entry timestamp
    std::time_t curTs = 0;
    std::tm timeStruct = {};
    std::istringstream entryStream(logEntry);
    if (entryStream >> std::get_time(&timeStruct, "%Y-%m-%dT%H:%M:%S"))
    {
        curTs = std::mktime(&timeStruct);
    }
    // If the timestamp isn't unique, increment the index
    if (curTs == prevTs)
    {
        index++;
    }
    else
    {
        // Otherwise, reset it
        index = 0;
    }
    // Save the timestamp
    prevTs = curTs;

    entryID = std::to_string(curTs);
    if (index > 0)
    {
        entryID += "_" + std::to_string(index);
    }
    return true;
}

inline static bool
    getTimestampFromID(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& entryID, uint64_t& timestamp,
                       uint64_t& index)
{
    if (entryID.empty())
    {
        return false;
    }
    // Convert the unique ID back to a timestamp to find the entry
    std::string_view tsStr(entryID);

    auto underscorePos = tsStr.find('_');
    if (underscorePos != std::string_view::npos)
    {
        // Timestamp has an index
        tsStr.remove_suffix(tsStr.size() - underscorePos);
        std::string_view indexStr(entryID);
        indexStr.remove_prefix(underscorePos + 1);
        auto [ptr, ec] = std::from_chars(
            indexStr.data(), indexStr.data() + indexStr.size(), index);
        if (ec != std::errc())
        {
            messages::resourceNotFound(asyncResp->res, "LogEntry", entryID);
            return false;
        }
    }
    // Timestamp has no index
    auto [ptr, ec] =
        std::from_chars(tsStr.data(), tsStr.data() + tsStr.size(), timestamp);
    if (ec != std::errc())
    {
        messages::resourceNotFound(asyncResp->res, "LogEntry", entryID);
        return false;
    }
    return true;
}

static bool
    getRedfishLogFiles(std::vector<std::filesystem::path>& redfishLogFiles)
{
    static const std::filesystem::path redfishLogDir = "/var/log";
    static const std::string redfishLogFilename = "redfish";

    // Loop through the directory looking for redfish log files
    for (const std::filesystem::directory_entry& dirEnt :
         std::filesystem::directory_iterator(redfishLogDir))
    {
        // If we find a redfish log file, save the path
        std::string filename = dirEnt.path().filename();
        if (filename.starts_with(redfishLogFilename))
        {
            redfishLogFiles.emplace_back(redfishLogDir / filename);
        }
    }
    // As the log files rotate, they are appended with a ".#" that is higher for
    // the older logs. Since we don't expect more than 10 log files, we
    // can just sort the list to get them in order from newest to oldest
    std::sort(redfishLogFiles.begin(), redfishLogFiles.end());

    return !redfishLogFiles.empty();
}

inline void parseDumpEntryFromDbusObject(
    const dbus::utility::ManagedObjectType::value_type& object,
    std::string& dumpStatus, uint64_t& size, uint64_t& timestampUs,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    for (const auto& interfaceMap : object.second)
    {
        if (interfaceMap.first == "xyz.openbmc_project.Common.Progress")
        {
            for (const auto& propertyMap : interfaceMap.second)
            {
                if (propertyMap.first == "Status")
                {
                    const auto* status =
                        std::get_if<std::string>(&propertyMap.second);
                    if (status == nullptr)
                    {
                        messages::internalError(asyncResp->res);
                        break;
                    }
                    dumpStatus = *status;
                }
            }
        }
        else if (interfaceMap.first == "xyz.openbmc_project.Dump.Entry")
        {
            for (const auto& propertyMap : interfaceMap.second)
            {
                if (propertyMap.first == "Size")
                {
                    const auto* sizePtr =
                        std::get_if<uint64_t>(&propertyMap.second);
                    if (sizePtr == nullptr)
                    {
                        messages::internalError(asyncResp->res);
                        break;
                    }
                    size = *sizePtr;
                    break;
                }
            }
        }
        else if (interfaceMap.first == "xyz.openbmc_project.Time.EpochTime")
        {
            for (const auto& propertyMap : interfaceMap.second)
            {
                if (propertyMap.first == "Elapsed")
                {
                    const uint64_t* usecsTimeStamp =
                        std::get_if<uint64_t>(&propertyMap.second);
                    if (usecsTimeStamp == nullptr)
                    {
                        messages::internalError(asyncResp->res);
                        break;
                    }
                    timestampUs = *usecsTimeStamp;
                    break;
                }
            }
        }
    }
}

static std::string getDumpEntriesPath(const std::string& dumpType)
{
    std::string entriesPath;

    if (dumpType == "BMC")
    {
        entriesPath = "/redfish/v1/Managers/bmc/LogServices/Dump/Entries/";
    }
    else if (dumpType == "FaultLog")
    {
        entriesPath = "/redfish/v1/Managers/bmc/LogServices/FaultLog/Entries/";
    }
    else if (dumpType == "System")
    {
        entriesPath = "/redfish/v1/Systems/system/LogServices/Dump/Entries/";
    }
    else
    {
        BMCWEB_LOG_ERROR << "getDumpEntriesPath() invalid dump type: "
                         << dumpType;
    }

    // Returns empty string on error
    return entriesPath;
}

inline void
    getDumpEntryCollection(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const std::string& dumpType)
{
    std::string entriesPath = getDumpEntriesPath(dumpType);
    if (entriesPath.empty())
    {
        messages::internalError(asyncResp->res);
        return;
    }

    crow::connections::systemBus->async_method_call(
        [asyncResp, entriesPath,
         dumpType](const boost::system::error_code& ec,
                   dbus::utility::ManagedObjectType& resp) {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "DumpEntry resp_handler got error " << ec;
            messages::internalError(asyncResp->res);
            return;
        }

        // Remove ending slash
        std::string odataIdStr = entriesPath;
        if (!odataIdStr.empty())
        {
            odataIdStr.pop_back();
        }

        asyncResp->res.jsonValue["@odata.type"] =
            "#LogEntryCollection.LogEntryCollection";
        asyncResp->res.jsonValue["@odata.id"] = std::move(odataIdStr);
        asyncResp->res.jsonValue["Name"] = dumpType + " Dump Entries";
        asyncResp->res.jsonValue["Description"] =
            "Collection of " + dumpType + " Dump Entries";

        nlohmann::json& entriesArray = asyncResp->res.jsonValue["Members"];
        entriesArray = nlohmann::json::array();
        std::string dumpEntryPath =
            "/xyz/openbmc_project/dump/" +
            std::string(boost::algorithm::to_lower_copy(dumpType)) + "/entry/";

        std::sort(resp.begin(), resp.end(), [](const auto& l, const auto& r) {
            return AlphanumLess<std::string>()(l.first.filename(),
                                               r.first.filename());
        });

        for (auto& object : resp)
        {
            if (object.first.str.find(dumpEntryPath) == std::string::npos)
            {
                continue;
            }
            uint64_t timestampUs = 0;
            uint64_t size = 0;
            std::string dumpStatus;
            nlohmann::json::object_t thisEntry;

            std::string entryID = object.first.filename();
            if (entryID.empty())
            {
                continue;
            }

            parseDumpEntryFromDbusObject(object, dumpStatus, size, timestampUs,
                                         asyncResp);

            if (dumpStatus !=
                    "xyz.openbmc_project.Common.Progress.OperationStatus.Completed" &&
                !dumpStatus.empty())
            {
                // Dump status is not Complete, no need to enumerate
                continue;
            }

            thisEntry["@odata.type"] = "#LogEntry.v1_9_0.LogEntry";
            thisEntry["@odata.id"] = entriesPath + entryID;
            thisEntry["Id"] = entryID;
            thisEntry["EntryType"] = "Event";
            thisEntry["Name"] = dumpType + " Dump Entry";
            thisEntry["Created"] =
                redfish::time_utils::getDateTimeUintUs(timestampUs);

            if (dumpType == "BMC")
            {
                thisEntry["DiagnosticDataType"] = "Manager";
                thisEntry["AdditionalDataURI"] =
                    entriesPath + entryID + "/attachment";
                thisEntry["AdditionalDataSizeBytes"] = size;
            }
            else if (dumpType == "System")
            {
                thisEntry["DiagnosticDataType"] = "OEM";
                thisEntry["OEMDiagnosticDataType"] = "System";
                thisEntry["AdditionalDataURI"] =
                    entriesPath + entryID + "/attachment";
                thisEntry["AdditionalDataSizeBytes"] = size;
            }
            entriesArray.push_back(std::move(thisEntry));
        }
        asyncResp->res.jsonValue["Members@odata.count"] = entriesArray.size();
        },
        "xyz.openbmc_project.Dump.Manager", "/xyz/openbmc_project/dump",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
}

inline void
    getDumpEntryById(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     const std::string& entryID, const std::string& dumpType)
{
    std::string entriesPath = getDumpEntriesPath(dumpType);
    if (entriesPath.empty())
    {
        messages::internalError(asyncResp->res);
        return;
    }

    crow::connections::systemBus->async_method_call(
        [asyncResp, entryID, dumpType,
         entriesPath](const boost::system::error_code& ec,
                      const dbus::utility::ManagedObjectType& resp) {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "DumpEntry resp_handler got error " << ec;
            messages::internalError(asyncResp->res);
            return;
        }

        bool foundDumpEntry = false;
        std::string dumpEntryPath =
            "/xyz/openbmc_project/dump/" +
            std::string(boost::algorithm::to_lower_copy(dumpType)) + "/entry/";

        for (const auto& objectPath : resp)
        {
            if (objectPath.first.str != dumpEntryPath + entryID)
            {
                continue;
            }

            foundDumpEntry = true;
            uint64_t timestampUs = 0;
            uint64_t size = 0;
            std::string dumpStatus;

            parseDumpEntryFromDbusObject(objectPath, dumpStatus, size,
                                         timestampUs, asyncResp);

            if (dumpStatus !=
                    "xyz.openbmc_project.Common.Progress.OperationStatus.Completed" &&
                !dumpStatus.empty())
            {
                // Dump status is not Complete
                // return not found until status is changed to Completed
                messages::resourceNotFound(asyncResp->res, dumpType + " dump",
                                           entryID);
                return;
            }

            asyncResp->res.jsonValue["@odata.type"] =
                "#LogEntry.v1_9_0.LogEntry";
            asyncResp->res.jsonValue["@odata.id"] = entriesPath + entryID;
            asyncResp->res.jsonValue["Id"] = entryID;
            asyncResp->res.jsonValue["EntryType"] = "Event";
            asyncResp->res.jsonValue["Name"] = dumpType + " Dump Entry";
            asyncResp->res.jsonValue["Created"] =
                redfish::time_utils::getDateTimeUintUs(timestampUs);

            if (dumpType == "BMC")
            {
                asyncResp->res.jsonValue["DiagnosticDataType"] = "Manager";
                asyncResp->res.jsonValue["AdditionalDataURI"] =
                    entriesPath + entryID + "/attachment";
                asyncResp->res.jsonValue["AdditionalDataSizeBytes"] = size;
            }
            else if (dumpType == "System")
            {
                asyncResp->res.jsonValue["DiagnosticDataType"] = "OEM";
                asyncResp->res.jsonValue["OEMDiagnosticDataType"] = "System";
                asyncResp->res.jsonValue["AdditionalDataURI"] =
                    entriesPath + entryID + "/attachment";
                asyncResp->res.jsonValue["AdditionalDataSizeBytes"] = size;
            }
        }
        if (!foundDumpEntry)
        {
            BMCWEB_LOG_ERROR << "Can't find Dump Entry";
            messages::internalError(asyncResp->res);
            return;
        }
        },
        "xyz.openbmc_project.Dump.Manager", "/xyz/openbmc_project/dump",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
}

inline void deleteDumpEntry(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& entryID,
                            const std::string& dumpType)
{
    auto respHandler =
        [asyncResp, entryID](const boost::system::error_code& ec) {
        BMCWEB_LOG_DEBUG << "Dump Entry doDelete callback: Done";
        if (ec)
        {
            if (ec.value() == EBADR)
            {
                messages::resourceNotFound(asyncResp->res, "LogEntry", entryID);
                return;
            }
            BMCWEB_LOG_ERROR << "Dump (DBus) doDelete respHandler got error "
                             << ec << " entryID=" << entryID;
            messages::internalError(asyncResp->res);
            return;
        }
    };
    crow::connections::systemBus->async_method_call(
        respHandler, "xyz.openbmc_project.Dump.Manager",
        "/xyz/openbmc_project/dump/" +
            std::string(boost::algorithm::to_lower_copy(dumpType)) + "/entry/" +
            entryID,
        "xyz.openbmc_project.Object.Delete", "Delete");
}

inline DumpCreationProgress
    mapDbusStatusToDumpProgress(const std::string& status)
{
    if (status ==
            "xyz.openbmc_project.Common.Progress.OperationStatus.Failed" ||
        status == "xyz.openbmc_project.Common.Progress.OperationStatus.Aborted")
    {
        return DumpCreationProgress::DUMP_CREATE_FAILED;
    }
    if (status ==
        "xyz.openbmc_project.Common.Progress.OperationStatus.Completed")
    {
        return DumpCreationProgress::DUMP_CREATE_SUCCESS;
    }
    return DumpCreationProgress::DUMP_CREATE_INPROGRESS;
}

inline DumpCreationProgress
    getDumpCompletionStatus(const dbus::utility::DBusPropertiesMap& values)
{
    for (const auto& [key, val] : values)
    {
        if (key == "Status")
        {
            const std::string* value = std::get_if<std::string>(&val);
            if (value == nullptr)
            {
                BMCWEB_LOG_ERROR << "Status property value is null";
                return DumpCreationProgress::DUMP_CREATE_FAILED;
            }
            return mapDbusStatusToDumpProgress(*value);
        }
    }
    return DumpCreationProgress::DUMP_CREATE_INPROGRESS;
}

inline std::string getDumpEntryPath(const std::string& dumpPath)
{
    if (dumpPath == "/xyz/openbmc_project/dump/bmc/entry")
    {
        return "/redfish/v1/Managers/bmc/LogServices/Dump/Entries/";
    }
    if (dumpPath == "/xyz/openbmc_project/dump/system/entry")
    {
        return "/redfish/v1/Systems/system/LogServices/Dump/Entries/";
    }
    return "";
}

inline void createDumpTaskCallback(
    task::Payload&& payload,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const sdbusplus::message::object_path& createdObjPath)
{
    const std::string dumpPath = createdObjPath.parent_path().str;
    const std::string dumpId = createdObjPath.filename();

    std::string dumpEntryPath = getDumpEntryPath(dumpPath);

    if (dumpEntryPath.empty())
    {
        BMCWEB_LOG_ERROR << "Invalid dump type received";
        messages::internalError(asyncResp->res);
        return;
    }

    crow::connections::systemBus->async_method_call(
        [asyncResp, payload, createdObjPath,
         dumpEntryPath{std::move(dumpEntryPath)},
         dumpId](const boost::system::error_code& ec,
                 const std::string& introspectXml) {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "Introspect call failed with error: "
                             << ec.message();
            messages::internalError(asyncResp->res);
            return;
        }

        // Check if the created dump object has implemented Progress
        // interface to track dump completion. If yes, fetch the "Status"
        // property of the interface, modify the task state accordingly.
        // Else, return task completed.
        tinyxml2::XMLDocument doc;

        doc.Parse(introspectXml.data(), introspectXml.size());
        tinyxml2::XMLNode* pRoot = doc.FirstChildElement("node");
        if (pRoot == nullptr)
        {
            BMCWEB_LOG_ERROR << "XML document failed to parse";
            messages::internalError(asyncResp->res);
            return;
        }
        tinyxml2::XMLElement* interfaceNode =
            pRoot->FirstChildElement("interface");

        bool isProgressIntfPresent = false;
        while (interfaceNode != nullptr)
        {
            const char* thisInterfaceName = interfaceNode->Attribute("name");
            if (thisInterfaceName != nullptr)
            {
                if (thisInterfaceName ==
                    std::string_view("xyz.openbmc_project.Common.Progress"))
                {
                    interfaceNode =
                        interfaceNode->NextSiblingElement("interface");
                    continue;
                }
                isProgressIntfPresent = true;
                break;
            }
            interfaceNode = interfaceNode->NextSiblingElement("interface");
        }

        std::shared_ptr<task::TaskData> task = task::TaskData::createTask(
            [createdObjPath, dumpEntryPath, dumpId, isProgressIntfPresent](
                const boost::system::error_code& err, sdbusplus::message_t& msg,
                const std::shared_ptr<task::TaskData>& taskData) {
            if (err)
            {
                BMCWEB_LOG_ERROR << createdObjPath.str
                                 << ": Error in creating dump";
                taskData->messages.emplace_back(messages::internalError());
                taskData->state = "Cancelled";
                return task::completed;
            }

            if (isProgressIntfPresent)
            {
                dbus::utility::DBusPropertiesMap values;
                std::string prop;
                msg.read(prop, values);

                DumpCreationProgress dumpStatus =
                    getDumpCompletionStatus(values);
                if (dumpStatus == DumpCreationProgress::DUMP_CREATE_FAILED)
                {
                    BMCWEB_LOG_ERROR << createdObjPath.str
                                     << ": Error in creating dump";
                    taskData->state = "Cancelled";
                    return task::completed;
                }

                if (dumpStatus == DumpCreationProgress::DUMP_CREATE_INPROGRESS)
                {
                    BMCWEB_LOG_DEBUG << createdObjPath.str
                                     << ": Dump creation task is in progress";
                    return !task::completed;
                }
            }

            nlohmann::json retMessage = messages::success();
            taskData->messages.emplace_back(retMessage);

            std::string headerLoc =
                "Location: " + dumpEntryPath + http_helpers::urlEncode(dumpId);
            taskData->payload->httpHeaders.emplace_back(std::move(headerLoc));

            BMCWEB_LOG_DEBUG << createdObjPath.str
                             << ": Dump creation task completed";
            taskData->state = "Completed";
            return task::completed;
            },
            "type='signal',interface='org.freedesktop.DBus.Properties',"
            "member='PropertiesChanged',path='" +
                createdObjPath.str + "'");

        // The task timer is set to max time limit within which the
        // requested dump will be collected.
        task->startTimer(std::chrono::minutes(6));
        task->populateResp(asyncResp->res);
        task->payload.emplace(payload);
        },
        "xyz.openbmc_project.Dump.Manager", createdObjPath,
        "org.freedesktop.DBus.Introspectable", "Introspect");
}

inline void createDump(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const crow::Request& req, const std::string& dumpType)
{
    std::string dumpPath = getDumpEntriesPath(dumpType);
    if (dumpPath.empty())
    {
        messages::internalError(asyncResp->res);
        return;
    }

    std::optional<std::string> diagnosticDataType;
    std::optional<std::string> oemDiagnosticDataType;

    if (!redfish::json_util::readJsonAction(
            req, asyncResp->res, "DiagnosticDataType", diagnosticDataType,
            "OEMDiagnosticDataType", oemDiagnosticDataType))
    {
        return;
    }

    if (dumpType == "System")
    {
        if (!oemDiagnosticDataType || !diagnosticDataType)
        {
            BMCWEB_LOG_ERROR
                << "CreateDump action parameter 'DiagnosticDataType'/'OEMDiagnosticDataType' value not found!";
            messages::actionParameterMissing(
                asyncResp->res, "CollectDiagnosticData",
                "DiagnosticDataType & OEMDiagnosticDataType");
            return;
        }
        if ((*oemDiagnosticDataType != "System") ||
            (*diagnosticDataType != "OEM"))
        {
            BMCWEB_LOG_ERROR << "Wrong parameter values passed";
            messages::internalError(asyncResp->res);
            return;
        }
        dumpPath = "/redfish/v1/Systems/system/LogServices/Dump/";
    }
    else if (dumpType == "BMC")
    {
        if (!diagnosticDataType)
        {
            BMCWEB_LOG_ERROR
                << "CreateDump action parameter 'DiagnosticDataType' not found!";
            messages::actionParameterMissing(
                asyncResp->res, "CollectDiagnosticData", "DiagnosticDataType");
            return;
        }
        if (*diagnosticDataType != "Manager")
        {
            BMCWEB_LOG_ERROR
                << "Wrong parameter value passed for 'DiagnosticDataType'";
            messages::internalError(asyncResp->res);
            return;
        }
        dumpPath = "/redfish/v1/Managers/bmc/LogServices/Dump/";
    }
    else
    {
        BMCWEB_LOG_ERROR << "CreateDump failed. Unknown dump type";
        messages::internalError(asyncResp->res);
        return;
    }

    std::vector<std::pair<std::string, std::variant<std::string, uint64_t>>>
        createDumpParamVec;

    crow::connections::systemBus->async_method_call(
        [asyncResp, payload(task::Payload(req)),
         dumpPath](const boost::system::error_code& ec,
                   const sdbusplus::message_t& msg,
                   const sdbusplus::message::object_path& objPath) mutable {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "CreateDump resp_handler got error " << ec;
            const sd_bus_error* dbusError = msg.get_error();
            if (dbusError == nullptr)
            {
                messages::internalError(asyncResp->res);
                return;
            }

            BMCWEB_LOG_ERROR << "CreateDump DBus error: " << dbusError->name
                             << " and error msg: " << dbusError->message;
            if (std::string_view(
                    "xyz.openbmc_project.Common.Error.NotAllowed") ==
                dbusError->name)
            {
                messages::resourceInStandby(asyncResp->res);
                return;
            }
            if (std::string_view(
                    "xyz.openbmc_project.Dump.Create.Error.Disabled") ==
                dbusError->name)
            {
                messages::serviceDisabled(asyncResp->res, dumpPath);
                return;
            }
            if (std::string_view(
                    "xyz.openbmc_project.Common.Error.Unavailable") ==
                dbusError->name)
            {
                messages::resourceInUse(asyncResp->res);
                return;
            }
            // Other Dbus errors such as:
            // xyz.openbmc_project.Common.Error.InvalidArgument &
            // org.freedesktop.DBus.Error.InvalidArgs are all related to
            // the dbus call that is made here in the bmcweb
            // implementation and has nothing to do with the client's
            // input in the request. Hence, returning internal error
            // back to the client.
            messages::internalError(asyncResp->res);
            return;
        }
        BMCWEB_LOG_DEBUG << "Dump Created. Path: " << objPath.str;
        createDumpTaskCallback(std::move(payload), asyncResp, objPath);
        },
        "xyz.openbmc_project.Dump.Manager",
        "/xyz/openbmc_project/dump/" +
            std::string(boost::algorithm::to_lower_copy(dumpType)),
        "xyz.openbmc_project.Dump.Create", "CreateDump", createDumpParamVec);
}

inline void clearDump(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const std::string& dumpType)
{
    std::string dumpTypeLowerCopy =
        std::string(boost::algorithm::to_lower_copy(dumpType));

    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code& ec) {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "clearDump resp_handler got error " << ec;
            messages::internalError(asyncResp->res);
            return;
        }
        },
        "xyz.openbmc_project.Dump.Manager",
        "/xyz/openbmc_project/dump/" + dumpTypeLowerCopy,
        "xyz.openbmc_project.Collection.DeleteAll", "DeleteAll");
}

inline static void
    parseCrashdumpParameters(const dbus::utility::DBusPropertiesMap& params,
                             std::string& filename, std::string& timestamp,
                             std::string& logfile)
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

inline void requestRoutesSystemLogServiceCollection(App& app)
{
    /**
     * Functions triggers appropriate requests on DBus
     */
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/LogServices/")
        .privileges(redfish::privileges::getLogServiceCollection)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemName) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        if (systemName != "system")
        {
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }

        // Collections don't include the static data added by SubRoute
        // because it has a duplicate entry for members
        asyncResp->res.jsonValue["@odata.type"] =
            "#LogServiceCollection.LogServiceCollection";
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Systems/system/LogServices";
        asyncResp->res.jsonValue["Name"] = "System Log Services Collection";
        asyncResp->res.jsonValue["Description"] =
            "Collection of LogServices for this Computer System";
        nlohmann::json& logServiceArray = asyncResp->res.jsonValue["Members"];
        logServiceArray = nlohmann::json::array();
        nlohmann::json::object_t eventLog;
        eventLog["@odata.id"] =
            "/redfish/v1/Systems/system/LogServices/EventLog";
        logServiceArray.push_back(std::move(eventLog));
#ifdef BMCWEB_ENABLE_REDFISH_DUMP_LOG
        nlohmann::json::object_t dumpLog;
        dumpLog["@odata.id"] = "/redfish/v1/Systems/system/LogServices/Dump";
        logServiceArray.push_back(std::move(dumpLog));
#endif

#ifdef BMCWEB_ENABLE_REDFISH_CPU_LOG
        nlohmann::json::object_t crashdump;
        crashdump["@odata.id"] =
            "/redfish/v1/Systems/system/LogServices/Crashdump";
        logServiceArray.push_back(std::move(crashdump));
#endif

#ifdef BMCWEB_ENABLE_REDFISH_HOST_LOGGER
        nlohmann::json::object_t hostlogger;
        hostlogger["@odata.id"] =
            "/redfish/v1/Systems/system/LogServices/HostLogger";
        logServiceArray.push_back(std::move(hostlogger));
#endif
        asyncResp->res.jsonValue["Members@odata.count"] =
            logServiceArray.size();

        constexpr std::array<std::string_view, 1> interfaces = {
            "xyz.openbmc_project.State.Boot.PostCode"};
        dbus::utility::getSubTreePaths(
            "/", 0, interfaces,
            [asyncResp](const boost::system::error_code& ec,
                        const dbus::utility::MapperGetSubTreePathsResponse&
                            subtreePath) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << ec;
                return;
            }

            for (const auto& pathStr : subtreePath)
            {
                if (pathStr.find("PostCode") != std::string::npos)
                {
                    nlohmann::json& logServiceArrayLocal =
                        asyncResp->res.jsonValue["Members"];
                    nlohmann::json::object_t member;
                    member["@odata.id"] =
                        "/redfish/v1/Systems/system/LogServices/PostCodes";

                    logServiceArrayLocal.push_back(std::move(member));

                    asyncResp->res.jsonValue["Members@odata.count"] =
                        logServiceArrayLocal.size();
                    return;
                }
            }
            });
        });
}

inline void requestRoutesEventLogService(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/LogServices/EventLog/")
        .privileges(redfish::privileges::getLogService)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemName) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        if (systemName != "system")
        {
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Systems/system/LogServices/EventLog";
        asyncResp->res.jsonValue["@odata.type"] =
            "#LogService.v1_1_0.LogService";
        asyncResp->res.jsonValue["Name"] = "Event Log Service";
        asyncResp->res.jsonValue["Description"] = "System Event Log Service";
        asyncResp->res.jsonValue["Id"] = "EventLog";
        asyncResp->res.jsonValue["OverWritePolicy"] = "WrapsWhenFull";

        std::pair<std::string, std::string> redfishDateTimeOffset =
            redfish::time_utils::getDateTimeOffsetNow();

        asyncResp->res.jsonValue["DateTime"] = redfishDateTimeOffset.first;
        asyncResp->res.jsonValue["DateTimeLocalOffset"] =
            redfishDateTimeOffset.second;

        asyncResp->res.jsonValue["Entries"]["@odata.id"] =
            "/redfish/v1/Systems/system/LogServices/EventLog/Entries";
        asyncResp->res.jsonValue["Actions"]["#LogService.ClearLog"] = {

            {"target",
             "/redfish/v1/Systems/system/LogServices/EventLog/Actions/LogService.ClearLog"}};
        });
}

inline void requestRoutesJournalEventLogClear(App& app)
{
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Systems/<str>/LogServices/EventLog/Actions/LogService.ClearLog/")
        .privileges({{"ConfigureComponents"}})
        .methods(boost::beast::http::verb::post)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemName) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        if (systemName != "system")
        {
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }
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
            [asyncResp](const boost::system::error_code& ec) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "Failed to reload rsyslog: " << ec;
                messages::internalError(asyncResp->res);
                return;
            }

            messages::success(asyncResp->res);
            },
            "org.freedesktop.systemd1", "/org/freedesktop/systemd1",
            "org.freedesktop.systemd1.Manager", "ReloadUnit", "rsyslog.service",
            "replace");
        });
}

enum class LogParseError
{
    success,
    parseFailed,
    messageIdNotInRegistry,
};

static LogParseError
    fillEventLogEntryJson(const std::string& logEntryID,
                          const std::string& logEntry,
                          nlohmann::json::object_t& logEntryJson)
{
    // The redfish log format is "<Timestamp> <MessageId>,<MessageArgs>"
    // First get the Timestamp
    size_t space = logEntry.find_first_of(' ');
    if (space == std::string::npos)
    {
        return LogParseError::parseFailed;
    }
    std::string timestamp = logEntry.substr(0, space);
    // Then get the log contents
    size_t entryStart = logEntry.find_first_not_of(' ', space);
    if (entryStart == std::string::npos)
    {
        return LogParseError::parseFailed;
    }
    std::string_view entry(logEntry);
    entry.remove_prefix(entryStart);
    // Use split to separate the entry into its fields
    std::vector<std::string> logEntryFields;
    bmcweb::split(logEntryFields, entry, ',');
    // We need at least a MessageId to be valid
    if (logEntryFields.empty())
    {
        return LogParseError::parseFailed;
    }
    std::string& messageID = logEntryFields[0];

    // Get the Message from the MessageRegistry
    const registries::Message* message = registries::getMessage(messageID);

    if (message == nullptr)
    {
        BMCWEB_LOG_WARNING << "Log entry not found in registry: " << logEntry;
        return LogParseError::messageIdNotInRegistry;
    }

    std::string msg = message->message;

    // Get the MessageArgs from the log if there are any
    std::span<std::string> messageArgs;
    if (logEntryFields.size() > 1)
    {
        std::string& messageArgsStart = logEntryFields[1];
        // If the first string is empty, assume there are no MessageArgs
        std::size_t messageArgsSize = 0;
        if (!messageArgsStart.empty())
        {
            messageArgsSize = logEntryFields.size() - 1;
        }

        messageArgs = {&messageArgsStart, messageArgsSize};

        // Fill the MessageArgs into the Message
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

    // Get the Created time from the timestamp. The log timestamp is in RFC3339
    // format which matches the Redfish format except for the fractional seconds
    // between the '.' and the '+', so just remove them.
    std::size_t dot = timestamp.find_first_of('.');
    std::size_t plus = timestamp.find_first_of('+');
    if (dot != std::string::npos && plus != std::string::npos)
    {
        timestamp.erase(dot, plus - dot);
    }

    // Fill in the log entry with the gathered data
    logEntryJson["@odata.type"] = "#LogEntry.v1_9_0.LogEntry";
    logEntryJson["@odata.id"] = crow::utility::urlFromPieces(
        "redfish", "v1", "Systems", "system", "LogServices", "EventLog",
        "Entries", logEntryID);
    logEntryJson["Name"] = "System Event Log Entry";
    logEntryJson["Id"] = logEntryID;
    logEntryJson["Message"] = std::move(msg);
    logEntryJson["MessageId"] = std::move(messageID);
    logEntryJson["MessageArgs"] = messageArgs;
    logEntryJson["EntryType"] = "Event";
    logEntryJson["Severity"] = message->messageSeverity;
    logEntryJson["Created"] = std::move(timestamp);
    return LogParseError::success;
}

inline void requestRoutesJournalEventLogEntryCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/LogServices/EventLog/Entries/")
        .privileges(redfish::privileges::getLogEntryCollection)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemName) {
        query_param::QueryCapabilities capabilities = {
            .canDelegateTop = true,
            .canDelegateSkip = true,
        };
        query_param::Query delegatedQuery;
        if (!redfish::setUpRedfishRouteWithDelegation(
                app, req, asyncResp, delegatedQuery, capabilities))
        {
            return;
        }
        if (systemName != "system")
        {
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }

        size_t top = delegatedQuery.top.value_or(query_param::Query::maxTop);
        size_t skip = delegatedQuery.skip.value_or(0);

        // Collections don't include the static data added by SubRoute
        // because it has a duplicate entry for members
        asyncResp->res.jsonValue["@odata.type"] =
            "#LogEntryCollection.LogEntryCollection";
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Systems/system/LogServices/EventLog/Entries";
        asyncResp->res.jsonValue["Name"] = "System Event Log Entries";
        asyncResp->res.jsonValue["Description"] =
            "Collection of System Event Log Entries";

        nlohmann::json& logEntryArray = asyncResp->res.jsonValue["Members"];
        logEntryArray = nlohmann::json::array();
        // Go through the log files and create a unique ID for each
        // entry
        std::vector<std::filesystem::path> redfishLogFiles;
        getRedfishLogFiles(redfishLogFiles);
        uint64_t entryCount = 0;
        std::string logEntry;

        // Oldest logs are in the last file, so start there and loop
        // backwards
        for (auto it = redfishLogFiles.rbegin(); it < redfishLogFiles.rend();
             it++)
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
                firstEntry = false;

                nlohmann::json::object_t bmcLogEntry;
                LogParseError status =
                    fillEventLogEntryJson(idStr, logEntry, bmcLogEntry);
                if (status == LogParseError::messageIdNotInRegistry)
                {
                    continue;
                }
                if (status != LogParseError::success)
                {
                    messages::internalError(asyncResp->res);
                    return;
                }

                entryCount++;
                // Handle paging using skip (number of entries to skip from the
                // start) and top (number of entries to display)
                if (entryCount <= skip || entryCount > skip + top)
                {
                    continue;
                }

                logEntryArray.push_back(std::move(bmcLogEntry));
            }
        }
        asyncResp->res.jsonValue["Members@odata.count"] = entryCount;
        if (skip + top < entryCount)
        {
            asyncResp->res.jsonValue["Members@odata.nextLink"] =
                "/redfish/v1/Systems/system/LogServices/EventLog/Entries?$skip=" +
                std::to_string(skip + top);
        }
        });
}

inline void requestRoutesJournalEventLogEntry(App& app)
{
    BMCWEB_ROUTE(
        app, "/redfish/v1/Systems/<str>/LogServices/EventLog/Entries/<str>/")
        .privileges(redfish::privileges::getLogEntry)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemName, const std::string& param) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }

        if (systemName != "system")
        {
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }

        const std::string& targetID = param;

        // Go through the log files and check the unique ID for each
        // entry to find the target entry
        std::vector<std::filesystem::path> redfishLogFiles;
        getRedfishLogFiles(redfishLogFiles);
        std::string logEntry;

        // Oldest logs are in the last file, so start there and loop
        // backwards
        for (auto it = redfishLogFiles.rbegin(); it < redfishLogFiles.rend();
             it++)
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
                firstEntry = false;

                if (idStr == targetID)
                {
                    nlohmann::json::object_t bmcLogEntry;
                    LogParseError status =
                        fillEventLogEntryJson(idStr, logEntry, bmcLogEntry);
                    if (status != LogParseError::success)
                    {
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    asyncResp->res.jsonValue.update(bmcLogEntry);
                    return;
                }
            }
        }
        // Requested ID was not found
        messages::resourceNotFound(asyncResp->res, "LogEntry", targetID);
        });
}

inline void requestRoutesDBusEventLogEntryCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/LogServices/EventLog/Entries/")
        .privileges(redfish::privileges::getLogEntryCollection)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemName) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        if (systemName != "system")
        {
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
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

        // DBus implementation of EventLog/Entries
        // Make call to Logging Service to find all log entry objects
        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code& ec,
                        const dbus::utility::ManagedObjectType& resp) {
            if (ec)
            {
                // TODO Handle for specific error code
                BMCWEB_LOG_ERROR
                    << "getLogEntriesIfaceData resp_handler got error " << ec;
                messages::internalError(asyncResp->res);
                return;
            }
            nlohmann::json& entriesArray = asyncResp->res.jsonValue["Members"];
            entriesArray = nlohmann::json::array();
            for (const auto& objectPath : resp)
            {
                const uint32_t* id = nullptr;
                const uint64_t* timestamp = nullptr;
                const uint64_t* updateTimestamp = nullptr;
                const std::string* severity = nullptr;
                const std::string* message = nullptr;
                const std::string* filePath = nullptr;
                const std::string* resolution = nullptr;
                bool resolved = false;
                const std::string* notify = nullptr;

                for (const auto& interfaceMap : objectPath.second)
                {
                    if (interfaceMap.first ==
                        "xyz.openbmc_project.Logging.Entry")
                    {
                        for (const auto& propertyMap : interfaceMap.second)
                        {
                            if (propertyMap.first == "Id")
                            {
                                id = std::get_if<uint32_t>(&propertyMap.second);
                            }
                            else if (propertyMap.first == "Timestamp")
                            {
                                timestamp =
                                    std::get_if<uint64_t>(&propertyMap.second);
                            }
                            else if (propertyMap.first == "UpdateTimestamp")
                            {
                                updateTimestamp =
                                    std::get_if<uint64_t>(&propertyMap.second);
                            }
                            else if (propertyMap.first == "Severity")
                            {
                                severity = std::get_if<std::string>(
                                    &propertyMap.second);
                            }
                            else if (propertyMap.first == "Resolution")
                            {
                                resolution = std::get_if<std::string>(
                                    &propertyMap.second);
                            }
                            else if (propertyMap.first == "Message")
                            {
                                message = std::get_if<std::string>(
                                    &propertyMap.second);
                            }
                            else if (propertyMap.first == "Resolved")
                            {
                                const bool* resolveptr =
                                    std::get_if<bool>(&propertyMap.second);
                                if (resolveptr == nullptr)
                                {
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                                resolved = *resolveptr;
                            }
                            else if (propertyMap.first ==
                                     "ServiceProviderNotify")
                            {
                                notify = std::get_if<std::string>(
                                    &propertyMap.second);
                                if (notify == nullptr)
                                {
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
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
                        for (const auto& propertyMap : interfaceMap.second)
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
                    severity == nullptr || timestamp == nullptr ||
                    updateTimestamp == nullptr)
                {
                    continue;
                }
                entriesArray.push_back({});
                nlohmann::json& thisEntry = entriesArray.back();
                thisEntry["@odata.type"] = "#LogEntry.v1_9_0.LogEntry";
                thisEntry["@odata.id"] = crow::utility::urlFromPieces(
                    "redfish", "v1", "Systems", "system", "LogServices",
                    "EventLog", "Entries", std::to_string(*id));
                thisEntry["Name"] = "System Event Log Entry";
                thisEntry["Id"] = std::to_string(*id);
                thisEntry["Message"] = *message;
                thisEntry["Resolved"] = resolved;
                if ((resolution != nullptr) && (!(*resolution).empty()))
                {
                    thisEntry["Resolution"] = *resolution;
                }
                std::optional<bool> notifyAction =
                    getProviderNotifyAction(*notify);
                if (notifyAction)
                {
                    thisEntry["ServiceProviderNotified"] = *notifyAction;
                }
                thisEntry["EntryType"] = "Event";
                thisEntry["Severity"] =
                    translateSeverityDbusToRedfish(*severity);
                thisEntry["Created"] =
                    redfish::time_utils::getDateTimeUintMs(*timestamp);
                thisEntry["Modified"] =
                    redfish::time_utils::getDateTimeUintMs(*updateTimestamp);
                if (filePath != nullptr)
                {
                    thisEntry["AdditionalDataURI"] =
                        "/redfish/v1/Systems/system/LogServices/EventLog/Entries/" +
                        std::to_string(*id) + "/attachment";
                }
            }
            std::sort(
                entriesArray.begin(), entriesArray.end(),
                [](const nlohmann::json& left, const nlohmann::json& right) {
                return (left["Id"] <= right["Id"]);
                });
            asyncResp->res.jsonValue["Members@odata.count"] =
                entriesArray.size();
            },
            "xyz.openbmc_project.Logging", "/xyz/openbmc_project/logging",
            "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
        });
}

inline void requestRoutesDBusEventLogEntry(App& app)
{
    BMCWEB_ROUTE(
        app, "/redfish/v1/Systems/<str>/LogServices/EventLog/Entries/<str>/")
        .privileges(redfish::privileges::getLogEntry)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemName, const std::string& param) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        if (systemName != "system")
        {
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }

        std::string entryID = param;
        dbus::utility::escapePathForDbus(entryID);

        // DBus implementation of EventLog/Entries
        // Make call to Logging Service to find all log entry objects
        sdbusplus::asio::getAllProperties(
            *crow::connections::systemBus, "xyz.openbmc_project.Logging",
            "/xyz/openbmc_project/logging/entry/" + entryID, "",
            [asyncResp, entryID](const boost::system::error_code& ec,
                                 const dbus::utility::DBusPropertiesMap& resp) {
            if (ec.value() == EBADR)
            {
                messages::resourceNotFound(asyncResp->res, "EventLogEntry",
                                           entryID);
                return;
            }
            if (ec)
            {
                BMCWEB_LOG_ERROR
                    << "EventLogEntry (DBus) resp_handler got error " << ec;
                messages::internalError(asyncResp->res);
                return;
            }
            const uint32_t* id = nullptr;
            const uint64_t* timestamp = nullptr;
            const uint64_t* updateTimestamp = nullptr;
            const std::string* severity = nullptr;
            const std::string* message = nullptr;
            const std::string* filePath = nullptr;
            const std::string* resolution = nullptr;
            bool resolved = false;
            const std::string* notify = nullptr;

            const bool success = sdbusplus::unpackPropertiesNoThrow(
                dbus_utils::UnpackErrorPrinter(), resp, "Id", id, "Timestamp",
                timestamp, "UpdateTimestamp", updateTimestamp, "Severity",
                severity, "Message", message, "Resolved", resolved,
                "Resolution", resolution, "Path", filePath,
                "ServiceProviderNotify", notify);

            if (!success)
            {
                messages::internalError(asyncResp->res);
                return;
            }

            if (id == nullptr || message == nullptr || severity == nullptr ||
                timestamp == nullptr || updateTimestamp == nullptr ||
                notify == nullptr)
            {
                messages::internalError(asyncResp->res);
                return;
            }

            asyncResp->res.jsonValue["@odata.type"] =
                "#LogEntry.v1_9_0.LogEntry";
            asyncResp->res.jsonValue["@odata.id"] =
                crow::utility::urlFromPieces(
                    "redfish", "v1", "Systems", "system", "LogServices",
                    "EventLog", "Entries", std::to_string(*id));
            asyncResp->res.jsonValue["Name"] = "System Event Log Entry";
            asyncResp->res.jsonValue["Id"] = std::to_string(*id);
            asyncResp->res.jsonValue["Message"] = *message;
            asyncResp->res.jsonValue["Resolved"] = resolved;
            std::optional<bool> notifyAction = getProviderNotifyAction(*notify);
            if (notifyAction)
            {
                asyncResp->res.jsonValue["ServiceProviderNotified"] =
                    *notifyAction;
            }
            if ((resolution != nullptr) && (!(*resolution).empty()))
            {
                asyncResp->res.jsonValue["Resolution"] = *resolution;
            }
            asyncResp->res.jsonValue["EntryType"] = "Event";
            asyncResp->res.jsonValue["Severity"] =
                translateSeverityDbusToRedfish(*severity);
            asyncResp->res.jsonValue["Created"] =
                redfish::time_utils::getDateTimeUintMs(*timestamp);
            asyncResp->res.jsonValue["Modified"] =
                redfish::time_utils::getDateTimeUintMs(*updateTimestamp);
            if (filePath != nullptr)
            {
                asyncResp->res.jsonValue["AdditionalDataURI"] =
                    "/redfish/v1/Systems/system/LogServices/EventLog/Entries/" +
                    std::to_string(*id) + "/attachment";
            }
            });
        });

    BMCWEB_ROUTE(
        app, "/redfish/v1/Systems/<str>/LogServices/EventLog/Entries/<str>/")
        .privileges(redfish::privileges::patchLogEntry)
        .methods(boost::beast::http::verb::patch)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemName, const std::string& entryId) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        if (systemName != "system")
        {
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }
        std::optional<bool> resolved;

        if (!json_util::readJsonPatch(req, asyncResp->res, "Resolved",
                                      resolved))
        {
            return;
        }
        BMCWEB_LOG_DEBUG << "Set Resolved";

        crow::connections::systemBus->async_method_call(
            [asyncResp, entryId](const boost::system::error_code& ec) {
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
            dbus::utility::DbusVariantType(*resolved));
        });

    BMCWEB_ROUTE(
        app, "/redfish/v1/Systems/<str>/LogServices/EventLog/Entries/<str>/")
        .privileges(redfish::privileges::deleteLogEntry)

        .methods(boost::beast::http::verb::delete_)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemName, const std::string& param) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        if (systemName != "system")
        {
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }
        BMCWEB_LOG_DEBUG << "Do delete single event entries.";

        std::string entryID = param;

        dbus::utility::escapePathForDbus(entryID);

        // Process response from Logging service.
        auto respHandler =
            [asyncResp, entryID](const boost::system::error_code& ec) {
            BMCWEB_LOG_DEBUG << "EventLogEntry (DBus) doDelete callback: Done";
            if (ec)
            {
                if (ec.value() == EBADR)
                {
                    messages::resourceNotFound(asyncResp->res, "LogEntry",
                                               entryID);
                    return;
                }
                // TODO Handle for specific error code
                BMCWEB_LOG_ERROR
                    << "EventLogEntry (DBus) doDelete respHandler got error "
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

inline void requestRoutesDBusEventLogEntryDownload(App& app)
{
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Systems/<str>/LogServices/EventLog/Entries/<str>/attachment")
        .privileges(redfish::privileges::getLogEntry)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemName, const std::string& param) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        if (http_helpers::isContentTypeAllowed(
                req.getHeaderValue("Accept"),
                http_helpers::ContentType::OctetStream, true))
        {
            asyncResp->res.result(boost::beast::http::status::bad_request);
            return;
        }
        if (systemName != "system")
        {
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }

        std::string entryID = param;
        dbus::utility::escapePathForDbus(entryID);

        crow::connections::systemBus->async_method_call(
            [asyncResp, entryID](const boost::system::error_code& ec,
                                 const sdbusplus::message::unix_fd& unixfd) {
            if (ec.value() == EBADR)
            {
                messages::resourceNotFound(asyncResp->res, "EventLogAttachment",
                                           entryID);
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
                BMCWEB_LOG_ERROR << "File size exceeds maximum allowed size of "
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
            std::string output = crow::utility::base64encode(strData);

            asyncResp->res.addHeader(boost::beast::http::field::content_type,
                                     "application/octet-stream");
            asyncResp->res.addHeader(
                boost::beast::http::field::content_transfer_encoding, "Base64");
            asyncResp->res.body() = std::move(output);
            },
            "xyz.openbmc_project.Logging",
            "/xyz/openbmc_project/logging/entry/" + entryID,
            "xyz.openbmc_project.Logging.Entry", "GetEntry");
        });
}

constexpr const char* hostLoggerFolderPath = "/var/log/console";

inline bool
    getHostLoggerFiles(const std::string& hostLoggerFilePath,
                       std::vector<std::filesystem::path>& hostLoggerFiles)
{
    std::error_code ec;
    std::filesystem::directory_iterator logPath(hostLoggerFilePath, ec);
    if (ec)
    {
        BMCWEB_LOG_ERROR << ec.message();
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
            BMCWEB_LOG_ERROR << "fail to expose host logs";
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

inline void fillHostLoggerEntryJson(const std::string& logEntryID,
                                    const std::string& msg,
                                    nlohmann::json::object_t& logEntryJson)
{
    // Fill in the log entry with the gathered data.
    logEntryJson["@odata.type"] = "#LogEntry.v1_9_0.LogEntry";
    logEntryJson["@odata.id"] = crow::utility::urlFromPieces(
        "redfish", "v1", "Systems", "system", "LogServices", "HostLogger",
        "Entries", logEntryID);
    logEntryJson["Name"] = "Host Logger Entry";
    logEntryJson["Id"] = logEntryID;
    logEntryJson["Message"] = msg;
    logEntryJson["EntryType"] = "Oem";
    logEntryJson["Severity"] = "OK";
    logEntryJson["OemRecordFormat"] = "Host Logger Entry";
}

inline void requestRoutesSystemHostLogger(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/LogServices/HostLogger/")
        .privileges(redfish::privileges::getLogService)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemName) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        if (systemName != "system")
        {
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Systems/system/LogServices/HostLogger";
        asyncResp->res.jsonValue["@odata.type"] =
            "#LogService.v1_1_0.LogService";
        asyncResp->res.jsonValue["Name"] = "Host Logger Service";
        asyncResp->res.jsonValue["Description"] = "Host Logger Service";
        asyncResp->res.jsonValue["Id"] = "HostLogger";
        asyncResp->res.jsonValue["Entries"]["@odata.id"] =
            "/redfish/v1/Systems/system/LogServices/HostLogger/Entries";
        });
}

inline void requestRoutesSystemHostLoggerCollection(App& app)
{
    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/<str>/LogServices/HostLogger/Entries/")
        .privileges(redfish::privileges::getLogEntry)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemName) {
        query_param::QueryCapabilities capabilities = {
            .canDelegateTop = true,
            .canDelegateSkip = true,
        };
        query_param::Query delegatedQuery;
        if (!redfish::setUpRedfishRouteWithDelegation(
                app, req, asyncResp, delegatedQuery, capabilities))
        {
            return;
        }
        if (systemName != "system")
        {
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Systems/system/LogServices/HostLogger/Entries";
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
            BMCWEB_LOG_ERROR << "fail to get host log file path";
            return;
        }
        // If we weren't provided top and skip limits, use the defaults.
        size_t skip = delegatedQuery.skip.value_or(0);
        size_t top = delegatedQuery.top.value_or(query_param::Query::maxTop);
        size_t logCount = 0;
        // This vector only store the entries we want to expose that
        // control by skip and top.
        std::vector<std::string> logEntries;
        if (!getHostLoggerEntries(hostLoggerFiles, skip, top, logEntries,
                                  logCount))
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
                logEntryArray.push_back(std::move(hostLogEntry));
            }

            asyncResp->res.jsonValue["Members@odata.count"] = logCount;
            if (skip + top < logCount)
            {
                asyncResp->res.jsonValue["Members@odata.nextLink"] =
                    "/redfish/v1/Systems/system/LogServices/HostLogger/Entries?$skip=" +
                    std::to_string(skip + top);
            }
        }
        });
}

inline void requestRoutesSystemHostLoggerLogEntry(App& app)
{
    BMCWEB_ROUTE(
        app, "/redfish/v1/Systems/<str>/LogServices/HostLogger/Entries/<str>/")
        .privileges(redfish::privileges::getLogEntry)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemName, const std::string& param) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        if (systemName != "system")
        {
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }
        const std::string& targetID = param;

        uint64_t idInt = 0;

        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        const char* end = targetID.data() + targetID.size();

        auto [ptr, ec] = std::from_chars(targetID.data(), end, idInt);
        if (ec == std::errc::invalid_argument ||
            ec == std::errc::result_out_of_range)
        {
            messages::resourceNotFound(asyncResp->res, "LogEntry", param);
            return;
        }

        std::vector<std::filesystem::path> hostLoggerFiles;
        if (!getHostLoggerFiles(hostLoggerFolderPath, hostLoggerFiles))
        {
            BMCWEB_LOG_ERROR << "fail to get host log file path";
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
        });
}

inline void handleBMCLogServicesCollectionGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    // Collections don't include the static data added by SubRoute
    // because it has a duplicate entry for members
    asyncResp->res.jsonValue["@odata.type"] =
        "#LogServiceCollection.LogServiceCollection";
    asyncResp->res.jsonValue["@odata.id"] =
        "/redfish/v1/Managers/bmc/LogServices";
    asyncResp->res.jsonValue["Name"] = "Open BMC Log Services Collection";
    asyncResp->res.jsonValue["Description"] =
        "Collection of LogServices for this Manager";
    nlohmann::json& logServiceArray = asyncResp->res.jsonValue["Members"];
    logServiceArray = nlohmann::json::array();

#ifdef BMCWEB_ENABLE_REDFISH_BMC_JOURNAL
    nlohmann::json::object_t journal;
    journal["@odata.id"] = "/redfish/v1/Managers/bmc/LogServices/Journal";
    logServiceArray.push_back(std::move(journal));
#endif

    asyncResp->res.jsonValue["Members@odata.count"] = logServiceArray.size();

#ifdef BMCWEB_ENABLE_REDFISH_DUMP_LOG
    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Collection.DeleteAll"};
    dbus::utility::getSubTreePaths(
        "/xyz/openbmc_project/dump", 0, interfaces,
        [asyncResp](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreePathsResponse& subTreePaths) {
        if (ec)
        {
            BMCWEB_LOG_ERROR
                << "handleBMCLogServicesCollectionGet respHandler got error "
                << ec;
            // Assume that getting an error simply means there are no dump
            // LogServices. Return without adding any error response.
            return;
        }

        nlohmann::json& logServiceArrayLocal =
            asyncResp->res.jsonValue["Members"];

        for (const std::string& path : subTreePaths)
        {
            if (path == "/xyz/openbmc_project/dump/bmc")
            {
                nlohmann::json::object_t member;
                member["@odata.id"] =
                    "/redfish/v1/Managers/bmc/LogServices/Dump";
                logServiceArrayLocal.push_back(std::move(member));
            }
            else if (path == "/xyz/openbmc_project/dump/faultlog")
            {
                nlohmann::json::object_t member;
                member["@odata.id"] =
                    "/redfish/v1/Managers/bmc/LogServices/FaultLog";
                logServiceArrayLocal.push_back(std::move(member));
            }
        }

        asyncResp->res.jsonValue["Members@odata.count"] =
            logServiceArrayLocal.size();
        });
#endif
}

inline void requestRoutesBMCLogServiceCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Managers/bmc/LogServices/")
        .privileges(redfish::privileges::getLogServiceCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleBMCLogServicesCollectionGet, std::ref(app)));
}

inline void requestRoutesBMCJournalLogService(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Managers/bmc/LogServices/Journal/")
        .privileges(redfish::privileges::getLogService)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        asyncResp->res.jsonValue["@odata.type"] =
            "#LogService.v1_1_0.LogService";
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Managers/bmc/LogServices/Journal";
        asyncResp->res.jsonValue["Name"] = "Open BMC Journal Log Service";
        asyncResp->res.jsonValue["Description"] = "BMC Journal Log Service";
        asyncResp->res.jsonValue["Id"] = "Journal";
        asyncResp->res.jsonValue["OverWritePolicy"] = "WrapsWhenFull";

        std::pair<std::string, std::string> redfishDateTimeOffset =
            redfish::time_utils::getDateTimeOffsetNow();
        asyncResp->res.jsonValue["DateTime"] = redfishDateTimeOffset.first;
        asyncResp->res.jsonValue["DateTimeLocalOffset"] =
            redfishDateTimeOffset.second;

        asyncResp->res.jsonValue["Entries"]["@odata.id"] =
            "/redfish/v1/Managers/bmc/LogServices/Journal/Entries";
        });
}

static int
    fillBMCJournalLogEntryJson(const std::string& bmcJournalLogEntryID,
                               sd_journal* journal,
                               nlohmann::json::object_t& bmcJournalLogEntryJson)
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
    bmcJournalLogEntryJson["@odata.type"] = "#LogEntry.v1_9_0.LogEntry";
    bmcJournalLogEntryJson["@odata.id"] = crow::utility::urlFromPieces(
        "redfish", "v1", "Managers", "bmc", "LogServices", "Journal", "Entries",
        bmcJournalLogEntryID);
    bmcJournalLogEntryJson["Name"] = "BMC Journal Entry";
    bmcJournalLogEntryJson["Id"] = bmcJournalLogEntryID;
    bmcJournalLogEntryJson["Message"] = std::move(message);
    bmcJournalLogEntryJson["EntryType"] = "Oem";
    bmcJournalLogEntryJson["Severity"] = severity <= 2   ? "Critical"
                                         : severity <= 4 ? "Warning"
                                                         : "OK";
    bmcJournalLogEntryJson["OemRecordFormat"] = "BMC Journal Entry";
    bmcJournalLogEntryJson["Created"] = std::move(entryTimeStr);
    return 0;
}

inline void requestRoutesBMCJournalLogEntryCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Managers/bmc/LogServices/Journal/Entries/")
        .privileges(redfish::privileges::getLogEntryCollection)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
        query_param::QueryCapabilities capabilities = {
            .canDelegateTop = true,
            .canDelegateSkip = true,
        };
        query_param::Query delegatedQuery;
        if (!redfish::setUpRedfishRouteWithDelegation(
                app, req, asyncResp, delegatedQuery, capabilities))
        {
            return;
        }

        size_t skip = delegatedQuery.skip.value_or(0);
        size_t top = delegatedQuery.top.value_or(query_param::Query::maxTop);

        // Collections don't include the static data added by SubRoute
        // because it has a duplicate entry for members
        asyncResp->res.jsonValue["@odata.type"] =
            "#LogEntryCollection.LogEntryCollection";
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Managers/bmc/LogServices/Journal/Entries";
        asyncResp->res.jsonValue["Name"] = "Open BMC Journal Entries";
        asyncResp->res.jsonValue["Description"] =
            "Collection of BMC Journal Entries";
        nlohmann::json& logEntryArray = asyncResp->res.jsonValue["Members"];
        logEntryArray = nlohmann::json::array();

        // Go through the journal and use the timestamp to create a
        // unique ID for each entry
        sd_journal* journalTmp = nullptr;
        int ret = sd_journal_open(&journalTmp, SD_JOURNAL_LOCAL_ONLY);
        if (ret < 0)
        {
            BMCWEB_LOG_ERROR << "failed to open journal: " << strerror(-ret);
            messages::internalError(asyncResp->res);
            return;
        }
        std::unique_ptr<sd_journal, decltype(&sd_journal_close)> journal(
            journalTmp, sd_journal_close);
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
            firstEntry = false;

            nlohmann::json::object_t bmcJournalLogEntry;
            if (fillBMCJournalLogEntryJson(idStr, journal.get(),
                                           bmcJournalLogEntry) != 0)
            {
                messages::internalError(asyncResp->res);
                return;
            }
            logEntryArray.push_back(std::move(bmcJournalLogEntry));
        }
        asyncResp->res.jsonValue["Members@odata.count"] = entryCount;
        if (skip + top < entryCount)
        {
            asyncResp->res.jsonValue["Members@odata.nextLink"] =
                "/redfish/v1/Managers/bmc/LogServices/Journal/Entries?$skip=" +
                std::to_string(skip + top);
        }
        });
}

inline void requestRoutesBMCJournalLogEntry(App& app)
{
    BMCWEB_ROUTE(app,
                 "/redfish/v1/Managers/bmc/LogServices/Journal/Entries/<str>/")
        .privileges(redfish::privileges::getLogEntry)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& entryID) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
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
            BMCWEB_LOG_ERROR << "failed to open journal: " << strerror(-ret);
            messages::internalError(asyncResp->res);
            return;
        }
        std::unique_ptr<sd_journal, decltype(&sd_journal_close)> journal(
            journalTmp, sd_journal_close);
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
            firstEntry = false;
        }
        // Confirm that the entry ID matches what was requested
        if (idStr != entryID)
        {
            messages::resourceNotFound(asyncResp->res, "LogEntry", entryID);
            return;
        }

        nlohmann::json::object_t bmcJournalLogEntry;
        if (fillBMCJournalLogEntryJson(entryID, journal.get(),
                                       bmcJournalLogEntry) != 0)
        {
            messages::internalError(asyncResp->res);
            return;
        }
        asyncResp->res.jsonValue.update(bmcJournalLogEntry);
        });
}

inline void
    getDumpServiceInfo(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& dumpType)
{
    std::string dumpPath;
    std::string overWritePolicy;
    bool collectDiagnosticDataSupported = false;

    if (dumpType == "BMC")
    {
        dumpPath = "/redfish/v1/Managers/bmc/LogServices/Dump";
        overWritePolicy = "WrapsWhenFull";
        collectDiagnosticDataSupported = true;
    }
    else if (dumpType == "FaultLog")
    {
        dumpPath = "/redfish/v1/Managers/bmc/LogServices/FaultLog";
        overWritePolicy = "Unknown";
        collectDiagnosticDataSupported = false;
    }
    else if (dumpType == "System")
    {
        dumpPath = "/redfish/v1/Systems/system/LogServices/Dump";
        overWritePolicy = "WrapsWhenFull";
        collectDiagnosticDataSupported = true;
    }
    else
    {
        BMCWEB_LOG_ERROR << "getDumpServiceInfo() invalid dump type: "
                         << dumpType;
        messages::internalError(asyncResp->res);
        return;
    }

    asyncResp->res.jsonValue["@odata.id"] = dumpPath;
    asyncResp->res.jsonValue["@odata.type"] = "#LogService.v1_2_0.LogService";
    asyncResp->res.jsonValue["Name"] = "Dump LogService";
    asyncResp->res.jsonValue["Description"] = dumpType + " Dump LogService";
    asyncResp->res.jsonValue["Id"] = std::filesystem::path(dumpPath).filename();
    asyncResp->res.jsonValue["OverWritePolicy"] = std::move(overWritePolicy);

    std::pair<std::string, std::string> redfishDateTimeOffset =
        redfish::time_utils::getDateTimeOffsetNow();
    asyncResp->res.jsonValue["DateTime"] = redfishDateTimeOffset.first;
    asyncResp->res.jsonValue["DateTimeLocalOffset"] =
        redfishDateTimeOffset.second;

    asyncResp->res.jsonValue["Entries"]["@odata.id"] = dumpPath + "/Entries";

    if (collectDiagnosticDataSupported)
    {
        asyncResp->res.jsonValue["Actions"]["#LogService.CollectDiagnosticData"]
                                ["target"] =
            dumpPath + "/Actions/LogService.CollectDiagnosticData";
    }

    constexpr std::array<std::string_view, 1> interfaces = {deleteAllInterface};
    dbus::utility::getSubTreePaths(
        "/xyz/openbmc_project/dump", 0, interfaces,
        [asyncResp, dumpType, dumpPath](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreePathsResponse& subTreePaths) {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "getDumpServiceInfo respHandler got error "
                             << ec;
            // Assume that getting an error simply means there are no dump
            // LogServices. Return without adding any error response.
            return;
        }

        const std::string dbusDumpPath =
            "/xyz/openbmc_project/dump/" +
            boost::algorithm::to_lower_copy(dumpType);

        for (const std::string& path : subTreePaths)
        {
            if (path == dbusDumpPath)
            {
                asyncResp->res
                    .jsonValue["Actions"]["#LogService.ClearLog"]["target"] =
                    dumpPath + "/Actions/LogService.ClearLog";
                break;
            }
        }
        });
}

inline void handleLogServicesDumpServiceGet(
    crow::App& app, const std::string& dumpType, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    getDumpServiceInfo(asyncResp, dumpType);
}

inline void handleLogServicesDumpServiceComputerSystemGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    if (chassisId != "system")
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem", chassisId);
        return;
    }
    getDumpServiceInfo(asyncResp, "System");
}

inline void handleLogServicesDumpEntriesCollectionGet(
    crow::App& app, const std::string& dumpType, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    getDumpEntryCollection(asyncResp, dumpType);
}

inline void handleLogServicesDumpEntriesCollectionComputerSystemGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    if (chassisId != "system")
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem", chassisId);
        return;
    }
    getDumpEntryCollection(asyncResp, "System");
}

inline void handleLogServicesDumpEntryGet(
    crow::App& app, const std::string& dumpType, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& dumpId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    getDumpEntryById(asyncResp, dumpId, dumpType);
}
inline void handleLogServicesDumpEntryComputerSystemGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& dumpId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    if (chassisId != "system")
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem", chassisId);
        return;
    }
    getDumpEntryById(asyncResp, dumpId, "System");
}

inline void handleLogServicesDumpEntryDelete(
    crow::App& app, const std::string& dumpType, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& dumpId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    deleteDumpEntry(asyncResp, dumpId, dumpType);
}

inline void handleLogServicesDumpEntryComputerSystemDelete(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& dumpId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    if (chassisId != "system")
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem", chassisId);
        return;
    }
    deleteDumpEntry(asyncResp, dumpId, "System");
}

inline void handleLogServicesDumpCollectDiagnosticDataPost(
    crow::App& app, const std::string& dumpType, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    createDump(asyncResp, req, dumpType);
}

inline void handleLogServicesDumpCollectDiagnosticDataComputerSystemPost(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    if (chassisId != "system")
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem", chassisId);
        return;
    }
    createDump(asyncResp, req, "System");
}

inline void handleLogServicesDumpClearLogPost(
    crow::App& app, const std::string& dumpType, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    clearDump(asyncResp, dumpType);
}

inline void handleLogServicesDumpClearLogComputerSystemPost(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    if (chassisId != "system")
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem", chassisId);
        return;
    }
    clearDump(asyncResp, "System");
}

inline void requestRoutesBMCDumpService(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Managers/bmc/LogServices/Dump/")
        .privileges(redfish::privileges::getLogService)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleLogServicesDumpServiceGet, std::ref(app), "BMC"));
}

inline void requestRoutesBMCDumpEntryCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Managers/bmc/LogServices/Dump/Entries/")
        .privileges(redfish::privileges::getLogEntryCollection)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleLogServicesDumpEntriesCollectionGet, std::ref(app), "BMC"));
}

inline void requestRoutesBMCDumpEntry(App& app)
{
    BMCWEB_ROUTE(app,
                 "/redfish/v1/Managers/bmc/LogServices/Dump/Entries/<str>/")
        .privileges(redfish::privileges::getLogEntry)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleLogServicesDumpEntryGet, std::ref(app), "BMC"));

    BMCWEB_ROUTE(app,
                 "/redfish/v1/Managers/bmc/LogServices/Dump/Entries/<str>/")
        .privileges(redfish::privileges::deleteLogEntry)
        .methods(boost::beast::http::verb::delete_)(std::bind_front(
            handleLogServicesDumpEntryDelete, std::ref(app), "BMC"));
}

inline void requestRoutesBMCDumpCreate(App& app)
{
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Managers/bmc/LogServices/Dump/Actions/LogService.CollectDiagnosticData/")
        .privileges(redfish::privileges::postLogService)
        .methods(boost::beast::http::verb::post)(
            std::bind_front(handleLogServicesDumpCollectDiagnosticDataPost,
                            std::ref(app), "BMC"));
}

inline void requestRoutesBMCDumpClear(App& app)
{
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Managers/bmc/LogServices/Dump/Actions/LogService.ClearLog/")
        .privileges(redfish::privileges::postLogService)
        .methods(boost::beast::http::verb::post)(std::bind_front(
            handleLogServicesDumpClearLogPost, std::ref(app), "BMC"));
}

inline void requestRoutesFaultLogDumpService(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Managers/bmc/LogServices/FaultLog/")
        .privileges(redfish::privileges::getLogService)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleLogServicesDumpServiceGet, std::ref(app), "FaultLog"));
}

inline void requestRoutesFaultLogDumpEntryCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Managers/bmc/LogServices/FaultLog/Entries/")
        .privileges(redfish::privileges::getLogEntryCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleLogServicesDumpEntriesCollectionGet,
                            std::ref(app), "FaultLog"));
}

inline void requestRoutesFaultLogDumpEntry(App& app)
{
    BMCWEB_ROUTE(app,
                 "/redfish/v1/Managers/bmc/LogServices/FaultLog/Entries/<str>/")
        .privileges(redfish::privileges::getLogEntry)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleLogServicesDumpEntryGet, std::ref(app), "FaultLog"));

    BMCWEB_ROUTE(app,
                 "/redfish/v1/Managers/bmc/LogServices/FaultLog/Entries/<str>/")
        .privileges(redfish::privileges::deleteLogEntry)
        .methods(boost::beast::http::verb::delete_)(std::bind_front(
            handleLogServicesDumpEntryDelete, std::ref(app), "FaultLog"));
}

inline void requestRoutesFaultLogDumpClear(App& app)
{
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Managers/bmc/LogServices/FaultLog/Actions/LogService.ClearLog/")
        .privileges(redfish::privileges::postLogService)
        .methods(boost::beast::http::verb::post)(std::bind_front(
            handleLogServicesDumpClearLogPost, std::ref(app), "FaultLog"));
}

inline void requestRoutesSystemDumpService(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/LogServices/Dump/")
        .privileges(redfish::privileges::getLogService)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleLogServicesDumpServiceComputerSystemGet, std::ref(app)));
}

inline void requestRoutesSystemDumpEntryCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/LogServices/Dump/Entries/")
        .privileges(redfish::privileges::getLogEntryCollection)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleLogServicesDumpEntriesCollectionComputerSystemGet,
            std::ref(app)));
}

inline void requestRoutesSystemDumpEntry(App& app)
{
    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/<str>/LogServices/Dump/Entries/<str>/")
        .privileges(redfish::privileges::getLogEntry)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleLogServicesDumpEntryComputerSystemGet, std::ref(app)));

    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/<str>/LogServices/Dump/Entries/<str>/")
        .privileges(redfish::privileges::deleteLogEntry)
        .methods(boost::beast::http::verb::delete_)(std::bind_front(
            handleLogServicesDumpEntryComputerSystemDelete, std::ref(app)));
}

inline void requestRoutesSystemDumpCreate(App& app)
{
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Systems/<str>/LogServices/Dump/Actions/LogService.CollectDiagnosticData/")
        .privileges(redfish::privileges::postLogService)
        .methods(boost::beast::http::verb::post)(std::bind_front(
            handleLogServicesDumpCollectDiagnosticDataComputerSystemPost,
            std::ref(app)));
}

inline void requestRoutesSystemDumpClear(App& app)
{
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Systems/<str>/LogServices/Dump/Actions/LogService.ClearLog/")
        .privileges(redfish::privileges::postLogService)
        .methods(boost::beast::http::verb::post)(std::bind_front(
            handleLogServicesDumpClearLogComputerSystemPost, std::ref(app)));
}

inline void requestRoutesCrashdumpService(App& app)
{
    // Note: Deviated from redfish privilege registry for GET & HEAD
    // method for security reasons.
    /**
     * Functions triggers appropriate requests on DBus
     */
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/LogServices/Crashdump/")
        // This is incorrect, should be:
        //.privileges(redfish::privileges::getLogService)
        .privileges({{"ConfigureManager"}})
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemName) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        if (systemName != "system")
        {
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }

        // Copy over the static data to include the entries added by
        // SubRoute
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Systems/system/LogServices/Crashdump";
        asyncResp->res.jsonValue["@odata.type"] =
            "#LogService.v1_2_0.LogService";
        asyncResp->res.jsonValue["Name"] = "Open BMC Oem Crashdump Service";
        asyncResp->res.jsonValue["Description"] = "Oem Crashdump Service";
        asyncResp->res.jsonValue["Id"] = "Oem Crashdump";
        asyncResp->res.jsonValue["OverWritePolicy"] = "WrapsWhenFull";
        asyncResp->res.jsonValue["MaxNumberOfRecords"] = 3;

        std::pair<std::string, std::string> redfishDateTimeOffset =
            redfish::time_utils::getDateTimeOffsetNow();
        asyncResp->res.jsonValue["DateTime"] = redfishDateTimeOffset.first;
        asyncResp->res.jsonValue["DateTimeLocalOffset"] =
            redfishDateTimeOffset.second;

        asyncResp->res.jsonValue["Entries"]["@odata.id"] =
            crow::utility::urlFromPieces("redfish", "v1", "Systems", "system",
                                         "LogServices", "Crashdump", "Entries");
        asyncResp->res.jsonValue["Actions"]["#LogService.ClearLog"]["target"] =
            "/redfish/v1/Systems/system/LogServices/Crashdump/Actions/LogService.ClearLog";
        asyncResp->res.jsonValue["Actions"]["#LogService.CollectDiagnosticData"]
                                ["target"] =
            "/redfish/v1/Systems/system/LogServices/Crashdump/Actions/LogService.CollectDiagnosticData";
        });
}

void inline requestRoutesCrashdumpClear(App& app)
{
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Systems/<str>/LogServices/Crashdump/Actions/LogService.ClearLog/")
        // This is incorrect, should be:
        //.privileges(redfish::privileges::postLogService)
        .privileges({{"ConfigureComponents"}})
        .methods(boost::beast::http::verb::post)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemName) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        if (systemName != "system")
        {
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }
        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code& ec,
                        const std::string&) {
            if (ec)
            {
                messages::internalError(asyncResp->res);
                return;
            }
            messages::success(asyncResp->res);
            },
            crashdumpObject, crashdumpPath, deleteAllInterface, "DeleteAll");
        });
}

static void
    logCrashdumpEntry(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const std::string& logID, nlohmann::json& logEntryJson)
{
    auto getStoredLogCallback =
        [asyncResp, logID,
         &logEntryJson](const boost::system::error_code& ec,
                        const dbus::utility::DBusPropertiesMap& params) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG << "failed to get log ec: " << ec.message();
            if (ec.value() ==
                boost::system::linux_error::bad_request_descriptor)
            {
                messages::resourceNotFound(asyncResp->res, "LogEntry", logID);
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

        std::string crashdumpURI =
            "/redfish/v1/Systems/system/LogServices/Crashdump/Entries/" +
            logID + "/" + filename;
        nlohmann::json::object_t logEntry;
        logEntry["@odata.type"] = "#LogEntry.v1_9_0.LogEntry";
        logEntry["@odata.id"] = crow::utility::urlFromPieces(
            "redfish", "v1", "Systems", "system", "LogServices", "Crashdump",
            "Entries", logID);
        logEntry["Name"] = "CPU Crashdump";
        logEntry["Id"] = logID;
        logEntry["EntryType"] = "Oem";
        logEntry["AdditionalDataURI"] = std::move(crashdumpURI);
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
    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, crashdumpObject,
        crashdumpPath + std::string("/") + logID, crashdumpInterface,
        std::move(getStoredLogCallback));
}

inline void requestRoutesCrashdumpEntryCollection(App& app)
{
    // Note: Deviated from redfish privilege registry for GET & HEAD
    // method for security reasons.
    /**
     * Functions triggers appropriate requests on DBus
     */
    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/<str>/LogServices/Crashdump/Entries/")
        // This is incorrect, should be.
        //.privileges(redfish::privileges::postLogEntryCollection)
        .privileges({{"ConfigureComponents"}})
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemName) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        if (systemName != "system")
        {
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }

        constexpr std::array<std::string_view, 1> interfaces = {
            crashdumpInterface};
        dbus::utility::getSubTreePaths(
            "/", 0, interfaces,
            [asyncResp](const boost::system::error_code& ec,
                        const std::vector<std::string>& resp) {
            if (ec)
            {
                if (ec.value() !=
                    boost::system::errc::no_such_file_or_directory)
                {
                    BMCWEB_LOG_DEBUG << "failed to get entries ec: "
                                     << ec.message();
                    messages::internalError(asyncResp->res);
                    return;
                }
            }
            asyncResp->res.jsonValue["@odata.type"] =
                "#LogEntryCollection.LogEntryCollection";
            asyncResp->res.jsonValue["@odata.id"] =
                "/redfish/v1/Systems/system/LogServices/Crashdump/Entries";
            asyncResp->res.jsonValue["Name"] = "Open BMC Crashdump Entries";
            asyncResp->res.jsonValue["Description"] =
                "Collection of Crashdump Entries";
            asyncResp->res.jsonValue["Members"] = nlohmann::json::array();
            asyncResp->res.jsonValue["Members@odata.count"] = 0;

            for (const std::string& path : resp)
            {
                const sdbusplus::message::object_path objPath(path);
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
    // Note: Deviated from redfish privilege registry for GET & HEAD
    // method for security reasons.

    BMCWEB_ROUTE(
        app, "/redfish/v1/Systems/<str>/LogServices/Crashdump/Entries/<str>/")
        // this is incorrect, should be
        // .privileges(redfish::privileges::getLogEntry)
        .privileges({{"ConfigureComponents"}})
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemName, const std::string& param) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        if (systemName != "system")
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
    // Note: Deviated from redfish privilege registry for GET & HEAD
    // method for security reasons.
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Systems/<str>/LogServices/Crashdump/Entries/<str>/<str>/")
        .privileges(redfish::privileges::getLogEntry)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& systemName, const std::string& logID,
               const std::string& fileName) {
        // Do not call getRedfishRoute here since the crashdump file is not a
        // Redfish resource.

        if (systemName != "system")
        {
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }

        auto getStoredLogCallback =
            [asyncResp, logID, fileName, url(boost::urls::url(req.urlView))](
                const boost::system::error_code& ec,
                const std::vector<
                    std::pair<std::string, dbus::utility::DbusVariantType>>&
                    resp) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "failed to get log ec: " << ec.message();
                messages::internalError(asyncResp->res);
                return;
            }

            std::string dbusFilename{};
            std::string dbusTimestamp{};
            std::string dbusFilepath{};

            parseCrashdumpParameters(resp, dbusFilename, dbusTimestamp,
                                     dbusFilepath);

            if (dbusFilename.empty() || dbusTimestamp.empty() ||
                dbusFilepath.empty())
            {
                messages::resourceNotFound(asyncResp->res, "LogEntry", logID);
                return;
            }

            // Verify the file name parameter is correct
            if (fileName != dbusFilename)
            {
                messages::resourceNotFound(asyncResp->res, "LogEntry", logID);
                return;
            }

            if (!std::filesystem::exists(dbusFilepath))
            {
                messages::resourceNotFound(asyncResp->res, "LogEntry", logID);
                return;
            }
            std::ifstream ifs(dbusFilepath, std::ios::in | std::ios::binary);
            asyncResp->res.body() =
                std::string(std::istreambuf_iterator<char>{ifs}, {});

            // Configure this to be a file download when accessed
            // from a browser
            asyncResp->res.addHeader(
                boost::beast::http::field::content_disposition, "attachment");
        };
        sdbusplus::asio::getAllProperties(
            *crow::connections::systemBus, crashdumpObject,
            crashdumpPath + std::string("/") + logID, crashdumpInterface,
            std::move(getStoredLogCallback));
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
    // Note: Deviated from redfish privilege registry for GET & HEAD
    // method for security reasons.
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Systems/<str>/LogServices/Crashdump/Actions/LogService.CollectDiagnosticData/")
        // The below is incorrect;  Should be ConfigureManager
        //.privileges(redfish::privileges::postLogService)
        .privileges({{"ConfigureComponents"}})
        .methods(boost::beast::http::verb::post)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemName) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }

        if (systemName != "system")
        {
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }

        std::string diagnosticDataType;
        std::string oemDiagnosticDataType;
        if (!redfish::json_util::readJsonAction(
                req, asyncResp->res, "DiagnosticDataType", diagnosticDataType,
                "OEMDiagnosticDataType", oemDiagnosticDataType))
        {
            return;
        }

        if (diagnosticDataType != "OEM")
        {
            BMCWEB_LOG_ERROR
                << "Only OEM DiagnosticDataType supported for Crashdump";
            messages::actionParameterValueFormatError(
                asyncResp->res, diagnosticDataType, "DiagnosticDataType",
                "CollectDiagnosticData");
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
            taskMatchStr = "type='signal',"
                           "interface='org.freedesktop.DBus.Properties',"
                           "member='PropertiesChanged',"
                           "arg0namespace='com.intel.crashdump'";
        }
        else if (oemDiagType == OEMDiagnosticType::telemetry)
        {
            iface = crashdumpTelemetryInterface;
            method = "GenerateTelemetryLog";
            taskMatchStr = "type='signal',"
                           "interface='org.freedesktop.DBus.Properties',"
                           "member='PropertiesChanged',"
                           "arg0namespace='com.intel.crashdump'";
        }
        else
        {
            BMCWEB_LOG_ERROR << "Unsupported OEMDiagnosticDataType: "
                             << oemDiagnosticDataType;
            messages::actionParameterValueFormatError(
                asyncResp->res, oemDiagnosticDataType, "OEMDiagnosticDataType",
                "CollectDiagnosticData");
            return;
        }

        auto collectCrashdumpCallback =
            [asyncResp, payload(task::Payload(req)),
             taskMatchStr](const boost::system::error_code& ec,
                           const std::string&) mutable {
            if (ec)
            {
                if (ec.value() == boost::system::errc::operation_not_supported)
                {
                    messages::resourceInStandby(asyncResp->res);
                }
                else if (ec.value() ==
                         boost::system::errc::device_or_resource_busy)
                {
                    messages::serviceTemporarilyUnavailable(asyncResp->res,
                                                            "60");
                }
                else
                {
                    messages::internalError(asyncResp->res);
                }
                return;
            }
            std::shared_ptr<task::TaskData> task = task::TaskData::createTask(
                [](const boost::system::error_code& err, sdbusplus::message_t&,
                   const std::shared_ptr<task::TaskData>& taskData) {
                if (!err)
                {
                    taskData->messages.emplace_back(messages::taskCompletedOK(
                        std::to_string(taskData->index)));
                    taskData->state = "Completed";
                }
                return task::completed;
                },
                taskMatchStr);

            task->startTimer(std::chrono::minutes(5));
            task->populateResp(asyncResp->res);
            task->payload.emplace(std::move(payload));
        };

        crow::connections::systemBus->async_method_call(
            std::move(collectCrashdumpCallback), crashdumpObject, crashdumpPath,
            iface, method);
        });
}

/**
 * DBusLogServiceActionsClear class supports POST method for ClearLog action.
 */
inline void requestRoutesDBusLogServiceActionsClear(App& app)
{
    /**
     * Function handles POST method request.
     * The Clear Log actions does not require any parameter.The action deletes
     * all entries found in the Entries collection for this Log Service.
     */

    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Systems/<str>/LogServices/EventLog/Actions/LogService.ClearLog/")
        .privileges(redfish::privileges::postLogService)
        .methods(boost::beast::http::verb::post)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemName) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        if (systemName != "system")
        {
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }
        BMCWEB_LOG_DEBUG << "Do delete all entries.";

        // Process response from Logging service.
        auto respHandler = [asyncResp](const boost::system::error_code& ec) {
            BMCWEB_LOG_DEBUG << "doClearLog resp_handler callback: Done";
            if (ec)
            {
                // TODO Handle for specific error code
                BMCWEB_LOG_ERROR << "doClearLog resp_handler got error " << ec;
                asyncResp->res.result(
                    boost::beast::http::status::internal_server_error);
                return;
            }

            asyncResp->res.result(boost::beast::http::status::no_content);
        };

        // Make call to Logging service to request Clear Log
        crow::connections::systemBus->async_method_call(
            respHandler, "xyz.openbmc_project.Logging",
            "/xyz/openbmc_project/logging",
            "xyz.openbmc_project.Collection.DeleteAll", "DeleteAll");
        });
}

/****************************************************
 * Redfish PostCode interfaces
 * using DBUS interface: getPostCodesTS
 ******************************************************/
inline void requestRoutesPostCodesLogService(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/LogServices/PostCodes/")
        .privileges(redfish::privileges::getLogService)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemName) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        if (systemName != "system")
        {
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Systems/system/LogServices/PostCodes";
        asyncResp->res.jsonValue["@odata.type"] =
            "#LogService.v1_1_0.LogService";
        asyncResp->res.jsonValue["Name"] = "POST Code Log Service";
        asyncResp->res.jsonValue["Description"] = "POST Code Log Service";
        asyncResp->res.jsonValue["Id"] = "PostCodes";
        asyncResp->res.jsonValue["OverWritePolicy"] = "WrapsWhenFull";
        asyncResp->res.jsonValue["Entries"]["@odata.id"] =
            "/redfish/v1/Systems/system/LogServices/PostCodes/Entries";

        std::pair<std::string, std::string> redfishDateTimeOffset =
            redfish::time_utils::getDateTimeOffsetNow();
        asyncResp->res.jsonValue["DateTime"] = redfishDateTimeOffset.first;
        asyncResp->res.jsonValue["DateTimeLocalOffset"] =
            redfishDateTimeOffset.second;

        asyncResp->res.jsonValue["Actions"]["#LogService.ClearLog"] = {
            {"target",
             "/redfish/v1/Systems/system/LogServices/PostCodes/Actions/LogService.ClearLog"}};
        });
}

inline void requestRoutesPostCodesClear(App& app)
{
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Systems/<str>/LogServices/PostCodes/Actions/LogService.ClearLog/")
        // The following privilege is incorrect;  It should be ConfigureManager
        //.privileges(redfish::privileges::postLogService)
        .privileges({{"ConfigureComponents"}})
        .methods(boost::beast::http::verb::post)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemName) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        if (systemName != "system")
        {
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }
        BMCWEB_LOG_DEBUG << "Do delete all postcodes entries.";

        // Make call to post-code service to request clear all
        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code& ec) {
            if (ec)
            {
                // TODO Handle for specific error code
                BMCWEB_LOG_ERROR << "doClearPostCodes resp_handler got error "
                                 << ec;
                asyncResp->res.result(
                    boost::beast::http::status::internal_server_error);
                messages::internalError(asyncResp->res);
                return;
            }
            },
            "xyz.openbmc_project.State.Boot.PostCode0",
            "/xyz/openbmc_project/State/Boot/PostCode0",
            "xyz.openbmc_project.Collection.DeleteAll", "DeleteAll");
        });
}

/**
 * @brief Parse post code ID and get the current value and index value
 *        eg: postCodeID=B1-2, currentValue=1, index=2
 *
 * @param[in]  postCodeID     Post Code ID
 * @param[out] currentValue   Current value
 * @param[out] index          Index value
 *
 * @return bool true if the parsing is successful, false the parsing fails
 */
inline static bool parsePostCode(const std::string& postCodeID,
                                 uint64_t& currentValue, uint16_t& index)
{
    std::vector<std::string> split;
    bmcweb::split(split, postCodeID, '-');
    if (split.size() != 2 || split[0].length() < 2 || split[0].front() != 'B')
    {
        return false;
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    const char* start = split[0].data() + 1;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    const char* end = split[0].data() + split[0].size();
    auto [ptrIndex, ecIndex] = std::from_chars(start, end, index);

    if (ptrIndex != end || ecIndex != std::errc())
    {
        return false;
    }

    start = split[1].data();

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    end = split[1].data() + split[1].size();
    auto [ptrValue, ecValue] = std::from_chars(start, end, currentValue);

    return ptrValue == end && ecValue == std::errc();
}

static bool fillPostCodeEntry(
    const std::shared_ptr<bmcweb::AsyncResp>& aResp,
    const boost::container::flat_map<
        uint64_t, std::tuple<uint64_t, std::vector<uint8_t>>>& postcode,
    const uint16_t bootIndex, const uint64_t codeIndex = 0,
    const uint64_t skip = 0, const uint64_t top = 0)
{
    // Get the Message from the MessageRegistry
    const registries::Message* message =
        registries::getMessage("OpenBMC.0.2.BIOSPOSTCode");

    uint64_t currentCodeIndex = 0;
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
        entryTimeStr = redfish::time_utils::getDateTimeUintUs(usecSinceEpoch);

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
            severity = message->messageSeverity;
        }

        // Format entry
        nlohmann::json::object_t bmcLogEntry;
        bmcLogEntry["@odata.type"] = "#LogEntry.v1_9_0.LogEntry";
        bmcLogEntry["@odata.id"] = crow::utility::urlFromPieces(
            "redfish", "v1", "Systems", "system", "LogServices", "PostCodes",
            "Entries", postcodeEntryID);
        bmcLogEntry["Name"] = "POST Code Log Entry";
        bmcLogEntry["Id"] = postcodeEntryID;
        bmcLogEntry["Message"] = std::move(msg);
        bmcLogEntry["MessageId"] = "OpenBMC.0.2.BIOSPOSTCode";
        bmcLogEntry["MessageArgs"] = std::move(messageArgs);
        bmcLogEntry["EntryType"] = "Event";
        bmcLogEntry["Severity"] = std::move(severity);
        bmcLogEntry["Created"] = entryTimeStr;
        if (!std::get<std::vector<uint8_t>>(code.second).empty())
        {
            bmcLogEntry["AdditionalDataURI"] =
                "/redfish/v1/Systems/system/LogServices/PostCodes/Entries/" +
                postcodeEntryID + "/attachment";
        }

        // codeIndex is only specified when querying single entry, return only
        // that entry in this case
        if (codeIndex != 0)
        {
            aResp->res.jsonValue.update(bmcLogEntry);
            return true;
        }

        nlohmann::json& logEntryArray = aResp->res.jsonValue["Members"];
        logEntryArray.push_back(std::move(bmcLogEntry));
    }

    // Return value is always false when querying multiple entries
    return false;
}

static void getPostCodeForEntry(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                                const std::string& entryId)
{
    uint16_t bootIndex = 0;
    uint64_t codeIndex = 0;
    if (!parsePostCode(entryId, codeIndex, bootIndex))
    {
        // Requested ID was not found
        messages::resourceNotFound(aResp->res, "LogEntry", entryId);
        return;
    }

    if (bootIndex == 0 || codeIndex == 0)
    {
        // 0 is an invalid index
        messages::resourceNotFound(aResp->res, "LogEntry", entryId);
        return;
    }

    crow::connections::systemBus->async_method_call(
        [aResp, entryId, bootIndex,
         codeIndex](const boost::system::error_code& ec,
                    const boost::container::flat_map<
                        uint64_t, std::tuple<uint64_t, std::vector<uint8_t>>>&
                        postcode) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG << "DBUS POST CODE PostCode response error";
            messages::internalError(aResp->res);
            return;
        }

        if (postcode.empty())
        {
            messages::resourceNotFound(aResp->res, "LogEntry", entryId);
            return;
        }

        if (!fillPostCodeEntry(aResp, postcode, bootIndex, codeIndex))
        {
            messages::resourceNotFound(aResp->res, "LogEntry", entryId);
            return;
        }
        },
        "xyz.openbmc_project.State.Boot.PostCode0",
        "/xyz/openbmc_project/State/Boot/PostCode0",
        "xyz.openbmc_project.State.Boot.PostCode", "GetPostCodesWithTimeStamp",
        bootIndex);
}

static void getPostCodeForBoot(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                               const uint16_t bootIndex,
                               const uint16_t bootCount,
                               const uint64_t entryCount, size_t skip,
                               size_t top)
{
    crow::connections::systemBus->async_method_call(
        [aResp, bootIndex, bootCount, entryCount, skip,
         top](const boost::system::error_code& ec,
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
            if (skip < endCount && (top + skip) > entryCount)
            {
                uint64_t thisBootSkip =
                    std::max(static_cast<uint64_t>(skip), entryCount) -
                    entryCount;
                uint64_t thisBootTop =
                    std::min(static_cast<uint64_t>(top + skip), endCount) -
                    entryCount;

                fillPostCodeEntry(aResp, postcode, bootIndex, 0, thisBootSkip,
                                  thisBootTop);
            }
            aResp->res.jsonValue["Members@odata.count"] = endCount;
        }

        // continue to previous bootIndex
        if (bootIndex < bootCount)
        {
            getPostCodeForBoot(aResp, static_cast<uint16_t>(bootIndex + 1),
                               bootCount, endCount, skip, top);
        }
        else if (skip + top < endCount)
        {
            aResp->res.jsonValue["Members@odata.nextLink"] =
                "/redfish/v1/Systems/system/LogServices/PostCodes/Entries?$skip=" +
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
                         size_t skip, size_t top)
{
    uint64_t entryCount = 0;
    sdbusplus::asio::getProperty<uint16_t>(
        *crow::connections::systemBus,
        "xyz.openbmc_project.State.Boot.PostCode0",
        "/xyz/openbmc_project/State/Boot/PostCode0",
        "xyz.openbmc_project.State.Boot.PostCode", "CurrentBootCycleCount",
        [aResp, entryCount, skip, top](const boost::system::error_code& ec,
                                       const uint16_t bootCount) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
            messages::internalError(aResp->res);
            return;
        }
        getPostCodeForBoot(aResp, 1, bootCount, entryCount, skip, top);
        });
}

inline void requestRoutesPostCodesEntryCollection(App& app)
{
    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/<str>/LogServices/PostCodes/Entries/")
        .privileges(redfish::privileges::getLogEntryCollection)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemName) {
        query_param::QueryCapabilities capabilities = {
            .canDelegateTop = true,
            .canDelegateSkip = true,
        };
        query_param::Query delegatedQuery;
        if (!redfish::setUpRedfishRouteWithDelegation(
                app, req, asyncResp, delegatedQuery, capabilities))
        {
            return;
        }

        if (systemName != "system")
        {
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }
        asyncResp->res.jsonValue["@odata.type"] =
            "#LogEntryCollection.LogEntryCollection";
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Systems/system/LogServices/PostCodes/Entries";
        asyncResp->res.jsonValue["Name"] = "BIOS POST Code Log Entries";
        asyncResp->res.jsonValue["Description"] =
            "Collection of POST Code Log Entries";
        asyncResp->res.jsonValue["Members"] = nlohmann::json::array();
        asyncResp->res.jsonValue["Members@odata.count"] = 0;
        size_t skip = delegatedQuery.skip.value_or(0);
        size_t top = delegatedQuery.top.value_or(query_param::Query::maxTop);
        getCurrentBootNumber(asyncResp, skip, top);
        });
}

inline void requestRoutesPostCodesEntryAdditionalData(App& app)
{
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Systems/<str>/LogServices/PostCodes/Entries/<str>/attachment/")
        .privileges(redfish::privileges::getLogEntry)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemName,
                   const std::string& postCodeID) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        if (http_helpers::isContentTypeAllowed(
                req.getHeaderValue("Accept"),
                http_helpers::ContentType::OctetStream, true))
        {
            asyncResp->res.result(boost::beast::http::status::bad_request);
            return;
        }
        if (systemName != "system")
        {
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }

        uint64_t currentValue = 0;
        uint16_t index = 0;
        if (!parsePostCode(postCodeID, currentValue, index))
        {
            messages::resourceNotFound(asyncResp->res, "LogEntry", postCodeID);
            return;
        }

        crow::connections::systemBus->async_method_call(
            [asyncResp, postCodeID, currentValue](
                const boost::system::error_code& ec,
                const std::vector<std::tuple<uint64_t, std::vector<uint8_t>>>&
                    postcodes) {
            if (ec.value() == EBADR)
            {
                messages::resourceNotFound(asyncResp->res, "LogEntry",
                                           postCodeID);
                return;
            }
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                messages::internalError(asyncResp->res);
                return;
            }

            size_t value = static_cast<size_t>(currentValue) - 1;
            if (value == std::string::npos || postcodes.size() < currentValue)
            {
                BMCWEB_LOG_ERROR << "Wrong currentValue value";
                messages::resourceNotFound(asyncResp->res, "LogEntry",
                                           postCodeID);
                return;
            }

            const auto& [tID, c] = postcodes[value];
            if (c.empty())
            {
                BMCWEB_LOG_INFO << "No found post code data";
                messages::resourceNotFound(asyncResp->res, "LogEntry",
                                           postCodeID);
                return;
            }
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            const char* d = reinterpret_cast<const char*>(c.data());
            std::string_view strData(d, c.size());

            asyncResp->res.addHeader(boost::beast::http::field::content_type,
                                     "application/octet-stream");
            asyncResp->res.addHeader(
                boost::beast::http::field::content_transfer_encoding, "Base64");
            asyncResp->res.body() = crow::utility::base64encode(strData);
            },
            "xyz.openbmc_project.State.Boot.PostCode0",
            "/xyz/openbmc_project/State/Boot/PostCode0",
            "xyz.openbmc_project.State.Boot.PostCode", "GetPostCodes", index);
        });
}

inline void requestRoutesPostCodesEntry(App& app)
{
    BMCWEB_ROUTE(
        app, "/redfish/v1/Systems/<str>/LogServices/PostCodes/Entries/<str>/")
        .privileges(redfish::privileges::getLogEntry)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemName, const std::string& targetID) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        if (systemName != "system")
        {
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }

        getPostCodeForEntry(asyncResp, targetID);
        });
}

} // namespace redfish
