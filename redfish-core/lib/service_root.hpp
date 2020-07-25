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

#include "node.hpp"

#include <utils/systemd_utils.hpp>

namespace redfish
{

class ServiceRoot : public Node
{
  public:
    ServiceRoot(App& app) : Node(app, "/redfish/v1/")
    {
        uuid = persistent_data::getConfig().systemUuid;
        entityPrivileges = {
            {boost::beast::http::verb::get, {}},
            {boost::beast::http::verb::head, {}},
            {boost::beast::http::verb::patch, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::put, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::post, {{"ConfigureComponents"}}}};
    }

  private:
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>&) override
    {
        res.jsonValue["@odata.type"] = "#ServiceRoot.v1_5_0.ServiceRoot";
        res.jsonValue["@odata.id"] = "/redfish/v1";
        res.jsonValue["Id"] = "RootService";
        res.jsonValue["Name"] = "Root Service";
        res.jsonValue["RedfishVersion"] = "1.9.0";
        res.jsonValue["Links"]["Sessions"] = {
            {"@odata.id", "/redfish/v1/SessionService/Sessions"}};
        res.jsonValue["AccountService"] = {
            {"@odata.id", "/redfish/v1/AccountService"}};
        res.jsonValue["Chassis"] = {{"@odata.id", "/redfish/v1/Chassis"}};
        res.jsonValue["JsonSchemas"] = {
            {"@odata.id", "/redfish/v1/JsonSchemas"}};
        res.jsonValue["Managers"] = {{"@odata.id", "/redfish/v1/Managers"}};
        res.jsonValue["SessionService"] = {
            {"@odata.id", "/redfish/v1/SessionService"}};
        res.jsonValue["Managers"] = {{"@odata.id", "/redfish/v1/Managers"}};
        res.jsonValue["Systems"] = {{"@odata.id", "/redfish/v1/Systems"}};
        res.jsonValue["Registries"] = {{"@odata.id", "/redfish/v1/Registries"}};

        res.jsonValue["UpdateService"] = {
            {"@odata.id", "/redfish/v1/UpdateService"}};
        res.jsonValue["UUID"] = uuid;
        res.jsonValue["CertificateService"] = {
            {"@odata.id", "/redfish/v1/CertificateService"}};
        res.jsonValue["Tasks"] = {{"@odata.id", "/redfish/v1/TaskService"}};
        res.jsonValue["EventService"] = {
            {"@odata.id", "/redfish/v1/EventService"}};
        res.end();
    }

    std::string uuid;
};

} // namespace redfish
