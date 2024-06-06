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
#include "generated/enums/log_entry.hpp"
#include "gzfile.hpp"
#include "http_utility.hpp"
#include "human_sort.hpp"
#include "query.hpp"
#include "registries.hpp"
#include "registries/base_message_registry.hpp"
#include "registries/openbmc_message_registry.hpp"
#include "registries/privilege_registry.hpp"
#include "task.hpp"
#include "task_messages.hpp"
#include "utils/dbus_utils.hpp"
#include "utils/error_log_utils.hpp"
#include "utils/json_utils.hpp"
#include "utils/name_utils.hpp"
#include "utils/time_utils.hpp"

#include <systemd/sd-id128.h>
#include <systemd/sd-journal.h>
#include <tinyxml2.h>
#include <unistd.h>

#include <boost/beast/http/verb.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/system/linux_error.hpp>
#include <boost/url/format.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <array>
#include <charconv>
#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <iterator>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <variant>

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

#ifdef BMCWEB_ENABLE_HW_ISOLATION
constexpr std::array<const char*, 3> hwIsolationEntryIfaces = {
    "xyz.openbmc_project.HardwareIsolation.Entry",
    "xyz.openbmc_project.Association.Definitions",
    "xyz.openbmc_project.Time.EpochTime"};

using RedfishResourceDBusInterfaces = std::string;
using RedfishResourceCollectionUri = std::string;
using RedfishUriListType = std::unordered_map<RedfishResourceDBusInterfaces,
                                              RedfishResourceCollectionUri>;

static const RedfishUriListType redfishUriList = {
    {"xyz.openbmc_project.Inventory.Item.Cpu",
     "/redfish/v1/Systems/system/Processors"},
    {"xyz.openbmc_project.Inventory.Item.Dimm",
     "/redfish/v1/Systems/system/Memory"},
    {"xyz.openbmc_project.Inventory.Item.CpuCore",
     "/redfish/v1/Systems/system/Processors/<str>/SubProcessors"}};

#endif // BMCWEB_ENABLE_HW_ISOLATION

enum class DumpCreationProgress
{
    DUMP_CREATE_SUCCESS,
    DUMP_CREATE_FAILED,
    DUMP_CREATE_INPROGRESS
};

namespace fs = std::filesystem;

using AssociationsValType =
    std::vector<std::tuple<std::string, std::string, std::string>>;
using GetManagedPropertyType = boost::container::flat_map<
    std::string,
    std::variant<std::string, bool, uint8_t, int16_t, uint16_t, int32_t,
                 uint32_t, int64_t, uint64_t, double, AssociationsValType>>;

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

inline std::string getDumpPath(std::string_view dumpType)
{
    std::string dbusDumpPath = "/xyz/openbmc_project/dump/";
    std::ranges::transform(dumpType, std::back_inserter(dbusDumpPath),
                           bmcweb::asciiToLower);

    return dbusDumpPath;
}

inline int getJournalMetadata(sd_journal* journal, std::string_view field,
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

inline int getJournalMetadata(sd_journal* journal, std::string_view field,
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

inline bool getEntryTimestamp(sd_journal* journal, std::string& entryTimestamp)
{
    int ret = 0;
    uint64_t timestamp = 0;
    ret = sd_journal_get_realtime_usec(journal, &timestamp);
    if (ret < 0)
    {
        BMCWEB_LOG_ERROR("Failed to read entry timestamp: {}", strerror(-ret));
        return false;
    }
    entryTimestamp = redfish::time_utils::getDateTimeUintUs(timestamp);
    return true;
}

inline bool getUniqueEntryID(sd_journal* journal, std::string& entryID,
                             const bool firstEntry = true)
{
    int ret = 0;
    static sd_id128_t prevBootID{};
    static uint64_t prevTs = 0;
    static int index = 0;
    if (firstEntry)
    {
        prevBootID = {};
        prevTs = 0;
    }

    // Get the entry timestamp
    uint64_t curTs = 0;
    sd_id128_t curBootID{};
    ret = sd_journal_get_monotonic_usec(journal, &curTs, &curBootID);
    if (ret < 0)
    {
        BMCWEB_LOG_ERROR("Failed to read entry timestamp: {}", strerror(-ret));
        return false;
    }
    // If the timestamp isn't unique on the same boot, increment the index
    bool sameBootIDs = sd_id128_equal(curBootID, prevBootID) != 0;
    if (sameBootIDs && (curTs == prevTs))
    {
        index++;
    }
    else
    {
        // Otherwise, reset it
        index = 0;
    }

    if (!sameBootIDs)
    {
        // Save the bootID
        prevBootID = curBootID;
    }
    // Save the timestamp
    prevTs = curTs;

    // make entryID as <bootID>_<timestamp>[_<index>]
    std::array<char, SD_ID128_STRING_MAX> bootIDStr{};
    sd_id128_to_string(curBootID, bootIDStr.data());
    entryID = std::format("{}_{}", bootIDStr.data(), curTs);
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

// Entry is formed like "BootID_timestamp" or "BootID_timestamp_index"
inline bool
    getTimestampFromID(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& entryID, sd_id128_t& bootID,
                       uint64_t& timestamp, uint64_t& index)
{
    if (entryID.empty())
    {
        return false;
    }

    // Convert the unique ID back to a bootID + timestamp to find the entry
    std::string_view entryIDStrView(entryID);
    auto underscore1Pos = entryIDStrView.find('_');
    if (underscore1Pos == std::string_view::npos)
    {
        // EntryID has no bootID or timestamp
        messages::resourceNotFound(asyncResp->res, "LogEntry", entryID);
        return false;
    }

    // EntryID has bootID + timestamp

    // Convert entryIDViewString to BootID
    // NOTE: bootID string which needs to be null-terminated for
    // sd_id128_from_string()
    std::string bootIDStr(entryID, 0, underscore1Pos);
    if (sd_id128_from_string(bootIDStr.c_str(), &bootID) < 0)
    {
        messages::resourceNotFound(asyncResp->res, "LogEntry", entryID);
        return false;
    }

    // Get the timestamp from entryID
    std::string_view timestampStrView = entryIDStrView;
    timestampStrView.remove_prefix(underscore1Pos + 1);

    // Check the index in timestamp
    auto underscore2Pos = timestampStrView.find('_');
    if (underscore2Pos != std::string_view::npos)
    {
        // Timestamp has an index
        timestampStrView.remove_suffix(timestampStrView.size() -
                                       underscore2Pos);
        std::string_view indexStr(timestampStrView);
        indexStr.remove_prefix(underscore2Pos + 1);
        auto [ptr, ec] = std::from_chars(indexStr.begin(), indexStr.end(),
                                         index);
        if (ec != std::errc())
        {
            messages::resourceNotFound(asyncResp->res, "LogEntry", entryID);
            return false;
        }
    }

    // Now timestamp has no index
    auto [ptr, ec] = std::from_chars(timestampStrView.begin(),
                                     timestampStrView.end(), timestamp);
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
    std::ranges::sort(redfishLogFiles);

    return !redfishLogFiles.empty();
}

inline log_entry::OriginatorTypes
    mapDbusOriginatorTypeToRedfish(const std::string& originatorType)
{
    if (originatorType ==
        "xyz.openbmc_project.Common.OriginatedBy.OriginatorTypes.Client")
    {
        return log_entry::OriginatorTypes::Client;
    }
    if (originatorType ==
        "xyz.openbmc_project.Common.OriginatedBy.OriginatorTypes.Internal")
    {
        return log_entry::OriginatorTypes::Internal;
    }
    if (originatorType ==
        "xyz.openbmc_project.Common.OriginatedBy.OriginatorTypes.SupportingService")
    {
        return log_entry::OriginatorTypes::SupportingService;
    }
    return log_entry::OriginatorTypes::Invalid;
}

inline void parseDumpEntryFromDbusObject(
    const dbus::utility::ManagedObjectType::value_type& object,
    std::string& dumpStatus, uint64_t& size, uint64_t& timestampUs,
    std::string& originatorId, log_entry::OriginatorTypes& originatorType,
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
        else if (interfaceMap.first ==
                 "xyz.openbmc_project.Common.OriginatedBy")
        {
            for (const auto& propertyMap : interfaceMap.second)
            {
                if (propertyMap.first == "OriginatorId")
                {
                    const std::string* id =
                        std::get_if<std::string>(&propertyMap.second);
                    if (id == nullptr)
                    {
                        messages::internalError(asyncResp->res);
                        break;
                    }
                    originatorId = *id;
                }

                if (propertyMap.first == "OriginatorType")
                {
                    const std::string* type =
                        std::get_if<std::string>(&propertyMap.second);
                    if (type == nullptr)
                    {
                        messages::internalError(asyncResp->res);
                        break;
                    }

                    originatorType = mapDbusOriginatorTypeToRedfish(*type);
                    if (originatorType == log_entry::OriginatorTypes::Invalid)
                    {
                        messages::internalError(asyncResp->res);
                        break;
                    }
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
        BMCWEB_LOG_ERROR("getDumpEntriesPath() invalid dump type: {}",
                         dumpType);
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

    sdbusplus::message::object_path path("/xyz/openbmc_project/dump");
    dbus::utility::getManagedObjects(
        "xyz.openbmc_project.Dump.Manager", path,
        [asyncResp, entriesPath,
         dumpType](const boost::system::error_code& ec,
                   const dbus::utility::ManagedObjectType& objects) {
        if (ec)
        {
            BMCWEB_LOG_ERROR("DumpEntry resp_handler got error {}", ec);
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
        if (dumpType == "BMC")
        {
            asyncResp->res.jsonValue["Name"] = "BMC Dump Entries";
            asyncResp->res.jsonValue["Description"] =
                "Collection of BMC Dump Entries";
        }
        else
        {
            asyncResp->res.jsonValue["Name"] = "System Dump Entries";
            asyncResp->res.jsonValue["Description"] =
                "Collection of System Dump Entries";
        }

        nlohmann::json& entriesArray = asyncResp->res.jsonValue["Members"];
        if (entriesArray.empty())
        {
            entriesArray = nlohmann::json::array();
        }
        std::string dumpEntryPath = getDumpPath(dumpType) + "/entry/";

        dbus::utility::ManagedObjectType resp(objects);
        std::ranges::sort(resp, [](const auto& l, const auto& r) {
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
            std::string originatorId;
            log_entry::OriginatorTypes originatorType =
                log_entry::OriginatorTypes::Internal;
            nlohmann::json::object_t thisEntry;

            std::string entryID = object.first.filename();
            if (entryID.empty())
            {
                continue;
            }

            parseDumpEntryFromDbusObject(object, dumpStatus, size, timestampUs,
                                         originatorId, originatorType,
                                         asyncResp);

            if (dumpStatus !=
                    "xyz.openbmc_project.Common.Progress.OperationStatus.Completed" &&
                !dumpStatus.empty())
            {
                // Dump status is not Complete, no need to enumerate
                continue;
            }

            thisEntry["@odata.type"] = "#LogEntry.v1_11_0.LogEntry";
            thisEntry["@odata.id"] = entriesPath + entryID;
            thisEntry["Id"] = entryID;
            thisEntry["EntryType"] = "Event";
            thisEntry["Name"] = dumpType + " Dump Entry";
            thisEntry["Created"] =
                redfish::time_utils::getDateTimeUintUs(timestampUs);
            thisEntry["AdditionalDataURI"] = entriesPath + entryID +
                                             "/attachment";
            thisEntry["AdditionalDataSizeBytes"] = size;

            if (!originatorId.empty())
            {
                thisEntry["Originator"] = originatorId;
                thisEntry["OriginatorType"] = originatorType;
            }

            if (dumpType == "BMC")
            {
                thisEntry["DiagnosticDataType"] = "Manager";
            }

            else if (dumpType == "System")
            {
                thisEntry["DiagnosticDataType"] = "OEM";
                thisEntry["OEMDiagnosticDataType"] = "System";
            }
            entriesArray.emplace_back(std::move(thisEntry));
        }
        asyncResp->res.jsonValue["Members@odata.count"] = entriesArray.size();
        asyncResp->res.jsonValue["Members"] = std::move(entriesArray);
    });
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

    sdbusplus::message::object_path path("/xyz/openbmc_project/dump");
    dbus::utility::getManagedObjects(
        "xyz.openbmc_project.Dump.Manager", path,
        [asyncResp, entryID, dumpType,
         entriesPath](const boost::system::error_code& ec,
                      const dbus::utility::ManagedObjectType& resp) {
        if (ec)
        {
            BMCWEB_LOG_ERROR("DumpEntry resp_handler got error {}", ec);
            messages::internalError(asyncResp->res);
            return;
        }

        bool foundDumpEntry = false;
        std::string dumpEntryPath = getDumpPath(dumpType) + "/entry/";

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
            std::string originatorId;
            log_entry::OriginatorTypes originatorType =
                log_entry::OriginatorTypes::Internal;

            parseDumpEntryFromDbusObject(objectPath, dumpStatus, size,
                                         timestampUs, originatorId,
                                         originatorType, asyncResp);

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
                "#LogEntry.v1_11_0.LogEntry";
            asyncResp->res.jsonValue["@odata.id"] = entriesPath + entryID;
            asyncResp->res.jsonValue["Id"] = entryID;
            asyncResp->res.jsonValue["EntryType"] = "Event";
            asyncResp->res.jsonValue["Name"] = dumpType + " Dump Entry";
            asyncResp->res.jsonValue["Created"] =
                redfish::time_utils::getDateTimeUintUs(timestampUs);
            asyncResp->res.jsonValue["AdditionalDataURI"] =
                entriesPath + entryID + "/attachment";
            asyncResp->res.jsonValue["AdditionalDataSizeBytes"] = size;

            if (!originatorId.empty())
            {
                asyncResp->res.jsonValue["Originator"] = originatorId;
                asyncResp->res.jsonValue["OriginatorType"] = originatorType;
            }

            if (dumpType == "BMC")
            {
                asyncResp->res.jsonValue["DiagnosticDataType"] = "Manager";
            }

            else if (dumpType == "System")
            {
                asyncResp->res.jsonValue["DiagnosticDataType"] = "OEM";
                asyncResp->res.jsonValue["OEMDiagnosticDataType"] = "System";
            }
        }
        if (!foundDumpEntry)
        {
            BMCWEB_LOG_WARNING("Can't find Dump Entry {}", entryID);
            messages::resourceNotFound(asyncResp->res, dumpType + " dump",
                                       entryID);
            return;
        }
    });
}

inline void deleteDumpEntry(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& entryID,
                            const std::string& dumpType)
{
    auto respHandler = [asyncResp,
                        entryID](const boost::system::error_code& ec,
                                 const sdbusplus::message::message& msg) {
        BMCWEB_LOG_DEBUG("Dump Entry doDelete callback: Done");
        if (ec)
        {
            if (ec.value() == EBADR)
            {
                messages::resourceNotFound(asyncResp->res, "LogEntry", entryID);
                return;
            }

            const sd_bus_error* dbusError = msg.get_error();
            if (dbusError == nullptr)
            {
                messages::internalError(asyncResp->res);
                return;
            }

            if (std::string_view(
                    "xyz.openbmc_project.Common.Error.Unavailable") ==
                dbusError->name)
            {
                messages::serviceTemporarilyUnavailable(asyncResp->res, "1");
                return;
            }

            BMCWEB_LOG_ERROR(
                "Dump (DBus) doDelete respHandler got error {} entryID={}", ec,
                entryID);
            messages::internalError(asyncResp->res);
            return;
        }
    };

    crow::connections::systemBus->async_method_call(
        respHandler, "xyz.openbmc_project.Dump.Manager",
        std::format("{}/entry/{}", getDumpPath(dumpType), entryID),
        "xyz.openbmc_project.Object.Delete", "Delete");
}
inline bool checkSizeLimit(int fd, crow::Response& res)
{
    long long int size = lseek(fd, 0, SEEK_END);
    if (size <= 0)
    {
        BMCWEB_LOG_ERROR("Failed to get size of file, lseek() returned {}",
                         size);
        messages::internalError(res);
        return false;
    }

    // Arbitrary max size of 20MB to accommodate BMC dumps
    constexpr long long int maxFileSize = 20LL * 1024LL * 1024LL;
    if (size > maxFileSize)
    {
        BMCWEB_LOG_ERROR("File size {} exceeds maximum allowed size of {}",
                         size, maxFileSize);
        messages::internalError(res);
        return false;
    }
    off_t rc = lseek(fd, 0, SEEK_SET);
    if (rc < 0)
    {
        BMCWEB_LOG_ERROR("Failed to reset file offset to 0");
        messages::internalError(res);
        return false;
    }
    return true;
}
inline void
    downloadEntryCallback(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& entryID,
                          const std::string& downloadEntryType,
                          const boost::system::error_code& ec,
                          const sdbusplus::message::unix_fd& unixfd)
{
    if (ec.value() == EBADR)
    {
        messages::resourceNotFound(asyncResp->res, "EntryAttachment", entryID);
        return;
    }
    if (ec)
    {
        BMCWEB_LOG_ERROR("DBUS response error: {}", ec);
        messages::internalError(asyncResp->res);
        return;
    }

    // Make sure we know how to process the retrieved entry attachment
    if ((downloadEntryType != "BMC") && (downloadEntryType != "System"))
    {
        BMCWEB_LOG_ERROR("downloadEntryCallback() invalid entry type: {}",
                         downloadEntryType);
        messages::internalError(asyncResp->res);
    }

    int fd = -1;
    fd = dup(unixfd);
    if (fd < 0)
    {
        BMCWEB_LOG_ERROR("Failed to open file");
        messages::internalError(asyncResp->res);
        return;
    }
    if (!checkSizeLimit(fd, asyncResp->res))
    {
        close(fd);
        return;
    }
    if (downloadEntryType == "System")
    {
        if (!asyncResp->res.openFd(fd, bmcweb::EncodingType::Base64))
        {
            messages::internalError(asyncResp->res);
            close(fd);
            return;
        }
        asyncResp->res.addHeader(
            boost::beast::http::field::content_transfer_encoding, "Base64");
        return;
    }
    if (!asyncResp->res.openFd(fd))
    {
        messages::internalError(asyncResp->res);
        close(fd);
        return;
    }
    asyncResp->res.addHeader(boost::beast::http::field::content_type,
                             "application/octet-stream");
}

inline void
    downloadDumpEntry(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const std::string& entryID, const std::string& dumpType)
{
    if (dumpType != "BMC")
    {
        BMCWEB_LOG_WARNING("Can't find Dump Entry {}", entryID);
        messages::resourceNotFound(asyncResp->res, dumpType + " dump", entryID);
        return;
    }

    std::string dumpEntryPath = std::format("{}/entry/{}",
                                            getDumpPath(dumpType), entryID);

    auto downloadDumpEntryHandler =
        [asyncResp, entryID,
         dumpType](const boost::system::error_code& ec,
                   const sdbusplus::message::unix_fd& unixfd) {
        downloadEntryCallback(asyncResp, entryID, dumpType, ec, unixfd);
    };

    crow::connections::systemBus->async_method_call(
        std::move(downloadDumpEntryHandler), "xyz.openbmc_project.Dump.Manager",
        dumpEntryPath, "xyz.openbmc_project.Dump.Entry", "GetFileHandle");
}

inline void
    downloadEventLogEntry(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& systemName,
                          const std::string& entryID,
                          const std::string& dumpType)
{
    if constexpr (bmcwebEnableMultiHost)
    {
        // Option currently returns no systems.  TBD
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    if (systemName != "system")
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    std::string entryPath =
        sdbusplus::message::object_path("/xyz/openbmc_project/logging/entry") /
        entryID;

    auto downloadEventLogEntryHandler =
        [asyncResp, entryID,
         dumpType](const boost::system::error_code& ec,
                   const sdbusplus::message::unix_fd& unixfd) {
        downloadEntryCallback(asyncResp, entryID, dumpType, ec, unixfd);
    };

    crow::connections::systemBus->async_method_call(
        std::move(downloadEventLogEntryHandler), "xyz.openbmc_project.Logging",
        entryPath, "xyz.openbmc_project.Logging.Entry", "GetEntry");
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
    getDumpCompletionStatus(const dbus::utility::DBusPropertiesMap& values,
                            const std::shared_ptr<task::TaskData>& taskData)
{
    for (const auto& [key, val] : values)
    {
        if (key == "Status")
        {
            const std::string* value = std::get_if<std::string>(&val);
            if (value == nullptr)
            {
                BMCWEB_LOG_ERROR("Status property value is null");
                return DumpCreationProgress::DUMP_CREATE_FAILED;
            }
            return mapDbusStatusToDumpProgress(*value);
        }
        // Only resource dumps will implement the interface with this
        // property. Hence the below if statement will be hit for
        // all the resource dumps only
        if (key == "DumpRequestStatus")
        {
            const std::string* value = std::get_if<std::string>(&val);
            if (value == nullptr)
            {
                BMCWEB_LOG_ERROR("DumpRequestStatus property value is null");
                taskData->messages.emplace_back(messages::internalError());
                return DumpCreationProgress::DUMP_CREATE_FAILED;
            }
            if ((*value).ends_with("PermissionDenied"))
            {
                BMCWEB_LOG_WARNING("DumpRequestStatus: Permission denied");
                taskData->messages.emplace_back(
                    messages::insufficientPrivilege());
                return DumpCreationProgress::DUMP_CREATE_FAILED;
            }
            if ((*value).ends_with("AcfFileInvalid") ||
                (*value).ends_with("PasswordInvalid"))
            {
                BMCWEB_LOG_WARNING(
                    "DumpRequestStatus: ACFFile Invalid/Password Invalid");
                taskData->messages.emplace_back(
                    messages::resourceAtUriUnauthorized(
                        boost::urls::url_view(taskData->payload->targetUri),
                        "Invalid Password/ACF File Invalid"));
                return DumpCreationProgress::DUMP_CREATE_FAILED;
            }
            if ((*value).ends_with("ResourceSelectorInvalid"))
            {
                BMCWEB_LOG_WARNING(
                    "DumpRequestStatus: Resource selector Invalid");
                taskData->messages.emplace_back(
                    messages::actionParameterUnknown("CollectDiagnosticData",
                                                     "Resource selector"));
                return DumpCreationProgress::DUMP_CREATE_FAILED;
            }
            if ((*value).ends_with("Success"))
            {
                taskData->state = "Running";
                return DumpCreationProgress::DUMP_CREATE_INPROGRESS;
            }
            return DumpCreationProgress::DUMP_CREATE_INPROGRESS;
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
        BMCWEB_LOG_ERROR("Invalid dump type received");
        messages::internalError(asyncResp->res);
        return;
    }

    crow::connections::systemBus->async_method_call(
        [asyncResp, payload = std::move(payload), createdObjPath,
         dumpEntryPath{std::move(dumpEntryPath)},
         dumpId](const boost::system::error_code& ec,
                 const std::string& introspectXml) {
        if (ec)
        {
            BMCWEB_LOG_ERROR("Introspect call failed with error: {}",
                             ec.message());
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
            BMCWEB_LOG_ERROR("XML document failed to parse");
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
                const boost::system::error_code& ec2, sdbusplus::message_t& msg,
                const std::shared_ptr<task::TaskData>& taskData) {
            if (ec2)
            {
                BMCWEB_LOG_ERROR("{}: Error in creating dump",
                                 createdObjPath.str);
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
                    getDumpCompletionStatus(values, taskData);
                if (dumpStatus == DumpCreationProgress::DUMP_CREATE_FAILED)
                {
                    BMCWEB_LOG_ERROR("{}: Error in creating dump",
                                     createdObjPath.str);
                    taskData->state = "Cancelled";
                    return task::completed;
                }

                if (dumpStatus == DumpCreationProgress::DUMP_CREATE_INPROGRESS)
                {
                    BMCWEB_LOG_DEBUG("{}: Dump creation task is in progress",
                                     createdObjPath.str);
                    return !task::completed;
                }
            }

            nlohmann::json retMessage = messages::success();
            taskData->messages.emplace_back(retMessage);

            boost::urls::url url;
            if ((createdObjPath.str).find("/system/") != std::string::npos)
            {
                url = boost::urls::format(
                    "/redfish/v1/Systems/system/LogServices/Dump/Entries/{}",
                    dumpId);
            }
            else if ((createdObjPath.str).find("/bmc/") != std::string::npos)
            {
                url = boost::urls::format(
                    "/redfish/v1/Managers/bmc/LogServices/Dump/Entries/{}",
                    dumpId);
            }
            std::string headerLoc = "Location: ";
            headerLoc += url.buffer();

            taskData->payload->httpHeaders.emplace_back(std::move(headerLoc));

            BMCWEB_LOG_DEBUG("{}: Dump creation task completed",
                             createdObjPath.str);
            taskData->state = "Completed";
            return task::completed;
        },
            "type='signal',interface='org.freedesktop.DBus.Properties',"
            "member='PropertiesChanged',path='" +
                createdObjPath.str + "'");

        // The task timer is set to max time limit within which the
        // requested dump will be collected.
        task->startTimer(std::chrono::minutes(20));
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

    std::vector<std::pair<std::string, std::variant<std::string, uint64_t>>>
        createDumpParams;
    std::string createDumpType;

    if (dumpType == "System")
    {
        if (!oemDiagnosticDataType || !diagnosticDataType)
        {
            BMCWEB_LOG_ERROR(
                "CreateDump action parameter 'DiagnosticDataType'/'OEMDiagnosticDataType' value not found!");
            messages::actionParameterMissing(
                asyncResp->res, "CollectDiagnosticData",
                "DiagnosticDataType & OEMDiagnosticDataType");
            return;
        }

        if ((*oemDiagnosticDataType != "System") &&
            (*diagnosticDataType != "OEM"))
        {
            BMCWEB_LOG_WARNING("Wrong parameter values passed");
            messages::invalidObject(
                asyncResp->res,
                boost::urls::format(
                    "/redfish/v1/Systems/system/LogServices/Dump/Actions/LogService.CollectDiagnosticData"));
            return;
        }
        if ((*oemDiagnosticDataType).starts_with("Resource"))
        {
            std::string resourceDumpType = *oemDiagnosticDataType;
            std::vector<std::variant<std::string, uint64_t>> resourceDumpParams;

            size_t pos = 0;
            while ((pos = resourceDumpType.find('_')) != std::string::npos)
            {
                resourceDumpParams.emplace_back(
                    resourceDumpType.substr(0, pos));
                if (resourceDumpParams.size() > 3)
                {
                    BMCWEB_LOG_WARNING(
                        "Invalid value for OEMDiagnosticDataType");
                    messages::invalidObject(
                        asyncResp->res,
                        boost::urls::format(
                            "/redfish/v1/Systems/system/LogServices/Dump/Actions/LogService.CollectDiagnosticData"));
                    return;
                }
                resourceDumpType.erase(0, pos + 1);
            }
            resourceDumpParams.emplace_back(resourceDumpType);

            if (resourceDumpParams.size() >= 2)
            {
                createDumpParams.emplace_back(
                    "com.ibm.Dump.Create.CreateParameters.VSPString",
                    resourceDumpParams[1]);
            }
            if (resourceDumpParams.size() == 3)
            {
                createDumpParams.emplace_back(
                    "com.ibm.Dump.Create.CreateParameters.Password",
                    resourceDumpParams[2]);
            }

            if (resourceDumpParams.size() > 3)
            {
                BMCWEB_LOG_WARNING("Invalid value for OEMDiagnosticDataType");
                messages::invalidObject(
                    asyncResp->res,
                    boost::urls::format(
                        "/redfish/v1/Systems/system/LogServices/Dump/Actions/LogService.CollectDiagnosticData"));
                return;
            }
            createDumpParams.emplace_back(
                "com.ibm.Dump.Create.CreateParameters.DumpType",
                "com.ibm.Dump.Create.DumpType.Resource");
        }
        else
        {
            if (*oemDiagnosticDataType != "System")
            {
                BMCWEB_LOG_WARNING("Invalid parameter values passed");
                messages::invalidObject(
                    asyncResp->res,
                    boost::urls::format(
                        "/redfish/v1/Systems/system/LogServices/Dump/Actions/LogService.CollectDiagnosticData"));
                return;
            }
            createDumpParams.emplace_back(
                "com.ibm.Dump.Create.CreateParameters.DumpType",
                "com.ibm.Dump.Create.DumpType.System");
        }
        createDumpType = "System";
        dumpPath = "/redfish/v1/Systems/system/LogServices/Dump/";
    }
    else if (dumpType == "BMC")
    {
        createDumpType = "BMC";
        if (!diagnosticDataType)
        {
            BMCWEB_LOG_ERROR(
                "CreateDump action parameter 'DiagnosticDataType' not found!");
            messages::actionParameterMissing(
                asyncResp->res, "CollectDiagnosticData", "DiagnosticDataType");
            return;
        }
        if (*diagnosticDataType != "Manager")
        {
            BMCWEB_LOG_ERROR(
                "Wrong parameter value passed for 'DiagnosticDataType'");
            messages::internalError(asyncResp->res);
            return;
        }
        if (oemDiagnosticDataType)
        {
            if (*oemDiagnosticDataType == "FaultData")
            {
                // Fault data dump
                createDumpParams.emplace_back(
                    "xyz.openbmc_project.Dump.Internal.Create.Type",
                    "xyz.openbmc_project.Dump.Internal.Create.Type.FaultData");
            }
            else
            {
                BMCWEB_LOG_WARNING("Invalid value for OEMDiagnosticDataType");
                messages::invalidObject(
                    asyncResp->res,
                    boost::urls::format(
                        "/redfish/v1/Managers/bmc/LogServices/Dump/Actions/LogService.CollectDiagnosticData"));
                return;
            }
        }
        dumpPath = "/redfish/v1/Managers/bmc/LogServices/Dump/";
    }
    else
    {
        BMCWEB_LOG_ERROR("CreateDump failed. Unknown dump type");
        messages::internalError(asyncResp->res);
        return;
    }

    createDumpParams.emplace_back(
        "xyz.openbmc_project.Dump.Create.CreateParameters.OriginatorId",
        req.session->clientIp);

    std::vector<std::pair<std::string, std::variant<std::string, uint64_t>>>
        createDumpParamVec(createDumpParams);

    if (req.session != nullptr)
    {
        createDumpParamVec.emplace_back(
            "xyz.openbmc_project.Dump.Create.CreateParameters.OriginatorId",
            req.session->clientIp);
        createDumpParamVec.emplace_back(
            "xyz.openbmc_project.Dump.Create.CreateParameters.OriginatorType",
            "xyz.openbmc_project.Common.OriginatedBy.OriginatorTypes.Client");
    }

    crow::connections::systemBus->async_method_call(
        [asyncResp, payload(task::Payload(req)),
         dumpPath](const boost::system::error_code& ec,
                   const sdbusplus::message_t& msg,
                   const sdbusplus::message::object_path& objPath) mutable {
        if (ec)
        {
            BMCWEB_LOG_ERROR("CreateDump resp_handler got error {}", ec);
            const sd_bus_error* dbusError = msg.get_error();
            if (dbusError == nullptr)
            {
                messages::internalError(asyncResp->res);
                return;
            }

            BMCWEB_LOG_ERROR("CreateDump DBus error: {} and error msg: {}",
                             dbusError->name, dbusError->message);
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
            if (std::string_view("org.freedesktop.DBus.Error.NoReply") ==
                dbusError->name)
            {
                // This will be returned as a result of createDump call
                // made when the dump manager is not responding.
                messages::serviceTemporarilyUnavailable(asyncResp->res, "60");
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
        BMCWEB_LOG_DEBUG("Dump Created. Path: {}", objPath.str);
        createDumpTaskCallback(std::move(payload), asyncResp, objPath);
    },
        "xyz.openbmc_project.Dump.Manager", getDumpPath(createDumpType),
        "xyz.openbmc_project.Dump.Create", "CreateDump", createDumpParamVec);
}

inline void clearDump(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const std::string& dumpType)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code& ec) {
        if (ec)
        {
            BMCWEB_LOG_ERROR("clearDump resp_handler got error {}", ec);
            messages::internalError(asyncResp->res);
            return;
        }
    },
        "xyz.openbmc_project.Dump.Manager", getDumpPath(dumpType),
        "xyz.openbmc_project.Collection.DeleteAll", "DeleteAll");
}

inline void
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
        if constexpr (bmcwebEnableMultiHost)
        {
            // Option currently returns no systems.  TBD
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
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
        logServiceArray.emplace_back(std::move(eventLog));
#ifdef BMCWEB_ENABLE_REDFISH_DUMP_LOG
        nlohmann::json::object_t dumpLog;
        dumpLog["@odata.id"] = "/redfish/v1/Systems/system/LogServices/Dump";
        logServiceArray.emplace_back(std::move(dumpLog));
#endif

#ifdef BMCWEB_ENABLE_REDFISH_CPU_LOG
        nlohmann::json::object_t crashdump;
        crashdump["@odata.id"] =
            "/redfish/v1/Systems/system/LogServices/Crashdump";
        logServiceArray.emplace_back(std::move(crashdump));
#endif

#ifdef BMCWEB_ENABLE_REDFISH_HOST_LOGGER
        nlohmann::json::object_t hostlogger;
        hostlogger["@odata.id"] =
            "/redfish/v1/Systems/system/LogServices/HostLogger";
        logServiceArray.emplace_back(std::move(hostlogger));
#endif

#ifdef BMCWEB_ENABLE_REDFISH_DBUS_LOG_ENTRIES
        Privileges effectiveUserPrivileges =
            redfish::getUserPrivileges(*req.session);

        if (isOperationAllowedWithPrivileges({{"ConfigureManager"}},
                                             effectiveUserPrivileges))
        {
            nlohmann::json::object_t item;
            item["@odata.id"] = "/redfish/v1/Systems/system/LogServices/CELog";
            logServiceArray.emplace_back(std::move(item));
        }
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
                BMCWEB_LOG_ERROR("{}", ec);
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

                    logServiceArrayLocal.emplace_back(std::move(member));

                    asyncResp->res.jsonValue["Members@odata.count"] =
                        logServiceArrayLocal.size();
                    return;
                }
            }
        });

#ifdef BMCWEB_ENABLE_LINUX_AUDIT_EVENTS
        nlohmann::json::object_t auditLog;
        auditLog["@odata.id"] =
            "/redfish/v1/Systems/system/LogServices/AuditLog";
        logServiceArray.push_back(std::move(auditLog));
#endif // BMCWEB_ENABLE_LINUX_AUDIT_EVENTS
#ifdef BMCWEB_ENABLE_HW_ISOLATION
        nlohmann::json& logServiceArrayLocal =
            asyncResp->res.jsonValue["Members"];
        logServiceArrayLocal.push_back(
            {{"@odata.id", "/redfish/v1/Systems/system/"
                           "LogServices/HardwareIsolation"}});
        asyncResp->res.jsonValue["Members@odata.count"] =
            logServiceArrayLocal.size();
#endif // BMCWEB_ENABLE_HW_ISOLATION
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
            "#LogService.v1_2_0.LogService";
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

inline void requestRoutesCELogService(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/LogServices/CELog/")
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
            "/redfish/v1/Systems/system/LogServices/CELog";
        asyncResp->res.jsonValue["@odata.type"] =
            "#LogService.v1_1_0.LogService";
        asyncResp->res.jsonValue["Name"] = "CE Log Service";
        asyncResp->res.jsonValue["Description"] = "System CE Log Service";
        asyncResp->res.jsonValue["Id"] = "CELog";
        asyncResp->res.jsonValue["OverWritePolicy"] = "WrapsWhenFull";

        std::pair<std::string, std::string> redfishDateTimeOffset =
            redfish::time_utils::getDateTimeOffsetNow();

        asyncResp->res.jsonValue["DateTime"] = redfishDateTimeOffset.first;
        asyncResp->res.jsonValue["DateTimeLocalOffset"] =
            redfishDateTimeOffset.second;

        asyncResp->res.jsonValue["Entries"]["@odata.id"] =
            "/redfish/v1/Systems/system/LogServices/CELog/Entries";
        asyncResp->res.jsonValue["Actions"]["#LogService.ClearLog"]["target"] =
            "/redfish/v1/Systems/system/LogServices/CELog/Actions/LogService.ClearLog";
    });
}

inline void requestRoutesJournalEventLogClear(App& app)
{
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Systems/<str>/LogServices/EventLog/Actions/LogService.ClearLog/")
        .privileges(redfish::privileges::
                        postLogServiceSubOverComputerSystemLogServiceCollection)
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
                BMCWEB_LOG_ERROR("Failed to reload rsyslog: {}", ec);
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
    auto logEntryIter = logEntryFields.begin();
    if (logEntryIter == logEntryFields.end())
    {
        return LogParseError::parseFailed;
    }
    std::string& messageID = *logEntryIter;
    // Get the Message from the MessageRegistry
    const registries::Message* message = registries::getMessage(messageID);

    logEntryIter++;
    if (message == nullptr)
    {
        BMCWEB_LOG_WARNING("Log entry not found in registry: {}", logEntry);
        return LogParseError::messageIdNotInRegistry;
    }

    std::vector<std::string_view> messageArgs(logEntryIter,
                                              logEntryFields.end());
    messageArgs.resize(message->numberOfArgs);

    std::string msg = redfish::registries::fillMessageArgs(messageArgs,
                                                           message->message);
    if (msg.empty())
    {
        return LogParseError::parseFailed;
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
    logEntryJson["@odata.id"] = boost::urls::format(
        "/redfish/v1/Systems/system/LogServices/EventLog/Entries/{}",
        logEntryID);
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
        if constexpr (bmcwebEnableMultiHost)
        {
            // Option currently returns no systems.  TBD
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
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
                LogParseError status = fillEventLogEntryJson(idStr, logEntry,
                                                             bmcLogEntry);
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

                logEntryArray.emplace_back(std::move(bmcLogEntry));
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
        if constexpr (bmcwebEnableMultiHost)
        {
            // Option currently returns no systems.  TBD
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
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

template <typename Callback>
void getHiddenPropertyValue(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& entryId, Callback&& callback)
{
    sdbusplus::asio::getProperty<bool>(
        *crow::connections::systemBus, "xyz.openbmc_project.Logging",
        "/xyz/openbmc_project/logging/entry/" + entryId,
        "org.open_power.Logging.PEL.Entry", "Hidden",
        [callback{std::forward<Callback>(callback)},
         asyncResp](const boost::system::error_code& ec, bool hidden) {
        if (ec)
        {
            BMCWEB_LOG_ERROR("DBUS response error: {}", ec);
            messages::internalError(asyncResp->res);
            return;
        }
        callback(hidden);
    });
}

inline void updateProperty(const std::optional<bool>& resolved,
                           const std::optional<bool>& managementSystemAck,
                           const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const std::string& entryId)
{
    if (resolved.has_value())
    {
        sdbusplus::asio::setProperty(
            *crow::connections::systemBus, "xyz.openbmc_project.Logging",
            "/xyz/openbmc_project/logging/entry/" + entryId,
            "xyz.openbmc_project.Logging.Entry", "Resolved", *resolved,
            [asyncResp](const boost::system::error_code& ec) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("DBUS response error {}", ec);
                messages::internalError(asyncResp->res);
                return;
            }
        });
        BMCWEB_LOG_DEBUG("Set Resolved");
    }

    if (managementSystemAck.has_value())
    {
        sdbusplus::asio::setProperty(
            *crow::connections::systemBus, "xyz.openbmc_project.Logging",
            "/xyz/openbmc_project/logging/entry/" + entryId,
            "org.open_power.Logging.PEL.Entry", "ManagementSystemAck",
            *managementSystemAck,
            [asyncResp](const boost::system::error_code& ec) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("DBUS response error {}", ec);
                messages::internalError(asyncResp->res);
                return;
            }
        });
        BMCWEB_LOG_DEBUG("Updated ManagementSystemAck Property");
    }
}

inline void
    deleteEventLogEntry(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& entryId)
{
    // Process response from Logging service.
    auto respHandler = [asyncResp,
                        entryId](const boost::system::error_code& ec) {
        BMCWEB_LOG_DEBUG("EventLogEntry (DBus) doDelete callback: Done");
        if (ec)
        {
            if (ec.value() == EBADR)
            {
                messages::resourceNotFound(asyncResp->res, "LogEntry", entryId);
                return;
            }
            // TODO Handle for specific error code
            BMCWEB_LOG_ERROR("EventLogEntry (DBus) doDelete "
                             "respHandler got error {}",
                             ec);
            messages::internalError(asyncResp->res);
            return;
        }
        asyncResp->res.result(boost::beast::http::status::ok);
    };

    // Make call to Logging service to request Delete Log
    crow::connections::systemBus->async_method_call(
        respHandler, "xyz.openbmc_project.Logging",
        "/xyz/openbmc_project/logging/entry/" + entryId,
        "xyz.openbmc_project.Object.Delete", "Delete");
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
        if constexpr (bmcwebEnableMultiHost)
        {
            // Option currently returns no systems.  TBD
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
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
        sdbusplus::message::object_path path("/xyz/openbmc_project/logging");
        dbus::utility::getManagedObjects(
            "xyz.openbmc_project.Logging", path,
            [asyncResp](const boost::system::error_code& ec,
                        const dbus::utility::ManagedObjectType& resp) {
            if (ec)
            {
                // TODO Handle for specific error code
                BMCWEB_LOG_ERROR(
                    "getLogEntriesIfaceData resp_handler got error {}", ec);
                messages::internalError(asyncResp->res);
                return;
            }
            nlohmann::json::array_t entriesArray;
            for (const auto& objectPath : resp)
            {
                const uint32_t* id = nullptr;
                const uint64_t* timestamp = nullptr;
                const uint64_t* updateTimestamp = nullptr;
                const std::string* severity = nullptr;
                const std::string* subsystem = nullptr;
                const std::string* filePath = nullptr;
                const std::string* resolution = nullptr;
                const std::string* eventId = nullptr;
                const bool* resolved = nullptr;
                const std::string* notify = nullptr;
                const bool* hidden = nullptr;
#ifdef BMCWEB_ENABLE_IBM_MANAGEMENT_CONSOLE
                const bool* managementSystemAck = nullptr;
#endif

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
                            else if (propertyMap.first == "EventId")
                            {
                                eventId = std::get_if<std::string>(
                                    &propertyMap.second);
                                if (eventId == nullptr)
                                {
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                            }
                            else if (propertyMap.first == "Resolved")
                            {
                                resolved =
                                    std::get_if<bool>(&propertyMap.second);
                                if (resolved == nullptr)
                                {
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
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
                        if (id == nullptr || severity == nullptr)
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
                    else if (interfaceMap.first ==
                             "org.open_power.Logging.PEL.Entry")
                    {
                        for (const auto& propertyMap : interfaceMap.second)
                        {
                            if (propertyMap.first == "Hidden")
                            {
                                hidden = std::get_if<bool>(&propertyMap.second);
                                if (hidden == nullptr)
                                {
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                            }
                            else if (propertyMap.first == "Subsystem")
                            {
                                subsystem = std::get_if<std::string>(
                                    &propertyMap.second);
                                if (subsystem == nullptr)
                                {
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                            }
#ifdef BMCWEB_ENABLE_IBM_MANAGEMENT_CONSOLE
                            else if (propertyMap.first == "ManagementSystemAck")
                            {
                                managementSystemAck =
                                    std::get_if<bool>(&propertyMap.second);
                                if (managementSystemAck == nullptr)
                                {
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                            }
#endif
                        }
                    }
                }
                // Object path without the
                // xyz.openbmc_project.Logging.Entry interface, ignore
                // and continue.
                if (id == nullptr || eventId == nullptr ||
                    severity == nullptr || timestamp == nullptr ||
                    updateTimestamp == nullptr || hidden == nullptr ||
                    subsystem == nullptr)
                {
                    continue;
                }

                // Hidden logs are part of CELogs, ignore and continue.
                if (*hidden)
                {
                    continue;
                }

                nlohmann::json& thisEntry = entriesArray.emplace_back();
                thisEntry["@odata.type"] = "#LogEntry.v1_9_0.LogEntry";
                thisEntry["@odata.id"] = boost::urls::format(
                    "/redfish/v1/Systems/system/LogServices/EventLog/Entries/{}",
                    std::to_string(*id));
                thisEntry["Name"] = "System Event Log Entry";
                thisEntry["Id"] = std::to_string(*id);
                thisEntry["EventId"] = *eventId;
                thisEntry["Message"] = (*eventId).substr(0, 8) +
                                       " event in subsystem: " + *subsystem;
                thisEntry["Resolved"] = *resolved;
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
                thisEntry["Oem"]["IBM"]["@odata.id"] = boost::urls::format(
                    "/redfish/v1/Systems/system/LogServices/EventLog/Entries/{}/OemPelAttachment",
                    std::to_string(*id));
                if (filePath != nullptr)
                {
                    thisEntry["AdditionalDataURI"] =
                        "/redfish/v1/Systems/system/LogServices/EventLog/Entries/" +
                        std::to_string(*id) + "/attachment";
                }
#ifdef BMCWEB_ENABLE_IBM_MANAGEMENT_CONSOLE
                thisEntry["Oem"]["OpenBMC"]["@odata.type"] =
                    "#OemLogEntry.v1_0_0.LogEntry";
                thisEntry["Oem"]["OpenBMC"]["ManagementSystemAck"] =
                    *managementSystemAck;
#endif
            }
            std::ranges::sort(entriesArray, [](const nlohmann::json& left,
                                               const nlohmann::json& right) {
                return (left["Id"] <= right["Id"]);
            });
            asyncResp->res.jsonValue["Members@odata.count"] =
                entriesArray.size();
            asyncResp->res.jsonValue["Members"] = std::move(entriesArray);
        });
    });
}

inline void requestRoutesDBusCELogEntryCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/LogServices/CELog/Entries/")
        .privileges(redfish::privileges::getLogEntryCollection)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemName) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        if constexpr (bmcwebEnableMultiHost)
        {
            // Option currently returns no systems.  TBD
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
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
            "/redfish/v1/Systems/system/LogServices/CELog/Entries";
        asyncResp->res.jsonValue["Name"] = "System Event Log Entries";
        asyncResp->res.jsonValue["Description"] =
            "Collection of System Event Log Entries";

        // DBus implementation of CELog/Entries
        // Make call to Logging Service to find all log entry objects
        sdbusplus::message::object_path path("/xyz/openbmc_project/logging");
        dbus::utility::getManagedObjects(
            "xyz.openbmc_project.Logging", path,
            [asyncResp](const boost::system::error_code& ec,
                        const dbus::utility::ManagedObjectType& resp) {
            if (ec)
            {
                // TODO Handle for specific error code
                BMCWEB_LOG_ERROR(
                    "getLogEntriesIfaceData resp_handler got error, {}", ec);
                messages::internalError(asyncResp->res);
                return;
            }
            nlohmann::json::array_t entriesArray;
            for (const auto& objectPath : resp)
            {
                const uint32_t* id = nullptr;
                const uint64_t* timestamp = nullptr;
                const uint64_t* updateTimestamp = nullptr;
                const std::string* severity = nullptr;
                const std::string* subsystem = nullptr;
                const std::string* filePath = nullptr;
                const std::string* eventId = nullptr;
                const std::string* resolution = nullptr;
                const bool* resolved = nullptr;
                const bool* hidden = nullptr;
                const std::string* notify = nullptr;
#ifdef BMCWEB_ENABLE_IBM_MANAGEMENT_CONSOLE
                bool managementSystemAck = false;
#endif

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
                            else if (propertyMap.first == "EventId")
                            {
                                eventId = std::get_if<std::string>(
                                    &propertyMap.second);
                                if (eventId == nullptr)
                                {
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                            }
                            else if (propertyMap.first == "Resolved")
                            {
                                resolved =
                                    std::get_if<bool>(&propertyMap.second);
                                if (resolved == nullptr)
                                {
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
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
                        if (id == nullptr || severity == nullptr)
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
                    else if (interfaceMap.first ==
                             "org.open_power.Logging.PEL.Entry")
                    {
                        for (const auto& propertyMap : interfaceMap.second)
                        {
                            if (propertyMap.first == "Hidden")
                            {
                                hidden = std::get_if<bool>(&propertyMap.second);
                                if (hidden == nullptr)
                                {
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                            }
                            else if (propertyMap.first == "Subsystem")
                            {
                                subsystem = std::get_if<std::string>(
                                    &propertyMap.second);
                                if (subsystem == nullptr)
                                {
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                            }
#ifdef BMCWEB_ENABLE_IBM_MANAGEMENT_CONSOLE
                            else if (propertyMap.first == "ManagementSystemAck")
                            {
                                const bool* managementSystemAckptr =
                                    std::get_if<bool>(&propertyMap.second);
                                if (managementSystemAckptr == nullptr)
                                {
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                                managementSystemAck = *managementSystemAckptr;
                            }
#endif
                        }
                    }
                }
                // Object path without the
                // xyz.openbmc_project.Logging.Entry interface, ignore
                // and continue.
                if (id == nullptr || eventId == nullptr ||
                    severity == nullptr || timestamp == nullptr ||
                    updateTimestamp == nullptr || hidden == nullptr ||
                    subsystem == nullptr)
                {
                    continue;
                }

                // Part of Event Logs, ignore and continue
                if (!(*hidden))
                {
                    continue;
                }

                nlohmann::json& thisEntry = entriesArray.emplace_back();
                thisEntry["@odata.type"] = "#LogEntry.v1_9_0.LogEntry";
                thisEntry["@odata.id"] = boost::urls::format(
                    "/redfish/v1/Systems/system/LogServices/CELog/Entries/{}",
                    std::to_string(*id));
                thisEntry["Name"] = "System Event Log Entry";
                thisEntry["Id"] = std::to_string(*id);
                thisEntry["EventId"] = *eventId;
                thisEntry["Message"] = (*eventId).substr(0, 8) +
                                       " event in subsystem: " + *subsystem;
                thisEntry["Resolved"] = *resolved;
                if ((resolution != nullptr) && (!(*resolution).empty()))
                {
                    thisEntry["Resolution"] = *resolution;
                }
                thisEntry["EntryType"] = "Event";
                thisEntry["Severity"] =
                    translateSeverityDbusToRedfish(*severity);
                thisEntry["Created"] =
                    redfish::time_utils::getDateTimeUintMs(*timestamp);
                thisEntry["Modified"] =
                    redfish::time_utils::getDateTimeUintMs(*updateTimestamp);
                std::optional<bool> notifyAction =
                    getProviderNotifyAction(*notify);
                if (notifyAction)
                {
                    thisEntry["ServiceProviderNotified"] = *notifyAction;
                }
                thisEntry["Oem"]["IBM"]["@odata.id"] = boost::urls::format(
                    "/redfish/v1/Systems/system/LogServices/CELog/Entries/{}/OemPelAttachment",
                    std::to_string(*id));
                if (filePath != nullptr)
                {
                    thisEntry["AdditionalDataURI"] = boost::urls::format(
                        "/redfish/v1/Systems/system/LogServices/CELog/Entries/{}/attachment",
                        std::to_string(*id));
                }
#ifdef BMCWEB_ENABLE_IBM_MANAGEMENT_CONSOLE
                thisEntry["Oem"]["OpenBMC"]["@odata.type"] =
                    "#OemLogEntry.v1_0_0.LogEntry";
                thisEntry["Oem"]["OpenBMC"]["ManagementSystemAck"] =
                    managementSystemAck;
#endif
            }
            std::ranges::sort(entriesArray, [](const nlohmann::json& left,
                                               const nlohmann::json& right) {
                return (left["Id"] <= right["Id"]);
            });
            asyncResp->res.jsonValue["Members@odata.count"] =
                entriesArray.size();
            asyncResp->res.jsonValue["Members"] = std::move(entriesArray);
        });
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
        if constexpr (bmcwebEnableMultiHost)
        {
            // Option currently returns no systems.  TBD
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
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
                BMCWEB_LOG_ERROR(
                    "EventLogEntry (DBus) resp_handler got error {}", ec);
                messages::internalError(asyncResp->res);
                return;
            }
            const uint32_t* id = nullptr;
            const uint64_t* timestamp = nullptr;
            const uint64_t* updateTimestamp = nullptr;
            const std::string* severity = nullptr;
            const std::string* eventId = nullptr;
            const std::string* subsystem = nullptr;
            const std::string* filePath = nullptr;
            const std::string* resolution = nullptr;
            const bool* resolved = nullptr;
            const std::string* notify = nullptr;
            const bool* hidden = nullptr;
#ifdef BMCWEB_ENABLE_IBM_MANAGEMENT_CONSOLE
            const bool* managementSystemAck = nullptr;
#endif

            const bool success = sdbusplus::unpackPropertiesNoThrow(
                dbus_utils::UnpackErrorPrinter(), resp, "Id", id, "Timestamp",
                timestamp, "UpdateTimestamp", updateTimestamp, "Severity",
                severity, "EventId", eventId, "Resolved", resolved,
                "Resolution", resolution, "Path", filePath, "Hidden", hidden,
                "ServiceProviderNotify", notify, "Subsystem", subsystem
#ifdef BMCWEB_ENABLE_IBM_MANAGEMENT_CONSOLE
                ,
                "ManagementSystemAck", managementSystemAck
#endif
            );

            if (!success)
            {
                messages::internalError(asyncResp->res);
                return;
            }

            if (id == nullptr || eventId == nullptr || severity == nullptr ||
                timestamp == nullptr || updateTimestamp == nullptr ||
                resolved == nullptr || notify == nullptr || hidden == nullptr ||
                subsystem == nullptr
#ifdef BMCWEB_ENABLE_IBM_MANAGEMENT_CONSOLE
                || managementSystemAck == nullptr
#endif
            )
            {
                messages::internalError(asyncResp->res);
                return;
            }

            // hidden log entry, report as Resource Not Found
            if (*hidden)
            {
                messages::resourceNotFound(asyncResp->res, "EventLogEntry",
                                           std::to_string(*id));
                return;
            }

            asyncResp->res.jsonValue["@odata.type"] =
                "#LogEntry.v1_9_0.LogEntry";
            asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
                "/redfish/v1/Systems/system/LogServices/EventLog/Entries/{}",
                std::to_string(*id));
            asyncResp->res.jsonValue["Name"] = "System Event Log Entry";
            asyncResp->res.jsonValue["Id"] = std::to_string(*id);
            asyncResp->res.jsonValue["Message"] =
                (*eventId).substr(0, 8) + " event in subsystem: " + *subsystem;
            asyncResp->res.jsonValue["Resolved"] = *resolved;
            asyncResp->res.jsonValue["EventId"] = *eventId;
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
            asyncResp->res
                .jsonValue["Oem"]["IBM"]["@odata.id"] = boost::urls::format(
                "/redfish/v1/Systems/system/LogServices/EventLog/Entries/{}/OemPelAttachment",
                std::to_string(*id));
            if (filePath != nullptr)
            {
                asyncResp->res.jsonValue["AdditionalDataURI"] =
                    "/redfish/v1/Systems/system/LogServices/EventLog/Entries/" +
                    std::to_string(*id) + "/attachment";
            }
#ifdef BMCWEB_ENABLE_IBM_MANAGEMENT_CONSOLE
            asyncResp->res.jsonValue["Oem"]["OpenBMC"]["@odata.type"] =
                "#OemLogEntry.v1_0_0.LogEntry";
            asyncResp->res.jsonValue["Oem"]["OpenBMC"]["ManagementSystemAck"] =
                *managementSystemAck;
#endif
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
        if constexpr (bmcwebEnableMultiHost)
        {
            // Option currently returns no systems.  TBD
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }
        if (systemName != "system")
        {
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }

        std::optional<bool> resolved;
        std::optional<nlohmann::json> oemObject;
        std::optional<bool> managementSystemAck;
        if (!json_util::readJsonPatch(req, asyncResp->res, "Resolved", resolved,
                                      "Oem", oemObject))
        {
            return;
        }

        if (oemObject)
        {
            std::optional<nlohmann::json> bmcOem;
            if (!json_util::readJson(*oemObject, asyncResp->res, "OpenBMC",
                                     bmcOem))
            {
                return;
            }
            if (!json_util::readJson(*bmcOem, asyncResp->res,
                                     "ManagementSystemAck",
                                     managementSystemAck))
            {
                BMCWEB_LOG_ERROR("Could not read managementSystemAck");
                return;
            }
        }

        auto updatePropertyCallback = [resolved, managementSystemAck, asyncResp,
                                       entryId](bool hiddenPropVal) {
            if (hiddenPropVal)
            {
                messages::resourceNotFound(asyncResp->res, "LogEntry", entryId);
                return;
            }
            updateProperty(resolved, managementSystemAck, asyncResp, entryId);
        };
        getHiddenPropertyValue(asyncResp, entryId,
                               std::move(updatePropertyCallback));
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
        if constexpr (bmcwebEnableMultiHost)
        {
            // Option currently returns no systems.  TBD
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }
        if (systemName != "system")
        {
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }
        BMCWEB_LOG_DEBUG("Do delete single event entries.");

        std::string entryID = param;

        dbus::utility::escapePathForDbus(entryID);

        auto deleteEventLogCallback = [asyncResp, entryID](bool hiddenPropVal) {
            if (hiddenPropVal)
            {
                messages::resourceNotFound(asyncResp->res, "LogEntry", entryID);
                return;
            }
            deleteEventLogEntry(asyncResp, entryID);
        };
        getHiddenPropertyValue(asyncResp, entryID,
                               std::move(deleteEventLogCallback));
    });
}

inline void requestRoutesDBusCELogEntry(App& app)
{
    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/<str>/LogServices/CELog/Entries/<str>/")
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

        // DBus implementation of CELog/Entries
        // Make call to Logging Service to find all log entry objects
        sdbusplus::asio::getAllProperties(
            *crow::connections::systemBus, "xyz.openbmc_project.Logging",
            "/xyz/openbmc_project/logging/entry/" + entryID, "",
            [asyncResp, entryID](const boost::system::error_code& ec,
                                 const dbus::utility::DBusPropertiesMap& resp) {
            if (ec.value() == EBADR)
            {
                messages::resourceNotFound(asyncResp->res, "LogEntry", entryID);
                return;
            }
            if (ec)
            {
                BMCWEB_LOG_ERROR("CELogEntry (DBus) resp_handler got error {}",
                                 ec);
                messages::internalError(asyncResp->res);
                return;
            }
            const uint32_t* id = nullptr;
            const uint64_t* timestamp = nullptr;
            const uint64_t* updateTimestamp = nullptr;
            const std::string* severity = nullptr;
            const std::string* eventId = nullptr;
            const std::string* subsystem = nullptr;
            const std::string* filePath = nullptr;
            const std::string* resolution = nullptr;
            const bool* resolved = nullptr;
            const std::string* notify = nullptr;
            const bool* hidden = nullptr;
#ifdef BMCWEB_ENABLE_IBM_MANAGEMENT_CONSOLE
            const bool* managementSystemAck = nullptr;
#endif

            const bool success = sdbusplus::unpackPropertiesNoThrow(
                dbus_utils::UnpackErrorPrinter(), resp, "Id", id, "Timestamp",
                timestamp, "UpdateTimestamp", updateTimestamp, "Severity",
                severity, "EventId", eventId, "Resolved", resolved,
                "Resolution", resolution, "Path", filePath, "Hidden", hidden,
                "ServiceProviderNotify", notify, "Subsystem", subsystem
#ifdef BMCWEB_ENABLE_IBM_MANAGEMENT_CONSOLE
                ,
                "ManagementSystemAck", managementSystemAck
#endif
            );

            if (!success)
            {
                messages::internalError(asyncResp->res);
                return;
            }

            if (id == nullptr || eventId == nullptr || severity == nullptr ||
                timestamp == nullptr || updateTimestamp == nullptr ||
                resolved == nullptr || hidden == nullptr || notify == nullptr ||
                subsystem == nullptr
#ifdef BMCWEB_ENABLE_IBM_MANAGEMENT_CONSOLE
                || managementSystemAck == nullptr
#endif
            )
            {
                messages::internalError(asyncResp->res);
                return;
            }

            // Not a hidden log entry, report Resource Not Found
            if (!(*hidden))
            {
                messages::resourceNotFound(asyncResp->res, "LogEntry",
                                           std::to_string(*id));
                return;
            }

            asyncResp->res.jsonValue["@odata.type"] =
                "#LogEntry.v1_9_0.LogEntry";
            asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
                "/redfish/v1/Systems/system/LogServices/CELog/Entries/{}",
                std::to_string(*id));
            asyncResp->res.jsonValue["Name"] = "System Event Log Entry";
            asyncResp->res.jsonValue["Id"] = std::to_string(*id);
            asyncResp->res.jsonValue["Message"] =
                (*eventId).substr(0, 8) + " event in subsystem: " + *subsystem;
            asyncResp->res.jsonValue["Resolved"] = *resolved;
            asyncResp->res.jsonValue["EventId"] = *eventId;
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
            std::optional<bool> notifyAction = getProviderNotifyAction(*notify);
            if (notifyAction)
            {
                asyncResp->res.jsonValue["ServiceProviderNotified"] =
                    *notifyAction;
            }
            asyncResp->res
                .jsonValue["Oem"]["IBM"]["@odata.id"] = boost::urls::format(
                "/redfish/v1/Systems/system/LogServices/CELog/Entries/{}/OemPelAttachment",
                std::to_string(*id));
            if (filePath != nullptr)
            {
                asyncResp->res
                    .jsonValue["AdditionalDataURI"] = boost::urls::format(
                    "/redfish/v1/Systems/system/LogServices/CELog/Entries/{}/attachment",
                    std::to_string(*id));
            }
#ifdef BMCWEB_ENABLE_IBM_MANAGEMENT_CONSOLE
            asyncResp->res.jsonValue["Oem"]["OpenBMC"]["@odata.type"] =
                "#OemLogEntry.v1_0_0.LogEntry";
            asyncResp->res.jsonValue["Oem"]["OpenBMC"]["ManagementSystemAck"] =
                *managementSystemAck;
#endif
        });
    });

    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/<str>/LogServices/CELog/Entries/<str>/")
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
        std::optional<nlohmann::json> oemObject;
        std::optional<bool> managementSystemAck;
        if (!json_util::readJsonPatch(req, asyncResp->res, "Resolved", resolved,
                                      "Oem", oemObject))
        {
            return;
        }

        if (oemObject)
        {
            std::optional<nlohmann::json> bmcOem;
            if (!json_util::readJson(*oemObject, asyncResp->res, "OpenBMC",
                                     bmcOem))
            {
                return;
            }
            if (!json_util::readJson(*bmcOem, asyncResp->res,
                                     "ManagementSystemAck",
                                     managementSystemAck))
            {
                BMCWEB_LOG_ERROR("Could not read managementSystemAck");
                return;
            }
        }

        auto updatePropertyCallback = [resolved, managementSystemAck, asyncResp,
                                       entryId](bool hiddenPropVal) {
            if (!hiddenPropVal)
            {
                messages::resourceNotFound(asyncResp->res, "LogEntry", entryId);
                return;
            }
            updateProperty(resolved, managementSystemAck, asyncResp, entryId);
        };
        getHiddenPropertyValue(asyncResp, entryId,
                               std::move(updatePropertyCallback));
    });

    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/<str>/LogServices/CELog/Entries/<str>/")
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
        BMCWEB_LOG_DEBUG("Do delete single event entries.");

        std::string entryID = param;

        dbus::utility::escapePathForDbus(entryID);

        auto deleteCELogCallback = [asyncResp, entryID](bool hiddenPropVal) {
            if (!hiddenPropVal)
            {
                messages::resourceNotFound(asyncResp->res, "LogEntry", entryID);
                return;
            }
            deleteEventLogEntry(asyncResp, entryID);
        };
        getHiddenPropertyValue(asyncResp, entryID,
                               std::move(deleteCELogCallback));
    });
}

inline void
    displayOemPelAttachment(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& entryID)
{
    auto respHandler = [asyncResp, entryID](const boost::system::error_code& ec,
                                            const std::string& pelJson) {
        if (ec.value() == EBADR)
        {
            messages::resourceNotFound(asyncResp->res, "OemPelAttachment",
                                       entryID);
            return;
        }
        if (ec)
        {
            BMCWEB_LOG_ERROR("DBUS response error {}", ec);
            messages::internalError(asyncResp->res);
            return;
        }

        asyncResp->res.jsonValue["Oem"]["IBM"]["PelJson"] = pelJson;
        asyncResp->res.jsonValue["Oem"]["@odata.type"] =
            "#OemLogEntryAttachment.Oem";
        asyncResp->res.jsonValue["Oem"]["IBM"]["@odata.type"] =
            "#OemLogEntryAttachment.IBM";
    };

    uint32_t id = 0;

    auto [ptrIndex, ecIndex] = std::from_chars(&*entryID.begin(),
                                               &*entryID.end(), id);

    if (ecIndex != std::errc())
    {
        BMCWEB_LOG_ERROR("Unable to convert to entryID {} to uint32_t",
                         entryID);
        messages::internalError(asyncResp->res);
        return;
    }

    crow::connections::systemBus->async_method_call(
        respHandler, "xyz.openbmc_project.Logging",
        "/xyz/openbmc_project/logging", "org.open_power.Logging.PEL",
        "GetPELJSON", id);
}

inline void requestRoutesDBusEventLogEntryDownloadPelJson(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/LogServices/EventLog/Entries/"
                      "<str>/OemPelAttachment")
        .privileges(redfish::privileges::getLogEntry)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemName, const std::string& param)

    {
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

        auto eventLogAttachmentCallback = [asyncResp,
                                           entryID](bool hiddenPropVal) {
            if (hiddenPropVal)
            {
                messages::resourceNotFound(asyncResp->res, "LogEntry", entryID);
                return;
            }
            displayOemPelAttachment(asyncResp, entryID);
        };
        getHiddenPropertyValue(asyncResp, entryID,
                               std::move(eventLogAttachmentCallback));
    });
}

inline void requestRoutesDBusCELogEntryDownloadPelJson(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/LogServices/CELog/Entries/"
                      "<str>/OemPelAttachment")
        .privileges(redfish::privileges::getLogEntry)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemName, const std::string& param)

    {
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

        auto eventLogAttachmentCallback = [asyncResp,
                                           entryID](bool hiddenPropVal) {
            if (!hiddenPropVal)
            {
                messages::resourceNotFound(asyncResp->res, "LogEntry", entryID);
                return;
            }
            displayOemPelAttachment(asyncResp, entryID);
        };
        getHiddenPropertyValue(asyncResp, entryID,
                               std::move(eventLogAttachmentCallback));
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

inline void fillHostLoggerEntryJson(std::string_view logEntryID,
                                    std::string_view msg,
                                    nlohmann::json::object_t& logEntryJson)
{
    // Fill in the log entry with the gathered data.
    logEntryJson["@odata.type"] = "#LogEntry.v1_9_0.LogEntry";
    logEntryJson["@odata.id"] = boost::urls::format(
        "/redfish/v1/Systems/system/LogServices/HostLogger/Entries/{}",
        logEntryID);
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
        if constexpr (bmcwebEnableMultiHost)
        {
            // Option currently returns no systems.  TBD
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
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
            "#LogService.v1_2_0.LogService";
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
        if constexpr (bmcwebEnableMultiHost)
        {
            // Option currently returns no systems.  TBD
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
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
                logEntryArray.emplace_back(std::move(hostLogEntry));
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
        if constexpr (bmcwebEnableMultiHost)
        {
            // Option currently returns no systems.  TBD
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }
        if (systemName != "system")
        {
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }
        std::string_view targetID = param;

        uint64_t idInt = 0;

        auto [ptr, ec] = std::from_chars(targetID.begin(), targetID.end(),
                                         idInt);
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
    logServiceArray.emplace_back(std::move(journal));
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
            BMCWEB_LOG_ERROR(
                "handleBMCLogServicesCollectionGet respHandler got error {}",
                ec);
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
                logServiceArrayLocal.emplace_back(std::move(member));
            }
            else if (path == "/xyz/openbmc_project/dump/faultlog")
            {
                nlohmann::json::object_t member;
                member["@odata.id"] =
                    "/redfish/v1/Managers/bmc/LogServices/FaultLog";
                logServiceArrayLocal.emplace_back(std::move(member));
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
            "#LogService.v1_2_0.LogService";
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
        BMCWEB_LOG_DEBUG("Failed to read SYSLOG_IDENTIFIER field: {}",
                         strerror(-ret));
    }
    if (!syslogID.empty())
    {
        message += std::string(syslogID) + ": ";
    }

    std::string_view msg;
    ret = getJournalMetadata(journal, "MESSAGE", msg);
    if (ret < 0)
    {
        BMCWEB_LOG_ERROR("Failed to read MESSAGE field: {}", strerror(-ret));
        return 1;
    }
    message += std::string(msg);

    // Get the severity from the PRIORITY field
    long int severity = 8; // Default to an invalid priority
    ret = getJournalMetadata(journal, "PRIORITY", 10, severity);
    if (ret < 0)
    {
        BMCWEB_LOG_DEBUG("Failed to read PRIORITY field: {}", strerror(-ret));
    }

    // Get the Created time from the timestamp
    std::string entryTimeStr;
    if (!getEntryTimestamp(journal, entryTimeStr))
    {
        return 1;
    }

    // Fill in the log entry with the gathered data
    bmcJournalLogEntryJson["@odata.type"] = "#LogEntry.v1_9_0.LogEntry";
    bmcJournalLogEntryJson["@odata.id"] = boost::urls::format(
        "/redfish/v1/Managers/bmc/LogServices/Journal/Entries/{}",
        bmcJournalLogEntryID);
    bmcJournalLogEntryJson["Name"] = "BMC Journal Entry";
    bmcJournalLogEntryJson["Id"] = bmcJournalLogEntryID;
    bmcJournalLogEntryJson["Message"] = std::move(message);
    bmcJournalLogEntryJson["EntryType"] = "Oem";
    log_entry::EventSeverity severityEnum = log_entry::EventSeverity::OK;
    if (severity <= 2)
    {
        severityEnum = log_entry::EventSeverity::Critical;
    }
    else if (severity <= 4)
    {
        severityEnum = log_entry::EventSeverity::Warning;
    }

    bmcJournalLogEntryJson["Severity"] = severityEnum;
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
            BMCWEB_LOG_ERROR("failed to open journal: {}", strerror(-ret));
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
            logEntryArray.emplace_back(std::move(bmcJournalLogEntry));
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
        sd_id128_t bootID{};
        uint64_t ts = 0;
        uint64_t index = 0;
        if (!getTimestampFromID(asyncResp, entryID, bootID, ts, index))
        {
            return;
        }

        sd_journal* journalTmp = nullptr;
        int ret = sd_journal_open(&journalTmp, SD_JOURNAL_LOCAL_ONLY);
        if (ret < 0)
        {
            BMCWEB_LOG_ERROR("failed to open journal: {}", strerror(-ret));
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
        ret = sd_journal_seek_monotonic_usec(journal.get(), bootID, ts);
        if (ret < 0)
        {
            BMCWEB_LOG_ERROR("failed to seek to an entry in journal{}",
                             strerror(-ret));
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
        BMCWEB_LOG_ERROR("getDumpServiceInfo() invalid dump type: {}",
                         dumpType);
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
            BMCWEB_LOG_ERROR("getDumpServiceInfo respHandler got error {}", ec);
            // Assume that getting an error simply means there are no dump
            // LogServices. Return without adding any error response.
            return;
        }
        std::string dbusDumpPath = getDumpPath(dumpType);
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
    const std::string& chassisId, const std::string& dumpEntryId)
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
    deleteDumpEntry(asyncResp, dumpEntryId, "System");
}

inline void handleLogServicesDumpEntryDownloadGet(
    crow::App& app, const std::string& dumpType, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& dumpId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    downloadDumpEntry(asyncResp, dumpId, dumpType);
}

inline void handleDBusEventLogEntryDownloadGet(
    crow::App& app, const std::string& dumpType, bool hidden,
    const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& entryID)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    if (!http_helpers::isContentTypeAllowed(
            req.getHeaderValue("Accept"),
            http_helpers::ContentType::OctetStream, true))
    {
        asyncResp->res.result(boost::beast::http::status::bad_request);
        return;
    }

    auto callback = [asyncResp, entryID, systemName, dumpType,
                     hidden](bool hiddenPropVal) {
        if (hiddenPropVal != hidden)
        {
            messages::resourceNotFound(asyncResp->res, "LogEntry", entryID);
            return;
        }
        downloadEventLogEntry(asyncResp, systemName, entryID, dumpType);
    };
    getHiddenPropertyValue(asyncResp, entryID, std::move(callback));
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
    const std::string& systemName)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    if constexpr (bmcwebEnableMultiHost)
    {
        // Option currently returns no systems.  TBD
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    if (systemName != "system")
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
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
    const std::string& systemName)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    if constexpr (bmcwebEnableMultiHost)
    {
        // Option currently returns no systems.  TBD
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    if (systemName != "system")
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
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

inline void requestRoutesBMCDumpEntryDownload(App& app)
{
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Managers/bmc/LogServices/Dump/Entries/<str>/attachment/")
        .privileges(redfish::privileges::getLogEntry)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleLogServicesDumpEntryDownloadGet, std::ref(app), "BMC"));
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

inline void requestRoutesDBusEventLogEntryDownload(App& app)
{
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Systems/<str>/LogServices/EventLog/Entries/<str>/attachment/")
        .privileges(redfish::privileges::getLogEntry)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleDBusEventLogEntryDownloadGet, std::ref(app),
                            "System", false));
}

inline void requestRoutesDBusCEEventLogEntryDownload(App& app)
{
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Systems/<str>/LogServices/CELog/Entries/<str>/attachment/")
        .privileges(redfish::privileges::getLogEntry)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleDBusEventLogEntryDownloadGet, std::ref(app), "System", true));
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
        .privileges(redfish::privileges::getLogService)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemName) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        if constexpr (bmcwebEnableMultiHost)
        {
            // Option currently returns no systems.  TBD
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
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
        asyncResp->res.jsonValue["Id"] = "Crashdump";
        asyncResp->res.jsonValue["OverWritePolicy"] = "WrapsWhenFull";
        asyncResp->res.jsonValue["MaxNumberOfRecords"] = 3;

        std::pair<std::string, std::string> redfishDateTimeOffset =
            redfish::time_utils::getDateTimeOffsetNow();
        asyncResp->res.jsonValue["DateTime"] = redfishDateTimeOffset.first;
        asyncResp->res.jsonValue["DateTimeLocalOffset"] =
            redfishDateTimeOffset.second;

        asyncResp->res.jsonValue["Entries"]["@odata.id"] =
            "/redfish/v1/Systems/system/LogServices/Crashdump/Entries";
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
        .privileges(redfish::privileges::
                        postLogServiceSubOverComputerSystemLogServiceCollection)
        .methods(boost::beast::http::verb::post)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemName) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        if constexpr (bmcwebEnableMultiHost)
        {
            // Option currently returns no systems.  TBD
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
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
            BMCWEB_LOG_DEBUG("failed to get log ec: {}", ec.message());
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
        logEntry["@odata.id"] = boost::urls::format(
            "/redfish/v1/Systems/system/LogServices/Crashdump/Entries/{}",
            logID);
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
        .privileges(redfish::privileges::getLogEntryCollection)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemName) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        if constexpr (bmcwebEnableMultiHost)
        {
            // Option currently returns no systems.  TBD
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
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
                    BMCWEB_LOG_DEBUG("failed to get entries ec: {}",
                                     ec.message());
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
        .privileges(redfish::privileges::getLogEntry)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemName, const std::string& param) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        if constexpr (bmcwebEnableMultiHost)
        {
            // Option currently returns no systems.  TBD
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
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

        if constexpr (bmcwebEnableMultiHost)
        {
            // Option currently returns no systems.  TBD
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }
        if (systemName != "system")
        {
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }

        auto getStoredLogCallback =
            [asyncResp, logID, fileName, url(boost::urls::url(req.url()))](
                const boost::system::error_code& ec,
                const std::vector<
                    std::pair<std::string, dbus::utility::DbusVariantType>>&
                    resp) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG("failed to get log ec: {}", ec.message());
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

            if (!asyncResp->res.openFile(dbusFilepath))
            {
                messages::resourceNotFound(asyncResp->res, "LogEntry", logID);
                return;
            }

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
        .privileges(redfish::privileges::
                        postLogServiceSubOverComputerSystemLogServiceCollection)
        .methods(boost::beast::http::verb::post)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemName) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }

        if constexpr (bmcwebEnableMultiHost)
        {
            // Option currently returns no systems.  TBD
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
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
            BMCWEB_LOG_ERROR(
                "Only OEM DiagnosticDataType supported for Crashdump");
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
            BMCWEB_LOG_ERROR("Unsupported OEMDiagnosticDataType: {}",
                             oemDiagnosticDataType);
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
                [](const boost::system::error_code& ec2, sdbusplus::message_t&,
                   const std::shared_ptr<task::TaskData>& taskData) {
                if (!ec2)
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
        if constexpr (bmcwebEnableMultiHost)
        {
            // Option currently returns no systems.  TBD
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }
        if (systemName != "system")
        {
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }
        BMCWEB_LOG_DEBUG("Do delete all entries.");

        // Process response from Logging service.
        auto respHandler = [asyncResp](const boost::system::error_code& ec) {
            BMCWEB_LOG_DEBUG("doClearLog resp_handler callback: Done");
            if (ec)
            {
                // TODO Handle for specific error code
                BMCWEB_LOG_ERROR("doClearLog resp_handler got error {}", ec);
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

/**
 * DBusLogServiceActionsClear class supports POST method for ClearLog action.
 */
inline void requestRoutesDBusCELogServiceActionsClear(App& app)
{
    /**
     * Function handles POST method request.
     * The Clear Log actions does not require any parameter.The action deletes
     * all entries found in the Entries collection for this Log Service.
     */

    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Systems/<str>/LogServices/CELog/Actions/LogService.ClearLog/")
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
        BMCWEB_LOG_DEBUG("Do delete all entries.");

        // Process response from Logging service.
        auto respHandler = [asyncResp](const boost::system::error_code& ec) {
            BMCWEB_LOG_DEBUG("doClearLog resp_handler callback: Done");
            if (ec)
            {
                // TODO Handle for specific error code
                BMCWEB_LOG_ERROR("doClearLog resp_handler got error {}", ec);
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
        if constexpr (bmcwebEnableMultiHost)
        {
            // Option currently returns no systems.  TBD
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
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
            "#LogService.v1_2_0.LogService";
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
        .privileges(redfish::privileges::
                        postLogServiceSubOverComputerSystemLogServiceCollection)
        .methods(boost::beast::http::verb::post)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemName) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        if constexpr (bmcwebEnableMultiHost)
        {
            // Option currently returns no systems.  TBD
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }
        if (systemName != "system")
        {
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }
        BMCWEB_LOG_DEBUG("Do delete all postcodes entries.");

        // Make call to post-code service to request clear all
        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code& ec) {
            if (ec)
            {
                // TODO Handle for specific error code
                BMCWEB_LOG_ERROR("doClearPostCodes resp_handler got error {}",
                                 ec);
                asyncResp->res.result(
                    boost::beast::http::status::internal_server_error);
                messages::internalError(asyncResp->res);
                return;
            }
            messages::success(asyncResp->res);
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
inline bool parsePostCode(std::string_view postCodeID, uint64_t& currentValue,
                          uint16_t& index)
{
    std::vector<std::string> split;
    bmcweb::split(split, postCodeID, '-');
    if (split.size() != 2)
    {
        return false;
    }
    std::string_view postCodeNumber = split[0];
    if (postCodeNumber.size() < 2)
    {
        return false;
    }
    if (postCodeNumber[0] != 'B')
    {
        return false;
    }
    postCodeNumber.remove_prefix(1);
    auto [ptrIndex, ecIndex] = std::from_chars(postCodeNumber.begin(),
                                               postCodeNumber.end(), index);
    if (ptrIndex != postCodeNumber.end() || ecIndex != std::errc())
    {
        return false;
    }

    std::string_view postCodeIndex = split[1];

    auto [ptrValue, ecValue] = std::from_chars(
        postCodeIndex.begin(), postCodeIndex.end(), currentValue);

    return ptrValue == postCodeIndex.end() && ecValue == std::errc();
}

static bool fillPostCodeEntry(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::container::flat_map<
        uint64_t, std::tuple<uint64_t, std::vector<uint8_t>>>& postcode,
    const uint16_t bootIndex, const uint64_t codeIndex = 0,
    const uint64_t skip = 0, const uint64_t top = 0)
{
    // Get the Message from the MessageRegistry
    const registries::Message* message =
        registries::getMessage("OpenBMC.0.2.BIOSPOSTCode");
    if (message == nullptr)
    {
        BMCWEB_LOG_ERROR("Couldn't find known message?");
        return false;
    }
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

        std::string bootIndexStr = std::to_string(bootIndex);
        std::string timeOffsetString = timeOffsetStr.str();
        std::string hexCodeStr = hexCode.str();

        std::array<std::string_view, 3> messageArgs = {
            bootIndexStr, timeOffsetString, hexCodeStr};

        std::string msg =
            redfish::registries::fillMessageArgs(messageArgs, message->message);
        if (msg.empty())
        {
            messages::internalError(asyncResp->res);
            return false;
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
        bmcLogEntry["@odata.id"] = boost::urls::format(
            "/redfish/v1/Systems/system/LogServices/PostCodes/Entries/{}",
            postcodeEntryID);
        bmcLogEntry["Name"] = "POST Code Log Entry";
        bmcLogEntry["Id"] = postcodeEntryID;
        bmcLogEntry["Message"] = std::move(msg);
        bmcLogEntry["MessageId"] = "OpenBMC.0.2.BIOSPOSTCode";
        bmcLogEntry["MessageArgs"] = messageArgs;
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
            asyncResp->res.jsonValue.update(bmcLogEntry);
            return true;
        }

        nlohmann::json& logEntryArray = asyncResp->res.jsonValue["Members"];
        logEntryArray.emplace_back(std::move(bmcLogEntry));
    }

    // Return value is always false when querying multiple entries
    return false;
}

static void
    getPostCodeForEntry(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& entryId)
{
    uint16_t bootIndex = 0;
    uint64_t codeIndex = 0;
    if (!parsePostCode(entryId, codeIndex, bootIndex))
    {
        // Requested ID was not found
        messages::resourceNotFound(asyncResp->res, "LogEntry", entryId);
        return;
    }

    if (bootIndex == 0 || codeIndex == 0)
    {
        // 0 is an invalid index
        messages::resourceNotFound(asyncResp->res, "LogEntry", entryId);
        return;
    }

    crow::connections::systemBus->async_method_call(
        [asyncResp, entryId, bootIndex,
         codeIndex](const boost::system::error_code& ec,
                    const boost::container::flat_map<
                        uint64_t, std::tuple<uint64_t, std::vector<uint8_t>>>&
                        postcode) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG("DBUS POST CODE PostCode response error");
            messages::internalError(asyncResp->res);
            return;
        }

        if (postcode.empty())
        {
            messages::resourceNotFound(asyncResp->res, "LogEntry", entryId);
            return;
        }

        if (!fillPostCodeEntry(asyncResp, postcode, bootIndex, codeIndex))
        {
            messages::resourceNotFound(asyncResp->res, "LogEntry", entryId);
            return;
        }
    },
        "xyz.openbmc_project.State.Boot.PostCode0",
        "/xyz/openbmc_project/State/Boot/PostCode0",
        "xyz.openbmc_project.State.Boot.PostCode", "GetPostCodesWithTimeStamp",
        bootIndex);
}

static void
    getPostCodeForBoot(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const uint16_t bootIndex, const uint16_t bootCount,
                       const uint64_t entryCount, size_t skip, size_t top)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, bootIndex, bootCount, entryCount, skip,
         top](const boost::system::error_code& ec,
              const boost::container::flat_map<
                  uint64_t, std::tuple<uint64_t, std::vector<uint8_t>>>&
                  postcode) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG("DBUS POST CODE PostCode response error");
            messages::internalError(asyncResp->res);
            return;
        }

        uint64_t endCount = entryCount;
        if (!postcode.empty())
        {
            endCount = entryCount + postcode.size();
            if (skip < endCount && (top + skip) > entryCount)
            {
                uint64_t thisBootSkip = std::max(static_cast<uint64_t>(skip),
                                                 entryCount) -
                                        entryCount;
                uint64_t thisBootTop =
                    std::min(static_cast<uint64_t>(top + skip), endCount) -
                    entryCount;

                fillPostCodeEntry(asyncResp, postcode, bootIndex, 0,
                                  thisBootSkip, thisBootTop);
            }
            asyncResp->res.jsonValue["Members@odata.count"] = endCount;
        }

        // continue to previous bootIndex
        if (bootIndex < bootCount)
        {
            getPostCodeForBoot(asyncResp, static_cast<uint16_t>(bootIndex + 1),
                               bootCount, endCount, skip, top);
        }
        else if (skip + top < endCount)
        {
            asyncResp->res.jsonValue["Members@odata.nextLink"] =
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
    getCurrentBootNumber(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         size_t skip, size_t top)
{
    uint64_t entryCount = 0;
    sdbusplus::asio::getProperty<uint16_t>(
        *crow::connections::systemBus,
        "xyz.openbmc_project.State.Boot.PostCode0",
        "/xyz/openbmc_project/State/Boot/PostCode0",
        "xyz.openbmc_project.State.Boot.PostCode", "CurrentBootCycleCount",
        [asyncResp, entryCount, skip, top](const boost::system::error_code& ec,
                                           const uint16_t bootCount) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG("DBUS response error {}", ec);
            messages::internalError(asyncResp->res);
            return;
        }
        getPostCodeForBoot(asyncResp, 1, bootCount, entryCount, skip, top);
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
        if constexpr (bmcwebEnableMultiHost)
        {
            // Option currently returns no systems.  TBD
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
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
        if (!http_helpers::isContentTypeAllowed(
                req.getHeaderValue("Accept"),
                http_helpers::ContentType::OctetStream, true))
        {
            asyncResp->res.result(boost::beast::http::status::bad_request);
            return;
        }
        if constexpr (bmcwebEnableMultiHost)
        {
            // Option currently returns no systems.  TBD
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
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
                BMCWEB_LOG_DEBUG("DBUS response error {}", ec);
                messages::internalError(asyncResp->res);
                return;
            }

            size_t value = static_cast<size_t>(currentValue) - 1;
            if (value == std::string::npos || postcodes.size() < currentValue)
            {
                BMCWEB_LOG_WARNING("Wrong currentValue value");
                messages::resourceNotFound(asyncResp->res, "LogEntry",
                                           postCodeID);
                return;
            }

            const auto& [tID, c] = postcodes[value];
            if (c.empty())
            {
                BMCWEB_LOG_WARNING("No found post code data");
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
            asyncResp->res.write(crow::utility::base64encode(strData));
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
        if constexpr (bmcwebEnableMultiHost)
        {
            // Option currently returns no systems.  TBD
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
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

#ifdef BMCWEB_ENABLE_LINUX_AUDIT_EVENTS
/****************************************************
 * Redfish AuditLog interfaces
 ******************************************************/
inline void handleLogServicesAuditLogGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    if constexpr (bmcwebEnableMultiHost)
    {
        // Option currently returns no systems.  TBD
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    if (systemName != "system")
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);

        return;
    }
    asyncResp->res.jsonValue["@odata.id"] =
        "/redfish/v1/Systems/system/LogServices/AuditLog";
    asyncResp->res.jsonValue["@odata.type"] = "#LogService.v1_2_0.LogService";
    asyncResp->res.jsonValue["Name"] = "Audit Log Service";
    asyncResp->res.jsonValue["Description"] = "Audit Log Service";
    asyncResp->res.jsonValue["Id"] = "AuditLog";
    asyncResp->res.jsonValue["OverWritePolicy"] = "WrapsWhenFull";
    asyncResp->res.jsonValue["Entries"]["@odata.id"] =
        "/redfish/v1/Systems/system/LogServices/AuditLog/Entries";

    std::pair<std::string, std::string> redfishDateTimeOffset =
        redfish::time_utils::getDateTimeOffsetNow();
    asyncResp->res.jsonValue["DateTime"] = redfishDateTimeOffset.first;
    asyncResp->res.jsonValue["DateTimeLocalOffset"] =
        redfishDateTimeOffset.second;
}

inline void requestRoutesAuditLogService(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/LogServices/AuditLog/")
        .privileges(redfish::privileges::getLogService)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleLogServicesAuditLogGet, std::ref(app)));
}

static LogParseError
    fillAuditLogEntryJson(const nlohmann::json& auditEntry,
                          nlohmann::json::object_t& logEntryJson)
{
    if (auditEntry.is_discarded())
    {
        return LogParseError::parseFailed;
    }

    std::string logEntryID;
    std::string entryTimeStr;
    std::string messageID;
    nlohmann::json messageArgs = nlohmann::json::array();
    for (const auto& item : auditEntry.items())
    {
        if (item.key() == "ID")
        {
            logEntryID = item.value();
        }
        else if (item.key() == "EventTimestamp")
        {
            uint64_t timestamp = item.value();
            entryTimeStr = redfish::time_utils::getDateTimeUint(timestamp);
        }
        else if (item.key() == "MessageId")
        {
            messageID = item.value();
        }
        else if (item.key() == "MessageArgs")
        {
            messageArgs = item.value();
        }
    }

    // Check that we found all of the expected fields.
    if (messageID.empty())
    {
        BMCWEB_LOG_ERROR("Missing MessageID");
        return LogParseError::parseFailed;
    }

    if (logEntryID.empty())
    {
        BMCWEB_LOG_ERROR("Missing ID");
        return LogParseError::parseFailed;
    }

    if (entryTimeStr.empty())
    {
        BMCWEB_LOG_ERROR("Missing Timestamp");
        return LogParseError::parseFailed;
    }

    // Get the Message from the MessageRegistry
    const registries::Message* message = registries::getMessage(messageID);

    if (message == nullptr)
    {
        BMCWEB_LOG_WARNING("Log entry not found in registry: {} ", messageID);
        return LogParseError::messageIdNotInRegistry;
    }

    std::string msg = message->message;

    // Fill the MessageArgs into the Message
    if (!messageArgs.empty())
    {
        if (messageArgs[0] != "USYS_CONFIG")
        {
            BMCWEB_LOG_WARNING("Unexpected audit log entry type: {}",
                               messageArgs[0]);
        }

        uint i = 0;
        for (auto messageArg : messageArgs)
        {
            if (messageArg == nullptr)
            {
                BMCWEB_LOG_DEBUG("Handle null messageArg: {}", i);
                messageArg = "";
                messageArgs[i] = "";
            }

            std::string argStr = "%" + std::to_string(++i);
            size_t argPos = msg.find(argStr);
            if (argPos != std::string::npos)
            {
                msg.replace(argPos, argStr.length(), messageArg);
            }
        }
    }

    // Fill in the log entry with the gathered data
    logEntryJson["@odata.type"] = "#LogEntry.v1_9_0.LogEntry";
    logEntryJson["@odata.id"] = boost::urls::format(
        "/redfish/v1/Systems/system/LogServices/AuditLog/Entries/{}",
        logEntryID);
    logEntryJson["Name"] = "Audit Log Entry";
    logEntryJson["Id"] = logEntryID;
    logEntryJson["MessageId"] = std::move(messageID);
    logEntryJson["Message"] = std::move(msg);
    logEntryJson["MessageArgs"] = std::move(messageArgs);
    logEntryJson["EntryType"] = "Event";
    logEntryJson["EventTimestamp"] = std::move(entryTimeStr);

    /* logEntryJson["Oem"]["IBM"]["@odata.id"] ?? */
    logEntryJson["Oem"]["IBM"]["@odata.type"] = "#OemLogEntry.v1_0_0.LogEntry";
    logEntryJson["Oem"]["IBM"]["AdditionalDataFullAuditLogURI"] =
        "/redfish/v1/Systems/system/LogServices/AuditLog/Entries/FullAudit/attachment";

    return LogParseError::success;
}

/**
 * @brief Read single line from POSIX stream
 * @details Maximum length of line read is 4096 characters. Longer lines than
 *          this will be truncated and a warning is logged. This is to guard
 *          against malformed data using unexpected amounts of memory.
 *
 * @param[in] logStream - POSIX stream
 * @param[out] logLine - Filled in with line read on success
 *
 * @return True if line was read even if truncated. False if EOF reached or
 *         error reading from the logStream.
 */
inline bool readLine(FILE* logStream, std::string& logLine)
{
    std::array<char, 4096> buffer{};

    if (fgets(buffer.data(), buffer.size(), logStream) == nullptr)
    {
        /* Failed to read, could be EOF.
         * Report error if not EOF.
         */
        if (feof(logStream) == 0)
        {
            BMCWEB_LOG_ERROR("Failure reading logStream: {}", errno);
        }
        return false;
    }

    logLine.assign(buffer.data());

    if (!logLine.ends_with('\n'))
    {
        /* Line was too long for the buffer.
         * Could repeat reads or increase size of buffer.
         * Don't expect log lines to be longer than 4096 so
         * read to get to end of this line and discard rest of the line.
         * Return just the part of the line which fit in the original
         * buffer and let the caller handle it.
         */
        std::array<char, 4096> skipBuf{};
        std::string skipLine;
        auto totalLength = logLine.size();

        do
        {
            if (fgets(skipBuf.data(), skipBuf.size(), logStream) != nullptr)
            {
                skipLine.assign(skipBuf.data());
                totalLength += skipLine.size();
            }

            if (ferror(logStream) != 0)
            {
                BMCWEB_LOG_ERROR("Failure reading logStream: {}", errno);
                break;
            }

            if (feof(logStream) != 0)
            {
                /* Reached EOF trying to find the end of this line. */
                break;
            }
        } while (!skipLine.ends_with('\n'));

        BMCWEB_LOG_WARNING(
            "Line too long for logStream, line is truncated. Line length: {}",
            totalLength);
    }

    /* Return success even if line was truncated */
    return true;
}

/**
 * @brief Reads the audit log entries and writes them to Members array
 *
 * @param[in] asyncResp - The redfish response to return.
 * @param[in] unixfd - File descriptor for Audit Log file
 * @param[in] skip - Query paramater skip value
 * @param[in] top - Query paramater top value
 */
inline void
    readAuditLogEntries(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const sdbusplus::message::unix_fd& unixfd, size_t skip,
                        size_t top)
{
    auto fd = dup(unixfd);
    if (fd == -1)
    {
        BMCWEB_LOG_ERROR("Failed to duplicate fd {}", static_cast<int>(unixfd));
        messages::internalError(asyncResp->res);
        return;
    }

    FILE* logStream = fdopen(fd, "r");
    if (logStream == nullptr)
    {
        BMCWEB_LOG_ERROR("Failed to open fd {}", fd);
        messages::internalError(asyncResp->res);
        close(fd);
        return;
    }

    nlohmann::json& logEntryArray = asyncResp->res.jsonValue["Members"];
    if (logEntryArray.empty())
    {
        logEntryArray = nlohmann::json::array();
    }

    uint64_t entryCount = 0;
    std::string logLine;
    while (readLine(logStream, logLine))
    {
        /* Note: entryCount counts all entries even ones with parse errors.
         *       This allows the top/skip semantics to work correctly and a
         *       consistent count to be returned.
         */
        entryCount++;
        BMCWEB_LOG_DEBUG("{}:logLine: {}", entryCount, logLine);

        /* Handle paging using skip (number of entries to skip from the
         * start) and top (number of entries to display).
         * Don't waste cycles parsing if we are going to skip sending this entry
         */
        if (entryCount <= skip || entryCount > skip + top)
        {
            BMCWEB_LOG_DEBUG("Query param skips, line={}", entryCount);
            continue;
        }

        nlohmann::json::object_t bmcLogEntry;

        auto auditEntry = nlohmann::json::parse(logLine, nullptr, false);

        LogParseError status = fillAuditLogEntryJson(auditEntry, bmcLogEntry);
        if (status != LogParseError::success)
        {
            BMCWEB_LOG_ERROR("Failed to parse line={}", entryCount);
            messages::internalError(asyncResp->res);
            continue;
        }

        logEntryArray.push_back(std::move(bmcLogEntry));
    }

    asyncResp->res.jsonValue["Members@odata.count"] = entryCount;

    if (skip + top < entryCount)
    {
        asyncResp->res
            .jsonValue["Members@odata.nextLink"] = boost::urls::format(
            "/redfish/v1/Systems/system/LogServices/AuditLog/Entries?$skip={}",
            std::to_string(skip + top));
    }

    /* Not writing to file, so can safely ignore error on close */
    (void)fclose(logStream);
}

/**
 * @brief Retrieves the targetID entry from the unixfd
 *
 * @param[in] asyncResp - The redfish response to return.
 * @param[in] unixfd - File descriptor for Audit Log file
 * @param[in] targetID - ID of entry to retrieve
 */
inline void
    getAuditLogEntryByID(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const sdbusplus::message::unix_fd& unixfd,
                         const std::string& targetID)
{
    bool found = false;

    auto fd = dup(unixfd);
    if (fd == -1)
    {
        BMCWEB_LOG_ERROR("Failed to duplicate fd {}", static_cast<int>(unixfd));
        messages::internalError(asyncResp->res);
        return;
    }

    FILE* logStream = fdopen(fd, "r");
    if (logStream == nullptr)
    {
        BMCWEB_LOG_ERROR("Failed to open fd {}", fd);
        messages::internalError(asyncResp->res);
        close(fd);
        return;
    }

    uint64_t entryCount = 0;
    std::string logLine;
    while (readLine(logStream, logLine))
    {
        entryCount++;
        BMCWEB_LOG_DEBUG("{}:logLine: {}", entryCount, logLine);

        auto auditEntry = nlohmann::json::parse(logLine, nullptr, false);
        auto idIt = auditEntry.find("ID");
        if (idIt != auditEntry.end() && *idIt == targetID)
        {
            found = true;
            nlohmann::json::object_t bmcLogEntry;
            LogParseError status = fillAuditLogEntryJson(auditEntry,
                                                         bmcLogEntry);
            if (status != LogParseError::success)
            {
                BMCWEB_LOG_ERROR("Failed to parse line={}", entryCount);
                messages::internalError(asyncResp->res);
            }
            else
            {
                asyncResp->res.jsonValue.update(bmcLogEntry);
            }
            break;
        }
    }

    if (!found)
    {
        messages::resourceNotFound(asyncResp->res, "LogEntry", targetID);
    }

    /* Not writing to file, so can safely ignore error on close */
    (void)fclose(logStream);
}

inline void handleLogServicesAuditLogEntriesCollectionGet(
    crow::App& app, const crow::Request& req,
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

    if constexpr (bmcwebEnableMultiHost)
    {
        // Option currently returns no systems.  TBD
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
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
        "/redfish/v1/Systems/system/LogServices/AuditLog/Entries";
    asyncResp->res.jsonValue["Name"] = "Audit Log Entries";
    asyncResp->res.jsonValue["Description"] = "Collection of Audit Log Entries";
    size_t skip = delegatedQuery.skip.value_or(0);
    size_t top = delegatedQuery.top.value_or(query_param::Query::maxTop);

    /* Create unique entry for each entry in log file.
     */
    crow::connections::systemBus->async_method_call(
        [asyncResp, skip, top](const boost::system::error_code& ec,
                               const sdbusplus::message::unix_fd& unixfd) {
        if (ec)
        {
            BMCWEB_LOG_ERROR("AuditLog resp_handler got error {}", ec.value());
            messages::internalError(asyncResp->res);
            return;
        }

        readAuditLogEntries(asyncResp, unixfd, skip, top);
    },
        "xyz.openbmc_project.Logging.AuditLog",
        "/xyz/openbmc_project/logging/auditlog",
        "xyz.openbmc_project.Logging.AuditLog", "GetLatestEntries", top);
}

inline void requestRoutesAuditLogEntryCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/LogServices/AuditLog/Entries/")
        .privileges(redfish::privileges::getLogEntryCollection)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleLogServicesAuditLogEntriesCollectionGet, std::ref(app)));
}

inline void handleLogServicesAuditLogEntryGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& targetID)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    if constexpr (bmcwebEnableMultiHost)
    {
        // Option currently returns no systems.  TBD
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    if (systemName != "system")
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    /* Search for entry matching targetID. */
    crow::connections::systemBus->async_method_call(
        [asyncResp, targetID](const boost::system::error_code& ec,
                              const sdbusplus::message::unix_fd& unixfd) {
        if (ec)
        {
            if (ec.value() == EBADR)
            {
                messages::resourceNotFound(asyncResp->res, "AuditLog",
                                           targetID);
                return;
            }
            BMCWEB_LOG_ERROR("AuditLog resp_handler got error {}", ec.value());
            messages::internalError(asyncResp->res);
            return;
        }

        getAuditLogEntryByID(asyncResp, unixfd, targetID);
    },
        "xyz.openbmc_project.Logging.AuditLog",
        "/xyz/openbmc_project/logging/auditlog",
        "xyz.openbmc_project.Logging.AuditLog", "GetLatestEntries",
        query_param::Query::maxTop);
}

inline void requestRoutesAuditLogEntry(App& app)
{
    BMCWEB_ROUTE(
        app, "/redfish/v1/Systems/<str>/LogServices/AuditLog/Entries/<str>/")
        .privileges(redfish::privileges::getLogEntry)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleLogServicesAuditLogEntryGet, std::ref(app)));
}

inline void getFullAuditLogAttachment(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const sdbusplus::message::unix_fd& unixfd)
{
    int fd = -1;
    fd = dup(unixfd);
    if (fd == -1)
    {
        BMCWEB_LOG_ERROR("Failed to duplicate fd {}", static_cast<int>(unixfd));
        messages::internalError(asyncResp->res);
        return;
    }

    long long int size = lseek(fd, 0, SEEK_END);
    if (size == -1)
    {
        BMCWEB_LOG_ERROR("Failed to get size of fd {}", fd);
        messages::internalError(asyncResp->res);
        close(fd);
        return;
    }

    /* Max file size based on default configuration:
     *   - Raw audit log: 10MB
     *   - Allow up to 20MB to adjust for JSON metadata
     */
    constexpr int maxFileSize = 20971520;
    if (size > maxFileSize)
    {
        BMCWEB_LOG_ERROR("File size {} exceeds maximum allowed size of {}",
                         size, maxFileSize);
        messages::internalError(asyncResp->res);
        close(fd);
        return;
    }

    std::vector<char> data(static_cast<size_t>(size));
    long long int rc = lseek(fd, 0, SEEK_SET);
    if (rc == -1)
    {
        BMCWEB_LOG_ERROR("Failed to seek fd {}", fd);
        messages::internalError(asyncResp->res);
        close(fd);
        return;
    }
    rc = read(fd, data.data(), data.size());
    if ((rc == -1) || (rc != size))
    {
        BMCWEB_LOG_ERROR("Failed to read fd {}", fd);
        messages::internalError(asyncResp->res);
        close(fd);
        return;
    }
    close(fd);

    std::string_view strData(data.data(), data.size());

    asyncResp->res.addHeader(boost::beast::http::field::content_type,
                             "application/octet-stream");
    asyncResp->res.addHeader(
        boost::beast::http::field::content_transfer_encoding, "Base64");
    asyncResp->res.write(crow::utility::base64encode(strData));
}

inline void handleFullAuditLogAttachment(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& entryID)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        BMCWEB_LOG_DEBUG("Route setup failed");
        return;
    }

    if (!http_helpers::isContentTypeAllowed(
            req.getHeaderValue("Accept"),
            http_helpers::ContentType::OctetStream, true))
    {
        BMCWEB_LOG_ERROR("Content type not allowed");
        asyncResp->res.result(boost::beast::http::status::bad_request);
        return;
    }
    if constexpr (bmcwebEnableMultiHost)
    {
        // Option currently returns no systems.  TBD
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    if (systemName != "system")
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    if (entryID != "FullAudit")
    {
        messages::resourceNotFound(asyncResp->res, "ID", entryID);
        return;
    }

    /* Download attachment */
    crow::connections::systemBus->async_method_call(
        [asyncResp, entryID](const boost::system::error_code& ec,
                             const sdbusplus::message::unix_fd& unixfd) {
        if (ec)
        {
            if (ec.value() == EBADR)
            {
                messages::resourceNotFound(asyncResp->res, "AuditLog", entryID);
                return;
            }
            BMCWEB_LOG_ERROR("AuditLog resp_handler got error {}", ec.value());
            messages::internalError(asyncResp->res);
            return;
        }

        getFullAuditLogAttachment(asyncResp, unixfd);
    },
        "xyz.openbmc_project.Logging.AuditLog",
        "/xyz/openbmc_project/logging/auditlog",
        "xyz.openbmc_project.Logging.AuditLog", "GetAuditLog");
}

inline void requestRoutesFullAuditLogDownload(App& app)
{
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Systems/<str>/LogServices/AuditLog/Entries/<str>/attachment")
        .privileges(redfish::privileges::getLogEntry)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleFullAuditLogAttachment, std::ref(app)));
}

#endif // BMCWEB_ENABLE_LINUX_AUDIT_EVENTS
#ifdef BMCWEB_ENABLE_HW_ISOLATION
/**
 * @brief API Used to add the supported HardwareIsolation LogServices Members
 *
 * @param[in] req - The HardwareIsolation redfish request (unused now).
 * @param[in] asyncResp - The redfish response to return.
 *
 * @return The redfish response in the given buffer.
 */
inline void getSystemHardwareIsolationLogService(
    const crow::Request& /* req */,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    asyncResp->res.jsonValue["@odata.id"] =
        "/redfish/v1/Systems/system/LogServices/"
        "HardwareIsolation";
    asyncResp->res.jsonValue["@odata.type"] = "#LogService.v1_2_0.LogService";
    asyncResp->res.jsonValue["Name"] = "Hardware Isolation LogService";
    asyncResp->res.jsonValue["Description"] =
        "Hardware Isolation LogService for system owned devices";
    asyncResp->res.jsonValue["Id"] = "HardwareIsolation";

    asyncResp->res.jsonValue["Entries"] = {
        {"@odata.id", "/redfish/v1/Systems/system/LogServices/"
                      "HardwareIsolation/Entries"}};

    asyncResp->res.jsonValue["Actions"] = {
        {"#LogService.ClearLog",
         {{"target", "/redfish/v1/Systems/system/LogServices/"
                     "HardwareIsolation/Actions/"
                     "LogService.ClearLog"}}}};
}

/**
 * @brief Workaround to handle DCM (Dual-Chip Module) package for Redfish
 *
 * This API will make sure processor modeled as dual chip module, If yes then,
 * replace the redfish processor id as "dcmN-cpuN" because redfish currently
 * does not support chip module concept.
 *
 * @param[in] dbusObjPath - The D-Bus object path to return the object instance
 *
 * @return the object instance with it parent instance id if the given object
 *         is a processor else the object instance alone.
 */
inline std::string
    getIsolatedHwItemId(const sdbusplus::message::object_path& dbusObjPath)
{
    std::string isolatedHwItemId;

    if ((dbusObjPath.filename().find("cpu") != std::string::npos) &&
        (dbusObjPath.parent_path().filename().find("dcm") != std::string::npos))
    {
        isolatedHwItemId = std::format("{}-{}",
                                       dbusObjPath.parent_path().filename(),
                                       dbusObjPath.filename());
    }
    else
    {
        isolatedHwItemId = dbusObjPath.filename();
    }
    return isolatedHwItemId;
}

/**
 * @brief API used to get redfish uri of the given dbus object and fill into
 *        "OriginOfCondition" property of LogEntry schema.
 *
 * @param[in] asyncResp - The redfish response to return.
 * @param[in] dbusObjPath - The DBus object path which represents redfishUri.
 * @param[in] entryJsonIdx - The json entry index to add isolated hardware
 *                            details in the appropriate entry json object.
 *
 * @return The redfish response with "OriginOfCondition" property of
 *         LogEntry schema if success else return the error
 */
inline void getRedfishUriByDbusObjPath(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const sdbusplus::message::object_path& dbusObjPath,
    const size_t entryJsonIdx)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, dbusObjPath,
         entryJsonIdx](const boost::system::error_code& ec,
                       const dbus::utility::MapperGetObject& objType) {
        if (ec || objType.empty())
        {
            BMCWEB_LOG_ERROR(
                "DBUS response error [{} : {}] when tried to get the RedfishURI of isolated hareware: {}",
                ec.value(), ec.message(), dbusObjPath.str);
            messages::internalError(asyncResp->res);
            return;
        }

        RedfishUriListType::const_iterator redfishUriIt;
        for (const auto& service : objType)
        {
            for (const auto& interface : service.second)
            {
                redfishUriIt = redfishUriList.find(interface);
                if (redfishUriIt != redfishUriList.end())
                {
                    // Found the Redfish URI of the isolated hardware unit.
                    break;
                }
            }
            if (redfishUriIt != redfishUriList.end())
            {
                // No need to check in the next service interface list
                break;
            }
        }

        if (redfishUriIt == redfishUriList.end())
        {
            BMCWEB_LOG_ERROR(
                "The object[{}] interface is not found in the Redfish URI list. Please add the respective D-Bus interface name",
                dbusObjPath.str);
            messages::internalError(asyncResp->res);
            return;
        }

        // Fill the isolated hardware object id along with the Redfish URI
        std::string redfishUri = redfishUriIt->second + "/" +
                                 getIsolatedHwItemId(dbusObjPath);

        // Make sure whether no need to fill the parent object id in the
        // isolated hardware Redfish URI.
        const std::string uriIdPattern{"<str>"};
        size_t uriIdPos = redfishUri.rfind(uriIdPattern);
        if (uriIdPos == std::string::npos)
        {
            if (entryJsonIdx > 0)
            {
                asyncResp->res.jsonValue["Members"][entryJsonIdx - 1]["Links"]
                                        ["OriginOfCondition"]["@odata.id"] =
                    redfishUri;
            }
            else
            {
                asyncResp->res.jsonValue["Links"]["OriginOfCondition"]
                                        ["@odata.id"] = redfishUri;
            }
            return;
        }

        // Fill the all parents Redfish URI id.
        // For example, the processors id for the core.
        // "/redfish/v1/Systems/system/Processors/<str>/SubProcessors/core0"
        crow::connections::systemBus->async_method_call(
            [asyncResp, dbusObjPath, entryJsonIdx, redfishUri, uriIdPos,
             uriIdPattern](const boost::system::error_code& ec1,
                           const dbus::utility::MapperGetSubTreeResponse&
                               subtree) mutable {
            if (ec1)
            {
                BMCWEB_LOG_ERROR(
                    "DBUS response error [{} : {}] when tried to fill the parent objects id in the RedfishURI: {} of the isolated hareware: {}",
                    ec1.value(), ec1.message(), redfishUri, dbusObjPath.str);

                messages::internalError(asyncResp->res);
                return;
            }

            while (uriIdPos != std::string::npos)
            {
                std::string parentRedfishUri = redfishUri.substr(0,
                                                                 uriIdPos - 1);
                RedfishUriListType::const_iterator parentRedfishUriIt =
                    std::find_if(redfishUriList.begin(), redfishUriList.end(),
                                 [parentRedfishUri](const auto& ele) {
                    return parentRedfishUri == ele.second;
                });

                if (parentRedfishUriIt == redfishUriList.end())
                {
                    BMCWEB_LOG_ERROR(
                        "Failed to fill Links:OriginOfCondition because unable to get parent Redfish URI [{}] DBus interface for the identified Redfish URI: {} of the given DBus object path: {} ",
                        parentRedfishUri, redfishUri, dbusObjPath.str);
                    messages::internalError(asyncResp->res);
                    return;
                }

                std::string parentObj;
                for (const auto& obj : subtree)
                {
                    if (dbusObjPath.str.find(
                            sdbusplus::message::object_path(obj.first)
                                .filename()) == std::string::npos)
                    {
                        // The object is not found in the isolated
                        // hardware object path
                        continue;
                    }

                    for (const auto& service : obj.second)
                    {
                        for (const auto& interface : service.second)
                        {
                            if (interface == parentRedfishUriIt->first)
                            {
                                parentObj = obj.first;
                                break;
                            }
                        }
                        if (!parentObj.empty())
                        {
                            break;
                        }
                    }
                    if (!parentObj.empty())
                    {
                        break;
                    }
                }

                if (parentObj.empty())
                {
                    BMCWEB_LOG_ERROR(
                        "Failed to fill Links:OriginOfCondition because unable to get parent DBus path for the identified parent Redfish URI: {} of the given DBus object path: {}",
                        parentRedfishUri, dbusObjPath.str);

                    messages::internalError(asyncResp->res);
                    return;
                }

                redfishUri.replace(
                    uriIdPos, uriIdPattern.length(),
                    getIsolatedHwItemId(
                        sdbusplus::message::object_path(parentObj)));

                uriIdPos = redfishUri.rfind(uriIdPattern);
            }

            if (entryJsonIdx > 0)
            {
                asyncResp->res.jsonValue["Members"][entryJsonIdx - 1]["Links"]
                                        ["OriginOfCondition"]["@odata.id"] =
                    redfishUri;
            }
            else
            {
                asyncResp->res.jsonValue["Links"]["OriginOfCondition"]
                                        ["@odata.id"] = redfishUri;
            }
        },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetSubTree",
            "/xyz/openbmc_project/inventory", 0, std::array<const char*, 0>{});
    },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetObject", dbusObjPath.str,
        std::array<const char*, 0>{});
}

/**
 * @brief API used to get "PrettyName" by using the given dbus object path
 *        and fill into "Message" property of LogEntry schema.
 *
 * @param[in] asyncResp - The redfish response to return.
 * @param[in] dbusObjPath - The DBus object path which represents redfishUri.
 * @param[in] entryJsonIdx - The json entry index to add isolated hardware
 *                            details in the appropriate entry json object.
 *
 * @return The redfish response with "Message" property of LogEntry schema
 *         if success else nothing in redfish response.
 */

inline void getPrettyNameByDbusObjPath(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const sdbusplus::message::object_path& dbusObjPath,
    const size_t entryJsonIdx)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, dbusObjPath,
         entryJsonIdx](const boost::system::error_code& ec,
                       const dbus::utility::MapperGetObject& objType) mutable {
        if (ec || objType.empty())
        {
            BMCWEB_LOG_ERROR(
                "DBUS response error [{} : {}] when tried to get the dbus name of isolated hareware: {}",
                ec.value(), ec.message(), dbusObjPath.str);
            messages::internalError(asyncResp->res);
            return;
        }

        if (objType.size() > 1)
        {
            BMCWEB_LOG_ERROR(
                "More than one dbus service implemented the xyz.openbmc_project.Inventory.Item interface to get the PrettyName");
            messages::internalError(asyncResp->res);
            return;
        }

        if (objType[0].first.empty())
        {
            BMCWEB_LOG_ERROR(
                "The retrieved dbus name is empty for the given dbus object: {}",
                dbusObjPath.str);
            messages::internalError(asyncResp->res);
            return;
        }

        if (entryJsonIdx > 0)
        {
            asyncResp->res.jsonValue["Members"][entryJsonIdx - 1]["Message"] =
                dbusObjPath.filename();
            auto msgPropPath = "/Members"_json_pointer;
            msgPropPath /= entryJsonIdx - 1;
            msgPropPath /= "Message";
            name_util::getPrettyName(asyncResp, dbusObjPath.str, objType,
                                     msgPropPath);
        }
        else
        {
            asyncResp->res.jsonValue["Message"] = dbusObjPath.filename();
            name_util::getPrettyName(asyncResp, dbusObjPath.str, objType,
                                     "/Message"_json_pointer);
        }
    },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetObject", dbusObjPath.str,
        std::array<const char*, 1>{"xyz.openbmc_project.Inventory.Item"});
}

/**
 * @brief API used to fill the isolated hardware details into LogEntry schema
 *        by using the given isolated dbus object which is present in
 *        xyz.openbmc_project.Association.Definitions::Associations of the
 *        HardwareIsolation dbus entry object.
 *
 * @param[in] asyncResp - The redfish response to return.
 * @param[in] dbusObjPath - The DBus object path which represents redfishUri.
 * @param[in] entryJsonIdx - The json entry index to add isolated hardware
 *                            details in the appropriate entry json object.
 *
 * @return The redfish response with appropriate redfish properties of the
 *         isolated hardware details into LogEntry schema if success else
 *         nothing in redfish response.
 */
inline void fillIsolatedHwDetailsByObjPath(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const sdbusplus::message::object_path& dbusObjPath,
    const size_t entryJsonIdx)
{
    // Fill Redfish uri of isolated hardware into "OriginOfCondition"
    if (dbusObjPath.filename().find("unit") != std::string::npos)
    {
        // If Isolated Hardware object name contain "unit" then that unit
        // is not modelled in inventory and redfish so the "OriginOfCondition"
        // should filled with it's parent (aka FRU of unit) path.
        getRedfishUriByDbusObjPath(asyncResp, dbusObjPath.parent_path(),
                                   entryJsonIdx);
    }
    else
    {
        getRedfishUriByDbusObjPath(asyncResp, dbusObjPath, entryJsonIdx);
    }

    // Fill PrettyName of isolated hardware into "Message"
    getPrettyNameByDbusObjPath(asyncResp, dbusObjPath, entryJsonIdx);
}

/**
 * @brief API used to fill isolated hardware details into LogEntry schema
 *        by using the given isolated dbus object.
 *
 * @param[in] asyncResp - The redfish response to return.
 * @param[in] entryJsonIdx - The json entry index to add isolated hardware
 *                            details. If passing less than or equal 0 then,
 *                            it will assume the given asyncResp jsonValue as
 *                            a single entry json object. If passing greater
 *                            than 0 then, it will assume the given asyncResp
 *                            jsonValue contains "Members" to fill in the
 *                            appropriate entry json object.
 * @param[in] dbusObjIt - The DBus object which contains isolated hardware
                         details.
 *
 * @return The redfish response with appropriate redfish properties of the
 *         isolated hardware details into LogEntry schema if success else
 *         failure response.
 */
inline void fillSystemHardwareIsolationLogEntry(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const size_t entryJsonIdx, GetManagedObjectsType::const_iterator& dbusObjIt)
{
    nlohmann::json& entryJson =
        (entryJsonIdx > 0
             ? asyncResp->res.jsonValue["Members"][entryJsonIdx - 1]
             : asyncResp->res.jsonValue);

    for (const auto& interface : dbusObjIt->second)
    {
        if (interface.first == "xyz.openbmc_project.HardwareIsolation.Entry")
        {
            for (const auto& property : interface.second)
            {
                if (property.first == "Severity")
                {
                    const std::string* severity =
                        std::get_if<std::string>(&property.second);
                    if (severity == nullptr)
                    {
                        BMCWEB_LOG_ERROR(
                            "Failed to get the Severity from object: {}",
                            dbusObjIt->first.str);
                        messages::internalError(asyncResp->res);
                        break;
                    }

                    if (*severity ==
                        "xyz.openbmc_project.HardwareIsolation.Entry.Type.Critical")
                    {
                        entryJson["Severity"] = "Critical";
                    }
                    else if (
                        (*severity ==
                         "xyz.openbmc_project.HardwareIsolation.Entry.Type.Warning") ||
                        (*severity ==
                         "xyz.openbmc_project.HardwareIsolation.Entry.Type.Manual"))
                    {
                        entryJson["Severity"] = "Warning";
                    }
                    else
                    {
                        BMCWEB_LOG_ERROR(
                            "Unsupported Severity[{}] from object: {}",
                            *severity, dbusObjIt->first.str);
                        messages::internalError(asyncResp->res);
                        break;
                    }
                }
                else if (property.first == "Resolved")
                {
                    const bool* resolved = std::get_if<bool>(&property.second);
                    if (resolved == nullptr)
                    {
                        BMCWEB_LOG_ERROR(
                            "Failed to get the Resolved from object: {}",
                            dbusObjIt->first.str);
                        messages::internalError(asyncResp->res);
                        break;
                    }
                    entryJson["Resolved"] = *resolved;
                }
            }
        }
        else if (interface.first == "xyz.openbmc_project.Time.EpochTime")
        {
            for (const auto& property : interface.second)
            {
                if (property.first == "Elapsed")
                {
                    const uint64_t* elapsdTime =
                        std::get_if<uint64_t>(&property.second);
                    if (elapsdTime == nullptr)
                    {
                        BMCWEB_LOG_ERROR(
                            "Failed to get the Elapsed time from object: {}",
                            dbusObjIt->first.str);
                        messages::internalError(asyncResp->res);
                        break;
                    }
                    entryJson["Created"] =
                        redfish::time_utils::getDateTimeUint((*elapsdTime));
                }
            }
        }
        else if (interface.first ==
                 "xyz.openbmc_project.Association.Definitions")
        {
            for (const auto& property : interface.second)
            {
                if (property.first == "Associations")
                {
                    const AssociationsValType* associations =
                        std::get_if<AssociationsValType>(&property.second);
                    if (associations == nullptr)
                    {
                        BMCWEB_LOG_ERROR(
                            "Failed to get the Associations from object: {}",
                            dbusObjIt->first.str);
                        messages::internalError(asyncResp->res);
                        break;
                    }
                    for (const auto& assoc : *associations)
                    {
                        if (std::get<0>(assoc) == "isolated_hw")
                        {
                            fillIsolatedHwDetailsByObjPath(
                                asyncResp,
                                sdbusplus::message::object_path(
                                    std::get<2>(assoc)),
                                entryJsonIdx);
                        }
                        else if (std::get<0>(assoc) == "isolated_hw_errorlog")
                        {
                            sdbusplus::message::object_path errPath =
                                std::get<2>(assoc);
                            entryJson["AdditionalDataURI"] = boost::urls::format(
                                "/redfish/v1/Systems/system/LogServices/EventLog/Entries/{}/attachment",
                                errPath.filename());
                        }
                    }
                }
            }
        }
    }

    entryJson["@odata.type"] = "#LogEntry.v1_9_0.LogEntry";
    entryJson["@odata.id"] = boost::urls::format(
        "/redfish/v1/Systems/system/LogServices/HardwareIsolation/Entries/{}",
        dbusObjIt->first.filename());
    entryJson["Id"] = dbusObjIt->first.filename();
    entryJson["Name"] = "Hardware Isolation Entry";
    entryJson["EntryType"] = "Event";
}

/**
 * @brief API Used to add the supported HardwareIsolation LogEntry Entries id
 *
 * @param[in] req - The HardwareIsolation redfish request (unused now).
 * @param[in] asyncResp - The redfish response to return.
 *
 * @return The redfish response in the given buffer.
 *
 * @note This function will return the available entries dbus object which are
 *       created by HardwareIsolation manager.
 */
inline void getSystemHardwareIsolationLogEntryCollection(
    const crow::Request& /* req */,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    auto getManagedObjectsHandler =
        [asyncResp](const boost::system::error_code& ec,
                    const GetManagedObjectsType& mgtObjs) {
        if (ec)
        {
            BMCWEB_LOG_ERROR(
                "DBUS response error [{} : {}] when tried to get the HardwareIsolation managed objects",
                ec.value(), ec.message());
            messages::internalError(asyncResp->res);
            return;
        }

        nlohmann::json& entriesArray = asyncResp->res.jsonValue["Members"];
        entriesArray = nlohmann::json::array();

        for (auto dbusObjIt = mgtObjs.begin(); dbusObjIt != mgtObjs.end();
             dbusObjIt++)
        {
            if (dbusObjIt->second.find(
                    "xyz.openbmc_project.HardwareIsolation.Entry") ==
                dbusObjIt->second.end())
            {
                // The retrieved object is not hardware isolation entry
                continue;
            }
            entriesArray.push_back(nlohmann::json::object());

            fillSystemHardwareIsolationLogEntry(asyncResp, entriesArray.size(),
                                                dbusObjIt);
        }

        asyncResp->res.jsonValue["Members@odata.count"] = entriesArray.size();

        asyncResp->res.jsonValue["@odata.type"] =
            "#LogEntryCollection.LogEntryCollection";
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Systems/system/LogServices/HardwareIsolation/Entries";
        asyncResp->res.jsonValue["Name"] = "Hardware Isolation Entries";
        asyncResp->res.jsonValue["Description"] =
            "Collection of System Hardware Isolation Entries";
    };

    // Get the DBus name of HardwareIsolation service
    crow::connections::systemBus->async_method_call(
        [asyncResp, getManagedObjectsHandler](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetObject& objType) {
        if (ec || objType.empty())
        {
            BMCWEB_LOG_ERROR(
                "DBUS response error [{} : {}] when tried to get the HardwareIsolation dbus name",
                ec.value(), ec.message());
            messages::internalError(asyncResp->res);
            return;
        }

        if (objType.size() > 1)
        {
            BMCWEB_LOG_ERROR(
                "More than one dbus service implemented the HardwareIsolation service");
            messages::internalError(asyncResp->res);
            return;
        }

        if (objType[0].first.empty())
        {
            BMCWEB_LOG_ERROR(
                "The retrieved HardwareIsolation dbus name is empty");
            messages::internalError(asyncResp->res);
            return;
        }

        // Fill the Redfish LogEntry schema for the retrieved
        // HardwareIsolation entries
        crow::connections::systemBus->async_method_call(
            getManagedObjectsHandler, objType[0].first,
            "/xyz/openbmc_project/hardware_isolation",
            "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
    },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetObject",
        "/xyz/openbmc_project/hardware_isolation",
        std::array<const char*, 1>{
            "xyz.openbmc_project.HardwareIsolation.Create"});
}

/**
 * @brief API Used to fill LogEntry schema by using the HardwareIsolation dbus
 *        entry object which will get by using the given entry id in redfish
 *        uri.
 *
 * @param[in] req - The HardwareIsolation redfish request (unused now).
 * @param[in] asyncResp - The redfish response to return.
 * @param[in] entryId - The entry id of HardwareIsolation entries to retrieve
 *                      the corresponding isolated hardware details.
 *
 * @return The redfish response in the given buffer with LogEntry schema
 *         members if success else will error.
 */
inline void getSystemHardwareIsolationLogEntryById(
    const crow::Request& /* req */,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& entryId)
{
    sdbusplus::message::object_path entryObjPath(std::format(
        "/xyz/openbmc_project/hardware_isolation/entry/{}", entryId));

    auto getManagedObjectsRespHandler =
        [asyncResp, entryObjPath](const boost::system::error_code& ec,
                                  const GetManagedObjectsType& mgtObjs) {
        if (ec)
        {
            BMCWEB_LOG_ERROR(
                "DBUS response error [{}:{}] when tried to get the HardwareIsolation managed objects",
                ec.value(), ec.message());
            messages::internalError(asyncResp->res);
            return;
        }

        bool entryIsPresent = false;
        for (auto dbusObjIt = mgtObjs.begin(); dbusObjIt != mgtObjs.end();
             dbusObjIt++)
        {
            if (dbusObjIt->first == entryObjPath)
            {
                entryIsPresent = true;
                fillSystemHardwareIsolationLogEntry(asyncResp, 0, dbusObjIt);
                break;
            }
        }

        if (!entryIsPresent)
        {
            messages::resourceNotFound(asyncResp->res, "Entry",
                                       entryObjPath.filename());
            return;
        }
    };

    auto getObjectRespHandler =
        [asyncResp, entryId, entryObjPath, getManagedObjectsRespHandler](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetObject& objType) {
        if (ec || objType.empty())
        {
            BMCWEB_LOG_ERROR(
                "DBUS response error [{} : {}] when tried to get the HardwareIsolation dbus name the given object path: {}",
                ec.value(), ec.message(), entryObjPath.str);

            if (ec.value() == EBADR)
            {
                messages::resourceNotFound(asyncResp->res, "Entry", entryId);
            }
            else
            {
                messages::internalError(asyncResp->res);
            }
            return;
        }

        if (objType.size() > 1)
        {
            BMCWEB_LOG_ERROR(
                "More than one dbus service implemented the HardwareIsolation service");
            messages::internalError(asyncResp->res);
            return;
        }

        if (objType[0].first.empty())
        {
            BMCWEB_LOG_ERROR(
                "The retrieved HardwareIsolation dbus name is empty");
            messages::internalError(asyncResp->res);
            return;
        }

        // Fill the Redfish LogEntry schema for the identified entry dbus object
        crow::connections::systemBus->async_method_call(
            getManagedObjectsRespHandler, objType[0].first,
            "/xyz/openbmc_project/hardware_isolation",
            "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
    };

    // Make sure the given entry id is present in hardware isolation
    // dbus entries and get the DBus name of that entry to fill LogEntry
    crow::connections::systemBus->async_method_call(
        getObjectRespHandler, "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetObject", entryObjPath.str,
        hwIsolationEntryIfaces);
}

/**
 * @brief API Used to deisolate the given HardwareIsolation entry.
 *
 * @param[in] req - The HardwareIsolation redfish request (unused now).
 * @param[in] asyncResp - The redfish response to return.
 * @param[in] entryId - The entry id of HardwareIsolation entries to deisolate.
 *
 * @return The redfish response in the given buffer.
 */
inline void deleteSystemHardwareIsolationLogEntryById(
    const crow::Request& /* req */,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& entryId)
{
    sdbusplus::message::object_path entryObjPath(
        std::string("/xyz/openbmc_project/hardware_isolation/entry") + "/" +
        entryId);

    // Make sure the given entry id is present in hardware isolation
    // entries and get the DBus name of that entry
    crow::connections::systemBus->async_method_call(
        [asyncResp, entryId,
         entryObjPath](const boost::system::error_code& ec,
                       const dbus::utility::MapperGetObject& objType) {
        if (ec || objType.empty())
        {
            BMCWEB_LOG_ERROR(
                "DBUS response error [{} : {}] when tried to get the HardwareIsolation dbus name the given object path: ",
                ec.value(), ec.message(), entryObjPath.str);
            if (ec.value() == EBADR)
            {
                messages::resourceNotFound(asyncResp->res, "Entry", entryId);
            }
            else
            {
                messages::internalError(asyncResp->res);
            }
            return;
        }

        if (objType.size() > 1)
        {
            BMCWEB_LOG_ERROR(
                "More than one dbus service implemented the HardwareIsolation service");
            messages::internalError(asyncResp->res);
            return;
        }

        if (objType[0].first.empty())
        {
            BMCWEB_LOG_ERROR(
                "The retrieved HardwareIsolation dbus name is empty");
            messages::internalError(asyncResp->res);
            return;
        }

        // Delete the respective dbus entry object
        crow::connections::systemBus->async_method_call(
            [asyncResp, entryObjPath](const boost::system::error_code& ec1) {
            if (ec1)
            {
                BMCWEB_LOG_ERROR(
                    "DBUS response error [{} : {}] when tried to delete the given object path: ",
                    ec1.value(), ec1.message(), entryObjPath.str);
                messages::internalError(asyncResp->res);
                return;
            }
            messages::success(asyncResp->res);
        },
            objType[0].first, entryObjPath.str,
            "xyz.openbmc_project.Object.Delete", "Delete");
    },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetObject", entryObjPath.str,
        hwIsolationEntryIfaces);
}

/**
 * @brief API Used to deisolate the all HardwareIsolation entries.
 *
 * @param[in] req - The HardwareIsolation redfish request (unused now).
 * @param[in] asyncResp - The redfish response to return.
 *
 * @return The redfish response in the given buffer.
 */
inline void postSystemHardwareIsolationLogServiceClearLog(
    const crow::Request& /* req */,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    // Get the DBus name of HardwareIsolation service
    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code& ec,
                    const dbus::utility::MapperGetObject& objType) {
        if (ec || objType.empty())
        {
            BMCWEB_LOG_ERROR(
                "DBUS response error [{} : {}] when tried to get the HardwareIsolation dbus name",
                ec.value(), ec.message());
            messages::internalError(asyncResp->res);
            return;
        }

        if (objType.size() > 1)
        {
            BMCWEB_LOG_ERROR(
                "More than one dbus service implemented the HardwareIsolation service");
            messages::internalError(asyncResp->res);
            return;
        }

        if (objType[0].first.empty())
        {
            BMCWEB_LOG_ERROR(
                "The retrieved HardwareIsolation dbus name is empty");
            messages::internalError(asyncResp->res);
            return;
        }

        // Delete all HardwareIsolation entries
        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code& ec1) {
            if (ec1)
            {
                BMCWEB_LOG_ERROR(
                    "DBUS response error [{} : {}] when tried to delete all HardwareIsolation entries",
                    ec1.value(), ec1.message());
                messages::internalError(asyncResp->res);
                return;
            }
            messages::success(asyncResp->res);
        },
            objType[0].first, "/xyz/openbmc_project/hardware_isolation",
            "xyz.openbmc_project.Collection.DeleteAll", "DeleteAll");
    },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetObject",
        "/xyz/openbmc_project/hardware_isolation",
        std::array<const char*, 1>{"xyz.openbmc_project.Collection.DeleteAll"});
}

/**
 * @brief API used to route the handler for HardwareIsolation Redfish
 *        LogServices URI
 *
 * @param[in] app - Crow app on which Redfish will initialize
 *
 * @return The appropriate redfish response for the given redfish request.
 */
inline void requestRoutesSystemHardwareIsolationLogService(App& app)
{
    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/system/LogServices/HardwareIsolation/")
        .privileges(redfish::privileges::getLogService)
        .methods(boost::beast::http::verb::get)(
            getSystemHardwareIsolationLogService);

    BMCWEB_ROUTE(
        app, "/redfish/v1/Systems/system/LogServices/HardwareIsolation/Entries")
        .privileges(redfish::privileges::getLogEntryCollection)
        .methods(boost::beast::http::verb::get)(
            getSystemHardwareIsolationLogEntryCollection);

    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/LogServices/"
                      "HardwareIsolation/Entries/<str>/")
        .privileges(redfish::privileges::getLogEntry)
        .methods(boost::beast::http::verb::get)(
            getSystemHardwareIsolationLogEntryById);

    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/LogServices/"
                      "HardwareIsolation/Entries/<str>/")
        .privileges(redfish::privileges::deleteLogEntry)
        .methods(boost::beast::http::verb::delete_)(
            deleteSystemHardwareIsolationLogEntryById);

    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/system/LogServices/HardwareIsolation/"
                 "Actions/LogService.ClearLog/")
        .privileges(redfish::privileges::postLogService)
        .methods(boost::beast::http::verb::post)(
            postSystemHardwareIsolationLogServiceClearLog);
}
#endif // BMCWEB_ENABLE_HW_ISOLATION

} // namespace redfish
