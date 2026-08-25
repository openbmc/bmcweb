// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "bmcweb_config.h"

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "http_body.hpp"
#include "http_request.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/dbus_event_log_entry.hpp"
#include "utils/dbus_utils.hpp"
#include "utils/json_utils.hpp"
#include "utils/log_services_utils.hpp"
#include "utils/time_utils.hpp"

#include <unistd.h>

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/url/format.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/message/native_types.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace redfish
{

// Renders the CPERRawLog LogEntry JSON for an entry already known to carry raw
// CPER. Membership in this service is decided by the presence of the
// xyz.openbmc_project.Logging.CPERRaw interface on the entry object (the plugin
// registers it only when a raw CPER artifact exists), not by any property read
// here. The processed CPER (Oem) stays on the phosphor-logging EventLog entry;
// the two are cross-linked 1:1 by entry Id via Links/RelatedLogEntries.
inline void fillRawCperLogEntryFromPropertyMap(
    const dbus::utility::DBusPropertiesMap& resp, const std::string& entryId,
    nlohmann::json& entryJson)
{
    const std::string* notificationType = nullptr;
    const std::string* sectionType = nullptr;

    // clang-format off
    sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), resp,
        "NotificationType", notificationType,
        "SectionType", sectionType
    );
    // clang-format on

    // A present SectionType indicates a single-section record.
    const bool isSection = (sectionType != nullptr && !sectionType->empty());

    entryJson["@odata.type"] = "#LogEntry.v1_9_0.LogEntry";
    entryJson["@odata.id"] = boost::urls::format(
        "/redfish/v1/Managers/{}/LogServices/CPERRawLog/Entries/{}",
        BMCWEB_REDFISH_MANAGER_URI_NAME, entryId);
    entryJson["Id"] = entryId;
    entryJson["Name"] = "Raw CPER Log Entry";
    entryJson["EntryType"] = "Event";
    entryJson["DiagnosticDataType"] = isSection ? "CPERSection" : "CPER";

    nlohmann::json& cper = entryJson["CPER"];
    if (notificationType != nullptr && !notificationType->empty())
    {
        cper["NotificationType"] = *notificationType;
    }
    if (isSection)
    {
        cper["SectionType"] = *sectionType;
    }

    // For CPER/CPERSection entries the raw binary is served via the attachment
    // (backed by the CPERRaw/GetCPERRaw interface).
    entryJson["AdditionalDataURI"] = boost::urls::format(
        "/redfish/v1/Managers/{}/LogServices/CPERRawLog/Entries/{}/attachment",
        BMCWEB_REDFISH_MANAGER_URI_NAME, entryId);

    // Link back to the processed phosphor-logging EventLog entry.
    std::string_view eventLogParent =
        (BMCWEB_REDFISH_EVENTLOG_LOCATION == "managers")
            ? "Managers"
            : "Systems";
    std::string_view eventLogMember =
        (BMCWEB_REDFISH_EVENTLOG_LOCATION == "managers")
            ? std::string_view(BMCWEB_REDFISH_MANAGER_URI_NAME)
            : std::string_view(BMCWEB_REDFISH_SYSTEM_URI_NAME);

    nlohmann::json::array_t related;
    nlohmann::json::object_t relatedRef;
    relatedRef["@odata.id"] =
        boost::urls::format("/redfish/v1/{}/{}/LogServices/EventLog/Entries/{}",
                            eventLogParent, eventLogMember, entryId);
    related.emplace_back(std::move(relatedRef));
    entryJson["Links"]["RelatedLogEntries@odata.count"] = related.size();
    entryJson["Links"]["RelatedLogEntries"] = std::move(related);
}

inline void handleManagersRawCperLogServiceGet(
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

    asyncResp->res.jsonValue["@odata.type"] = "#LogService.v1_2_0.LogService";
    asyncResp->res.jsonValue["@odata.id"] =
        boost::urls::format("/redfish/v1/Managers/{}/LogServices/CPERRawLog",
                            BMCWEB_REDFISH_MANAGER_URI_NAME);
    asyncResp->res.jsonValue["Name"] = "Raw CPER Log Service";
    asyncResp->res.jsonValue["Description"] =
        "Raw CPER Log Service holding the raw CPER binary for CPER events";
    asyncResp->res.jsonValue["Id"] = "CPERRawLog";
    asyncResp->res.jsonValue["Entries"]["@odata.id"] = boost::urls::format(
        "/redfish/v1/Managers/{}/LogServices/CPERRawLog/Entries",
        BMCWEB_REDFISH_MANAGER_URI_NAME);

    std::pair<std::string, std::string> redfishDateTimeOffset =
        redfish::time_utils::getDateTimeOffsetNow();
    asyncResp->res.jsonValue["DateTime"] = redfishDateTimeOffset.first;
    asyncResp->res.jsonValue["DateTimeLocalOffset"] =
        redfishDateTimeOffset.second;
}

inline void afterManagersRawCperEntryCollection(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const dbus::utility::ManagedObjectType& resp)
{
    if (ec)
    {
        if (ec.value() == EBADR || ec == boost::system::errc::host_unreachable)
        {
            BMCWEB_LOG_DEBUG(
                "CPERRawLog entry collection unavailable on DBus, returning empty collection: {}",
                ec);
            asyncResp->res.jsonValue["Members@odata.count"] = 0;
            asyncResp->res.jsonValue["Members"] = nlohmann::json::array();
            return;
        }
        BMCWEB_LOG_ERROR(
            "CPERRawLog getManagedObjects resp_handler got error {}", ec);
        messages::internalError(asyncResp->res);
        return;
    }

    nlohmann::json::array_t entriesArray;
    for (const auto& objectPath : resp)
    {
        // Must be a phosphor-logging entry.
        auto isEntry =
            std::ranges::find_if(objectPath.second, [](const auto& object) {
                return object.first == "xyz.openbmc_project.Logging.Entry";
            });
        if (isEntry == objectPath.second.end())
        {
            continue;
        }

        dbus::utility::DBusPropertiesMap propsFlattened;
        for (const auto& interfaceMap : objectPath.second)
        {
            for (const auto& propertyMap : interfaceMap.second)
            {
                propsFlattened.emplace_back(propertyMap.first,
                                            propertyMap.second);
            }
        }

        // Raw-CPER membership: the CPERProcessed/CPERRaw interfaces are
        // attached to non-CPER entries (e.g. sensor threshold events) too, so
        // their presence isn't sufficient. A set NotificationType is the signal
        // that the entry has a raw CPER record. Decided synchronously here to
        // avoid a per-entry GetCPERRaw fd probe that would stall the
        // collection.
        const std::string* notificationType = nullptr;
        sdbusplus::unpackPropertiesNoThrow(dbus_utils::UnpackErrorPrinter(),
                                           propsFlattened, "NotificationType",
                                           notificationType);
        if (notificationType == nullptr || notificationType->empty())
        {
            continue;
        }

        std::optional<DbusEventLogEntry> optEntry =
            fillDbusEventLogEntryFromPropertyMap(propsFlattened);
        if (!optEntry.has_value())
        {
            continue;
        }

        std::string entryId = std::to_string(optEntry->Id);
        nlohmann::json& entryJson = entriesArray.emplace_back();
        fillRawCperLogEntryFromPropertyMap(propsFlattened, entryId, entryJson);
    }

    redfish::json_util::sortJsonArrayByKey(entriesArray, "Id");
    asyncResp->res.jsonValue["Members@odata.count"] = entriesArray.size();
    asyncResp->res.jsonValue["Members"] = std::move(entriesArray);
}

inline void handleManagersRawCperEntryCollection(
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

    asyncResp->res.jsonValue["@odata.type"] =
        "#LogEntryCollection.LogEntryCollection";
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Managers/{}/LogServices/CPERRawLog/Entries",
        BMCWEB_REDFISH_MANAGER_URI_NAME);
    asyncResp->res.jsonValue["Name"] = "Raw CPER Log Entries";
    asyncResp->res.jsonValue["Description"] =
        "Collection of Raw CPER Log Entries";

    sdbusplus::object_path path("/xyz/openbmc_project/logging");
    dbus::utility::getManagedObjects(
        "xyz.openbmc_project.Logging", path,
        [asyncResp](const boost::system::error_code& ec,
                    const dbus::utility::ManagedObjectType& managedResp) {
            afterManagersRawCperEntryCollection(asyncResp, ec, managedResp);
        });
}

inline void afterManagersRawCperEntryGet(
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
        BMCWEB_LOG_ERROR("CPERRawLog LogEntry (DBus) resp_handler got error {}",
                         ec);
        messages::internalError(asyncResp->res);
        return;
    }

    // Membership: the CPERProcessed/CPERRaw interfaces are attached to non-CPER
    // entries (e.g. sensor threshold events) too, so a set NotificationType is
    // the signal that a raw CPER record exists. Absent/empty => not part of
    // this service.
    const std::string* notificationType = nullptr;
    sdbusplus::unpackPropertiesNoThrow(dbus_utils::UnpackErrorPrinter(), resp,
                                       "NotificationType", notificationType);
    if (notificationType == nullptr || notificationType->empty())
    {
        messages::resourceNotFound(asyncResp->res, "LogEntry", entryId);
        return;
    }

    fillRawCperLogEntryFromPropertyMap(resp, entryId, asyncResp->res.jsonValue);
}

inline void handleManagersRawCperEntryGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& managerId, const std::string& entryId)
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

    std::string dbusEntryId = entryId;
    dbus::utility::escapePathForDbus(dbusEntryId);

    dbus::utility::getAllProperties(
        "xyz.openbmc_project.Logging",
        "/xyz/openbmc_project/logging/entry/" + dbusEntryId, "",
        std::bind_front(afterManagersRawCperEntryGet, asyncResp, entryId));
}

inline void afterManagersRawCperEntryAttachment(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& entryId, const boost::system::error_code& ec,
    const sdbusplus::message::unix_fd& unixfd)
{
    if (ec.value() == EBADR)
    {
        messages::resourceNotFound(asyncResp->res, "LogEntry", entryId);
        return;
    }
    if (ec)
    {
        // GetCPERRaw fails when the entry carries no raw CPER artifact.
        messages::resourceNotFound(asyncResp->res, "LogEntry", entryId);
        return;
    }

    int fd = dup(unixfd);
    if (fd < 0)
    {
        BMCWEB_LOG_ERROR("Failed to dup CPER fd");
        messages::internalError(asyncResp->res);
        return;
    }
    if (!log_services_utils::checkSizeLimit(fd, asyncResp->res))
    {
        close(fd);
        return;
    }
    // Stream the raw CPER binary straight from the fd as octet-stream.
    if (!asyncResp->res.openFd(fd))
    {
        messages::internalError(asyncResp->res);
        close(fd);
        return;
    }
    asyncResp->res.addHeader(boost::beast::http::field::content_type,
                             "application/octet-stream");
}

inline void handleManagersRawCperEntryAttachment(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& managerId, const std::string& entryId)
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

    std::string dbusEntryId = entryId;
    dbus::utility::escapePathForDbus(dbusEntryId);

    // Fetch the raw CPER binary via the authoritative fd interface; the fd both
    // supplies the bytes and confirms membership in this service.
    // async_method_call introspects the handler via callable_traits, so it must
    // be a plain lambda (not std::bind_front, whose operator() is templated).
    dbus::utility::async_method_call(
        [asyncResp, entryId](const boost::system::error_code& ec,
                             const sdbusplus::message::unix_fd& unixfd) {
            afterManagersRawCperEntryAttachment(asyncResp, entryId, ec, unixfd);
        },
        "xyz.openbmc_project.Logging",
        "/xyz/openbmc_project/logging/entry/" + dbusEntryId,
        "xyz.openbmc_project.Logging.CPERRaw", "GetCPERRaw");
}

inline void requestRoutesManagersRawCper(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Managers/<str>/LogServices/CPERRawLog/")
        .privileges(redfish::privileges::getLogService)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleManagersRawCperLogServiceGet, std::ref(app)));

    BMCWEB_ROUTE(app,
                 "/redfish/v1/Managers/<str>/LogServices/CPERRawLog/Entries/")
        .privileges(redfish::privileges::getLogEntryCollection)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleManagersRawCperEntryCollection, std::ref(app)));

    BMCWEB_ROUTE(
        app, "/redfish/v1/Managers/<str>/LogServices/CPERRawLog/Entries/<str>/")
        .privileges(redfish::privileges::getLogEntry)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleManagersRawCperEntryGet, std::ref(app)));

    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Managers/<str>/LogServices/CPERRawLog/Entries/<str>/attachment/")
        .privileges(redfish::privileges::getLogEntry)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleManagersRawCperEntryAttachment, std::ref(app)));
}

} // namespace redfish
