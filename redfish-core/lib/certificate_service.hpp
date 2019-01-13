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

#include <boost/container/flat_map.hpp>

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
        res.jsonValue["Description"] = "Service for certficiate upload";
        res.jsonValue["Name"] = "Certificate Service";
        res.jsonValue["HttpPushUri"] = "/redfish/v1/CertificateService";
        res.jsonValue["ServiceEnabled"] = true;
        res.jsonValue["CertificateLocations"] = {
            {"@odata.id",
             "/redfish/v1/CertificateService/CertificateLocations"}};
        res.end();
    }
}; // CertificateService

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
        res.jsonValue["@odata.type"] =
            "#CertificateLocations.v1_0_0.CertificateLocations";
        res.jsonValue["@odata.id"] =
            "/redfish/v1/CertificateService/CertificateLocations";
        res.jsonValue["@odata.context"] =
            "/redfish/v1/$metadata#CertificateLocations.CertificateLocations";
        res.jsonValue["Id"] = "CertificateLocations";
        res.jsonValue["Description"] = "Location of certificates";
        res.jsonValue["Name"] = "Certificate Locations";
        res.jsonValue["HttpPushUri"] =
            "/redfish/v1/CertificateService/CertificateLocations";
        res.jsonValue["Links"]["Certificates"] = {
            {"@odata.id",
             "/redfish/v1/Managers/BMC/NetworkProtocol/HTTPS/Certificates/1"}};
        res.jsonValue["ServiceEnabled"] = true;
        res.end();
    }
}; // CertificateLocations

class HttpsCertificate : public Node
{
  public:
    template <typename CrowApp>
    HttpsCertificate(CrowApp &app) :
        Node(app,
             "/redfish/v1/Managers/bmc/NetworkProtocol/HTTPS/Certificates/1/")
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
        res.jsonValue["@odata.type"] = "#Certificate.v1_0_0.Certificate";
        res.jsonValue["@odata.id"] =
            "/redfish/v1/Managers/bmc/NetworkProtocol/HTTPS/Certificates/1";
        res.jsonValue["@odata.context"] =
            "/redfish/v1/$metadata#Certificate.Certificate";
        res.jsonValue["Id"] = "1";
        res.jsonValue["Description"] = "Certificate";
        res.jsonValue["Name"] = "Certificate";
        res.jsonValue["HttpPushUri"] =
            "/redfish/v1/Managers/bmc/NetworkProtocol/HTTPS/Certificates/1";
        res.jsonValue["ServiceEnabled"] = true;
        res.jsonValue["CertificateType"] = "PEM";
        res.end();
    }

    void doPost(crow::Response &res, const crow::Request &req,
                const std::vector<std::string> &params) override
    {
        std::string filepath("/tmp/" + boost::uuids::to_string(
                                           boost::uuids::random_generator()()));
        std::ofstream out(filepath, std::ofstream::out | std::ofstream::binary |
                                        std::ofstream::trunc);
        out << req.body;
        out.close();

        auto asyncResp = std::make_shared<AsyncResp>(res);
        crow::connections::systemBus->async_method_call(
            [asyncResp, filepath](const boost::system::error_code error_code) {
                if (error_code)
                {
                    BMCWEB_LOG_DEBUG << "error_code = " << error_code;
                    BMCWEB_LOG_DEBUG << "error msg = " << error_code.message();
                    messages::internalError(asyncResp->res);
                    std::remove(filepath.c_str());
                    return;
                }
                std::remove(filepath.c_str());
                messages::success(asyncResp->res);
            },
            "xyz.openbmc_project.Certs.Manager.Server.Https",
            "/xyz/openbmc_project/certs/server/https",
            "xyz.openbmc_project.Certs.Install", "Install", filepath);
    }
}; // HttpsCertificate
} // namespace redfish
