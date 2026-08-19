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

inline void handleManagersLogServicesDumpServiceGet(
    crow::App& app, dump_utils::DumpType dumpType, const crow::Request& req,
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

inline void handleManagersLogServicesDumpEntriesCollectionGet(
    crow::App& app, dump_utils::DumpType dumpType, const crow::Request& req,
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

inline void handleManagersLogServicesDumpEntryGet(
    crow::App& app, dump_utils::DumpType dumpType, const crow::Request& req,
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

inline void handleManagersLogServicesDumpEntryDelete(
    crow::App& app, dump_utils::DumpType dumpType, const crow::Request& req,
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

inline void handleManagersLogServicesDumpEntryDownloadGet(
    crow::App& app, dump_utils::DumpType dumpType, const crow::Request& req,
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

inline void handleManagersLogServicesDumpCollectDiagnosticDataPost(
    crow::App& app, dump_utils::DumpType dumpType, const crow::Request& req,
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

inline void handleManagersLogServicesDumpClearLogPost(
    crow::App& app, dump_utils::DumpType dumpType, const crow::Request& req,
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

inline void requestRoutesManagersLogServicesDumpAndFaultLog(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Managers/<str>/LogServices/Dump/")
        .privileges(redfish::privileges::getLogService)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleManagersLogServicesDumpServiceGet,
                            std::ref(app), dump_utils::DumpType::BMC));

    BMCWEB_ROUTE(app, "/redfish/v1/Managers/<str>/LogServices/Dump/Entries/")
        .privileges(redfish::privileges::getLogEntryCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleManagersLogServicesDumpEntriesCollectionGet,
                            std::ref(app), dump_utils::DumpType::BMC));

    BMCWEB_ROUTE(app,
                 "/redfish/v1/Managers/<str>/LogServices/Dump/Entries/<str>/")
        .privileges(redfish::privileges::getLogEntry)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleManagersLogServicesDumpEntryGet,
                            std::ref(app), dump_utils::DumpType::BMC));

    BMCWEB_ROUTE(app,
                 "/redfish/v1/Managers/<str>/LogServices/Dump/Entries/<str>/")
        .privileges(redfish::privileges::deleteLogEntry)
        .methods(boost::beast::http::verb::delete_)(
            std::bind_front(handleManagersLogServicesDumpEntryDelete,
                            std::ref(app), dump_utils::DumpType::BMC));

    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Managers/<str>/LogServices/Dump/Entries/<str>/attachment/")
        .privileges(redfish::privileges::getLogEntry)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleManagersLogServicesDumpEntryDownloadGet,
                            std::ref(app), dump_utils::DumpType::BMC));

    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Managers/<str>/LogServices/Dump/Actions/LogService.CollectDiagnosticData/")
        .privileges(redfish::privileges::postLogService)
        .methods(boost::beast::http::verb::post)(std::bind_front(
            handleManagersLogServicesDumpCollectDiagnosticDataPost,
            std::ref(app), dump_utils::DumpType::BMC));

    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Managers/<str>/LogServices/Dump/Actions/LogService.ClearLog/")
        .privileges(redfish::privileges::postLogService)
        .methods(boost::beast::http::verb::post)(
            std::bind_front(handleManagersLogServicesDumpClearLogPost,
                            std::ref(app), dump_utils::DumpType::BMC));

    /* FaultLog routes */

    BMCWEB_ROUTE(app, "/redfish/v1/Managers/<str>/LogServices/FaultLog/")
        .privileges(redfish::privileges::getLogService)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleManagersLogServicesDumpServiceGet,
                            std::ref(app), dump_utils::DumpType::FaultLog));

    BMCWEB_ROUTE(app,
                 "/redfish/v1/Managers/<str>/LogServices/FaultLog/Entries/")
        .privileges(redfish::privileges::getLogEntryCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleManagersLogServicesDumpEntriesCollectionGet,
                            std::ref(app), dump_utils::DumpType::FaultLog));

    BMCWEB_ROUTE(
        app, "/redfish/v1/Managers/<str>/LogServices/FaultLog/Entries/<str>/")
        .privileges(redfish::privileges::getLogEntry)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleManagersLogServicesDumpEntryGet,
                            std::ref(app), dump_utils::DumpType::FaultLog));

    BMCWEB_ROUTE(
        app, "/redfish/v1/Managers/<str>/LogServices/FaultLog/Entries/<str>/")
        .privileges(redfish::privileges::deleteLogEntry)
        .methods(boost::beast::http::verb::delete_)(
            std::bind_front(handleManagersLogServicesDumpEntryDelete,
                            std::ref(app), dump_utils::DumpType::FaultLog));

    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Managers/<str>/LogServices/FaultLog/Actions/LogService.ClearLog/")
        .privileges(redfish::privileges::postLogService)
        .methods(boost::beast::http::verb::post)(
            std::bind_front(handleManagersLogServicesDumpClearLogPost,
                            std::ref(app), dump_utils::DumpType::FaultLog));
}
} // namespace redfish
