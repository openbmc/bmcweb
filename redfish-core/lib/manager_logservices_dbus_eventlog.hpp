// SPDX-FileCopyrightText: Copyright OpenBMC Authors
// SPDX-FileCopyrightText: Copyright 2018 Intel Corporation
#pragma once

#include "bmcweb_config.h"

#include "app.hpp"
#include "async_resp.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/eventlog_utils.hpp"

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

#include <memory>
#include <string>
#include <string_view>

namespace redfish
{

inline void requestRoutesBMCDBusEventLogEntryCollection(App& app)
{
    BMCWEB_ROUTE(app,
                 "/redfish/v1/Managers/<str>/LogServices/EventLog/Entries/")
        .privileges(redfish::privileges::getLogEntryCollection)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& managerId) {
                if (!redfish::setUpRedfishRoute(app, req, asyncResp))
                {
                    return;
                }
                if (managerId != BMCWEB_REDFISH_MANAGER_URI_NAME)
                {
                    messages::resourceNotFound(asyncResp->res, "Manager",
                                               managerId);
                    return;
                }
                eventlog_utils::dBusEventLogEntryCollection(asyncResp,
                                                            "Managers");
            });
}

inline void requestRoutesBMCDBusEventLogEntry(App& app)
{
    BMCWEB_ROUTE(
        app, "/redfish/v1/Managers/<str>/LogServices/EventLog/Entries/<str>/")
        .privileges(redfish::privileges::getLogEntry)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& managerId, const std::string& entryId) {
                if (!redfish::setUpRedfishRoute(app, req, asyncResp))
                {
                    return;
                }
                if (managerId != BMCWEB_REDFISH_MANAGER_URI_NAME)
                {
                    messages::resourceNotFound(asyncResp->res, "Manager",
                                               managerId);
                    return;
                }

                eventlog_utils::dBusEventLogEntryGet(asyncResp, "Managers",
                                                     entryId);
            });

    BMCWEB_ROUTE(
        app, "/redfish/v1/Managers/<str>/LogServices/EventLog/Entries/<str>/")
        .privileges(redfish::privileges::patchLogEntry)
        .methods(boost::beast::http::verb::patch)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& managerId, const std::string& entryId) {
                if (!redfish::setUpRedfishRoute(app, req, asyncResp))
                {
                    return;
                }
                if (managerId != BMCWEB_REDFISH_MANAGER_URI_NAME)
                {
                    messages::resourceNotFound(asyncResp->res, "Manager",
                                               managerId);
                    return;
                }

                eventlog_utils::dBusEventLogEntryPatch(req, asyncResp, entryId);
            });

    BMCWEB_ROUTE(
        app, "/redfish/v1/Managers/<str>/LogServices/EventLog/Entries/<str>/")
        .privileges(
            redfish::privileges::
                deleteLogEntrySubOverComputerSystemLogServiceCollectionLogServiceLogEntryCollection)
        .methods(boost::beast::http::verb::delete_)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& managerId, const std::string& param) {
                if (!redfish::setUpRedfishRoute(app, req, asyncResp))
                {
                    return;
                }
                if (managerId != BMCWEB_REDFISH_MANAGER_URI_NAME)
                {
                    messages::resourceNotFound(asyncResp->res, "Manager",
                                               managerId);
                    return;
                }
                eventlog_utils::dBusEventLogEntryDelete(asyncResp, param);
            });
}

/**
 * DBusLogServiceActionsClear class supports POST method for ClearLog action.
 */
inline void requestRoutesBMCDBusLogServiceActionsClear(App& app)
{
    /**
     * Function handles POST method request.
     * The Clear Log actions does not require any parameter.The action deletes
     * all entries found in the Entries collection for this Log Service.
     */

    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Managers/<str>/LogServices/EventLog/Actions/LogService.ClearLog/")
        .privileges(redfish::privileges::
                        postLogServiceSubOverComputerSystemLogServiceCollection)
        .methods(boost::beast::http::verb::post)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& managerId) {
                if (!redfish::setUpRedfishRoute(app, req, asyncResp))
                {
                    return;
                }
                if (managerId != BMCWEB_REDFISH_MANAGER_URI_NAME)
                {
                    messages::resourceNotFound(asyncResp->res, "Manager",
                                               managerId);
                    return;
                }
                eventlog_utils::dBusLogServiceActionsClear(asyncResp);
            });
}

inline void requestRoutesBMCDBusEventLogEntryDownload(App& app)
{
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Managers/<str>/LogServices/EventLog/Entries/<str>/attachment/")
        .privileges(redfish::privileges::getLogEntry)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& managerId, const std::string& entryId) {
                if (managerId != BMCWEB_REDFISH_MANAGER_URI_NAME)
                {
                    messages::resourceNotFound(asyncResp->res, "Manager",
                                               managerId);
                    return;
                }
                eventlog_utils::handleDBusEventLogEntryDownloadGet(
                    app, "System", req, asyncResp, entryId);
            });
}
} // namespace redfish
