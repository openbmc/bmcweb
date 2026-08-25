// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "bmcweb_config.h"

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/log_entry.hpp"
#include "http_body.hpp"
#include "http_request.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/collection.hpp"
#include "utils/dbus_event_log_entry.hpp"
#include "utils/dbus_utils.hpp"
#include "utils/log_services_utils.hpp"
#include "utils/time_utils.hpp"

#include <asm-generic/errno.h>
#include <unistd.h>

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/url/format.hpp>
#include <boost/url/url.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/message/native_types.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <array>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace redfish
{

// Renders the CPER LogEntry JSON for an entry already known to carry raw
// CPER. Membership in this service is decided by the presence of the
// xyz.openbmc_project.Logging.CPER.Raw interface on the entry object (the
// plugin registers it only when a raw CPER artifact exists), not by any
// property read here. The processed CPER (Oem) stays on the phosphor-logging
// EventLog entry, which links here 1:1 by entry Id via Links/RelatedLogEntries;
// the raw entry deliberately does not link back, to avoid a $expand cycle.
inline void fillRawCperLogEntryFromPropertyMap(
    const dbus::utility::DBusPropertiesMap& resp, const std::string& entryId,
    nlohmann::json& entryJson)
{
    std::string_view parentCollection;
    std::string_view parentMemberId;
    if constexpr (BMCWEB_REDFISH_EVENTLOG_LOCATION == "systems")
    {
        parentCollection = "Systems";
        parentMemberId = BMCWEB_REDFISH_SYSTEM_URI_NAME;
    }
    if constexpr (BMCWEB_REDFISH_EVENTLOG_LOCATION == "managers")
    {
        parentCollection = "Managers";
        parentMemberId = BMCWEB_REDFISH_MANAGER_URI_NAME;
    }

    const std::string* diagnosticDataType = nullptr;
    const std::string* notificationType = nullptr;
    const std::string* sectionType = nullptr;

    // clang-format off
    sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), resp,
        "DiagnosticDataType", diagnosticDataType,
        "NotificationType", notificationType,
        "SectionType", sectionType
    );
    // clang-format on

    entryJson["@odata.type"] = "#LogEntry.v1_15_0.LogEntry";
    entryJson["@odata.id"] =
        boost::urls::format("/redfish/v1/{}/{}/LogServices/CPER/Entries/{}",
                            parentCollection, parentMemberId, entryId);
    entryJson["Id"] = entryId;
    entryJson["Name"] = "Raw CPER Log Entry";
    entryJson["EntryType"] = "Event";
    log_entry::LogDiagnosticDataTypes dataType =
        log_entry::LogDiagnosticDataTypes::Invalid;
    if (diagnosticDataType != nullptr)
    {
        dataType =
            translateDiagnosticDataTypeDbusToRedfish(*diagnosticDataType);
        if (dataType != log_entry::LogDiagnosticDataTypes::Invalid)
        {
            entryJson["DiagnosticDataType"] = dataType;
        }
        else
        {
            BMCWEB_LOG_WARNING("Unrecognized CPER DiagnosticDataType {}",
                               *diagnosticDataType);
        }
    }

    // Per LogEntry schema, each GUID is only valid for its DiagnosticDataType.
    nlohmann::json::object_t cper;
    if (dataType == log_entry::LogDiagnosticDataTypes::CPER &&
        notificationType != nullptr && !notificationType->empty())
    {
        cper["NotificationType"] = *notificationType;
    }
    if (dataType == log_entry::LogDiagnosticDataTypes::CPERSection &&
        sectionType != nullptr && !sectionType->empty())
    {
        cper["SectionType"] = *sectionType;
    }
    entryJson["CPER"] = std::move(cper);

    // For CPER/CPERSection entries the raw binary is served via the attachment
    // (backed by CPER.Raw GetFileHandle).
    entryJson["AdditionalDataURI"] = boost::urls::format(
        "/redfish/v1/{}/{}/LogServices/CPER/Entries/{}/attachment",
        parentCollection, parentMemberId, entryId);
}

inline void handleRawCperLogServiceGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& parentId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    std::string_view parentCollection;
    std::string_view parentMemberId;
    std::string_view parentType;
    if constexpr (BMCWEB_REDFISH_EVENTLOG_LOCATION == "systems")
    {
        parentCollection = "Systems";
        parentMemberId = BMCWEB_REDFISH_SYSTEM_URI_NAME;
        parentType = "ComputerSystem";
    }
    if constexpr (BMCWEB_REDFISH_EVENTLOG_LOCATION == "managers")
    {
        parentCollection = "Managers";
        parentMemberId = BMCWEB_REDFISH_MANAGER_URI_NAME;
        parentType = "Manager";
    }
    if (parentId != parentMemberId)
    {
        messages::resourceNotFound(asyncResp->res, parentType, parentId);
        return;
    }

    asyncResp->res.jsonValue["@odata.type"] = "#LogService.v1_2_0.LogService";
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/{}/{}/LogServices/CPER", parentCollection, parentMemberId);
    asyncResp->res.jsonValue["Name"] = "Raw CPER Log Service";
    asyncResp->res.jsonValue["Description"] =
        "Raw CPER Log Service holding the raw CPER binary for CPER events";
    asyncResp->res.jsonValue["Id"] = "CPER";
    asyncResp->res.jsonValue["Entries"]["@odata.id"] =
        boost::urls::format("/redfish/v1/{}/{}/LogServices/CPER/Entries",
                            parentCollection, parentMemberId);

    std::pair<std::string, std::string> redfishDateTimeOffset =
        redfish::time_utils::getDateTimeOffsetNow();
    asyncResp->res.jsonValue["DateTime"] = redfishDateTimeOffset.first;
    asyncResp->res.jsonValue["DateTimeLocalOffset"] =
        redfishDateTimeOffset.second;
}

inline void handleRawCperEntryCollection(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& parentId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    std::string_view parentCollection;
    std::string_view parentMemberId;
    std::string_view parentType;
    if constexpr (BMCWEB_REDFISH_EVENTLOG_LOCATION == "systems")
    {
        parentCollection = "Systems";
        parentMemberId = BMCWEB_REDFISH_SYSTEM_URI_NAME;
        parentType = "ComputerSystem";
    }
    if constexpr (BMCWEB_REDFISH_EVENTLOG_LOCATION == "managers")
    {
        parentCollection = "Managers";
        parentMemberId = BMCWEB_REDFISH_MANAGER_URI_NAME;
        parentType = "Manager";
    }
    if (parentId != parentMemberId)
    {
        messages::resourceNotFound(asyncResp->res, parentType, parentId);
        return;
    }

    boost::urls::url collectionPath =
        boost::urls::format("/redfish/v1/{}/{}/LogServices/CPER/Entries",
                            parentCollection, parentMemberId);
    asyncResp->res.jsonValue["@odata.type"] =
        "#LogEntryCollection.LogEntryCollection";
    asyncResp->res.jsonValue["@odata.id"] = collectionPath;
    asyncResp->res.jsonValue["Name"] = "Raw CPER Log Entries";
    asyncResp->res.jsonValue["Description"] =
        "Collection of Raw CPER Log Entries";

    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Logging.CPER.Raw"};
    collection_util::getCollectionMembers(asyncResp, collectionPath, interfaces,
                                          "/xyz/openbmc_project/logging");
}

inline void afterRawCperEntryGetObject(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& entryId, const dbus::utility::DBusPropertiesMap& resp,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetObject& object)
{
    // Mapper reports a missing object/interface as io_error.
    if (ec == boost::system::errc::io_error || ec.value() == EBADR)
    {
        messages::resourceNotFound(asyncResp->res, "LogEntry", entryId);
        return;
    }
    if (ec)
    {
        BMCWEB_LOG_ERROR("CPER GetObject got error {}", ec);
        messages::internalError(asyncResp->res);
        return;
    }
    if (object.empty())
    {
        messages::resourceNotFound(asyncResp->res, "LogEntry", entryId);
        return;
    }
    fillRawCperLogEntryFromPropertyMap(resp, entryId, asyncResp->res.jsonValue);
}

inline void afterRawCperEntryGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& entryId, const boost::system::error_code& ec,
    const dbus::utility::DBusPropertiesMap& resp)
{
    if (ec.value() == EBADR)
    {
        messages::resourceNotFound(asyncResp->res, "LogEntry", entryId);
        return;
    }
    if (ec)
    {
        BMCWEB_LOG_ERROR("CPER LogEntry (DBus) resp_handler got error {}", ec);
        messages::internalError(asyncResp->res);
        return;
    }

    // Membership is signalled by the xyz.openbmc_project.Logging.CPER.Raw
    // interface, which has no properties, so detect it via the object mapper
    // rather than the property map returned by GetAll.
    constexpr std::array<std::string_view, 1> rawCperIfaces = {
        "xyz.openbmc_project.Logging.CPER.Raw"};
    dbus::utility::getDbusObject(
        sdbusplus::object_path("/xyz/openbmc_project/logging/entry") / entryId,
        rawCperIfaces,
        [asyncResp, entryId,
         resp](const boost::system::error_code& ec2,
               const dbus::utility::MapperGetObject& object) {
            afterRawCperEntryGetObject(asyncResp, entryId, resp, ec2, object);
        });
}

inline void handleRawCperEntryGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& parentId, const std::string& entryId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    std::string_view parentMemberId;
    std::string_view parentType;
    if constexpr (BMCWEB_REDFISH_EVENTLOG_LOCATION == "systems")
    {
        parentMemberId = BMCWEB_REDFISH_SYSTEM_URI_NAME;
        parentType = "ComputerSystem";
    }
    if constexpr (BMCWEB_REDFISH_EVENTLOG_LOCATION == "managers")
    {
        parentMemberId = BMCWEB_REDFISH_MANAGER_URI_NAME;
        parentType = "Manager";
    }
    if (parentId != parentMemberId)
    {
        messages::resourceNotFound(asyncResp->res, parentType, parentId);
        return;
    }

    dbus::utility::getAllProperties(
        "xyz.openbmc_project.Logging",
        sdbusplus::object_path("/xyz/openbmc_project/logging/entry") / entryId,
        "", std::bind_front(afterRawCperEntryGet, asyncResp, entryId));
}

inline void afterRawCperEntryAttachment(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& entryId, const boost::system::error_code& ec,
    const sdbusplus::message::unix_fd& unixfd)
{
    // EBADR: no such entry, or the entry has no CPER.Raw interface.
    if (ec.value() == EBADR)
    {
        messages::resourceNotFound(asyncResp->res, "LogEntry", entryId);
        return;
    }
    if (ec)
    {
        BMCWEB_LOG_ERROR("CPER GetFileHandle got error {}", ec);
        messages::internalError(asyncResp->res);
        return;
    }

    int fd = dup(unixfd);
    if (fd < 0)
    {
        BMCWEB_LOG_ERROR("Failed to dup CPER fd");
        messages::internalError(asyncResp->res);
        return;
    }
    // The response body's file handle owns fd from here and closes it.
    if (!asyncResp->res.openFd(fd))
    {
        BMCWEB_LOG_ERROR("Failed to open CPER fd");
        messages::internalError(asyncResp->res);
        return;
    }
    if (!log_services_utils::checkSizeLimit(fd))
    {
        messages::internalError(asyncResp->res);
        return;
    }
    asyncResp->res.addHeader(boost::beast::http::field::content_type,
                             "application/octet-stream");
}

inline void handleRawCperEntryAttachment(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& parentId, const std::string& entryId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    std::string_view parentMemberId;
    std::string_view parentType;
    if constexpr (BMCWEB_REDFISH_EVENTLOG_LOCATION == "systems")
    {
        parentMemberId = BMCWEB_REDFISH_SYSTEM_URI_NAME;
        parentType = "ComputerSystem";
    }
    if constexpr (BMCWEB_REDFISH_EVENTLOG_LOCATION == "managers")
    {
        parentMemberId = BMCWEB_REDFISH_MANAGER_URI_NAME;
        parentType = "Manager";
    }
    if (parentId != parentMemberId)
    {
        messages::resourceNotFound(asyncResp->res, parentType, parentId);
        return;
    }

    // Entries without raw CPER data have no CPER.Raw interface, so this fails
    // with EBADR for them.
    // Must be a lambda: async_method_call can't deduce std::bind_front args.
    dbus::utility::async_method_call(
        [asyncResp, entryId](const boost::system::error_code& ec,
                             const sdbusplus::message::unix_fd& unixfd) {
            afterRawCperEntryAttachment(asyncResp, entryId, ec, unixfd);
        },
        "xyz.openbmc_project.Logging",
        sdbusplus::object_path("/xyz/openbmc_project/logging/entry") / entryId,
        "xyz.openbmc_project.Logging.CPER.Raw", "GetFileHandle");
}

inline void requestRoutesRawCper(App& app)
{
    if constexpr (BMCWEB_REDFISH_EVENTLOG_LOCATION == "managers")
    {
        BMCWEB_ROUTE(app, "/redfish/v1/Managers/<str>/LogServices/CPER/")
            .privileges(redfish::privileges::getLogService)
            .methods(boost::beast::http::verb::get)(
                std::bind_front(handleRawCperLogServiceGet, std::ref(app)));

        BMCWEB_ROUTE(app,
                     "/redfish/v1/Managers/<str>/LogServices/CPER/Entries/")
            .privileges(redfish::privileges::getLogEntryCollection)
            .methods(boost::beast::http::verb::get)(
                std::bind_front(handleRawCperEntryCollection, std::ref(app)));

        BMCWEB_ROUTE(
            app, "/redfish/v1/Managers/<str>/LogServices/CPER/Entries/<str>/")
            .privileges(redfish::privileges::getLogEntry)
            .methods(boost::beast::http::verb::get)(
                std::bind_front(handleRawCperEntryGet, std::ref(app)));

        BMCWEB_ROUTE(
            app,
            "/redfish/v1/Managers/<str>/LogServices/CPER/Entries/<str>/attachment/")
            .privileges(redfish::privileges::getLogEntry)
            .methods(boost::beast::http::verb::get)(
                std::bind_front(handleRawCperEntryAttachment, std::ref(app)));
    }
    else
    {
        BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/LogServices/CPER/")
            .privileges(redfish::privileges::getLogService)
            .methods(boost::beast::http::verb::get)(
                std::bind_front(handleRawCperLogServiceGet, std::ref(app)));

        BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/LogServices/CPER/Entries/")
            .privileges(redfish::privileges::getLogEntryCollection)
            .methods(boost::beast::http::verb::get)(
                std::bind_front(handleRawCperEntryCollection, std::ref(app)));

        BMCWEB_ROUTE(
            app, "/redfish/v1/Systems/<str>/LogServices/CPER/Entries/<str>/")
            .privileges(redfish::privileges::getLogEntry)
            .methods(boost::beast::http::verb::get)(
                std::bind_front(handleRawCperEntryGet, std::ref(app)));

        BMCWEB_ROUTE(
            app,
            "/redfish/v1/Systems/<str>/LogServices/CPER/Entries/<str>/attachment/")
            .privileges(redfish::privileges::getLogEntry)
            .methods(boost::beast::http::verb::get)(
                std::bind_front(handleRawCperEntryAttachment, std::ref(app)));
    }
}

} // namespace redfish
