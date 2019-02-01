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

#include <systemd/sd-id128.h>

namespace redfish
{

class ServiceRoot : public Node
{
  public:
    ServiceRoot(CrowApp& app) : Node(app, "/redfish/v1/")
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {}},
            {boost::beast::http::verb::head, {}},
            {boost::beast::http::verb::patch, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::put, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::post, {{"ConfigureComponents"}}}};
    }

  private:
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        res.jsonValue["@odata.type"] = "#ServiceRoot.v1_1_1.ServiceRoot";
        res.jsonValue["@odata.id"] = "/redfish/v1";
        res.jsonValue["@odata.context"] =
            "/redfish/v1/$metadata#ServiceRoot.ServiceRoot";
        res.jsonValue["Id"] = "RootService";
        res.jsonValue["Name"] = "Root Service";
        res.jsonValue["RedfishVersion"] = "1.6.1";
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

        res.jsonValue["UUID"] = getUuid();
        res.end();
    }

    const std::string getUuid()
    {
        std::string ret;
        // This ID needs to match the one in ipmid
        sd_id128_t appId = SD_ID128_MAKE(e0, e1, 73, 76, 64, 61, 47, da, a5, 0c,
                                         d0, cc, 64, 12, 45, 78);
        sd_id128_t machineId = SD_ID128_NULL;

        if (sd_id128_get_machine_app_specific(appId, &machineId) == 0)
        {
            std::array<char, SD_ID128_STRING_MAX> str;
            ret = sd_id128_to_string(machineId, str.data());
            ret.insert(8, 1, '-');
            ret.insert(13, 1, '-');
            ret.insert(18, 1, '-');
            ret.insert(23, 1, '-');
        }

        return ret;
    }
};

} // namespace redfish
