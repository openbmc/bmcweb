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
        res.jsonValue["@odata.id"] = "/redfish/v1/";
        res.jsonValue["@odata.context"] =
            "/redfish/v1/$metadata#ServiceRoot.ServiceRoot";
        res.jsonValue["Id"] = "RootService";
        res.jsonValue["Name"] = "Root Service";
        res.jsonValue["RedfishVersion"] = "1.1.0";
        res.jsonValue["Links"]["Sessions"] = {
            {"@odata.id", "/redfish/v1/SessionService/Sessions"}};
        res.jsonValue["AccountService"] = {
            {"@odata.id", "/redfish/v1/AccountService"}};
        res.jsonValue["Chassis"] = {{"@odata.id", "/redfish/v1/Chassis"}};
        res.jsonValue["JsonSchemas"] = {
            {"@odata.id", "/redfish/v1/JsonSchemas"}};
        res.jsonValue["SessionService"] = {
            {"@odata.id", "/redfish/v1/UpdateService"}};
        res.jsonValue["Systems"] = {{"@odata.id", "/redfish/v1/Systems"}};
        res.jsonValue["UpdateService"] = {
            {"@odata.id", "/redfish/v1/UpdateService"}};

        res.jsonValue["UUID"] = getUuid();
        res.end();
    }

    const std::string getUuid()
    {
        // If we are using a version of systemd that can get the app specific
        // uuid, use that
#ifdef sd_id128_get_machine_app_specific
        std::array<char, SD_ID128_STRING_MAX> string;
        sd_id128_t id = SD_ID128_NULL;

        // This ID needs to match the one in ipmid
        int r = sd_id128_get_machine_app_specific(
            SD_ID128_MAKE(e0, e1, 73, 76, 64, 61, 47, da, a5, 0c, d0, cc, 64,
                          12, 45, 78),
            &id);
        if (r < 0)
        {
            return "00000000-0000-0000-0000-000000000000";
        }
        return string.data();
#else
        return "00000000-0000-0000-0000-000000000000";
#endif
    }
};

} // namespace redfish
