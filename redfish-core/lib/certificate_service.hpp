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

#include <utils/json_utils.hpp>

namespace redfish
{

using VariantType = sdbusplus::message::variant<bool, std::string, uint64_t>;
const char *httpsServiceName = "xyz.openbmc_project.Certs.Manager.Server.Https";
const char *httpsObjectPath = "/xyz/openbmc_project/certs/server/https";
const char *ldapServiceName = "xyz.openbmc_project.Certs.Manager.Client.Ldap";
const char *ldapObjectPath = "/xyz/openbmc_project/certs/client/ldap";
const char *certInstallIntf = "xyz.openbmc_project.Certs.Install";
const char *certPropIntf = "xyz.openbmc_project.Certs.Certificate";
const char *dbusPropIntf = "org.freedesktop.DBus.Properties";
const char *dbusObjManagerIntf = "org.freedesktop.DBus.ObjectManager";

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
        auto &replaceCert =
            res.jsonValue["Actions"]["#CertificateService.ReplaceCertificate"];
        replaceCert["target"] = "/redfish/v1/CertificateService/Actions/"
                                "CertificateService.ReplaceCertificate";
        replaceCert["CertificateType@Redfish.AllowableValues"] = {
            {"PEM", "PKCS7"}};
        res.end();
    }
}; // CertificateService

class CertificateActionsReplaceCertificate : public Node
{
  public:
    CertificateActionsReplaceCertificate(CrowApp &app) :
        Node(app, "/redfish/v1/CertificateService/Actions/"
                  "CertificateService.ReplaceCertificate/")
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
    void doPost(crow::Response &res, const crow::Request &req,
                const std::vector<std::string> &params) override
    {
        std::string certificate, certificateType, certificateUri;
        if (!json_util::readJson(req, res, "CertificateString", certificate,
                                 "CertificateType", certificateType,
                                 "CertificateUri", certificateUri))
        {
            BMCWEB_LOG_ERROR << "Required parameters are missing";
            return;
        }

        std::string filepath("/tmp/" + boost::uuids::to_string(
                                           boost::uuids::random_generator()()));
        std::ofstream out(filepath, std::ofstream::out | std::ofstream::binary |
                                        std::ofstream::trunc);
        out << req.body;
        out.close();

        auto asyncResp = std::make_shared<AsyncResp>(res);
        auto replaceCertificate =
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

        auto lastIndex = certificateUri.rfind("/");
        if (lastIndex == std::string::npos)
        {
            lastIndex = 0;
        }
        else
        {
            lastIndex += 1;
        }
        if (boost::starts_with(
                certificateUri,
                "/redfish/v1/Managers/bmc/NetworkProtocol/HTTPS/Certificates/"))
        {

            std::string path = std::string(httpsObjectPath) + "/" +
                               certificateUri.substr(lastIndex);
            crow::connections::systemBus->async_method_call(
                std::move(replaceCertificate), httpsServiceName, path,
                certInstallIntf, "Install", filepath);
        }
        else if (boost::starts_with(
                     certificateUri,
                     "/redfish/v1/AccountService/LDAP/Certificates/"))
        {
            std::string path = std::string(ldapObjectPath) + "/" +
                               certificateUri.substr(lastIndex);
            crow::connections::systemBus->async_method_call(
                std::move(replaceCertificate), ldapServiceName, path,
                certInstallIntf, "Install", filepath);
        }
        else
        {
            BMCWEB_LOG_ERROR << "Unsupported certificate URI" << certificateUri;
            return;
        }
    }
}; // CertificateActionsReplaceCertificate

void getCertificateProperties(std::shared_ptr<AsyncResp> asyncResp,
                              const std::string &service,
                              const std::string &path)
{
    auto getAllProperties =
        [asyncResp](const boost::system::error_code ec,
                    const std::vector<std::pair<std::string, VariantType>>
                        &properties) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "DBUS response error: " << ec;
                messages::internalError(asyncResp->res);
                return;
            }
            // TODO: need to support additional properties after back-end
            // support is available
            for (const auto &property : properties)
            {
                if (property.first == "CertificateString")
                {
                    const std::string *value =
                        sdbusplus::message::variant_ns::get_if<std::string>(
                            &property.second);
                    if (value != nullptr)
                    {
                        asyncResp->res.jsonValue["CertificateString"] = *value;
                    }
                }
            }
        };
    crow::connections::systemBus->async_method_call(std::move(getAllProperties),
                                                    service, path, dbusPropIntf,
                                                    "GetAll", certPropIntf);
}

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
        res.jsonValue["CertificateType"] = "PEM";
        res.jsonValue["KeyUsage"] = {"ServerAuthentication"};
        auto asyncResp = std::make_shared<AsyncResp>(res);
        std::string path =
            std::string(httpsObjectPath) + "/" + std::to_string(certId);
        getCertificateProperties(asyncResp, httpsServiceName, path);
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

void getCertificateLocations(std::shared_ptr<AsyncResp> asyncResp,
                             const std::string &certURI,
                             const std::string &service,
                             const std::string &path)
{
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

        getCertificateLocations(asyncResp,
                                "/redfish/v1/AccountService/LDAP/Certificates/",
                                ldapServiceName, ldapObjectPath);
    }
}; // CertificateLocations

class LDAPCertificateCollection : public Node
{
  public:
    template <typename CrowApp>
    LDAPCertificateCollection(CrowApp &app) :
        Node(app, "/redfish/v1/AccountService/LDAP/Certificates/")
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
            "/redfish/v1/AccountService/LDAP/Certificates";
        res.jsonValue["@odata.type"] =
            "#CertificateCollection.CertificatesCollection";
        res.jsonValue["@odata.context"] =
            "/redfish/v1/$metadata#CertificateCollection.CertificateCollection";
        res.jsonValue["Name"] = "LDAP Certificates Collection";
        res.jsonValue["Description"] =
            "A Collection of LDAP certificate instances";
        res.jsonValue["Name"] = "LDAP Certificate Collection";
        auto asyncResp = std::make_shared<AsyncResp>(res);
        auto getCertificateList =
            [asyncResp](const boost::system::error_code ec,
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
                        {{"@odata.id",
                          "/redfish/v1/AccountService/LDAP/Certificates/" +
                              path.substr(lastIndex)}});
                }
                asyncResp->res.jsonValue["Members@odata.count"] = certs.size();
            };
        crow::connections::systemBus->async_method_call(
            std::move(getCertificateList), ldapServiceName, ldapObjectPath,
            dbusObjManagerIntf, "GetManagedObjects");
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
            std::move(installCertificate), ldapServiceName, ldapObjectPath,
            certInstallIntf, "Install", filepath);
    }
}; // LDAPCertificateCollection

class LDAPCertificate : public Node
{
  public:
    template <typename CrowApp>
    LDAPCertificate(CrowApp &app) :
        Node(app, "/redfish/v1/AccountService/LDAP/Certificates/<str>/",
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
        res.jsonValue["@odata.id"] =
            "/redfish/v1/AccountService/LDAP/Certificates/" +
            std::to_string(certId);
        res.jsonValue["@odata.type"] = "#Certificate.v1_0_0.Certificate";
        res.jsonValue["@odata.context"] =
            "/redfish/v1/$metadata#Certificate.Certificate";
        res.jsonValue["Id"] = std::to_string(certId);
        res.jsonValue["Name"] = "LDAP Certificate";
        res.jsonValue["Description"] = "LDAP Certificate";
        res.jsonValue["CertificateType"] = "PEM";
        res.jsonValue["KeyUsage"] = {"ClientAuthentication"};
        std::string path =
            std::string(ldapObjectPath) + "/" + std::to_string(certId);
        auto asyncResp = std::make_shared<AsyncResp>(res);
        getCertificateProperties(asyncResp, ldapServiceName, path);
    }
}; // LDAPCertificate

} // namespace redfish
