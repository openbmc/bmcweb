// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "bmcweb_config.h"

#include "app.hpp"
#include "async_resp.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/dump_utils.hpp"

#include <boost/beast/http/verb.hpp>

#include <functional>
#include <memory>
#include <string>

namespace redfish
{

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

    dump_utils::getDumpServiceInfo(asyncResp, dumpType);
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
    dump_utils::getDumpEntryCollection(asyncResp, dumpType);
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
    dump_utils::getDumpEntryById(asyncResp, dumpId, dumpType);
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
    dump_utils::deleteDumpEntry(asyncResp, dumpId, dumpType);
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
    dump_utils::downloadDumpEntry(asyncResp, dumpId, dumpType);
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

    dump_utils::createDump(asyncResp, req, dumpType);
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
    dump_utils::clearDump(asyncResp, dumpType);
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

} // namespace redfish
