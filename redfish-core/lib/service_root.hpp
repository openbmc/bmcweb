/*
// Copyright (c) 2018 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
#pragma once

#include "bmcweb_config.h"

#include "app.hpp"
#include "async_resp.hpp"
#include "http_request.hpp"
#include "persistent_data.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/systemd_utils.hpp"

#include <nlohmann/json.hpp>

namespace redfish
{

inline void
    handleServiceRootHead(App& app, const crow::Request& req,
                          const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/ServiceRoot/ServiceRoot.json>; rel=describedby");
}

inline void handleServiceRootGetImpl(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    std::string uuid = persistent_data::getConfig().systemUuid;
    asyncResp->res.jsonValue["@odata.type"] =
        "#ServiceRoot.v1_15_0.ServiceRoot";
    asyncResp->res.jsonValue["@odata.id"] = "/redfish/v1";
    asyncResp->res.jsonValue["Id"] = "RootService";
    asyncResp->res.jsonValue["Name"] = "Root Service";
    asyncResp->res.jsonValue["RedfishVersion"] = "1.9.0";
    asyncResp->res.jsonValue["Links"]["Sessions"]["@odata.id"] =
        "/redfish/v1/SessionService/Sessions";
    asyncResp->res.jsonValue["AccountService"]["@odata.id"] =
        "/redfish/v1/AccountService";
#ifdef BMCWEB_ENABLE_REDFISH_AGGREGATION
    asyncResp->res.jsonValue["AggregationService"]["@odata.id"] =
        "/redfish/v1/AggregationService";
#endif
    asyncResp->res.jsonValue["Chassis"]["@odata.id"] = "/redfish/v1/Chassis";
    asyncResp->res.jsonValue["JsonSchemas"]["@odata.id"] =
        "/redfish/v1/JsonSchemas";
    asyncResp->res.jsonValue["Managers"]["@odata.id"] = "/redfish/v1/Managers";
    asyncResp->res.jsonValue["SessionService"]["@odata.id"] =
        "/redfish/v1/SessionService";
    asyncResp->res.jsonValue["Systems"]["@odata.id"] = "/redfish/v1/Systems";
    asyncResp->res.jsonValue["Registries"]["@odata.id"] =
        "/redfish/v1/Registries";
    asyncResp->res.jsonValue["UpdateService"]["@odata.id"] =
        "/redfish/v1/UpdateService";
    asyncResp->res.jsonValue["UUID"] = uuid;
    asyncResp->res.jsonValue["CertificateService"]["@odata.id"] =
        "/redfish/v1/CertificateService";
    asyncResp->res.jsonValue["Tasks"]["@odata.id"] = "/redfish/v1/TaskService";
    asyncResp->res.jsonValue["EventService"]["@odata.id"] =
        "/redfish/v1/EventService";
    asyncResp->res.jsonValue["TelemetryService"]["@odata.id"] =
        "/redfish/v1/TelemetryService";
    asyncResp->res.jsonValue["Cables"]["@odata.id"] = "/redfish/v1/Cables";

    asyncResp->res.jsonValue["Links"]["ManagerProvidingService"]["@odata.id"] =
        "/redfish/v1/Managers/bmc";

    nlohmann::json& protocolFeatures =
        asyncResp->res.jsonValue["ProtocolFeaturesSupported"];
    protocolFeatures["ExcerptQuery"] = false;

    protocolFeatures["ExpandQuery"]["ExpandAll"] =
        bmcwebInsecureEnableQueryParams;
    // This is the maximum level defined in ServiceRoot.v1_13_0.json
    if (bmcwebInsecureEnableQueryParams)
    {
        protocolFeatures["ExpandQuery"]["MaxLevels"] = 6;
    }
    protocolFeatures["ExpandQuery"]["Levels"] = bmcwebInsecureEnableQueryParams;
    protocolFeatures["ExpandQuery"]["Links"] = bmcwebInsecureEnableQueryParams;
    protocolFeatures["ExpandQuery"]["NoLinks"] =
        bmcwebInsecureEnableQueryParams;
    protocolFeatures["FilterQuery"] = false;
    protocolFeatures["OnlyMemberQuery"] = true;
    protocolFeatures["SelectQuery"] = true;
    protocolFeatures["DeepOperations"]["DeepPOST"] = false;
    protocolFeatures["DeepOperations"]["DeepPATCH"] = false;
}
inline void
    handleServiceRootGet(App& app, const crow::Request& req,
                         const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    handleServiceRootHead(app, req, asyncResp);
    handleServiceRootGetImpl(asyncResp);
}

inline void requestRoutesServiceRoot(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/")
        .privileges(redfish::privileges::headServiceRoot)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handleServiceRootHead, std::ref(app)));
    BMCWEB_ROUTE(app, "/redfish/v1/")
        .privileges(redfish::privileges::getServiceRoot)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleServiceRootGet, std::ref(app)));
}

} // namespace redfish
