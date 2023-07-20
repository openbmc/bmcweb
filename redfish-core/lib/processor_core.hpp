// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
// SPDX-FileCopyrightText: Copyright 2018 Intel Corporation
#pragma once

#include "bmcweb_config.h"

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/resource.hpp"
#include "http_request.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/collection.hpp"
#include "utils/json_utils.hpp"

#include <asm-generic/errno.h>

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/message/native_types.hpp>

#include <algorithm>
#include <array>
#include <format>
#include <functional>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace redfish
{

// Interfaces which imply a D-Bus object represents a Processor Core
constexpr std::array<std::string_view, 1> procCoreInterfaces = {
    "xyz.openbmc_project.Inventory.Item.CpuCore"};

inline void handleSubProcessorCollectionHead(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& /* processorId */)
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

    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/ProcessorCollection/ProcessorCollection.json>; rel=describedby");
}

inline void doHandleSubProcessorCollectionGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& processorId,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreePathsResponse& coreSubTreePaths)
{
    if (ec)
    {
        if (ec.value() != boost::system::errc::io_error)
        {
            BMCWEB_LOG_ERROR("DBUS response error {}", ec.value());
            messages::internalError(asyncResp->res);
            return;
        }
        BMCWEB_LOG_WARNING("Processor {} not found.", processorId);
        messages::resourceNotFound(asyncResp->res, "Processor", processorId);
        return;
    }

    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/ProcessorCollection/ProcessorCollection.json>; rel=describedby");

    asyncResp->res.jsonValue["@odata.type"] =
        "#ProcessorCollection.ProcessorCollection";
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Systems/{}/Processors/{}/SubProcessors",
        BMCWEB_REDFISH_SYSTEM_URI_NAME, processorId);
    asyncResp->res.jsonValue["Name"] = "SubProcessor Collection";

    asyncResp->res.jsonValue["Members"] = nlohmann::json::array();

    std::vector<std::string> coreIdNames;
    for (const std::string& corePath : coreSubTreePaths)
    {
        std::string coreId =
            sdbusplus::message::object_path(corePath).filename();
        if (!coreId.empty())
        {
            coreIdNames.emplace_back(std::move(coreId));
        }
    }

    std::ranges::sort(coreIdNames, AlphanumLess<std::string>());

    nlohmann::json& members = asyncResp->res.jsonValue["Members"];
    for (const std::string& coreId : coreIdNames)
    {
        nlohmann::json item;
        item["@odata.id"] = boost::urls::format(
            "/redfish/v1/Systems/{}/Processors/{}/SubProcessors/{}", systemName,
            processorId, coreId);
        members.emplace_back(std::move(item));
    }
    asyncResp->res.jsonValue["Members@odata.count"] = members.size();
}

inline void handleSubProcessorCollectionGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& processorId)
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

    dbus::utility::getAssociatedSubTreePathsById(
        processorId, "/xyz/openbmc_project/inventory", processorInterfaces,
        "containing", procCoreInterfaces,
        std::bind_front(doHandleSubProcessorCollectionGet, asyncResp,
                        systemName, processorId));
}

inline void requestRoutesSubProcessors(App& app)
{
    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/<str>/Processors/<str>/SubProcessors/")
        .privileges(redfish::privileges::headProcessorCollection)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handleSubProcessorCollectionHead, std::ref(app)));

    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/<str>/Processors/<str>/SubProcessors/")
        .privileges(redfish::privileges::getProcessorCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleSubProcessorCollectionGet, std::ref(app)));
}

} // namespace redfish
