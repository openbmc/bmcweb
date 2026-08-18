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
    dump_utils::getDumpServiceInfo(asyncResp, "System");
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
    dump_utils::getDumpEntryCollection(asyncResp, "System");
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
    dump_utils::getDumpEntryById(asyncResp, dumpId, "System");
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
    dump_utils::deleteDumpEntry(asyncResp, dumpId, "System");
}

inline void handleLogServicesSystemDumpEntryDownloadGet(
    crow::App& app, const std::string& dumpType, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& dumpId)
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

    dump_utils::downloadDumpEntry(asyncResp, dumpId, dumpType);
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
    dump_utils::createDump(asyncResp, req, "System");
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
    dump_utils::clearDump(asyncResp, "System");
}

inline void requestRoutesSystemsLogServicesDump(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/LogServices/Dump/")
        .privileges(redfish::privileges::getLogService)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleLogServicesDumpServiceComputerSystemGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/LogServices/Dump/Entries/")
        .privileges(redfish::privileges::getLogEntryCollection)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleLogServicesDumpEntriesCollectionComputerSystemGet,
            std::ref(app)));

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

    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Systems/<str>/LogServices/Dump/Entries/<str>/attachment/")
        .privileges(redfish::privileges::getLogEntry)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleLogServicesSystemDumpEntryDownloadGet,
                            std::ref(app), "System"));

    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Systems/<str>/LogServices/Dump/Actions/LogService.CollectDiagnosticData/")
        .privileges(redfish::privileges::
                        postLogServiceSubOverComputerSystemLogServiceCollection)
        .methods(boost::beast::http::verb::post)(std::bind_front(
            handleLogServicesDumpCollectDiagnosticDataComputerSystemPost,
            std::ref(app)));

    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Systems/<str>/LogServices/Dump/Actions/LogService.ClearLog/")
        .privileges(redfish::privileges::
                        postLogServiceSubOverComputerSystemLogServiceCollection)
        .methods(boost::beast::http::verb::post)(std::bind_front(
            handleLogServicesDumpClearLogComputerSystemPost, std::ref(app)));
}
} // namespace redfish
