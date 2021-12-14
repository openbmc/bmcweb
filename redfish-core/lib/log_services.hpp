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

#include "gzfile.hpp"
#include "http_utility.hpp"
#include "human_sort.hpp"
#include "registries.hpp"
#include "registries/base_message_registry.hpp"
#include "registries/openbmc_message_registry.hpp"
#include "task.hpp"

#include <systemd/sd-journal.h>
#include <unistd.h>

#include <app.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/beast/http.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/system/linux_error.hpp>
#include <error_messages.hpp>
#include <registries/privilege_registry.hpp>

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

namespace message_registries
{
static const Message*
    getMessageFromRegistry(const std::string& messageKey,
                           const std::span<const MessageEntry> registry)
{
    std::span<const MessageEntry>::iterator messageIt = std::find_if(
        registry.begin(), registry.end(),
        [&messageKey](const MessageEntry& messageEntry) {
            return !std::strcmp(messageEntry.first, messageKey.c_str());
        });
    if (messageIt != registry.end())
    {
        return &messageIt->second;
    }

    return nullptr;
}

static const Message* getMessage(const std::string_view& messageID)
{
    // Redfish MessageIds are in the form
    // RegistryName.MajorVersion.MinorVersion.MessageKey, so parse it to find
    // the right Message
    std::vector<std::string> fields;
    fields.reserve(4);
    boost::split(fields, messageID, boost::is_any_of("."));
    std::string& registryName = fields[0];
    std::string& messageKey = fields[3];

    // Find the right registry and check it for the MessageKey
    if (std::string(base::header.registryPrefix) == registryName)
    {
        return getMessageFromRegistry(
            messageKey, std::span<const MessageEntry>(base::registry));
    }
    if (std::string(openbmc::header.registryPrefix) == registryName)
    {
        return getMessageFromRegistry(
            messageKey, std::span<const MessageEntry>(openbmc::registry));
    }
    return nullptr;
}
} // namespace message_registries

namespace fs = std::filesystem;

using GetManagedPropertyType = boost::container::flat_map<
    std::string, std::variant<std::string, bool, uint8_t, int16_t, uint16_t,
                              int32_t, uint32_t, int64_t, uint64_t, double>>;

using GetManagedObjectsType = boost::container::flat_map<
    sdbusplus::message::object_path,
    boost::container::flat_map<std::string, GetManagedPropertyType>>;

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

inline static int getJournalMetadata(sd_journal* journal,
                                     const std::string_view& field,
                                     std::string_view& contents)
{
    const char* data = nullptr;
    size_t length = 0;
    int ret = 0;
    // Get the metadata from the requested field of the journal entry
    ret = sd_journal_get_data(journal, field.data(),
                              reinterpret_cast<const void**>(&data), &length);
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
                                     const std::string_view& field,
                                     const int& base, long int& contents)
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
    entryTimestamp = crow::utility::getDateTimeUint(timestamp / 1000 / 1000);
    return true;
}

static bool getSkipParam(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const crow::Request& req, uint64_t& skip)
{
    boost::urls::query_params_view::iterator it = req.urlParams.find("$skip");
    if (it != req.urlParams.end())
    {
        std::string skipParam = it->value();
        char* ptr = nullptr;
        skip = std::strtoul(skipParam.c_str(), &ptr, 10);
        if (skipParam.empty() || *ptr != '\0')
        {

            messages::queryParameterValueTypeError(
                asyncResp->res, std::string(skipParam), "$skip");
            return false;
        }
    }
    return true;
}

static constexpr const uint64_t maxEntriesPerPage = 1000;
static bool getTopParam(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const crow::Request& req, uint64_t& top)
{
    boost::urls::query_params_view::iterator it = req.urlParams.find("$top");
    if (it != req.urlParams.end())
    {
        std::string topParam = it->value();
        char* ptr = nullptr;
        top = std::strtoul(topParam.c_str(), &ptr, 10);
        if (topParam.empty() || *ptr != '\0')
        {
            messages::queryParameterValueTypeError(
                asyncResp->res, std::string(topParam), "$top");
            return false;
        }
        if (top < 1U || top > maxEntriesPerPage)
        {

            messages::queryParameterOutOfRange(
                asyncResp->res, std::to_string(top), "$top",
                "1-" + std::to_string(maxEntriesPerPage));
            return false;
        }
    }
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
    if (underscorePos != tsStr.npos)
    {
        // Timestamp has an index
        tsStr.remove_suffix(tsStr.size() - underscorePos);
        std::string_view indexStr(entryID);
        indexStr.remove_prefix(underscorePos + 1);
        auto [ptr, ec] = std::from_chars(
            indexStr.data(), indexStr.data() + indexStr.size(), index);
        if (ec != std::errc())
        {
            messages::resourceMissingAtURI(asyncResp->res, entryID);
            return false;
        }
    }
    // Timestamp has no index
    auto [ptr, ec] =
        std::from_chars(tsStr.data(), tsStr.data() + tsStr.size(), timestamp);
    if (ec != std::errc())
    {
        messages::resourceMissingAtURI(asyncResp->res, entryID);
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
        if (boost::starts_with(filename, redfishLogFilename))
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

inline void
    getDumpEntryCollection(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const std::string& dumpType)
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
        BMCWEB_LOG_ERROR << "Invalid dump type" << dumpType;
        messages::internalError(asyncResp->res);
        return;
    }

    crow::connections::systemBus->async_method_call(
        [asyncResp, dumpPath, dumpType](const boost::system::error_code ec,
                                        GetManagedObjectsType& resp) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "DumpEntry resp_handler got error " << ec;
                messages::internalError(asyncResp->res);
                return;
            }

            nlohmann::json& entriesArray = asyncResp->res.jsonValue["Members"];
            entriesArray = nlohmann::json::array();
            std::string dumpEntryPath =
                "/xyz/openbmc_project/dump/" +
                std::string(boost::algorithm::to_lower_copy(dumpType)) +
                "/entry/";

            for (auto& object : resp)
            {
                if (object.first.str.find(dumpEntryPath) == std::string::npos)
                {
                    continue;
                }
                uint64_t timestamp = 0;
                uint64_t size = 0;
                std::string dumpStatus;
                nlohmann::json thisEntry;

                std::string entryID = object.first.filename();
                if (entryID.empty())
                {
                    continue;
                }

                for (auto& interfaceMap : object.second)
                {
                    if (interfaceMap.first ==
                        "xyz.openbmc_project.Common.Progress")
                    {
                        for (auto& propertyMap : interfaceMap.second)
                        {
                            if (propertyMap.first == "Status")
                            {
                                auto status = std::get_if<std::string>(
                                    &propertyMap.second);
                                if (status == nullptr)
                                {
                                    messages::internalError(asyncResp->res);
                                    break;
                                }
                                dumpStatus = *status;
                            }
                        }
                    }
                    else if (interfaceMap.first ==
                             "xyz.openbmc_project.Dump.Entry")
                    {

                        for (auto& propertyMap : interfaceMap.second)
                        {
                            if (propertyMap.first == "Size")
                            {
                                auto sizePtr =
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
                    else if (interfaceMap.first ==
                             "xyz.openbmc_project.Time.EpochTime")
                    {

                        for (auto& propertyMap : interfaceMap.second)
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
                                timestamp = (*usecsTimeStamp / 1000 / 1000);
                                break;
                            }
                        }
                    }
                }

                if (dumpStatus !=
                        "xyz.openbmc_project.Common.Progress.OperationStatus.Completed" &&
                    !dumpStatus.empty())
                {
                    // Dump status is not Complete, no need to enumerate
                    continue;
                }

                thisEntry["@odata.type"] = "#LogEntry.v1_8_0.LogEntry";
                thisEntry["@odata.id"] = dumpPath + entryID;
                thisEntry["Id"] = entryID;
                thisEntry["EntryType"] = "Event";
                thisEntry["Created"] =
                    crow::utility::getDateTimeUint(timestamp);
                thisEntry["Name"] = dumpType + " Dump Entry";

                thisEntry["AdditionalDataSizeBytes"] = size;

                if (dumpType == "BMC")
                {
                    thisEntry["DiagnosticDataType"] = "Manager";
                    thisEntry["AdditionalDataURI"] =
                        "/redfish/v1/Managers/bmc/LogServices/Dump/Entries/" +
                        entryID + "/attachment";
                }
                else if (dumpType == "System")
                {
                    thisEntry["DiagnosticDataType"] = "OEM";
                    thisEntry["OEMDiagnosticDataType"] = "System";
                    thisEntry["AdditionalDataURI"] =
                        "/redfish/v1/Systems/system/LogServices/Dump/Entries/" +
                        entryID + "/attachment";
                }
                entriesArray.push_back(std::move(thisEntry));
            }
            asyncResp->res.jsonValue["Members@odata.count"] =
                entriesArray.size();
        },
        "xyz.openbmc_project.Dump.Manager", "/xyz/openbmc_project/dump",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
}

inline void
    getDumpEntryById(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     const std::string& entryID, const std::string& dumpType)
{
    std::string dumpPath;
    if (dumpType == "BMC")
    {
        dumpPath = "/redfish/v1/Managers/bmc/LogServices/Dump/Entries/";
    }
    else if (dumpType == "FaultLog")
    {
        dumpPath = "/redfish/v1/Managers/faultlog/LogServices/Dump/Entries/";
    }
    else if (dumpType == "System")
    {
        dumpPath = "/redfish/v1/Systems/system/LogServices/Dump/Entries/";
    }
    else
    {
        BMCWEB_LOG_ERROR << "Invalid dump type" << dumpType;
        messages::internalError(asyncResp->res);
        return;
    }

    crow::connections::systemBus->async_method_call(
        [asyncResp, entryID, dumpPath, dumpType](
            const boost::system::error_code ec, GetManagedObjectsType& resp) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "DumpEntry resp_handler got error " << ec;
                messages::internalError(asyncResp->res);
                return;
            }

            bool foundDumpEntry = false;
            std::string dumpEntryPath =
                "/xyz/openbmc_project/dump/" +
                std::string(boost::algorithm::to_lower_copy(dumpType)) +
                "/entry/";

            for (auto& objectPath : resp)
            {
                if (objectPath.first.str != dumpEntryPath + entryID)
                {
                    continue;
                }

                foundDumpEntry = true;
                uint64_t timestamp = 0;
                uint64_t size = 0;
                std::string dumpStatus;

                for (auto& interfaceMap : objectPath.second)
                {
                    if (interfaceMap.first ==
                        "xyz.openbmc_project.Common.Progress")
                    {
                        for (auto& propertyMap : interfaceMap.second)
                        {
                            if (propertyMap.first == "Status")
                            {
                                auto status = std::get_if<std::string>(
                                    &propertyMap.second);
                                if (status == nullptr)
                                {
                                    messages::internalError(asyncResp->res);
                                    break;
                                }
                                dumpStatus = *status;
                            }
                        }
                    }
                    else if (interfaceMap.first ==
                             "xyz.openbmc_project.Dump.Entry")
                    {
                        for (auto& propertyMap : interfaceMap.second)
                        {
                            if (propertyMap.first == "Size")
                            {
                                auto sizePtr =
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
                    else if (interfaceMap.first ==
                             "xyz.openbmc_project.Time.EpochTime")
                    {
                        for (auto& propertyMap : interfaceMap.second)
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
                                timestamp = *usecsTimeStamp / 1000 / 1000;
                                break;
                            }
                        }
                    }
                }

                if (dumpStatus !=
                        "xyz.openbmc_project.Common.Progress.OperationStatus.Completed" &&
                    !dumpStatus.empty())
                {
                    // Dump status is not Complete
                    // return not found until status is changed to Completed
                    messages::resourceNotFound(asyncResp->res,
                                               dumpType + " dump", entryID);
                    return;
                }

                asyncResp->res.jsonValue["@odata.type"] =
                    "#LogEntry.v1_8_0.LogEntry";
                asyncResp->res.jsonValue["@odata.id"] = dumpPath + entryID;
                asyncResp->res.jsonValue["Id"] = entryID;
                asyncResp->res.jsonValue["EntryType"] = "Event";
                asyncResp->res.jsonValue["Created"] =
                    crow::utility::getDateTimeUint(timestamp);
                asyncResp->res.jsonValue["Name"] = dumpType + " Dump Entry";

                asyncResp->res.jsonValue["AdditionalDataSizeBytes"] = size;

                if (dumpType == "BMC")
                {
                    asyncResp->res.jsonValue["DiagnosticDataType"] = "Manager";
                    asyncResp->res.jsonValue["AdditionalDataURI"] =
                        "/redfish/v1/Managers/bmc/LogServices/Dump/Entries/" +
                        entryID + "/attachment";
                }
                else if (dumpType == "FaultLog")
                {
                    asyncResp->res.jsonValue["DiagnosticDataType"] = "OEM"; //?
                    asyncResp->res.jsonValue["OEMDiagnosticDataType"] =
                        "FaultLog"; //?
                    asyncResp->res.jsonValue["AdditionalDataURI"] =
                        "/redfish/v1/Systems/system/LogServices/Dump/Entries/" +
                        entryID + "/attachment";
                }
                else if (dumpType == "System")
                {
                    asyncResp->res.jsonValue["DiagnosticDataType"] = "OEM";
                    asyncResp->res.jsonValue["OEMDiagnosticDataType"] =
                        "System";
                    asyncResp->res.jsonValue["AdditionalDataURI"] =
                        "/redfish/v1/Systems/system/LogServices/Dump/Entries/" +
                        entryID + "/attachment";
                }
            }
            if (foundDumpEntry == false)
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
    auto respHandler = [asyncResp,
                        entryID](const boost::system::error_code ec) {
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

inline void
    createDumpTaskCallback(task::Payload&& payload,
                           const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const uint32_t& dumpId, const std::string& dumpPath,
                           const std::string& dumpType)
{
    std::shared_ptr<task::TaskData> task = task::TaskData::createTask(
        [dumpId, dumpPath, dumpType](
            boost::system::error_code err, sdbusplus::message::message& m,
            const std::shared_ptr<task::TaskData>& taskData) {
            if (err)
            {
                BMCWEB_LOG_ERROR << "Error in creating a dump";
                taskData->state = "Cancelled";
                return task::completed;
            }
            std::vector<std::pair<
                std::string,
                std::vector<std::pair<std::string, std::variant<std::string>>>>>
                interfacesList;

            sdbusplus::message::object_path objPath;

            m.read(objPath, interfacesList);

            if (objPath.str ==
                "/xyz/openbmc_project/dump/" +
                    std::string(boost::algorithm::to_lower_copy(dumpType)) +
                    "/entry/" + std::to_string(dumpId))
            {
                nlohmann::json retMessage = messages::success();
                taskData->messages.emplace_back(retMessage);

                std::string headerLoc =
                    "Location: " + dumpPath + std::to_string(dumpId);
                taskData->payload->httpHeaders.emplace_back(
                    std::move(headerLoc));

                taskData->state = "Completed";
                return task::completed;
            }
            return task::completed;
        },
        "type='signal',interface='org.freedesktop.DBus."
        "ObjectManager',"
        "member='InterfacesAdded', "
        "path='/xyz/openbmc_project/dump'");

    task->startTimer(std::chrono::minutes(3));
    task->populateResp(asyncResp->res);
    task->payload.emplace(std::move(payload));
}

inline void createDump(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const crow::Request& req, const std::string& dumpType)
{

    std::string dumpPath;
    if (dumpType == "BMC")
    {
        dumpPath = "/redfish/v1/Managers/bmc/LogServices/Dump/Entries/";
    }
    else if (dumpType == "FaultLog")
    {
        dumpPath = "/redfish/v1/Managers/faultlog/LogServices/Dump/Entries/";
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
    else if (dumpType == "BMC" || dumpType == "FaultLog")
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
                << "Wrong parameter value passed for 'DiagnosticDataType!'";
            messages::invalidObject(asyncResp->res,
                                    "BMC Dump creation parameters");
            return;
        }

        BMCWEB_LOG_DEBUG << "log_services.hpp createDump: " << "/xyz/openbmc_project/dump/" +
            std::string(boost::algorithm::to_lower_copy(dumpType));

        if (dumpType == "BMC")
        {

            //using DumpCreateParams = std::map<std::string, std::variant<std::string, uint64_t>>;
            std::map<std::string, std::variant<std::string, uint64_t>> myEmptyMap;

            if (myEmptyMap.empty())
            {
                BMCWEB_LOG_DEBUG << "myEmptyMap is empty";
            }
            else
            {
                BMCWEB_LOG_DEBUG << "myEmptyMap isn't empty";
            }

            crow::connections::systemBus->async_method_call(
                [asyncResp, payload(task::Payload(req))/*, dumpPath, dumpType*/]
                                                    (const boost::system::error_code ec,
                                                    /*const uint32_t& dumpId*/
                                                    sdbusplus::message::object_path objPath) mutable {

                    if (ec)
                    {
                        BMCWEB_LOG_ERROR << "CreateDump resp_handler got error " << ec;
                        messages::internalError(asyncResp->res);
                        return;
                    }

                    BMCWEB_LOG_DEBUG << "CreateDump end " << objPath.str;

                    messages::success(asyncResp->res);

                    /*BMCWEB_LOG_DEBUG << "Dump Created. Id: " << dumpId;

                    createDumpTaskCallback(std::move(payload), asyncResp, dumpId, dumpPath, dumpType);*/
                },
                "xyz.openbmc_project.Dump.Manager",
                "/xyz/openbmc_project/dump/" +
                    std::string(boost::algorithm::to_lower_copy(dumpType)),
                "xyz.openbmc_project.Dump.Create", "CreateDump", myEmptyMap);
        }
    }
}

inline void clearDump(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const std::string& dumpType)
{
    std::string dumpTypeLowerCopy =
        std::string(boost::algorithm::to_lower_copy(dumpType));

    crow::connections::systemBus->async_method_call(
        [asyncResp, dumpType](const boost::system::error_code ec,
                              const std::vector<std::string>& subTreePaths) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "resp_handler got error " << ec;
                messages::internalError(asyncResp->res);
                return;
            }

            for (const std::string& path : subTreePaths)
            {
                sdbusplus::message::object_path objPath(path);

                std::string logID = objPath.filename();
                if (logID.empty())
                {
                    continue;
                }
                deleteDumpEntry(asyncResp, logID, dumpType);
            }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
        "/xyz/openbmc_project/dump/" + dumpTypeLowerCopy, 0,
        std::array<std::string, 1>{"xyz.openbmc_project.Dump.Entry." +
                                   dumpType});
}

inline static void parseCrashdumpParameters(
    const std::vector<std::pair<std::string, VariantType>>& params,
    std::string& filename, std::string& timestamp, std::string& logfile)
{
    for (auto property : params)
    {
        if (property.first == "Timestamp")
        {
            const std::string* value =
                std::get_if<std::string>(&property.second);
            if (value != nullptr)
            {
                timestamp = *value;
            }
        }
        else if (property.first == "Filename")
        {
            const std::string* value =
                std::get_if<std::string>(&property.second);
            if (value != nullptr)
            {
                filename = *value;
            }
        }
        else if (property.first == "Log")
        {
            const std::string* value =
                std::get_if<std::string>(&property.second);
            if (value != nullptr)
            {
                logfile = *value;
            }
        }
    }
}

constexpr char const* postCodeIface = "xyz.openbmc_project.State.Boot.PostCode";
inline void requestRoutesSystemLogServiceCollection(App& app)
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

#ifdef BMCWEB_ENABLE_REDFISH_HOST_LOGGER
                logServiceArray.push_back(
                    {{"@odata.id",
                      "/redfish/v1/Systems/system/LogServices/HostLogger"}});
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
                                    {{"@odata.id",
                                      "/redfish/v1/Systems/system/LogServices/PostCodes"}});
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

inline void requestRoutesEventLogService(App& app)
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

                {"target",
                 "/redfish/v1/Systems/system/LogServices/EventLog/Actions/LogService.ClearLog"}};
        });
}

inline void requestRoutesJournalEventLogClear(App& app)
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

static int fillEventLogEntryJson(const std::string& logEntryID,
                                 const std::string& logEntry,
                                 nlohmann::json& logEntryJson)
{
    // The redfish log format is "<Timestamp> <MessageId>,<MessageArgs>"
    // First get the Timestamp
    size_t space = logEntry.find_first_of(' ');
    if (space == std::string::npos)
    {
        return 1;
    }
    std::string timestamp = logEntry.substr(0, space);
    // Then get the log contents
    size_t entryStart = logEntry.find_first_not_of(' ', space);
    if (entryStart == std::string::npos)
    {
        return 1;
    }
    std::string_view entry(logEntry);
    entry.remove_prefix(entryStart);
    // Use split to separate the entry into its fields
    std::vector<std::string> logEntryFields;
    boost::split(logEntryFields, entry, boost::is_any_of(","),
                 boost::token_compress_on);
    // We need at least a MessageId to be valid
    if (logEntryFields.size() < 1)
    {
        return 1;
    }
    std::string& messageID = logEntryFields[0];

    // Get the Message from the MessageRegistry
    const message_registries::Message* message =
        message_registries::getMessage(messageID);

    std::string msg;
    std::string severity;
    if (message != nullptr)
    {
        msg = message->message;
        severity = message->severity;
    }

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
    logEntryJson = {
        {"@odata.type", "#LogEntry.v1_8_0.LogEntry"},
        {"@odata.id",
         "/redfish/v1/Systems/system/LogServices/EventLog/Entries/" +
             logEntryID},
        {"Name", "System Event Log Entry"},
        {"Id", logEntryID},
        {"Message", std::move(msg)},
        {"MessageId", std::move(messageID)},
        {"MessageArgs", messageArgs},
        {"EntryType", "Event"},
        {"Severity", std::move(severity)},
        {"Created", std::move(timestamp)}};
    return 0;
}

inline void requestRoutesJournalEventLogEntryCollection(App& app)
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

inline void requestRoutesJournalEventLogEntry(App& app)
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

inline void requestRoutesDBusEventLogEntryCollection(App& app)
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
                            "/redfish/v1/Systems/system/LogServices/EventLog/Entries/" +
                            std::to_string(*id);
                        thisEntry["Name"] = "System Event Log Entry";
                        thisEntry["Id"] = std::to_string(*id);
                        thisEntry["Message"] = *message;
                        thisEntry["Resolved"] = resolved;
                        thisEntry["EntryType"] = "Event";
                        thisEntry["Severity"] =
                            translateSeverityDbusToRedfish(*severity);
                        thisEntry["Created"] =
                            crow::utility::getDateTimeStdtime(timestamp);
                        thisEntry["Modified"] =
                            crow::utility::getDateTimeStdtime(updateTimestamp);
                        if (filePath != nullptr)
                        {
                            thisEntry["AdditionalDataURI"] =
                                "/redfish/v1/Systems/system/LogServices/EventLog/Entries/" +
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

inline void requestRoutesDBusEventLogEntry(App& app)
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
                            BMCWEB_LOG_ERROR
                                << "EventLogEntry (DBus) resp_handler got error "
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
                            "/redfish/v1/Systems/system/LogServices/EventLog/Entries/" +
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
                            crow::utility::getDateTimeStdtime(timestamp);
                        asyncResp->res.jsonValue["Modified"] =
                            crow::utility::getDateTimeStdtime(updateTimestamp);
                        if (filePath != nullptr)
                        {
                            asyncResp->res.jsonValue["AdditionalDataURI"] =
                                "/redfish/v1/Systems/system/LogServices/EventLog/attachment/" +
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
        "/redfish/v1/Systems/system/LogServices/EventLog/Entries/<str>/attachment")
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
        if (boost::starts_with(filename, "log"))
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

inline bool
    getHostLoggerEntries(std::vector<std::filesystem::path>& hostLoggerFiles,
                         uint64_t& skip, uint64_t& top,
                         std::vector<std::string>& logEntries, size_t& logCount)
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
                                    nlohmann::json& logEntryJson)
{
    // Fill in the log entry with the gathered data.
    logEntryJson = {
        {"@odata.type", "#LogEntry.v1_4_0.LogEntry"},
        {"@odata.id",
         "/redfish/v1/Systems/system/LogServices/HostLogger/Entries/" +
             logEntryID},
        {"Name", "Host Logger Entry"},
        {"Id", logEntryID},
        {"Message", msg},
        {"EntryType", "Oem"},
        {"Severity", "OK"},
        {"OemRecordFormat", "Host Logger Entry"}};
}

inline void requestRoutesSystemHostLogger(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/LogServices/HostLogger/")
        .privileges(redfish::privileges::getLogService)
        .methods(
            boost::beast::http::verb::
                get)([](const crow::Request&,
                        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
            asyncResp->res.jsonValue["@odata.id"] =
                "/redfish/v1/Systems/system/LogServices/HostLogger";
            asyncResp->res.jsonValue["@odata.type"] =
                "#LogService.v1_1_0.LogService";
            asyncResp->res.jsonValue["Name"] = "Host Logger Service";
            asyncResp->res.jsonValue["Description"] = "Host Logger Service";
            asyncResp->res.jsonValue["Id"] = "HostLogger";
            asyncResp->res.jsonValue["Entries"] = {
                {"@odata.id",
                 "/redfish/v1/Systems/system/LogServices/HostLogger/Entries"}};
        });
}

inline void requestRoutesSystemHostLoggerCollection(App& app)
{
    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/system/LogServices/HostLogger/Entries/")
        .privileges(redfish::privileges::getLogEntry)
        .methods(
            boost::beast::http::verb::
                get)([](const crow::Request& req,
                        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
            uint64_t skip = 0;
            uint64_t top = maxEntriesPerPage; // Show max 1000 entries by
                                              // default, allow range 1 to
                                              // 1000 entries per page.
            if (!getSkipParam(asyncResp, req, skip))
            {
                return;
            }
            if (!getTopParam(asyncResp, req, top))
            {
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
            if (logEntries.size() == 0)
            {
                asyncResp->res.jsonValue["Members@odata.count"] = logCount;
                return;
            }
            if (logEntries.size() > 0)
            {
                for (size_t i = 0; i < logEntries.size(); i++)
                {
                    logEntryArray.push_back({});
                    nlohmann::json& hostLogEntry = logEntryArray.back();
                    fillHostLoggerEntryJson(std::to_string(skip + i),
                                            logEntries[i], hostLogEntry);
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
        app, "/redfish/v1/Systems/system/LogServices/HostLogger/Entries/<str>/")
        .privileges(redfish::privileges::getLogEntry)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& param) {
                const std::string& targetID = param;

                uint64_t idInt = 0;
                auto [ptr, ec] = std::from_chars(
                    targetID.data(), targetID.data() + targetID.size(), idInt);
                if (ec == std::errc::invalid_argument)
                {
                    messages::resourceMissingAtURI(asyncResp->res, targetID);
                    return;
                }
                if (ec == std::errc::result_out_of_range)
                {
                    messages::resourceMissingAtURI(asyncResp->res, targetID);
                    return;
                }

                std::vector<std::filesystem::path> hostLoggerFiles;
                if (!getHostLoggerFiles(hostLoggerFolderPath, hostLoggerFiles))
                {
                    BMCWEB_LOG_ERROR << "fail to get host log file path";
                    return;
                }

                size_t logCount = 0;
                uint64_t top = 1;
                std::vector<std::string> logEntries;
                // We can get specific entry by skip and top. For example, if we
                // want to get nth entry, we can set skip = n-1 and top = 1 to
                // get that entry
                if (!getHostLoggerEntries(hostLoggerFiles, idInt, top,
                                          logEntries, logCount))
                {
                    messages::internalError(asyncResp->res);
                    return;
                }

                if (!logEntries.empty())
                {
                    fillHostLoggerEntryJson(targetID, logEntries[0],
                                            asyncResp->res.jsonValue);
                    return;
                }

                // Requested ID was not found
                messages::resourceMissingAtURI(asyncResp->res, targetID);
            });
}

inline void requestRoutesBMCLogServiceCollection(App& app)
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

inline void requestRoutesBMCJournalLogService(App& app)
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

inline void requestRoutesBMCJournalLogEntryCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Managers/bmc/LogServices/Journal/Entries/")
        .privileges(redfish::privileges::getLogEntryCollection)
        .methods(
            boost::beast::http::verb::
                get)([](const crow::Request& req,
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
            nlohmann::json& logEntryArray = asyncResp->res.jsonValue["Members"];
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

inline void requestRoutesBMCDumpService(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Managers/bmc/LogServices/Dump/")
        .privileges(redfish::privileges::getLogService)
        .methods(
            boost::beast::http::verb::
                get)([](const crow::Request&,
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
            asyncResp->res.jsonValue["DateTime"] = redfishDateTimeOffset.first;
            asyncResp->res.jsonValue["DateTimeLocalOffset"] =
                redfishDateTimeOffset.second;

            asyncResp->res.jsonValue["Entries"] = {
                {"@odata.id",
                 "/redfish/v1/Managers/bmc/LogServices/Dump/Entries"}};
            asyncResp->res.jsonValue["Actions"] = {
                {"#LogService.ClearLog",
                 {{"target",
                   "/redfish/v1/Managers/bmc/LogServices/Dump/Actions/LogService.ClearLog"}}},
                {"#LogService.CollectDiagnosticData",
                 {{"target",
                   "/redfish/v1/Managers/bmc/LogServices/Dump/Actions/LogService.CollectDiagnosticData"}}}};
        });
}

inline void requestRoutesBMCDumpEntryCollection(App& app)
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

inline void requestRoutesBMCDumpEntry(App& app)
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

inline void requestRoutesBMCDumpCreate(App& app)
{

    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Managers/bmc/LogServices/Dump/Actions/LogService.CollectDiagnosticData/")
        .privileges(redfish::privileges::postLogService)
        .methods(boost::beast::http::verb::post)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                createDump(asyncResp, req, "BMC");
            });
}

inline void requestRoutesBMCDumpClear(App& app)
{
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Managers/bmc/LogServices/Dump/Actions/LogService.ClearLog/")
        .privileges(redfish::privileges::postLogService)
        .methods(boost::beast::http::verb::post)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                clearDump(asyncResp, "BMC");
            });
}

inline void requestRoutesSystemDumpService(App& app)
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
                       "/redfish/v1/Systems/system/LogServices/Dump/Actions/LogService.ClearLog"}}},
                    {"#LogService.CollectDiagnosticData",
                     {{"target",
                       "/redfish/v1/Systems/system/LogServices/Dump/Actions/LogService.CollectDiagnosticData"}}}};
            });
}

inline void requestRoutesSystemDumpEntryCollection(App& app)
{

    /**
     * Functions triggers appropriate requests on DBus
     */
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/LogServices/Dump/Entries/")
        .privileges(redfish::privileges::getLogEntryCollection)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                asyncResp->res.jsonValue["@odata.type"] =
                    "#LogEntryCollection.LogEntryCollection";
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/Systems/system/LogServices/Dump/Entries";
                asyncResp->res.jsonValue["Name"] = "System Dump Entries";
                asyncResp->res.jsonValue["Description"] =
                    "Collection of System Dump Entries";

                getDumpEntryCollection(asyncResp, "System");
            });
}

inline void requestRoutesSystemDumpEntry(App& app)
{
    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/system/LogServices/Dump/Entries/<str>/")
        .privileges(redfish::privileges::getLogEntry)

        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& param) {
                getDumpEntryById(asyncResp, param, "System");
            });

    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/system/LogServices/Dump/Entries/<str>/")
        .privileges(redfish::privileges::deleteLogEntry)
        .methods(boost::beast::http::verb::delete_)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& param) {
                deleteDumpEntry(asyncResp, param, "system");
            });
}

inline void requestRoutesSystemDumpCreate(App& app)
{
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Systems/system/LogServices/Dump/Actions/LogService.CollectDiagnosticData/")
        .privileges(redfish::privileges::postLogService)
        .methods(boost::beast::http::verb::post)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)

            { createDump(asyncResp, req, "System"); });
}

inline void requestRoutesSystemDumpClear(App& app)
{
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Systems/system/LogServices/Dump/Actions/LogService.ClearLog/")
        .privileges(redfish::privileges::postLogService)
        .methods(boost::beast::http::verb::post)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)

            { clearDump(asyncResp, "System"); });
}

inline void requestRoutesCrashdumpService(App& app)
{
    // Note: Deviated from redfish privilege registry for GET & HEAD
    // method for security reasons.
    /**
     * Functions triggers appropriate requests on DBus
     */
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/LogServices/Crashdump/")
        // This is incorrect, should be:
        //.privileges(redfish::privileges::getLogService)
        .privileges({{"ConfigureManager"}})
        .methods(
            boost::beast::http::verb::
                get)([](const crow::Request&,
                        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
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
                crow::utility::getDateTimeOffsetNow();
            asyncResp->res.jsonValue["DateTime"] = redfishDateTimeOffset.first;
            asyncResp->res.jsonValue["DateTimeLocalOffset"] =
                redfishDateTimeOffset.second;

            asyncResp->res.jsonValue["Entries"] = {
                {"@odata.id",
                 "/redfish/v1/Systems/system/LogServices/Crashdump/Entries"}};
            asyncResp->res.jsonValue["Actions"] = {
                {"#LogService.ClearLog",
                 {{"target",
                   "/redfish/v1/Systems/system/LogServices/Crashdump/Actions/LogService.ClearLog"}}},
                {"#LogService.CollectDiagnosticData",
                 {{"target",
                   "/redfish/v1/Systems/system/LogServices/Crashdump/Actions/LogService.CollectDiagnosticData"}}}};
        });
}

void inline requestRoutesCrashdumpClear(App& app)
{
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Systems/system/LogServices/Crashdump/Actions/LogService.ClearLog/")
        // This is incorrect, should be:
        //.privileges(redfish::privileges::postLogService)
        .privileges({{"ConfigureComponents"}})
        .methods(boost::beast::http::verb::post)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                crow::connections::systemBus->async_method_call(
                    [asyncResp](const boost::system::error_code ec,
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

static void
    logCrashdumpEntry(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const std::string& logID, nlohmann::json& logEntryJson)
{
    auto getStoredLogCallback =
        [asyncResp, logID, &logEntryJson](
            const boost::system::error_code ec,
            const std::vector<std::pair<std::string, VariantType>>& params) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "failed to get log ec: " << ec.message();
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
                messages::resourceMissingAtURI(asyncResp->res, logID);
                return;
            }

            std::string crashdumpURI =
                "/redfish/v1/Systems/system/LogServices/Crashdump/Entries/" +
                logID + "/" + filename;
            logEntryJson = {{"@odata.type", "#LogEntry.v1_7_0.LogEntry"},
                            {"@odata.id", "/redfish/v1/Systems/system/"
                                          "LogServices/Crashdump/Entries/" +
                                              logID},
                            {"Name", "CPU Crashdump"},
                            {"Id", logID},
                            {"EntryType", "Oem"},
                            {"AdditionalDataURI", std::move(crashdumpURI)},
                            {"DiagnosticDataType", "OEM"},
                            {"OEMDiagnosticDataType", "PECICrashdump"},
                            {"Created", std::move(timestamp)}};
        };
    crow::connections::systemBus->async_method_call(
        std::move(getStoredLogCallback), crashdumpObject,
        crashdumpPath + std::string("/") + logID,
        "org.freedesktop.DBus.Properties", "GetAll", crashdumpInterface);
}

inline void requestRoutesCrashdumpEntryCollection(App& app)
{
    // Note: Deviated from redfish privilege registry for GET & HEAD
    // method for security reasons.
    /**
     * Functions triggers appropriate requests on DBus
     */
    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/system/LogServices/Crashdump/Entries/")
        // This is incorrect, should be.
        //.privileges(redfish::privileges::postLogEntryCollection)
        .privileges({{"ConfigureComponents"}})
        .methods(
            boost::beast::http::verb::
                get)([](const crow::Request&,
                        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
            // Collections don't include the static data added by SubRoute
            // because it has a duplicate entry for members
            auto getLogEntriesCallback = [asyncResp](
                                             const boost::system::error_code ec,
                                             const std::vector<std::string>&
                                                 resp) {
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
                nlohmann::json& logEntryArray =
                    asyncResp->res.jsonValue["Members"];
                logEntryArray = nlohmann::json::array();
                std::vector<std::string> logIDs;
                // Get the list of log entries and build up an empty array big
                // enough to hold them
                for (const std::string& objpath : resp)
                {
                    // Get the log ID
                    std::size_t lastPos = objpath.rfind('/');
                    if (lastPos == std::string::npos)
                    {
                        continue;
                    }
                    logIDs.emplace_back(objpath.substr(lastPos + 1));

                    // Add a space for the log entry to the array
                    logEntryArray.push_back({});
                }
                // Now go through and set up async calls to fill in the entries
                size_t index = 0;
                for (const std::string& logID : logIDs)
                {
                    // Add the log entry to the array
                    logCrashdumpEntry(asyncResp, logID, logEntryArray[index++]);
                }
                asyncResp->res.jsonValue["Members@odata.count"] =
                    logEntryArray.size();
            };
            crow::connections::systemBus->async_method_call(
                std::move(getLogEntriesCallback),
                "xyz.openbmc_project.ObjectMapper",
                "/xyz/openbmc_project/object_mapper",
                "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths", "", 0,
                std::array<const char*, 1>{crashdumpInterface});
        });
}

inline void requestRoutesCrashdumpEntry(App& app)
{
    // Note: Deviated from redfish privilege registry for GET & HEAD
    // method for security reasons.

    BMCWEB_ROUTE(
        app, "/redfish/v1/Systems/system/LogServices/Crashdump/Entries/<str>/")
        // this is incorrect, should be
        // .privileges(redfish::privileges::getLogEntry)
        .privileges({{"ConfigureComponents"}})
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& param) {
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

inline void requestRoutesCrashdumpCollect(App& app)
{
    // Note: Deviated from redfish privilege registry for GET & HEAD
    // method for security reasons.
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Systems/system/LogServices/Crashdump/Actions/LogService.CollectDiagnosticData/")
        // The below is incorrect;  Should be ConfigureManager
        //.privileges(redfish::privileges::postLogService)
        .privileges({{"ConfigureComponents"}})
        .methods(
            boost::beast::http::verb::
                post)([](const crow::Request& req,
                         const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
            std::string diagnosticDataType;
            std::string oemDiagnosticDataType;
            if (!redfish::json_util::readJson(
                    req, asyncResp->res, "DiagnosticDataType",
                    diagnosticDataType, "OEMDiagnosticDataType",
                    oemDiagnosticDataType))
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

            auto collectCrashdumpCallback = [asyncResp,
                                             payload(task::Payload(req))](
                                                const boost::system::error_code
                                                    ec,
                                                const std::string&) mutable {
                if (ec)
                {
                    if (ec.value() ==
                        boost::system::errc::operation_not_supported)
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
                    [](boost::system::error_code err,
                       sdbusplus::message::message&,
                       const std::shared_ptr<task::TaskData>& taskData) {
                        if (!err)
                        {
                            taskData->messages.emplace_back(
                                messages::taskCompletedOK(
                                    std::to_string(taskData->index)));
                            taskData->state = "Completed";
                        }
                        return task::completed;
                    },
                    "type='signal',interface='org.freedesktop.DBus."
                    "Properties',"
                    "member='PropertiesChanged',arg0namespace='com.intel.crashdump'");
                task->startTimer(std::chrono::minutes(5));
                task->populateResp(asyncResp->res);
                task->payload.emplace(std::move(payload));
            };

            if (oemDiagnosticDataType == "OnDemand")
            {
                crow::connections::systemBus->async_method_call(
                    std::move(collectCrashdumpCallback), crashdumpObject,
                    crashdumpPath, crashdumpOnDemandInterface,
                    "GenerateOnDemandLog");
            }
            else if (oemDiagnosticDataType == "Telemetry")
            {
                crow::connections::systemBus->async_method_call(
                    std::move(collectCrashdumpCallback), crashdumpObject,
                    crashdumpPath, crashdumpTelemetryInterface,
                    "GenerateTelemetryLog");
            }
            else
            {
                BMCWEB_LOG_ERROR << "Unsupported OEMDiagnosticDataType: "
                                 << oemDiagnosticDataType;
                messages::actionParameterValueFormatError(
                    asyncResp->res, oemDiagnosticDataType,
                    "OEMDiagnosticDataType", "CollectDiagnosticData");
                return;
            }
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
        "/redfish/v1/Systems/system/LogServices/EventLog/Actions/LogService.ClearLog/")
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

/****************************************************
 * Redfish PostCode interfaces
 * using DBUS interface: getPostCodesTS
 ******************************************************/
inline void requestRoutesPostCodesLogService(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/LogServices/PostCodes/")
        .privileges(redfish::privileges::getLogService)
        .methods(
            boost::beast::http::verb::
                get)([](const crow::Request&,
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
                 {{"@odata.id",
                   "/redfish/v1/Systems/system/LogServices/PostCodes/Entries"}}}};

            std::pair<std::string, std::string> redfishDateTimeOffset =
                crow::utility::getDateTimeOffsetNow();
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
        "/redfish/v1/Systems/system/LogServices/PostCodes/Actions/LogService.ClearLog/")
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
        entryTimeStr =
            crow::utility::getDateTimeUint(usecSinceEpoch / 1000 / 1000);

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
        bmcLogEntry = {
            {"@odata.type", "#LogEntry.v1_8_0.LogEntry"},
            {"@odata.id",
             "/redfish/v1/Systems/system/LogServices/PostCodes/Entries/" +
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

inline void requestRoutesPostCodesEntryCollection(App& app)
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

inline void requestRoutesPostCodesEntryAdditionalData(App& app)
{
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Systems/system/LogServices/PostCodes/Entries/<str>/attachment/")
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

inline void requestRoutesPostCodesEntry(App& app)
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
                    "/redfish/v1/Systems/system/LogServices/PostCodes/Entries";
                asyncResp->res.jsonValue["Name"] = "BIOS POST Code Log Entries";
                asyncResp->res.jsonValue["Description"] =
                    "Collection of POST Code Log Entries";
                asyncResp->res.jsonValue["Members"] = nlohmann::json::array();
                asyncResp->res.jsonValue["Members@odata.count"] = 0;

                getPostCodeForEntry(asyncResp, bootIndex, codeIndex);
            });
}

} // namespace redfish
