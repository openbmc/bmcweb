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
class CertificateService : public Node
{
  public:
    CertificateService(CrowApp &app) :
        Node(app, "/redfish/v1/CertificateService/")
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::put, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::post, {{"ConfigureComponents"}}}};
    }

  private:
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        res.jsonValue["@odata.type"] =
            "#CertificateService.v1_0_0.CertificateService",
        res.jsonValue["@odata.id"] = "/redfish/v1/CertificateService";
        res.jsonValue["@odata.context"] =
            "/redfish/v1/$metadata#CertificateService.CertificateService";
        res.jsonValue["Id"] = "CertificateService";
        res.jsonValue["Description"] =
            "Represents actions available to manage certificates and links to "
            "where certificates are installed";
        res.jsonValue["CertificateLocations"] = {
            {"@odata.id", "/redfish/v1/Certificates"}};
        // TODO (devenrao) need to add action to replace certificate
        res.end();
    }
}; // CertificateService

class CertificateCollection : public Node
{
  public:
    template <typename CrowApp>
    CertificateCollection(CrowApp &app) :
        Node(app, "/redfish/v1/CertificateService/Certificates/")
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::put, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::post, {{"ConfigureComponents"}}}};
    }

  private:
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        res.jsonValue["@odata.id"] =
            "/redfish/v1/CertificateService/Certificates";
        res.jsonValue["@odata.type"] =
            "#CertificateCollection.CertificatesCollection";
        res.jsonValue["@odata.context"] =
            "/redfish/v1/$metadata#CertificateCollection.CertificateCollection";
        res.jsonValue["Name"] = "Certificates Collection";
        // TODO (devenrao) need back-end support to query for certificates
        res.end();
    }
}; // CertificateCollection

class CertificateLocations : public Node
{
  public:
    template <typename CrowApp>
    CertificateLocations(CrowApp &app) : Node(app, "/redfish/v1/Certificates/")
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::put, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::post, {{"ConfigureComponents"}}}};
    }

  private:
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        res.jsonValue["@odata.id"] = "/redfish/v1/Certificates";
        res.jsonValue["@odata.type"] =
            "#CertificateLocations.CertificateLocations";
        res.jsonValue["@odata.context"] =
            "/redfish/v1/$metadata#CertificateLocations.CertificateLocations";
        res.jsonValue["Name"] = "Certificate Locations";
        res.jsonValue["Id"] = "CertificateService";
        // TODO (devenrao) need back-end support to query for certificates
        res.end();
    }
}; // CertificateCollection
} // namespace redfish
