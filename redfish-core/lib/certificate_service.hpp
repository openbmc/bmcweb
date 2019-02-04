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
using ErrorCode = boost::system::error_code;
using Params = std::vector<std::string>;

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
        // Will be fixed in the next patch set
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
        // Will be fixed in the next patch set
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
               const Params &params) override
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
        auto asyncResp = std::make_shared<AsyncResp>(res);
        auto getCertificateList = [asyncResp](const ErrorCode ec,
                                              const ManagedObjectType &certs) {
            auto &members = asyncResp->res.jsonValue["Members"];
            for (auto &cert : certs)
            {
                const std::string &path =
                    static_cast<const std::string &>(cert.first);
                auto lastIndex = path.rfind("/");
                if (lastIndex == std::string::npos)
                {
                    lastIndex = 0;
                }
                else
                {
                    lastIndex += 1;
                }
                members.push_back(
                    {{"@odata.id", "/redfish/v1/Managers/bmc/NetworkProtocol/"
                                   "HTTPS/Certificates/" +
                                       path.substr(lastIndex)}});
            }
            asyncResp->res.jsonValue["Members@odata.count"] = certs.size();
        };
        crow::connections::systemBus->async_method_call(
            std::move(getCertificateList),
            "xyz.openbmc_project.Certs.Manager.Server.Https",
            "/xyz/openbmc_project/certs/server/https",
            "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
    }

    void doPost(crow::Response &res, const crow::Request &req,
                const Params &params) override
    {
        std::string filepath("/tmp/" + boost::uuids::to_string(
                                           boost::uuids::random_generator()()));
        std::ofstream out(filepath, std::ofstream::out | std::ofstream::binary |
                                        std::ofstream::trunc);
        out << req.body;
        out.close();

        auto asyncResp = std::make_shared<AsyncResp>(res);
        auto installCertificate = [asyncResp, filepath](const ErrorCode ec) {
            if (ec)
            {
                messages::internalError(asyncResp->res);
                std::remove(filepath.c_str());
                return;
            }
            std::remove(filepath.c_str());
            messages::success(asyncResp->res);
        };
        crow::connections::systemBus->async_method_call(
            std::move(installCertificate),
            "xyz.openbmc_project.Certs.Manager.Server.Https",
            "/xyz/openbmc_project/certs/server/https",
            "xyz.openbmc_project.Certs.Install", "Install", filepath);
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
        auto asyncResp = std::make_shared<AsyncResp>(res);
        auto getCertificateList = [asyncResp](const ErrorCode ec,
                                              const ManagedObjectType &certs) {
            auto &members = asyncResp->res.jsonValue["Links"]["Certificates"];
            for (auto &cert : certs)
            {
                const std::string &path =
                    static_cast<const std::string &>(cert.first);
                auto lastIndex = path.rfind("/");
                if (lastIndex == std::string::npos)
                {
                    lastIndex = 0;
                }
                else
                {
                    lastIndex += 1;
                }
                members.push_back(
                    {{"@odata.id", "/redfish/v1/Managers/bmc/NetworkProtocol/"
                                   "HTTPS/Certificates/" +
                                       path.substr(lastIndex)}});
            }
            asyncResp->res.jsonValue["Links"]["Certificates@odata.count"] =
                members.size();
        };
        crow::connections::systemBus->async_method_call(
            std::move(getCertificateList),
            "xyz.openbmc_project.Certs.Manager.Server.Https",
            "/xyz/openbmc_project/certs/server/https",
            "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
    }
}; // CertificateLocations
} // namespace redfish
