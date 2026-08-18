// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include "bmcweb_config.h"

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/log_entry.hpp"
#include "generated/enums/log_service.hpp"
#include "http_request.hpp"
#include "human_sort.hpp"
#include "logging.hpp"
#include "str_utility.hpp"
#include "task.hpp"
#include "utils/etag_utils.hpp"
#include "utils/json_utils.hpp"
#include "utils/log_services_utils.hpp"
#include "utils/time_utils.hpp"

#include <asm-generic/errno.h>
#include <systemd/sd-bus-protocol.h>
#include <tinyxml2.h>

#include <boost/url/format.hpp>
#include <boost/url/url.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/message/native_types.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <format>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace redfish
{
namespace dump_utils
{

constexpr const char* deleteAllInterface =
    "xyz.openbmc_project.Collection.DeleteAll";

enum class DumpCreationProgress
{
    DUMP_CREATE_SUCCESS,
    DUMP_CREATE_FAILED,
    DUMP_CREATE_INPROGRESS
};

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

static boost::urls::url getDumpEntriesPath(const std::string& dumpType)
{
    boost::urls::url entriesPath;

    if (dumpType == "BMC")
    {
        entriesPath = boost::urls::format(
            "/redfish/v1/Managers/{}/LogServices/Dump/Entries",
            BMCWEB_REDFISH_MANAGER_URI_NAME);
    }
    else if (dumpType == "FaultLog")
    {
        entriesPath = boost::urls::format(
            "/redfish/v1/Managers/{}/LogServices/FaultLog/Entries",
            BMCWEB_REDFISH_MANAGER_URI_NAME);
    }
    else if (dumpType == "System")
    {
        entriesPath = boost::urls::format(
            "/redfish/v1/Systems/{}/LogServices/Dump/Entries",
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
    boost::urls::url entriesPath = getDumpEntriesPath(dumpType);
    if (entriesPath.empty())
    {
        messages::internalError(asyncResp->res);
        return;
    }

    sdbusplus::object_path path("/xyz/openbmc_project/dump");
    dbus::utility::getManagedObjects(
        "xyz.openbmc_project.Dump.Manager", path,
        // ast-grep-ignore: long-lambda
        [asyncResp, entriesPath,
         dumpType](const boost::system::error_code& ec,
                   const dbus::utility::ManagedObjectType& objects) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("DumpEntry resp_handler got error {}", ec);
                messages::internalError(asyncResp->res);
                return;
            }

            asyncResp->res.jsonValue["@odata.type"] =
                "#LogEntryCollection.LogEntryCollection";
            asyncResp->res.jsonValue["@odata.id"] = entriesPath;
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
                if (object.first.str.contains(dumpEntryPath))
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
                thisEntry["@odata.id"] =
                    boost::urls::format("{}/{}", entriesPath, entryID);
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
                    thisEntry["AdditionalDataURI"] = boost::urls::format(
                        "{}/{}/attachment", entriesPath, entryID);
                    thisEntry["AdditionalDataSizeBytes"] = size;
                }
                else if (dumpType == "System")
                {
                    thisEntry["DiagnosticDataType"] = "OEM";
                    thisEntry["OEMDiagnosticDataType"] = "System";
                    thisEntry["AdditionalDataURI"] = boost::urls::format(
                        "{}/{}/attachment", entriesPath, entryID);
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
    boost::urls::url entriesPath = getDumpEntriesPath(dumpType);
    if (entriesPath.empty())
    {
        messages::internalError(asyncResp->res);
        return;
    }

    sdbusplus::object_path path("/xyz/openbmc_project/dump");
    dbus::utility::getManagedObjects(
        "xyz.openbmc_project.Dump.Manager", path,
        // ast-grep-ignore: long-lambda
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
                asyncResp->res.jsonValue["@odata.id"] =
                    boost::urls::format("{}/{}", entriesPath, entryID);
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
                        boost::urls::format("{}/{}/attachment", entriesPath,
                                            entryID);
                    asyncResp->res.jsonValue["AdditionalDataSizeBytes"] = size;
                }
                else if (dumpType == "System")
                {
                    asyncResp->res.jsonValue["DiagnosticDataType"] = "OEM";
                    asyncResp->res.jsonValue["OEMDiagnosticDataType"] =
                        "System";
                    asyncResp->res.jsonValue["AdditionalDataURI"] =
                        boost::urls::format("{}/{}/attachment", entriesPath,
                                            entryID);
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
    // ast-grep-ignore: long-lambda
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
    if (dumpType != "BMC" && dumpType != "System")
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

inline void createDumpTaskCallback(
    task::Payload&& payload,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const sdbusplus::object_path& createdObjPath)
{
    const std::string dumpId = createdObjPath.filename();

    dbus::utility::async_method_call(
        asyncResp,
        // ast-grep-ignore: long-lambda
        [asyncResp, payload = std::move(payload), createdObjPath,
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
                // ast-grep-ignore: long-lambda
                [createdObjPath, dumpId, isProgressIntfPresent](
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
    boost::urls::url dumpPath;
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
        dumpPath =
            boost::urls::format("/redfish/v1/Systems/{}/LogServices/Dump/",
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
        dumpPath =
            boost::urls::format("/redfish/v1/Managers/{}/LogServices/Dump/",
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
                   const sdbusplus::object_path& objPath) mutable {
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
                    messages::serviceDisabled(asyncResp->res, dumpPath.c_str());
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

inline void getDumpServiceInfo(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& dumpType)
{
    std::string serviceId;
    boost::urls::url dumpPath;
    log_service::OverWritePolicy overWritePolicy =
        log_service::OverWritePolicy::Invalid;
    bool collectDiagnosticDataSupported = false;

    if (dumpType == "BMC")
    {
        serviceId = "Dump";
        dumpPath =
            boost::urls::format("/redfish/v1/Managers/{}/LogServices/{}",
                                BMCWEB_REDFISH_MANAGER_URI_NAME, serviceId);
        overWritePolicy = log_service::OverWritePolicy::WrapsWhenFull;
        collectDiagnosticDataSupported = true;
    }
    else if (dumpType == "FaultLog")
    {
        serviceId = "FaultLog";
        dumpPath =
            boost::urls::format("/redfish/v1/Managers/{}/LogServices/{}",
                                BMCWEB_REDFISH_MANAGER_URI_NAME, serviceId);
        overWritePolicy = log_service::OverWritePolicy::Unknown;
        collectDiagnosticDataSupported = false;
    }
    else if (dumpType == "System")
    {
        serviceId = "Dump";
        dumpPath =
            boost::urls::format("/redfish/v1/Systems/{}/LogServices/{}",
                                BMCWEB_REDFISH_SYSTEM_URI_NAME, serviceId);
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
    asyncResp->res.jsonValue["Id"] = serviceId;
    asyncResp->res.jsonValue["OverWritePolicy"] = overWritePolicy;

    std::pair<std::string, std::string> redfishDateTimeOffset =
        redfish::time_utils::getDateTimeOffsetNow();
    asyncResp->res.jsonValue["DateTime"] = redfishDateTimeOffset.first;
    asyncResp->res.jsonValue["DateTimeLocalOffset"] =
        redfishDateTimeOffset.second;

    asyncResp->res.jsonValue["Entries"]["@odata.id"] =
        boost::urls::format("{}/Entries", dumpPath);

    if (collectDiagnosticDataSupported)
    {
        asyncResp->res.jsonValue["Actions"]["#LogService.CollectDiagnosticData"]
                                ["target"] = boost::urls::format(
            "{}/Actions/LogService.CollectDiagnosticData", dumpPath);
    }

    etag_utils::setEtagOmitDateTimeHandler(asyncResp);

    constexpr std::array<std::string_view, 1> interfaces = {deleteAllInterface};
    dbus::utility::getSubTreePaths(
        "/xyz/openbmc_project/dump", 0, interfaces,
        // ast-grep-ignore: long-lambda
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
                                            ["target"] = boost::urls::format(
                        "{}/Actions/LogService.ClearLog", dumpPath);
                    break;
                }
            }
        });
}

} // namespace dump_utils
} // namespace redfish
