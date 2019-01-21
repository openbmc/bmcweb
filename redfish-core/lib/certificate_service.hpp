/*
// Copyright (c) 2018 IBM Corporation
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
        res.jsonValue["Name"] = "Certificate Service";
        res.jsonValue["Description"] =
            "Actions available to manage certificates";
        res.jsonValue["CertificateLocations"] = {
            {"@odata.id",
             "/redfish/v1/CertificateService/CertificateLocations"}};
        // TODO need to add action to replace certificate
        res.end();
    }
}; // CertificateService

class HTTPSCertificate : public Node
{
  public:
    template <typename CrowApp>
    HTTPSCertificate(CrowApp &app) :
        Node(app,
             "/redfish/v1/Managers/bmc/NetworkProtocol/HTTPS/Certificates/"
             "<str>/",
             std::string())
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::put, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::post, {{"ConfigureComponents"}}}};
    }

    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        if (params.size() != 1)
        {
            messages::internalError(res);
            return;
        }
        auto certId = std::atoi(params[0].c_str());
        res.jsonValue["@odata.id"] = "/redfish/v1/Managers/bmc/NetworkProtocol/"
                                     "HTTPS/Certificates/" +
                                     std::to_string(certId);
        res.jsonValue["@odata.type"] = "#Certificate.v1_0_0.Certificate";
        res.jsonValue["@odata.context"] =
            "/redfish/v1/$metadata#Certificate.Certificate";
        res.jsonValue["Id"] = std::to_string(certId);
        res.jsonValue["Name"] = "HTTPS Certificate";
        res.jsonValue["Description"] = "HTTPS Certificate";

        // TODO: Need back-end support to query for all the properties
        // for the specified certificate URI
        res.end();
    }
}; // HTTPSCertificate

class HTTPSCertificateCollection : public Node
{
  public:
    template <typename CrowApp>
    HTTPSCertificateCollection(CrowApp &app) :
        Node(app,
             "/redfish/v1/Managers/bmc/NetworkProtocol/HTTPS/Certificates/")
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::put, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::post, {{"ConfigureComponents"}}}};
    }
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        res.jsonValue["@odata.id"] =
            "/redfish/v1/Managers/bmc/NetworkProtocol/HTTPS/Certificates";
        res.jsonValue["@odata.type"] =
            "#CertificateCollection.CertificatesCollection";
        res.jsonValue["@odata.context"] =
            "/redfish/v1/$metadata#CertificateCollection.CertificateCollection";
        res.jsonValue["Name"] = "HTTPS Certificates Collection";
        res.jsonValue["Description"] =
            "A Collection of HTTPS certificate instances";

        res.jsonValue["Name"] = "Manager Collection";
        res.jsonValue["Members@odata.count"] = 1;
        res.jsonValue["Members"] = {
            {{"@odata.id", "/redfish/v1/Managers/bmc/NetworkProtocol/HTTPS/"
                           "Certificates/1"}}};
        res.end();
    }

    void doPost(crow::Response &res, const crow::Request &req,
                const std::vector<std::string> &params) override
    {
        // TODO: need back-end support to create certificate specifying
        // the properties and certificate string
    }
}; // HTTPSCertificateCollection

class CertificateLocations : public Node
{
  public:
    template <typename CrowApp>
    CertificateLocations(CrowApp &app) :
        Node(app, "/redfish/v1/CertificateService/CertificateLocations/")
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
            "/redfish/v1/CertificateService/CertificateLocations";
        res.jsonValue["@odata.type"] =
            "#CertificateLocations.v1_0_0.CertificateLocations";
        res.jsonValue["@odata.context"] =
            "/redfish/v1/$metadata#CertificateLocations.CertificateLocations";
        res.jsonValue["Name"] = "Certificate Locations";
        res.jsonValue["Id"] = "CertificateLocations";
        res.jsonValue["Description"] =
            "Defines a resource that an administrator can use in order to "
            "locate all certificates installed on a given service";
        // TODO (devenrao) need back-end support to query for certificates
        // at present assuming one https certifiate is present
        res.jsonValue["Links"]["Certificates"] = {
            {{"@odata.id", "/redfish/v1/Managers/bmc/NetworkProtocol/HTTPS/"
                           "Certificates/1"}}};
        res.jsonValue["Links"]["Certificates@odata.count"] = 1;
        res.end();
    }
}; // CertificateLocations
} // namespace redfish
