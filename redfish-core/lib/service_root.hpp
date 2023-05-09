/*
Copyright (c) 2018 Intel Corporation

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
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
    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/ServiceRoot/ServiceRoot.json>; rel=describedby");

    std::string uuid = persistent_data::getConfig().systemUuid;
    boost::json::object& jsonValue = asyncResp->res.response.body().jsonValue2;

    jsonValue["@odata.type"] = "#ServiceRoot.v1_15_0.ServiceRoot";
    jsonValue["@odata.id"] = "/redfish/v1";
    jsonValue["Id"] = "RootService";
    jsonValue["Name"] = "Root Service";
    jsonValue["RedfishVersion"] = "1.17.0";

    jsonValue["AccountService"].emplace_object()["@odata.id"] =
        "/redfish/v1/AccountService";

    if constexpr (BMCWEB_REDFISH_AGGREGATION)
    {
        jsonValue["AggregationService"].emplace_object()["@odata.id"] =
            "/redfish/v1/AggregationService";
    }
    jsonValue["Chassis"].emplace_object()["@odata.id"] = "/redfish/v1/Chassis";
    jsonValue["JsonSchemas"].emplace_object()["@odata.id"] =
        "/redfish/v1/JsonSchemas";
    jsonValue["Managers"].emplace_object()["@odata.id"] =
        "/redfish/v1/Managers";
    jsonValue["SessionService"].emplace_object()["@odata.id"] =
        "/redfish/v1/SessionService";
    jsonValue["Systems"].emplace_object()["@odata.id"] = "/redfish/v1/Systems";
    jsonValue["Registries"].emplace_object()["@odata.id"] =
        "/redfish/v1/Registries";
    jsonValue["UpdateService"].emplace_object()["@odata.id"] =
        "/redfish/v1/UpdateService";
    jsonValue["UUID"] = uuid;
    jsonValue["CertificateService"].emplace_object()["@odata.id"] =
        "/redfish/v1/CertificateService";
    jsonValue["Tasks"].emplace_object()["@odata.id"] =
        "/redfish/v1/TaskService";
    jsonValue["EventService"].emplace_object()["@odata.id"] =
        "/redfish/v1/EventService";
    jsonValue["TelemetryService"].emplace_object()["@odata.id"] =
        "/redfish/v1/TelemetryService";
    jsonValue["Cables"].emplace_object()["@odata.id"] = "/redfish/v1/Cables";

    boost::json::object& links = jsonValue["Links"].emplace_object();
    links["ManagerProvidingService"].emplace_object()["@odata.id"] =
        boost::urls::format("/redfish/v1/Managers/{}",
                            BMCWEB_REDFISH_MANAGER_URI_NAME)
            .buffer();

    links["Sessions"].emplace_object()["@odata.id"] =
        "/redfish/v1/SessionService/Sessions";

    boost::json::object& protocolFeatures =
        jsonValue["ProtocolFeaturesSupported"].emplace_object();
    protocolFeatures["ExcerptQuery"] = false;

    boost::json::object& expandQuery =
        protocolFeatures["ExpandQuery"].emplace_object();
    expandQuery["ExpandAll"] = BMCWEB_INSECURE_ENABLE_REDFISH_QUERY;
    // This is the maximum level defined in ServiceRoot.v1_13_0.json
    if constexpr (BMCWEB_INSECURE_ENABLE_REDFISH_QUERY)
    {
        expandQuery["MaxLevels"] = 6;
    }
    expandQuery["Levels"] = BMCWEB_INSECURE_ENABLE_REDFISH_QUERY;
    expandQuery["Links"] = BMCWEB_INSECURE_ENABLE_REDFISH_QUERY;
    expandQuery["NoLinks"] = BMCWEB_INSECURE_ENABLE_REDFISH_QUERY;
    protocolFeatures["FilterQuery"] = BMCWEB_INSECURE_ENABLE_REDFISH_QUERY;
    protocolFeatures["OnlyMemberQuery"] = true;
    protocolFeatures["SelectQuery"] = true;
    boost::json::object& deepOps =
        protocolFeatures["DeepOperations"].emplace_object();
    deepOps["DeepPOST"] = false;
    deepOps["DeepPATCH"] = false;
}
inline void
    handleServiceRootGet(App& app, const crow::Request& req,
                         const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

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
