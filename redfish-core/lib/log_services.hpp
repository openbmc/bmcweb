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

#include <array>
#include <cstddef>
#include <boost/container/flat_map.hpp>
#include <boost/algorithm/string.hpp>
#include <utility>
#include "registries.hpp"
#include "registries/base_message_registry.hpp"
#include "registries/openbmc_message_registry.hpp"

#include "../../include/dbus_singleton.hpp"
#include "../../include/dbus_utility.hpp"

#include <systemd/sd-journal.h>
#include <unistd.h>

#include <app_class_decl.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/beast/core/span.hpp>
#include <boost/system/linux_error.hpp>
//#include <boost/beast/http.hpp>
#include <error_messages.hpp>
#include <registries/privilege_registry.hpp>

#include <charconv>
#include <filesystem>
#include <optional>
#include <string_view>
#include <variant>

using crow::App;

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
static const Message* getMessageFromRegistry(
    const std::string& messageKey,
    const boost::beast::span<const MessageEntry> registry)
{
    boost::beast::span<const MessageEntry>::const_iterator messageIt =
        std::find_if(registry.cbegin(), registry.cend(),
                     [&messageKey](const MessageEntry& messageEntry) {
                         return !std::strcmp(messageEntry.first,
                                             messageKey.c_str());
                     });
    if (messageIt != registry.cend())
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
            messageKey, boost::beast::span<const MessageEntry>(base::registry));
    }
    if (std::string(openbmc::header.registryPrefix) == registryName)
    {
        return getMessageFromRegistry(
            messageKey,
            boost::beast::span<const MessageEntry>(openbmc::registry));
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
    entryTimestamp = crow::utility::getDateTime(
        static_cast<std::time_t>(timestamp / 1000 / 1000));
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
                std::time_t timestamp;
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
                                timestamp =
                                    static_cast<std::time_t>(*usecsTimeStamp);
                                break;
                            }
                        }
                    }
                }

                if (dumpStatus != "xyz.openbmc_project.Common.Progress."
                                  "OperationStatus.Completed" &&
                    !dumpStatus.empty())
                {
                    // Dump status is not Complete, no need to enumerate
                    continue;
                }

                thisEntry["@odata.type"] = "#LogEntry.v1_8_0.LogEntry";
                thisEntry["@odata.id"] = dumpPath + entryID;
                thisEntry["Id"] = entryID;
                thisEntry["EntryType"] = "Event";
                thisEntry["Created"] = crow::utility::getDateTime(timestamp);
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
                std::time_t timestamp;
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
                                timestamp =
                                    static_cast<std::time_t>(*usecsTimeStamp);
                                break;
                            }
                        }
                    }
                }

                if (dumpStatus != "xyz.openbmc_project.Common.Progress."
                                  "OperationStatus.Completed" &&
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
                    crow::utility::getDateTime(timestamp);
                asyncResp->res.jsonValue["Name"] = dumpType + " Dump Entry";

                asyncResp->res.jsonValue["AdditionalDataSizeBytes"] = size;

                if (dumpType == "BMC")
                {
                    asyncResp->res.jsonValue["DiagnosticDataType"] = "Manager";
                    asyncResp->res.jsonValue["AdditionalDataURI"] =
                        "/redfish/v1/Managers/bmc/LogServices/Dump/Entries/" +
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
                             << ec;
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

void
    createDumpTaskCallback(const crow::Request& req,
                           const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const uint32_t& dumpId, const std::string& dumpPath,
                           const std::string& dumpType);

inline void createDump(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const crow::Request& req, const std::string& dumpType);

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

// Copied from Chassis
using VariantType = std::variant<bool, std::string, uint64_t, uint32_t>;
using ManagedObjectsType = std::vector<std::pair<
    sdbusplus::message::object_path,
    std::vector<std::pair<std::string,
                          std::vector<std::pair<std::string, VariantType>>>>>>;

using PropertiesType = boost::container::flat_map<std::string, VariantType>;

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
void requestRoutesSystemLogServiceCollection(App& app);
void requestRoutesEventLogService(App& app);
void requestRoutesJournalEventLogClear(App& app);

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
    boost::beast::span<std::string> messageArgs;
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

void requestRoutesJournalEventLogEntryCollection(App& app);
void requestRoutesJournalEventLogEntry(App& app);
void requestRoutesDBusEventLogEntryCollection(App& app);
void requestRoutesDBusEventLogEntry(App& app);
void requestRoutesDBusEventLogEntryDownload(App& app);
void requestRoutesBMCLogServiceCollection(App& app);
void requestRoutesBMCJournalLogService(App& app);

static int fillBMCJournalLogEntryJson(const std::string& bmcJournalLogEntryID,
                                      sd_journal* journal,
                                      nlohmann::json& bmcJournalLogEntryJson);

void requestRoutesBMCJournalLogEntryCollection(App& app);
void requestRoutesBMCJournalLogEntry(App& app);
void requestRoutesBMCDumpService(App& app);
void requestRoutesBMCDumpEntryCollection(App& app);
void requestRoutesBMCDumpEntry(App& app);
void requestRoutesBMCDumpCreate(App& app);
void requestRoutesBMCDumpClear(App& app);
void requestRoutesSystemDumpService(App& app);

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
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/LogServices/Dump/"
                      "Actions/"
                      "LogService.CollectDiagnosticData/")
        .privileges(redfish::privileges::postLogService)
        .methods(boost::beast::http::verb::post)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)

            { createDump(asyncResp, req, "System"); });
}

inline void requestRoutesSystemDumpClear(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/LogServices/Dump/"
                      "Actions/"
                      "LogService.ClearLog/")
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
                 {{"target", "/redfish/v1/Systems/system/LogServices/Crashdump/"
                             "Actions/LogService.ClearLog"}}},
                {"#LogService.CollectDiagnosticData",
                 {{"target", "/redfish/v1/Systems/system/LogServices/Crashdump/"
                             "Actions/LogService.CollectDiagnosticData"}}}};
        });
}

void inline requestRoutesCrashdumpClear(App& app)
{
    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/system/LogServices/Crashdump/Actions/"
                 "LogService.ClearLog/")
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

// moved to task.hpp
// void requestRoutesCrashdumpFile(App& app);
// void requestRoutesCrashdumpCollect(App& app);

/**
 * DBusLogServiceActionsClear class supports POST method for ClearLog action.
 */
void requestRoutesDBusLogServiceActionsClear(App& app);

/****************************************************
 * Redfish PostCode interfaces
 * using DBUS interface: getPostCodesTS
 ******************************************************/
void requestRoutesPostCodesLogService(App& app);
void requestRoutesPostCodesClear(App& app);

void requestRoutesPostCodesEntryCollection(App& app);

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


void requestRoutesPostCodesEntryAdditionalData(App& app);

void requestRoutesPostCodesEntry(App& app);

} // namespace redfish
