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

namespace redfish
{

class Roles : public Node
{
  public:
    Roles(CrowApp& app) :
        Node(app, "/redfish/v1/AccountService/Roles/Administrator/")
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        res.jsonValue["@odata.id"] =
            "/redfish/v1/AccountService/Roles/Administrator";
        res.jsonValue["@odata.type"] = "#Role.v1_0_2.Role";
        res.jsonValue["@odata.context"] = "/redfish/v1/$metadata#Role.Role";
        res.jsonValue["Id"] = "Administrator";
        res.jsonValue["Name"] = "User Role";
        res.jsonValue["Description"] = "Administrator User Role";
        res.jsonValue["IsPredefined"] = true;
        res.jsonValue["AssignedPrivileges"] = {
            "Login", "ConfigureManager", "ConfigureUsers", "ConfigureSelf",
            "ConfigureComponents"};
        res.jsonValue["OemPrivileges"] = nlohmann::json::array();
        res.end();
    }
};

class RoleCollection : public Node
{
  public:
    RoleCollection(CrowApp& app) :
        Node(app, "/redfish/v1/AccountService/Roles/")
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        res.jsonValue["@odata.id"] = "/redfish/v1/AccountService/Roles";
        res.jsonValue["@odata.type"] = "#RoleCollection.RoleCollection";
        res.jsonValue["@odata.context"] =
            "/redfish/v1/$metadata#RoleCollection.RoleCollection";
        res.jsonValue["Name"] = "Roles Collection";
        res.jsonValue["Description"] = "BMC User Roles";
        res.jsonValue["Members@odata.count"] = 1;
        res.jsonValue["Members"] = {
            {{"@odata.id", "/redfish/v1/AccountService/Roles/Administrator"}}};
        res.end();
    }
};

} // namespace redfish
