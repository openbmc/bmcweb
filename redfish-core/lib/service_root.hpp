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

#include <app.hpp>
#include <utils/systemd_utils.hpp>
#include <static/dataModel/ServiceRoot_v1.h>
#include <static/serialize/json_serviceroot.h>

namespace redfish
{

inline void requestRoutesServiceRoot(App& app)
{
    std::string uuid = persistent_data::getConfig().systemUuid;
    BMCWEB_ROUTE(app, "/redfish/v1/")
        .privileges({})
        .methods(boost::beast::http::verb::get)(
            [uuid](const crow::Request&,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                ServiceRoot_v1_ServiceRoot serviceRoot;
                //TODO deal with odata.type ?
                //asyncResp->res.jsonValue["@odata.type"] =
                //    "#ServiceRoot.v1_5_0.ServiceRoot";
                serviceRoot.id = "RootService";
                //TODO deal with Id?
                //asyncResp->res.jsonValue["@odata.id"] = "/redfish/v1";
                serviceRoot.name = "Root Service";
                serviceRoot.redfishVersion = "1.9.0";
                serviceRoot.links.sessions = "/redfish/v1/SessionService/Sessions";
                serviceRoot.accountService = "/redfish/v1/AccountService";
                serviceRoot.chassis = "/redfish/v1/Chassis";
                serviceRoot.jsonSchemas = "/redfish/v1/JsonSchemas";
                serviceRoot.managers = "Managers";
                serviceRoot.sessionService =  "/redfish/v1/SessionService";
                serviceRoot.systems = "/redfish/v1/Systems";
                serviceRoot.registries = "/redfish/v1/Registries";
                serviceRoot.updateService = "/redfish/v1/UpdateService";
                serviceRoot.UUID = uuid;
                serviceRoot.certificateService = "/redfish/v1/CertificateService";
                serviceRoot.tasks = "/redfish/v1/TaskService";
                serviceRoot.eventService = "/redfish/v1/EventService";
                serviceRoot.telemetryService = "/redfish/v1/TelemetryService";

                json_serialize_serviceroot(asyncResp, &serviceRoot);
            });
}

} // namespace redfish
