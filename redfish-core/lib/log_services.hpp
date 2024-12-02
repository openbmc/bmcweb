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
#include "http_body.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "human_sort.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "str_utility.hpp"
#include "task.hpp"
#include "task_messages.hpp"
#include "utils/dbus_utils.hpp"
#include "utils/etag_utils.hpp"
#include "utils/eventlog_utils.hpp"
#include "utils/json_utils.hpp"
#include "utils/log_services_utils.hpp"
#include "utils/time_utils.hpp"

#include <asm-generic/errno.h>
#include <systemd/sd-bus.h>
#include <tinyxml2.h>
#include <unistd.h>

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/system/linux_error.hpp>
#include <boost/url/format.hpp>
#include <boost/url/url.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/message/native_types.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <format>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
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

enum class DumpCreationProgress
{
    DUMP_CREATE_SUCCESS,
    DUMP_CREATE_FAILED,
    DUMP_CREATE_INPROGRESS
};

inline std::string getDumpPath(std::string_view dumpType)
{
    std::string dbusDumpPath = "/xyz/openbmc_project/dump/";
    std::ranges::transform(dumpType, std::back_inserter(dbusDumpPath),
                           bmcweb::asciiToLower);

    return dbusDumpPath;
}

inline log_entry::OriginatorTypes mapDbusOriginatorTypeToRedfish(
    const std::string& originatorType)
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
        entriesPath =
            std::format("/redfish/v1/Managers/{}/LogServices/Dump/Entries/",
                        BMCWEB_REDFISH_MANAGER_URI_NAME);
    }
    else if (dumpType == "FaultLog")
    {
        entriesPath =
            std::format("/redfish/v1/Managers/{}/LogServices/FaultLog/Entries/",
                        BMCWEB_REDFISH_MANAGER_URI_NAME);
    }
    else if (dumpType == "System")
    {
        entriesPath =
            std::format("/redfish/v1/Systems/{}/LogServices/Dump/Entries/",
                        BMCWEB_REDFISH_SYSTEM_URI_NAME);
    }
    else
    {
        BMCWEB_LOG_ERROR("getDumpEntriesPath() invalid dump type: {}",
                         dumpType);
    }

    // Returns empty string on error
    return entriesPath;
}

inline void getDumpEntryCollection(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
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
            asyncResp->res.jsonValue["Name"] = dumpType + " Dump Entries";
            asyncResp->res.jsonValue["Description"] =
                "Collection of " + dumpType + " Dump Entries";

            nlohmann::json::array_t entriesArray;
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

                parseDumpEntryFromDbusObject(object, dumpStatus, size,
                                             timestampUs, originatorId,
                                             originatorType, asyncResp);

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

                if (!originatorId.empty())
                {
                    thisEntry["Originator"] = originatorId;
                    thisEntry["OriginatorType"] = originatorType;
                }

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
                entriesArray.emplace_back(std::move(thisEntry));
            }
            asyncResp->res.jsonValue["Members@odata.count"] =
                entriesArray.size();
            asyncResp->res.jsonValue["Members"] = std::move(entriesArray);
        });
}

inline void getDumpEntryById(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
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
                    messages::resourceNotFound(asyncResp->res,
                                               dumpType + " dump", entryID);
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

                if (!originatorId.empty())
                {
                    asyncResp->res.jsonValue["Originator"] = originatorId;
                    asyncResp->res.jsonValue["OriginatorType"] = originatorType;
                }

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
                    asyncResp->res.jsonValue["OEMDiagnosticDataType"] =
                        "System";
                    asyncResp->res.jsonValue["AdditionalDataURI"] =
                        entriesPath + entryID + "/attachment";
                    asyncResp->res.jsonValue["AdditionalDataSizeBytes"] = size;
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
                        entryID](const boost::system::error_code& ec) {
        BMCWEB_LOG_DEBUG("Dump Entry doDelete callback: Done");
        if (ec)
        {
            if (ec.value() == EBADR)
            {
                messages::resourceNotFound(asyncResp->res, "LogEntry", entryID);
                return;
            }
            BMCWEB_LOG_ERROR(
                "Dump (DBus) doDelete respHandler got error {} entryID={}", ec,
                entryID);
            messages::internalError(asyncResp->res);
            return;
        }
    };

    dbus::utility::async_method_call(
        asyncResp, respHandler, "xyz.openbmc_project.Dump.Manager",
        std::format("{}/entry/{}", getDumpPath(dumpType), entryID),
        "xyz.openbmc_project.Object.Delete", "Delete");
}

inline void downloadDumpEntry(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& entryID, const std::string& dumpType)
{
    if (dumpType != "BMC")
    {
        BMCWEB_LOG_WARNING("Can't find Dump Entry {}", entryID);
        messages::resourceNotFound(asyncResp->res, dumpType + " dump", entryID);
        return;
    }

    std::string dumpEntryPath =
        std::format("{}/entry/{}", getDumpPath(dumpType), entryID);

    auto downloadDumpEntryHandler =
        [asyncResp, entryID,
         dumpType](const boost::system::error_code& ec,
                   const sdbusplus::message::unix_fd& unixfd) {
            log_services_utils::downloadEntryCallback(asyncResp, entryID,
                                                      dumpType, ec, unixfd);
        };

    dbus::utility::async_method_call(
        asyncResp, std::move(downloadDumpEntryHandler),
        "xyz.openbmc_project.Dump.Manager", dumpEntryPath,
        "xyz.openbmc_project.Dump.Entry", "GetFileHandle");
}

inline DumpCreationProgress mapDbusStatusToDumpProgress(
    const std::string& status)
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

inline DumpCreationProgress getDumpCompletionStatus(
    const dbus::utility::DBusPropertiesMap& values)
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
    }
    return DumpCreationProgress::DUMP_CREATE_INPROGRESS;
}

inline std::string getDumpEntryPath(const std::string& dumpPath)
{
    if (dumpPath == "/xyz/openbmc_project/dump/bmc/entry")
    {
        return std::format("/redfish/v1/Managers/{}/LogServices/Dump/Entries/",
                           BMCWEB_REDFISH_MANAGER_URI_NAME);
    }
    if (dumpPath == "/xyz/openbmc_project/dump/system/entry")
    {
        return std::format("/redfish/v1/Systems/{}/LogServices/Dump/Entries/",
                           BMCWEB_REDFISH_SYSTEM_URI_NAME);
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

    dbus::utility::async_method_call(
        asyncResp,
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
                const char* thisInterfaceName =
                    interfaceNode->Attribute("name");
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
                    const boost::system::error_code& ec2,
                    sdbusplus::message_t& msg,
                    const std::shared_ptr<task::TaskData>& taskData) {
                    if (ec2)
                    {
                        BMCWEB_LOG_ERROR("{}: Error in creating dump",
                                         createdObjPath.str);
                        taskData->messages.emplace_back(
                            messages::internalError());
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
                        if (dumpStatus ==
                            DumpCreationProgress::DUMP_CREATE_FAILED)
                        {
                            BMCWEB_LOG_ERROR("{}: Error in creating dump",
                                             createdObjPath.str);
                            taskData->state = "Cancelled";
                            return task::completed;
                        }

                        if (dumpStatus ==
                            DumpCreationProgress::DUMP_CREATE_INPROGRESS)
                        {
                            BMCWEB_LOG_DEBUG(
                                "{}: Dump creation task is in progress",
                                createdObjPath.str);
                            return !task::completed;
                        }
                    }

                    nlohmann::json retMessage = messages::success();
                    taskData->messages.emplace_back(retMessage);

                    boost::urls::url url = boost::urls::format(
                        "/redfish/v1/Managers/{}/LogServices/Dump/Entries/{}",
                        BMCWEB_REDFISH_MANAGER_URI_NAME, dumpId);

                    std::string headerLoc = "Location: ";
                    headerLoc += url.buffer();

                    taskData->payload->httpHeaders.emplace_back(
                        std::move(headerLoc));

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
            task->startTimer(std::chrono::minutes(6));
            task->payload.emplace(payload);
            task->populateResp(asyncResp->res);
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

    if (!redfish::json_util::readJsonAction(               //
            req, asyncResp->res,                           //
            "DiagnosticDataType", diagnosticDataType,      //
            "OEMDiagnosticDataType", oemDiagnosticDataType //
            ))
    {
        return;
    }

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
        if ((*oemDiagnosticDataType != "System") ||
            (*diagnosticDataType != "OEM"))
        {
            BMCWEB_LOG_ERROR("Wrong parameter values passed");
            messages::internalError(asyncResp->res);
            return;
        }
        dumpPath = std::format("/redfish/v1/Systems/{}/LogServices/Dump/",
                               BMCWEB_REDFISH_SYSTEM_URI_NAME);
    }
    else if (dumpType == "BMC")
    {
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
        dumpPath = std::format("/redfish/v1/Managers/{}/LogServices/Dump/",
                               BMCWEB_REDFISH_MANAGER_URI_NAME);
    }
    else
    {
        BMCWEB_LOG_ERROR("CreateDump failed. Unknown dump type");
        messages::internalError(asyncResp->res);
        return;
    }

    std::vector<std::pair<std::string, std::variant<std::string, uint64_t>>>
        createDumpParamVec;

    if (req.session != nullptr)
    {
        createDumpParamVec.emplace_back(
            "xyz.openbmc_project.Dump.Create.CreateParameters.OriginatorId",
            req.session->clientIp);
        createDumpParamVec.emplace_back(
            "xyz.openbmc_project.Dump.Create.CreateParameters.OriginatorType",
            "xyz.openbmc_project.Common.OriginatedBy.OriginatorTypes.Client");
    }

    dbus::utility::async_method_call(
        asyncResp,
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
        "xyz.openbmc_project.Dump.Manager", getDumpPath(dumpType),
        "xyz.openbmc_project.Dump.Create", "CreateDump", createDumpParamVec);
}

inline void clearDump(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const std::string& dumpType)
{
    dbus::utility::async_method_call(
        asyncResp,
        [asyncResp](const boost::system::error_code& ec) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("clearDump resp_handler got error {}", ec);
                messages::internalError(asyncResp->res);
                return;
            }
            messages::success(asyncResp->res);
        },
        "xyz.openbmc_project.Dump.Manager", getDumpPath(dumpType),
        "xyz.openbmc_project.Collection.DeleteAll", "DeleteAll");
}

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
    if constexpr (!BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
        if (systemName != BMCWEB_REDFISH_SYSTEM_URI_NAME)
        {
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }
    }

    // Collections don't include the static data added by SubRoute
    // because it has a duplicate entry for members
    asyncResp->res.jsonValue["@odata.type"] =
        "#LogServiceCollection.LogServiceCollection";
    asyncResp->res.jsonValue["@odata.id"] =
        std::format("/redfish/v1/Systems/{}/LogServices", systemName);
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
            std::format("/redfish/v1/Systems/{}/LogServices/EventLog",
                        BMCWEB_REDFISH_SYSTEM_URI_NAME);
        logServiceArray.emplace_back(std::move(eventLog));
    }

    if constexpr (BMCWEB_REDFISH_DUMP_LOG &&
                  !BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
        nlohmann::json::object_t dumpLog;
        dumpLog["@odata.id"] =
            std::format("/redfish/v1/Systems/{}/LogServices/Dump", systemName);
        logServiceArray.emplace_back(std::move(dumpLog));
    }

    if constexpr (BMCWEB_REDFISH_CPU_LOG &&
                  !BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
        nlohmann::json::object_t crashdump;
        crashdump["@odata.id"] = std::format(
            "/redfish/v1/Systems/{}/LogServices/Crashdump", systemName);
        logServiceArray.emplace_back(std::move(crashdump));
    }

    if constexpr (BMCWEB_REDFISH_HOST_LOGGER &&
                  !BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
        nlohmann::json::object_t hostlogger;
        hostlogger["@odata.id"] = std::format(
            "/redfish/v1/Systems/{}/LogServices/HostLogger", systemName);
        logServiceArray.emplace_back(std::move(hostlogger));
    }
    asyncResp->res.jsonValue["Members@odata.count"] = logServiceArray.size();

    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.State.Boot.PostCode"};
    dbus::utility::getSubTreePaths(
        "/", 0, interfaces,
        [asyncResp, systemName](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreePathsResponse& subtreePath) {
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
                    member["@odata.id"] = boost::urls::format(
                        "/redfish/v1/Systems/{}/LogServices/PostCodes",
                        systemName);

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
        asyncResp, eventlog_utils::LogServiceParent::Systems);
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
        asyncResp, eventlog_utils::LogServiceParent::Managers);
}

inline void getDumpServiceInfo(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& dumpType)
{
    std::string dumpPath;
    log_service::OverWritePolicy overWritePolicy =
        log_service::OverWritePolicy::Invalid;
    bool collectDiagnosticDataSupported = false;

    if (dumpType == "BMC")
    {
        dumpPath = std::format("/redfish/v1/Managers/{}/LogServices/Dump",
                               BMCWEB_REDFISH_MANAGER_URI_NAME);
        overWritePolicy = log_service::OverWritePolicy::WrapsWhenFull;
        collectDiagnosticDataSupported = true;
    }
    else if (dumpType == "FaultLog")
    {
        dumpPath = std::format("/redfish/v1/Managers/{}/LogServices/FaultLog",
                               BMCWEB_REDFISH_MANAGER_URI_NAME);
        overWritePolicy = log_service::OverWritePolicy::Unknown;
        collectDiagnosticDataSupported = false;
    }
    else if (dumpType == "System")
    {
        dumpPath = std::format("/redfish/v1/Systems/{}/LogServices/Dump",
                               BMCWEB_REDFISH_SYSTEM_URI_NAME);
        overWritePolicy = log_service::OverWritePolicy::WrapsWhenFull;
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
    asyncResp->res.jsonValue["OverWritePolicy"] = overWritePolicy;

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

    etag_utils::setEtagOmitDateTimeHandler(asyncResp);

    constexpr std::array<std::string_view, 1> interfaces = {deleteAllInterface};
    dbus::utility::getSubTreePaths(
        "/xyz/openbmc_project/dump", 0, interfaces,
        [asyncResp, dumpType, dumpPath](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreePathsResponse& subTreePaths) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("getDumpServiceInfo respHandler got error {}",
                                 ec);
                // Assume that getting an error simply means there are no dump
                // LogServices. Return without adding any error response.
                return;
            }
            std::string dbusDumpPath = getDumpPath(dumpType);
            for (const std::string& path : subTreePaths)
            {
                if (path == dbusDumpPath)
                {
                    asyncResp->res.jsonValue["Actions"]["#LogService.ClearLog"]
                                            ["target"] =
                        dumpPath + "/Actions/LogService.ClearLog";
                    break;
                }
            }
        });
}

inline void handleLogServicesDumpServiceGet(
    crow::App& app, const std::string& dumpType, const crow::Request& req,
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
    if (chassisId != BMCWEB_REDFISH_SYSTEM_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem", chassisId);
        return;
    }
    getDumpServiceInfo(asyncResp, "System");
}

inline void handleLogServicesDumpEntriesCollectionGet(
    crow::App& app, const std::string& dumpType, const crow::Request& req,
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
    if (chassisId != BMCWEB_REDFISH_SYSTEM_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem", chassisId);
        return;
    }
    getDumpEntryCollection(asyncResp, "System");
}

inline void handleLogServicesDumpEntryGet(
    crow::App& app, const std::string& dumpType, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& managerId, const std::string& dumpId)
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
    if (chassisId != BMCWEB_REDFISH_SYSTEM_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem", chassisId);
        return;
    }
    getDumpEntryById(asyncResp, dumpId, "System");
}

inline void handleLogServicesDumpEntryDelete(
    crow::App& app, const std::string& dumpType, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& managerId, const std::string& dumpId)
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
    if (chassisId != BMCWEB_REDFISH_SYSTEM_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem", chassisId);
        return;
    }
    deleteDumpEntry(asyncResp, dumpId, "System");
}

inline void handleLogServicesDumpEntryDownloadGet(
    crow::App& app, const std::string& dumpType, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& managerId, const std::string& dumpId)
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
    downloadDumpEntry(asyncResp, dumpId, dumpType);
}

inline void handleLogServicesDumpCollectDiagnosticDataPost(
    crow::App& app, const std::string& dumpType, const crow::Request& req,
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
    createDump(asyncResp, req, "System");
}

inline void handleLogServicesDumpClearLogPost(
    crow::App& app, const std::string& dumpType, const crow::Request& req,
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
    clearDump(asyncResp, "System");
}

inline void requestRoutesBMCDumpService(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Managers/<str>/LogServices/Dump/")
        .privileges(redfish::privileges::getLogService)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleLogServicesDumpServiceGet, std::ref(app), "BMC"));
}

inline void requestRoutesBMCDumpEntryCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Managers/<str>/LogServices/Dump/Entries/")
        .privileges(redfish::privileges::getLogEntryCollection)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleLogServicesDumpEntriesCollectionGet, std::ref(app), "BMC"));
}

inline void requestRoutesBMCDumpEntry(App& app)
{
    BMCWEB_ROUTE(app,
                 "/redfish/v1/Managers/<str>/LogServices/Dump/Entries/<str>/")
        .privileges(redfish::privileges::getLogEntry)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleLogServicesDumpEntryGet, std::ref(app), "BMC"));

    BMCWEB_ROUTE(app,
                 "/redfish/v1/Managers/<str>/LogServices/Dump/Entries/<str>/")
        .privileges(redfish::privileges::deleteLogEntry)
        .methods(boost::beast::http::verb::delete_)(std::bind_front(
            handleLogServicesDumpEntryDelete, std::ref(app), "BMC"));
}

inline void requestRoutesBMCDumpEntryDownload(App& app)
{
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Managers/<str>/LogServices/Dump/Entries/<str>/attachment/")
        .privileges(redfish::privileges::getLogEntry)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleLogServicesDumpEntryDownloadGet, std::ref(app), "BMC"));
}

inline void requestRoutesBMCDumpCreate(App& app)
{
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Managers/<str>/LogServices/Dump/Actions/LogService.CollectDiagnosticData/")
        .privileges(redfish::privileges::postLogService)
        .methods(boost::beast::http::verb::post)(
            std::bind_front(handleLogServicesDumpCollectDiagnosticDataPost,
                            std::ref(app), "BMC"));
}

inline void requestRoutesBMCDumpClear(App& app)
{
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Managers/<str>/LogServices/Dump/Actions/LogService.ClearLog/")
        .privileges(redfish::privileges::postLogService)
        .methods(boost::beast::http::verb::post)(std::bind_front(
            handleLogServicesDumpClearLogPost, std::ref(app), "BMC"));
}

inline void requestRoutesFaultLogDumpService(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Managers/<str>/LogServices/FaultLog/")
        .privileges(redfish::privileges::getLogService)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleLogServicesDumpServiceGet, std::ref(app), "FaultLog"));
}

inline void requestRoutesFaultLogDumpEntryCollection(App& app)
{
    BMCWEB_ROUTE(app,
                 "/redfish/v1/Managers/<str>/LogServices/FaultLog/Entries/")
        .privileges(redfish::privileges::getLogEntryCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleLogServicesDumpEntriesCollectionGet,
                            std::ref(app), "FaultLog"));
}

inline void requestRoutesFaultLogDumpEntry(App& app)
{
    BMCWEB_ROUTE(
        app, "/redfish/v1/Managers/<str>/LogServices/FaultLog/Entries/<str>/")
        .privileges(redfish::privileges::getLogEntry)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleLogServicesDumpEntryGet, std::ref(app), "FaultLog"));

    BMCWEB_ROUTE(
        app, "/redfish/v1/Managers/<str>/LogServices/FaultLog/Entries/<str>/")
        .privileges(redfish::privileges::deleteLogEntry)
        .methods(boost::beast::http::verb::delete_)(std::bind_front(
            handleLogServicesDumpEntryDelete, std::ref(app), "FaultLog"));
}

inline void requestRoutesFaultLogDumpClear(App& app)
{
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Managers/<str>/LogServices/FaultLog/Actions/LogService.ClearLog/")
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
        .privileges(redfish::privileges::
                        postLogServiceSubOverComputerSystemLogServiceCollection)
        .methods(boost::beast::http::verb::post)(std::bind_front(
            handleLogServicesDumpCollectDiagnosticDataComputerSystemPost,
            std::ref(app)));
}

inline void requestRoutesSystemDumpClear(App& app)
{
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Systems/<str>/LogServices/Dump/Actions/LogService.ClearLog/")
        .privileges(redfish::privileges::
                        postLogServiceSubOverComputerSystemLogServiceCollection)
        .methods(boost::beast::http::verb::post)(std::bind_front(
            handleLogServicesDumpClearLogComputerSystemPost, std::ref(app)));
}

inline void requestRoutesCrashdumpService(App& app)
{
    /**
     * Functions triggers appropriate requests on DBus
     */
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/LogServices/Crashdump/")
        .privileges(redfish::privileges::getLogService)
        .methods(
            boost::beast::http::verb::
                get)([&app](const crow::Request& req,
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
            asyncResp->res.jsonValue["@odata.id"] =
                std::format("/redfish/v1/Systems/{}/LogServices/Crashdump",
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

            asyncResp->res.jsonValue["Entries"]["@odata.id"] = std::format(
                "/redfish/v1/Systems/{}/LogServices/Crashdump/Entries",
                BMCWEB_REDFISH_SYSTEM_URI_NAME);
            asyncResp->res.jsonValue["Actions"]["#LogService.ClearLog"]
                                    ["target"] = std::format(
                "/redfish/v1/Systems/{}/LogServices/Crashdump/Actions/LogService.ClearLog",
                BMCWEB_REDFISH_SYSTEM_URI_NAME);
            asyncResp->res
                .jsonValue["Actions"]["#LogService.CollectDiagnosticData"]
                          ["target"] = std::format(
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

            std::string crashdumpURI =
                std::format(
                    "/redfish/v1/Systems/{}/LogServices/Crashdump/Entries/",
                    BMCWEB_REDFISH_SYSTEM_URI_NAME) +
                logID + "/" + filename;
            nlohmann::json::object_t logEntry;
            logEntry["@odata.type"] = "#LogEntry.v1_9_0.LogEntry";
            logEntry["@odata.id"] = boost::urls::format(
                "/redfish/v1/Systems/{}/LogServices/Crashdump/Entries/{}",
                BMCWEB_REDFISH_SYSTEM_URI_NAME, logID);
            logEntry["Name"] = "CPU Crashdump";
            logEntry["Id"] = logID;
            logEntry["EntryType"] = log_entry::LogEntryType::Oem;
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
        .methods(
            boost::beast::http::verb::
                get)([&app](const crow::Request& req,
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
                    asyncResp->res.jsonValue["@odata.id"] = std::format(
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
