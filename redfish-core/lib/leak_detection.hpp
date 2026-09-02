// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "app.hpp"
#include "async_resp.hpp"
#include "generated/enums/resource.hpp"
#include "http_request.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/chassis_utils.hpp"

#include <boost/beast/http/verb.hpp>
#include <boost/url/format.hpp>

#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace redfish
{

inline void doLeakDetection(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& chassisId,
                            const std::optional<std::string>& validChassisPath)
{
    if (!validChassisPath)
    {
        messages::resourceNotFound(asyncResp->res, "Chassis", chassisId);
        return;
    }

    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/LeakDetection/LeakDetection.json>; rel=describedby");
    asyncResp->res.jsonValue["@odata.type"] =
        "#LeakDetection.v1_2_0.LeakDetection";
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Chassis/{}/ThermalSubsystem/LeakDetection", chassisId);
    asyncResp->res.jsonValue["Id"] = "LeakDetection";
    asyncResp->res.jsonValue["Name"] = "Leak Detection Systems";

    asyncResp->res
        .jsonValue["LeakDetectors"]["@odata.id"] = boost::urls::format(
        "/redfish/v1/Chassis/{}/ThermalSubsystem/LeakDetection/LeakDetectors",
        chassisId);

    asyncResp->res.jsonValue["Status"]["State"] = resource::State::Enabled;
    asyncResp->res.jsonValue["Status"]["Health"] = resource::Health::OK;
}

inline void handleLeakDetectionHead(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    redfish::chassis_utils::getValidChassisPath(
        asyncResp, chassisId,
        [asyncResp,
         chassisId](const std::optional<std::string>& validChassisPath) {
            if (!validChassisPath)
            {
                messages::resourceNotFound(asyncResp->res, "Chassis",
                                           chassisId);
                return;
            }
            asyncResp->res.addHeader(
                boost::beast::http::field::link,
                "</redfish/v1/JsonSchemas/LeakDetection/LeakDetection.json>; rel=describedby");
        });
}

inline void handleLeakDetectionGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    redfish::chassis_utils::getValidChassisPath(
        asyncResp, chassisId,
        std::bind_front(doLeakDetection, asyncResp, chassisId));
}

inline void requestRoutesLeakDetection(App& app)
{
    BMCWEB_ROUTE(app,
                 "/redfish/v1/Chassis/<str>/ThermalSubsystem/LeakDetection/")
        .privileges(redfish::privileges::headLeakDetection)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handleLeakDetectionHead, std::ref(app)));

    BMCWEB_ROUTE(app,
                 "/redfish/v1/Chassis/<str>/ThermalSubsystem/LeakDetection/")
        .privileges(redfish::privileges::getLeakDetection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleLeakDetectionGet, std::ref(app)));
}

} // namespace redfish
