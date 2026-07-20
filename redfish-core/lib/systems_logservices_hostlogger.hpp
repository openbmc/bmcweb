// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "bmcweb_config.h"

#include "app.hpp"
#include "async_resp.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "log_parser.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/log_services_utils.hpp"
#include "utils/query_param.hpp"
#include "utils/systems_utils.hpp"

#include <boost/beast/http/verb.hpp>
#include <boost/url/format.hpp>
#include <nlohmann/json.hpp>

#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <memory>
#include <ranges>
#include <string>
#include <string_view>

namespace redfish
{

inline void handleSystemsLogServicesHostloggerGet(
    App& app, const crow::Request& req,
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

    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Systems/{}/LogServices/HostLogger", systemName);
    asyncResp->res.jsonValue["@odata.type"] = "#LogService.v1_2_0.LogService";
    asyncResp->res.jsonValue["Name"] = "Host Logger Service";
    asyncResp->res.jsonValue["Description"] = "Host Logger Service";
    asyncResp->res.jsonValue["Id"] = "HostLogger";
    asyncResp->res.jsonValue["Entries"]["@odata.id"] = boost::urls::format(
        "/redfish/v1/Systems/{}/LogServices/HostLogger/Entries", systemName);
}

inline void processSystemsLogServicesHostloggerEntriesGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, size_t skip, size_t top,
    const uint64_t computerSystemIndex)
{
    const auto& parser = log_parser::Parser::requestParser(
        log_services_utils::LogService::HostLogger,
        log_services_utils::LogServiceParentCollection::Systems, systemName,
        computerSystemIndex);

    if (!parser)
    {
        BMCWEB_LOG_ERROR("Parser request failed");
        messages::internalError(asyncResp->res);
        return;
    }

    nlohmann::json& logEntryArray = asyncResp->res.jsonValue["Members"];
    logEntryArray = nlohmann::json::array();

    parser->getLogEntryCollection(logEntryArray, skip, top);

    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Systems/{}/LogServices/HostLogger/Entries", systemName);
    asyncResp->res.jsonValue["@odata.type"] =
        "#LogEntryCollection.LogEntryCollection";
    asyncResp->res.jsonValue["Name"] = "HostLogger Entries";
    asyncResp->res.jsonValue["Description"] =
        "Collection of HostLogger Entries";
    asyncResp->res.jsonValue["Members@odata.count"] = logEntryArray.size();

    if (skip + top < logEntryArray.size())
    {
        asyncResp->res.jsonValue["Members@odata.nextLink"] = std::format(
            "/redfish/v1/Systems/{}/LogServices/HostLogger/Entries?$skip={}",
            systemName, std::to_string(skip + top));
    }
}

inline void handleSystemsLogServicesHostloggerEntriesGet(
    App& app, const crow::Request& req,
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
    if constexpr (!BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
        if (systemName != BMCWEB_REDFISH_SYSTEM_URI_NAME)
        {
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }
    }

    size_t skip = delegatedQuery.skip.value_or(0);
    size_t top = delegatedQuery.top.value_or(query_param::Query::maxTop);

    systems_utils::getComputerSystemIndex(
        asyncResp, systemName,
        std::bind_front(processSystemsLogServicesHostloggerEntriesGet,
                        asyncResp, systemName, skip, top));
}

inline void processSystemsLogServicesHostLoggerEntriesEntryGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, std::string_view param,
    const uint64_t computerSystemIndex)
{
    const auto& parser = log_parser::Parser::requestParser(
        log_services_utils::LogService::HostLogger,
        log_services_utils::LogServiceParentCollection::Systems, systemName,
        computerSystemIndex);

    if (!parser)
    {
        BMCWEB_LOG_ERROR("Parser request failed");
        messages::internalError(asyncResp->res);
        return;
    }

    nlohmann::json::object_t logEntry = parser->getLogEntry(param);

    if (logEntry.empty())
    {
        messages::resourceNotFound(asyncResp->res, "LogEntry", param);
        return;
    }

    asyncResp->res.jsonValue.update(logEntry);
}

inline void handleSystemsLogServicesHostloggerEntriesEntryGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& param)
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

    systems_utils::getComputerSystemIndex(
        asyncResp, systemName,
        std::bind_front(processSystemsLogServicesHostLoggerEntriesEntryGet,
                        asyncResp, systemName, param));
}

inline void requestRoutesSystemsLogServiceHostlogger(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/LogServices/HostLogger/")
        .privileges(redfish::privileges::getLogService)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleSystemsLogServicesHostloggerGet, std::ref(app)));
    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/<str>/LogServices/HostLogger/Entries/")
        .privileges(redfish::privileges::getLogEntryCollection)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleSystemsLogServicesHostloggerEntriesGet, std::ref(app)));

    BMCWEB_ROUTE(
        app, "/redfish/v1/Systems/<str>/LogServices/HostLogger/Entries/<str>/")
        .privileges(redfish::privileges::getLogEntry)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleSystemsLogServicesHostloggerEntriesEntryGet, std::ref(app)));
}

} // namespace redfish
