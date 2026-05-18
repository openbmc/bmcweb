// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "bmcweb_config.h"

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "logging.hpp"
#include "processor.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/dbus_utils.hpp"
#include "utils/json_utils.hpp"

#include <boost/beast/http/verb.hpp>
#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/message/native_types.hpp>

#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace redfish
{

// ECC mode is published by a Control.Processor.EccMode object under
// /xyz/openbmc_project/control/processor, linked to the processor inventory
// item through a controlled_by association. The Settings sub-resource reads
// and writes the Enabled property (the mode requested for the next reset) on
// that control object.
constexpr std::array<std::string_view, 1> eccModeControlInterfaces = {
    "xyz.openbmc_project.Control.Processor.EccMode"};

inline void afterGetPendingEccMode(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec, bool enabled)
{
    if (ec)
    {
        if (ec.value() == EBADR || ec == boost::system::errc::io_error)
        {
            return;
        }
        BMCWEB_LOG_ERROR("DBus error reading pending ECC mode: {}",
                         ec.message());
        messages::internalError(asyncResp->res);
        return;
    }
    asyncResp->res.jsonValue["MemorySummary"]["ECCModeEnabled"] = enabled;
}

inline void afterGetEccModeObjectForSettingsGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec)
    {
        if (ec.value() == EBADR || ec == boost::system::errc::io_error)
        {
            return;
        }
        BMCWEB_LOG_ERROR("DBus error getting ECC mode object: {}",
                         ec.message());
        messages::internalError(asyncResp->res);
        return;
    }
    if (subtree.empty())
    {
        return;
    }
    // The association is filtered to the EccMode interface, so the subtree
    // holds the single control object that exposes it.
    const auto& [controlPath, serviceMap] = subtree.front();
    if (serviceMap.empty())
    {
        BMCWEB_LOG_ERROR("No service hosts ECC mode object {}", controlPath);
        messages::internalError(asyncResp->res);
        return;
    }
    dbus::utility::getProperty<bool>(
        serviceMap.front().first, controlPath,
        "xyz.openbmc_project.Control.Processor.EccMode", "Enabled",
        std::bind_front(afterGetPendingEccMode, asyncResp));
}

inline void afterGetProcessorObjectForSettingsGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& objectPath,
    const dbus::utility::MapperServiceMap& /*serviceMap*/)
{
    dbus::utility::getAssociatedSubTree(
        sdbusplus::message::object_path(objectPath) / "controlled_by",
        sdbusplus::message::object_path("/xyz/openbmc_project/control"), 0,
        eccModeControlInterfaces,
        std::bind_front(afterGetEccModeObjectForSettingsGet, asyncResp));
}

inline void getPendingEccModeEnabled(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& processorId)
{
    getProcessorObject(
        asyncResp, processorId,
        std::bind_front(afterGetProcessorObjectForSettingsGet, asyncResp));
}

inline void afterGetEccModeObjectForSettingsPatch(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp, bool eccModeEnabled,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("DBus error getting ECC mode object: {}",
                         ec.message());
        messages::internalError(asyncResp->res);
        return;
    }
    if (subtree.empty())
    {
        BMCWEB_LOG_ERROR("No ECC mode control object for processor");
        messages::internalError(asyncResp->res);
        return;
    }
    const auto& [controlPath, serviceMap] = subtree.front();
    if (serviceMap.empty())
    {
        BMCWEB_LOG_ERROR("No service hosts ECC mode object {}", controlPath);
        messages::internalError(asyncResp->res);
        return;
    }
    setDbusProperty(asyncResp, "MemorySummary/ECCModeEnabled",
                    serviceMap.front().first, controlPath,
                    "xyz.openbmc_project.Control.Processor.EccMode", "Enabled",
                    eccModeEnabled);
}

inline void afterGetProcessorObjectForSettingsPatch(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp, bool eccModeEnabled,
    const std::string& objectPath,
    const dbus::utility::MapperServiceMap& /*serviceMap*/)
{
    dbus::utility::getAssociatedSubTree(
        sdbusplus::message::object_path(objectPath) / "controlled_by",
        sdbusplus::message::object_path("/xyz/openbmc_project/control"), 0,
        eccModeControlInterfaces,
        std::bind_front(afterGetEccModeObjectForSettingsPatch, asyncResp,
                        eccModeEnabled));
}

inline void handleProcessorSettingsGet(
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
        "</redfish/v1/JsonSchemas/Processor/Processor.json>; rel=describedby");
    asyncResp->res.jsonValue["@odata.type"] = "#Processor.v1_18_0.Processor";
    asyncResp->res.jsonValue["@odata.id"] =
        boost::urls::format("/redfish/v1/Systems/{}/Processors/{}/Settings",
                            BMCWEB_REDFISH_SYSTEM_URI_NAME, processorId);
    asyncResp->res.jsonValue["Id"] = "Settings";
    asyncResp->res.jsonValue["Name"] = processorId + " Pending Settings";

    nlohmann::json::object_t applyTime;
    applyTime["@odata.type"] = "#Settings.v1_3_3.PreferredApplyTime";
    applyTime["ApplyTime"] = "OnReset";
    asyncResp->res.jsonValue["@Redfish.SettingsApplyTime"] =
        std::move(applyTime);

    getPendingEccModeEnabled(asyncResp, processorId);
}

inline void handleProcessorSettingsHead(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& /*processorId*/)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
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
        "</redfish/v1/JsonSchemas/Processor/Processor.json>; rel=describedby");
}

inline void handleProcessorSettingsPatch(
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

    std::optional<nlohmann::json::object_t> memorySummary;
    if (!json_util::readJsonPatch(         //
            req, asyncResp->res,           //
            "MemorySummary", memorySummary //
            ))
    {
        return;
    }
    if (!memorySummary)
    {
        return;
    }

    std::optional<bool> eccModeEnabled;
    if (!json_util::readJsonObject(          //
            *memorySummary, asyncResp->res,  //
            "ECCModeEnabled", eccModeEnabled //
            ))
    {
        return;
    }
    if (!eccModeEnabled)
    {
        return;
    }

    getProcessorObject(asyncResp, processorId,
                       std::bind_front(afterGetProcessorObjectForSettingsPatch,
                                       asyncResp, *eccModeEnabled));
}

inline void requestRoutesProcessorSettings(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Processors/<str>/Settings/")
        .privileges(redfish::privileges::headProcessor)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handleProcessorSettingsHead, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Processors/<str>/Settings/")
        .privileges(redfish::privileges::getProcessor)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleProcessorSettingsGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Processors/<str>/Settings/")
        .privileges(redfish::privileges::patchProcessor)
        .methods(boost::beast::http::verb::patch)(
            std::bind_front(handleProcessorSettingsPatch, std::ref(app)));
}

} // namespace redfish
