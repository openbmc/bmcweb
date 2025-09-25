// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
// SPDX-FileCopyrightText: Copyright 2018 Intel Corporation
#pragma once

#include "app.hpp"
#include "async_resp.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/eventlog_utils.hpp"
#include "utils/query_param.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/system/linux_error.hpp>
#include <boost/url/format.hpp>
#include <boost/url/url.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/message/native_types.hpp>
#include <sdbusplus/unpack_properties.hpp>

namespace redfish
{

inline void beforeHandleBMCLogServiceEventLogLogEntryCollection(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& managerId)
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
    if (managerId != BMCWEB_REDFISH_MANAGER_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "Manager", managerId);
        return;
    }

    eventlog_utils::handleSystemsAndBMCLogServiceEventLogLogEntryCollection(
        asyncResp, delegatedQuery, eventlog_utils::LogServiceParent::Managers);
}

inline void beforeHandleBMCJournalEventLogEntry(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& managerId, const std::string& param)
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
    eventlog_utils::handleSystemsAndBMCLogServiceEventLogEntriesGet(
        asyncResp, param, eventlog_utils::LogServiceParent::Managers);
}

inline void beforeHandleBMCJournalEventLogClear(

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
    eventlog_utils::handleSystemsAndBMCLogServicesEventLogActionsClearPost(
        asyncResp);
}

inline void requestRoutesBMCJournalEventLogEntryCollection(App& app)
{
    BMCWEB_ROUTE(app,
                 "/redfish/v1/Managers/<str>/LogServices/EventLog/Entries/")
        .privileges(redfish::privileges::getLogEntryCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(beforeHandleBMCLogServiceEventLogLogEntryCollection,
                            std::ref(app)));
}

inline void requestRoutesBMCJournalEventLogEntry(App& app)
{
    BMCWEB_ROUTE(
        app, "/redfish/v1/Managers/<str>/LogServices/EventLog/Entries/<str>/")
        .privileges(redfish::privileges::getLogEntry)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            beforeHandleBMCJournalEventLogEntry, std::ref(app)));
}

inline void requestRoutesBMCJournalEventLogClear(App& app)
{
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Managers/<str>/LogServices/EventLog/Actions/LogService.ClearLog/")
        .privileges(redfish::privileges::
                        postLogServiceSubOverComputerSystemLogServiceCollection)
        .methods(boost::beast::http::verb::post)(std::bind_front(
            beforeHandleBMCJournalEventLogClear, std::ref(app)));
}
} // namespace redfish
