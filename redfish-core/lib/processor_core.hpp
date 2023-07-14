// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "bmcweb_config.h"

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/processor.hpp"
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

#include <array>
#include <format>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace redfish
{

// Interfaces which imply a D-Bus object represents a Processor Core
constexpr std::array<std::string_view, 1> procCoreInterfaces = {
    "xyz.openbmc_project.Inventory.Item.CpuCore"};

inline void doHandleSubProcessorCoreHead(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& processorId, const std::string& coreId,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& coreSubTree)
{
    if (ec)
    {
        if (ec.value() == boost::system::errc::io_error)
        {
            BMCWEB_LOG_WARNING("Processor {} not found.", processorId);
            messages::resourceNotFound(asyncResp->res, "Processor",
                                       processorId);
            return;
        }
        BMCWEB_LOG_ERROR("DBUS response error {}", ec.value());
        messages::internalError(asyncResp->res);
        return;
    }

    const auto& it =
        std::ranges::find_if(coreSubTree, [coreId](const auto& coreMap) {
            return sdbusplus::object_path(coreMap.first).filename() == coreId;
        });
    if (it == coreSubTree.end())
    {
        BMCWEB_LOG_WARNING("Core {} not found.", coreId);
        messages::resourceNotFound(asyncResp->res, "Processor", coreId);
        return;
    }

    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/Processor/Processor.json>; rel=describedby");
}

inline void handleSubProcessorCoreHead(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& processorId,
    const std::string& coreId)
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

    dbus::utility::getAssociatedSubTreeById(
        processorId, "/xyz/openbmc_project/inventory", processorInterfaces,
        "containing", procCoreInterfaces,
        std::bind_front(doHandleSubProcessorCoreHead, asyncResp, processorId,
                        coreId));
}

inline void getSubProcessorCoreData(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& processorId,
    const std::string& coreId)
{
    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/Processor/Processor.json>; rel=describedby");
    asyncResp->res.jsonValue["@odata.type"] = "#Processor.v1_18_0.Processor";
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Systems/{}/Processors/{}/SubProcessors/{}", systemName,
        processorId, coreId);
    asyncResp->res.jsonValue["Name"] = "SubProcessor";
    asyncResp->res.jsonValue["Id"] = coreId;
    asyncResp->res.jsonValue["ProcessorType"] = processor::ProcessorType::Core;
}

inline void doHandleSubProcessorCoreGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& processorId,
    const std::string& coreId, const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& coreSubTree)
{
    if (ec)
    {
        if (ec.value() == boost::system::errc::io_error)
        {
            BMCWEB_LOG_WARNING("Processor {} not found.", processorId);
            messages::resourceNotFound(asyncResp->res, "Processor",
                                       processorId);
            return;
        }
        BMCWEB_LOG_ERROR("DBUS response error {}", ec.value());
        messages::internalError(asyncResp->res);
        return;
    }

    const auto& it =
        std::ranges::find_if(coreSubTree, [coreId](const auto& coreMap) {
            return sdbusplus::object_path(coreMap.first).filename() == coreId;
        });
    if (it == coreSubTree.end())
    {
        BMCWEB_LOG_WARNING("Core {} not found.", coreId);
        messages::resourceNotFound(asyncResp->res, "Processor", coreId);
        return;
    }
    getSubProcessorCoreData(asyncResp, systemName, processorId, coreId);
}

inline void handleSubProcessorCoreGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& processorId,
    const std::string& coreId)
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

    dbus::utility::getAssociatedSubTreeById(
        processorId, "/xyz/openbmc_project/inventory", processorInterfaces,
        "containing", procCoreInterfaces,
        std::bind_front(doHandleSubProcessorCoreGet, asyncResp, systemName,
                        processorId, coreId));
}

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
    if (ec == boost::system::errc::io_error)
    {
        // getAssociatedSubTreePathsById() returns io_error if processorId does
        // not exist
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
        "/redfish/v1/Systems/{}/Processors/{}/SubProcessors", systemName,
        processorId);
    asyncResp->res.jsonValue["Name"] = "SubProcessor Collection";

    collection_util::handleCollectionMembers(
        asyncResp,
        boost::urls::format(
            "/redfish/v1/Systems/{}/Processors/{}/SubProcessors", systemName,
            processorId),
        nlohmann::json::json_pointer("/Members"), ec, coreSubTreePaths);
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
    BMCWEB_ROUTE(
        app, "/redfish/v1/Systems/<str>/Processors/<str>/SubProcessors/<str>/")
        .privileges(redfish::privileges::headProcessor)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handleSubProcessorCoreHead, std::ref(app)));

    BMCWEB_ROUTE(
        app, "/redfish/v1/Systems/<str>/Processors/<str>/SubProcessors/<str>/")
        .privileges(redfish::privileges::getProcessor)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleSubProcessorCoreGet, std::ref(app)));

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
