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

#include <memory>
#include <optional>
#include <string>

namespace redfish
{

inline void getPendingEccModeEnabled(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& processorId)
{
    dbus::utility::getProperty<bool>(
        gpuSensorService, eccModePath(processorId), eccModeInterface, "Enabled",
        [asyncResp](const boost::system::error_code& ec, bool enabled) {
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
            asyncResp->res.jsonValue["MemorySummary"]["ECCModeEnabled"] =
                enabled;
        });
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

    setDbusProperty(asyncResp, "MemorySummary/ECCModeEnabled", gpuSensorService,
                    eccModePath(processorId), eccModeInterface, "Enabled",
                    *eccModeEnabled);
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
