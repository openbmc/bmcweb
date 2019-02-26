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
static const char *httpsObjectPath = "/xyz/openbmc_project/certs/server/https";
static const char *certInstallIntf = "xyz.openbmc_project.Certs.Install";
static const char *dbusObjManagerIntf = "org.freedesktop.DBus.ObjectManager";
static const char *mapperBusName = "xyz.openbmc_project.ObjectMapper";
static const char *mapperObjectPath = "/xyz/openbmc_project/object_mapper";
static const char *mapperIntf = "xyz.openbmc_project.ObjectMapper";

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
        // TODO: Issue#61 No entries are available for Certificate
        // sevice at https://www.dmtf.org/standards/redfish
        // "redfish standard registries". Need to modify after DMTF
        // publish Privilege details for certificate service
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
        res.jsonValue = {
            {"@odata.type", "#CertificateService.v1_0_0.CertificateService"},
            {"@odata.id", "/redfish/v1/CertificateService"},
            {"@odata.context",
             "/redfish/v1/$metadata#CertificateService.CertificateService"},
            {"Id", "CertificateService"},
            {"Name", "Certificate Service"},
            {"Description", "Actions available to manage certificates"}};
        res.jsonValue["CertificateLocations"] = {
            {"@odata.id",
             "/redfish/v1/CertificateService/CertificateLocations"}};
        res.end();
    }
}; // CertificateService

namespace fs = std::filesystem;

/**
 * @brief Find the ID specified in the URL
 * Finds the numbers specified after the last "/" in the URL and returns.
 * @param[in] path URL
 * @return 0 on failure and number on success
 */
int getIDFromURL(const std::string &path)
{
    fs::path url(path);
    return atoi((url.filename()).c_str());
}

/**
 * Class to create a temporary certificate file for uploading to system
 */
class CertificateFile
{
  public:
    CertificateFile() = delete;
    CertificateFile(const CertificateFile &) = delete;
    CertificateFile &operator=(const CertificateFile &) = delete;
    CertificateFile(CertificateFile &&) = delete;
    CertificateFile &operator=(CertificateFile &&) = delete;
    CertificateFile(const std::string &certString)
    {
        char dirTemplate[] = "/tmp/Certs.XXXXXX";
        auto tempDirectory = mkdtemp(dirTemplate);
        if (tempDirectory)
        {
            certDirectory = tempDirectory;
            certificateFile = certDirectory / "cert.pem";
            std::ofstream out(certificateFile, std::ofstream::out |
                                                   std::ofstream::binary |
                                                   std::ofstream::trunc);
            out << certString;
            out.close();
            BMCWEB_LOG_DEBUG << "Certificate file" << certificateFile;
        }
    }
    ~CertificateFile()
    {
        if (fs::exists(certDirectory))
        {
            try
            {
                fs::remove_all(certDirectory);
            }
            catch (const fs::filesystem_error &e)
            {
                BMCWEB_LOG_ERROR << "Failed to remove temp directory"
                                 << certDirectory;
            }
        }
    }
    std::string getCertFilePath()
    {
        return certificateFile;
    }

  private:
    fs::path certificateFile;
    fs::path certDirectory;
};

using GetObjectType =
    std::vector<std::pair<std::string, std::vector<std::string>>>;

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
        res.jsonValue = {
            {"@odata.id",
             "/redfish/v1/Managers/bmc/NetworkProtocol/HTTPS/Certificates"},
            {"@odata.type", "#CertificateCollection.CertificateCollection"},
            {"@odata.context",
             "/redfish/v1/"
             "$metadata#CertificateCollection.CertificateCollection"},
            {"Name", "HTTPS Certificates Collection"},
            {"Description", "A Collection of HTTPS certificate instances"}};
        auto asyncResp = std::make_shared<AsyncResp>(res);
        auto getCertList = [asyncResp](const boost::system::error_code ec,
                                       const ManagedObjectType &certs) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "DBUS response error: " << ec;
                messages::internalError(asyncResp->res);
                return;
            }
            nlohmann::json &members = asyncResp->res.jsonValue["Members"];
            members = nlohmann::json::array();
            for (const auto &cert : certs)
            {
                auto id = getIDFromURL(cert.first.str);
                if (id)
                {
                    members.push_back(
                        {{"@odata.id", "/redfish/v1/Managers/bmc/"
                                       "NetworkProtocol/HTTPS/Certificates/" +
                                           std::to_string(id)}});
                }
            }
            asyncResp->res.jsonValue["Members@odata.count"] = members.size();
        };

        auto getServiceName = [asyncResp, getCertList(std::move(getCertList))](
                                  const boost::system::error_code ec,
                                  const GetObjectType &resp) {
            if (ec || resp.empty())
            {
                BMCWEB_LOG_ERROR << "DBUS response error: " << ec;
                messages::internalError(asyncResp->res);
                return;
            }
            std::string service = resp.begin()->first;
            crow::connections::systemBus->async_method_call(
                std::move(getCertList), service, httpsObjectPath,
                dbusObjManagerIntf, "GetManagedObjects");
        };
        crow::connections::systemBus->async_method_call(
            std::move(getServiceName), mapperBusName, mapperObjectPath,
            mapperIntf, "GetObject", httpsObjectPath,
            std::array<std::string, 0>());
    }

    void doPost(crow::Response &res, const crow::Request &req,
                const std::vector<std::string> &params) override
    {
        std::shared_ptr<CertificateFile> certFile =
            std::make_shared<CertificateFile>(req.body);

        auto asyncResp = std::make_shared<AsyncResp>(res);
        auto installCert = [asyncResp,
                            certFile](const boost::system::error_code ec) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "DBUS response error: " << ec;
                messages::internalError(asyncResp->res);
                return;
            }
            messages::created(asyncResp->res);
            BMCWEB_LOG_DEBUG << "HTTPS certificate install file="
                             << certFile->getCertFilePath();
        };

        auto getServiceName = [asyncResp, installCert(std::move(installCert)),
                               certFile](const boost::system::error_code ec,
                                         const GetObjectType &resp) {
            if (ec || resp.empty())
            {
                BMCWEB_LOG_ERROR << "DBUS response error: " << ec;
                messages::internalError(asyncResp->res);
                return;
            }
            std::string service = resp.begin()->first;
            crow::connections::systemBus->async_method_call(
                std::move(installCert), service, httpsObjectPath,
                certInstallIntf, "Install", certFile->getCertFilePath());
        };
        crow::connections::systemBus->async_method_call(
            std::move(getServiceName), mapperBusName, mapperObjectPath,
            mapperIntf, "GetObject", httpsObjectPath,
            std::array<std::string, 0>());
    }
}; // HTTPSCertificateCollection

/**
 * @brief Retrieve the certificates installed list and append to the response
 *
 * @param[in] asyncResp Shared pointer to the response message
 * @param[in] certURL  Path of the certificate object
 * @param[in] path  Path of the D-Bus service object
 * @return None
 */
static void getCertificateLocations(std::shared_ptr<AsyncResp> &asyncResp,
                                    const std::string &certURL,
                                    const std::string &path)
{
    BMCWEB_LOG_DEBUG << "getCertificateLocations URI=" << certURL
                     << " Path=" << path;
    auto getCertLocations = [asyncResp,
                             certURL](const boost::system::error_code ec,
                                      const ManagedObjectType &certs) {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "DBUS response error: " << ec;
            messages::internalError(asyncResp->res);
            return;
        }
        nlohmann::json &links =
            asyncResp->res.jsonValue["Links"]["Certificates"];
        for (auto &cert : certs)
        {
            auto id = getIDFromURL(cert.first.str);
            if (id)
            {
                links.push_back({{"@odata.id", certURL + std::to_string(id)}});
            }
        }
        asyncResp->res.jsonValue["Links"]["Certificates@odata.count"] =
            links.size();
    };

    auto getServiceName =
        [asyncResp, getCertLocations(std::move(getCertLocations)),
         path](const boost::system::error_code ec, const GetObjectType &resp) {
            if (ec || resp.empty())
            {
                BMCWEB_LOG_ERROR << "DBUS response error: " << ec;
                messages::internalError(asyncResp->res);
                return;
            }
            std::string service = resp.begin()->first;
            crow::connections::systemBus->async_method_call(
                std::move(getCertLocations), service, path, dbusObjManagerIntf,
                "GetManagedObjects");
        };
    crow::connections::systemBus->async_method_call(
        std::move(getServiceName), mapperBusName, mapperObjectPath, mapperIntf,
        "GetObject", path, std::array<std::string, 0>());
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
        res.jsonValue = {
            {"@odata.id",
             "/redfish/v1/CertificateService/CertificateLocations"},
            {"@odata.type",
             "#CertificateLocations.v1_0_0.CertificateLocations"},
            {"@odata.context",
             "/redfish/v1/$metadata#CertificateLocations.CertificateLocations"},
            {"Name", "Certificate Locations"},
            {"Id", "CertificateLocations"},
            {"Description",
             "Defines a resource that an administrator can use in order to"
             "locate all certificates installed on a given service"}};
        auto asyncResp = std::make_shared<AsyncResp>(res);
        nlohmann::json &links =
            asyncResp->res.jsonValue["Links"]["Certificates"];
        links = nlohmann::json::array();
        getCertificateLocations(
            asyncResp,
            "/redfish/v1/Managers/bmc/NetworkProtocol/HTTPS/Certificates/",
            httpsObjectPath);
    }
}; // CertificateLocations
} // namespace redfish
