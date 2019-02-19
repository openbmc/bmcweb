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
static const char *httpsServiceName =
    "xyz.openbmc_project.Certs.Manager.Server.Https";
static const char *httpsObjectPath = "/xyz/openbmc_project/certs/server/https";
static const char *certInstallIntf = "xyz.openbmc_project.Certs.Install";
static const char *dbusObjManagerIntf = "org.freedesktop.DBus.ObjectManager";

/**
 * The Certificate schema defines a Certificate Service which represents the
 * actions available to manage certificates and links to where certificates
 * are installed.
 */
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
        res.end();
    }
}; // CertificateService

/**
 * Collection of HTTPS certificates
 */
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
        BMCWEB_LOG_DEBUG << "HTTPSCertificateCollection::doGet ";
        res.jsonValue["@odata.id"] =
            "/redfish/v1/Managers/bmc/NetworkProtocol/HTTPS/Certificates";
        res.jsonValue["@odata.type"] =
            "#CertificateCollection.CertificatesCollection";
        res.jsonValue["@odata.context"] =
            "/redfish/v1/$metadata#CertificateCollection.CertificateCollection";
        res.jsonValue["Name"] = "HTTPS Certificates Collection";
        res.jsonValue["Description"] =
            "A Collection of HTTPS certificate instances";
        res.jsonValue["Name"] = "HTTPS Certificate Collection";
        auto asyncResp = std::make_shared<AsyncResp>(res);
        auto getCertificateList = [asyncResp](
                                      const boost::system::error_code ec,
                                      const ManagedObjectType &certs) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "DBUS response error: " << ec;
                messages::internalError(asyncResp->res);
                return;
            }
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
            std::move(getCertificateList), httpsServiceName, httpsObjectPath,
            "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
    }

    void doPost(crow::Response &res, const crow::Request &req,
                const std::vector<std::string> &params) override
    {
        BMCWEB_LOG_DEBUG << "HTTPSCertificateCollection::doPost";

        std::string filepath("/tmp/" + boost::uuids::to_string(
                                           boost::uuids::random_generator()()));
        std::ofstream out(filepath, std::ofstream::out | std::ofstream::binary |
                                        std::ofstream::trunc);
        out << req.body;
        out.close();

        auto asyncResp = std::make_shared<AsyncResp>(res);
        auto installCertificate =
            [asyncResp, filepath](const boost::system::error_code ec) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "DBUS response error: " << ec;
                    messages::internalError(asyncResp->res);
                    std::remove(filepath.c_str());
                    return;
                }
                std::remove(filepath.c_str());
                messages::success(asyncResp->res);
            };
        crow::connections::systemBus->async_method_call(
            std::move(installCertificate), httpsServiceName, httpsObjectPath,
            certInstallIntf, "Install", filepath);
    }
}; // HTTPSCertificateCollection

/**
 * @brief Retrieve the certificates installed list and append to the response
 *
 * @param[in] asyncResp Shared pointer to the response message
 * @param[in] certURI  Path of the certificate object
 * @param[in] service  D-Bus service
 * @param[in] path  Path of the D-Bus service object
 * @return None
 */
void getCertificateLocations(std::shared_ptr<AsyncResp> asyncResp,
                             const std::string &certURI,
                             const std::string &service,
                             const std::string &path)
{
    BMCWEB_LOG_DEBUG << "getCertificateLocations URI=" << certURI
                     << " Service=" << service << " Path=" << path;
    auto getCertLocations = [asyncResp,
                             certURI](const boost::system::error_code ec,
                                      const ManagedObjectType &certs) {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "DBUS response error: " << ec;
            messages::internalError(asyncResp->res);
            return;
        }
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
                {{"@odata.id", certURI + path.substr(lastIndex)}});
        }
        asyncResp->res.jsonValue["Links"]["Certificates@odata.count"] =
            members.size();
    };
    crow::connections::systemBus->async_method_call(
        std::move(getCertLocations), service, path, dbusObjManagerIntf,
        "GetManagedObjects");
}

/**
 * The certificate location schema defines a resource that an administrator
 * can use in order to locate all certificates installed on a given service.
 */
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

        getCertificateLocations(
            asyncResp,
            "/redfish/v1/Managers/bmc/NetworkProtocol/HTTPS/Certificates/",
            httpsServiceName, httpsObjectPath);
    }
}; // CertificateLocations
} // namespace redfish
