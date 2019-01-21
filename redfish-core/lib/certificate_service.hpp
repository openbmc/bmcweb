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

#include <variant>
namespace redfish
{
static const char *httpsServiceName =
    "xyz.openbmc_project.Certs.Manager.Server.Https";
static const char *httpsObjectPath = "/xyz/openbmc_project/certs/server/https";
static const char *ldapServiceName =
    "xyz.openbmc_project.Certs.Manager.Client.Ldap";
static const char *ldapObjectPath = "/xyz/openbmc_project/certs/client/ldap";
static const char *certInstallIntf = "xyz.openbmc_project.Certs.Install";
static const char *certPropIntf = "xyz.openbmc_project.Certs.Certificate";
static const char *dbusPropIntf = "org.freedesktop.DBus.Properties";
static const char *dbusObjManagerIntf = "org.freedesktop.DBus.ObjectManager";

using PropertyType =
    std::variant<std::string, uint64_t, std::vector<std::string>>;
using PropertiesMap = boost::container::flat_map<std::string, PropertyType>;
using CertIssuerMap = std::map<std::string, std::string>;

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
        auto &replaceCert =
            res.jsonValue["Actions"]["#CertificateService.ReplaceCertificate"];
        replaceCert["target"] = "/redfish/v1/CertificateService/Actions/"
                                "CertificateService.ReplaceCertificate";
        replaceCert["CertificateType@Redfish.AllowableValues"] = {
            {"PEM", "PKCS7"}};
        res.end();
    }
}; // CertificateService

/**
 * Action to replace an existing certificate
 */
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
            BMCWEB_LOG_DEBUG << "Replace certificate for service"
                             << httpsServiceName;
            crow::connections::systemBus->async_method_call(
                std::move(replaceCertificate), httpsServiceName, path,
                certInstallIntf, "Install", filepath);
        }
        else if (boost::starts_with(
                     certificateUri,
                     "/redfish/v1/AccountService/LDAP/Certificates/"))
        {
            BMCWEB_LOG_DEBUG << "Replace certificate for service"
                             << ldapServiceName;
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

/**
 * @brief Converts EPOC time to redfish time format
 *
 * @param[in] epocTime     Time value in EPOC format
 * @return Time value in redfish format
 */
static std::string getDateTime(const uint64_t &epocTime)
{
    std::array<char, 128> dateTime;
    std::string redfishDateTime("0000-00-00T00:00:00Z00:00");
    std::time_t time = static_cast<std::time_t>(epocTime);
    if (std::strftime(dateTime.begin(), dateTime.size(), "%FT%T%z",
                      std::localtime(&time)))
    {
        // insert the colon required by the ISO 8601 standard
        redfishDateTime = std::string(dateTime.data());
        redfishDateTime.insert(redfishDateTime.end() - 2, ':');
    }
    return redfishDateTime;
}

/**
 * @brief Trim blank spaces at the beginning of the string
 *
 * @param[in] str string to trim spaces on the left side
 * @return None
 */
static void ltrim(std::string &str)
{
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](int ch) {
                  return !std::isspace(ch);
              }));
}

/**
 * @brief Parse the comma sperated key value pairs and return as a map
 *
 * @param[in] str Issuer/Subject string retrieved from the Certificate
 * @return Map map of Issuer/Subject values
 */
static CertIssuerMap parseIssuerSubject(const std::string &str)
{
    std::istringstream iss(str);
    CertIssuerMap issuerMap;
    std::string token;
    while (std::getline(iss, token, ','))
    {
        size_t pos = token.find('=');
        std::string first = std::move(token.substr(0, pos));
        std::string second = std::move(token.substr(pos + 1));
        ltrim(first);
        ltrim(second);
        issuerMap.emplace(std::pair<std::string, std::string>(
            std::move(first), std::move(second)));
    }
    return issuerMap;
}

/**
 * @brief Retrieve the certificates properties and append to the response
 * message
 *
 * @param[in] asyncResp Shared pointer to the response message
 * @param[in] service  D-Bus service
 * @param[in] path  Path of the D-Bus service object
 * @return None
 */
static void getCertificateProperties(std::shared_ptr<AsyncResp> asyncResp,
                                     const std::string &service,
                                     const std::string &path)
{
    BMCWEB_LOG_DEBUG << "Update certificate properties service=" << service
                     << " Path=" << path;
    auto getAllProperties = [asyncResp](const boost::system::error_code ec,
                                        const PropertiesMap &properties) {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "DBUS response error: " << ec;
            messages::internalError(asyncResp->res);
            return;
        }
        for (const auto &property : properties)
        {
            if (property.first == "CertificateString")
            {
                asyncResp->res.jsonValue["CertificateString"] = "";
                auto value = std::get_if<std::string>(&property.second);
                if (value)
                {
                    asyncResp->res.jsonValue["CertificateString"] = *value;
                }
            }
            else if (property.first == "KeyUsage")
            {
                auto &keyUsage = asyncResp->res.jsonValue["KeyUsage"];
                keyUsage = nlohmann::json::array();
                auto value =
                    std::get_if<std::vector<std::string>>(&property.second);
                if (value)
                {
                    for (const auto usage : *value)
                    {
                        keyUsage.push_back(usage);
                    }
                }
            }
            else if (property.first == "Issuer")
            {
                auto value = std::get_if<std::string>(&property.second);
                if (value)
                {
                    CertIssuerMap issuerMap =
                        std::move(parseIssuerSubject(*value));
                    auto elem = issuerMap.find("L");
                    if (elem != issuerMap.end())
                    {
                        asyncResp->res.jsonValue["Issuer"]["City"] =
                            std::move(elem->second);
                    }
                    elem = issuerMap.find("CN");
                    if (elem != issuerMap.end())
                    {
                        asyncResp->res.jsonValue["Issuer"]["CommonName"] =
                            std::move(elem->second);
                    }
                    elem = issuerMap.find("C");
                    if (elem != issuerMap.end())
                    {
                        asyncResp->res.jsonValue["Issuer"]["Country"] =
                            std::move(elem->second);
                    }
                    elem = issuerMap.find("O");
                    if (elem != issuerMap.end())
                    {
                        asyncResp->res.jsonValue["Issuer"]["Organization"] =
                            std::move(elem->second);
                    }
                    elem = issuerMap.find("OU");
                    if (elem != issuerMap.end())
                    {
                        asyncResp->res
                            .jsonValue["Issuer"]["OrganizationalUnit"] =
                            std::move(elem->second);
                    }
                    elem = issuerMap.find("ST");
                    if (elem != issuerMap.end())
                    {
                        asyncResp->res.jsonValue["Issuer"]["State"] =
                            std::move(elem->second);
                    }
                }
            }
            else if (property.first == "Subject")
            {
                auto value = std::get_if<std::string>(&property.second);
                if (value)
                {
                    std::map<std::string, std::string> subjectMap =
                        std::move(parseIssuerSubject(*value));
                    auto elem = subjectMap.find("L");
                    if (elem != subjectMap.end())
                    {
                        asyncResp->res.jsonValue["Subject"]["City"] =
                            std::move(elem->second);
                    }
                    elem = subjectMap.find("CN");
                    if (elem != subjectMap.end())
                    {
                        asyncResp->res.jsonValue["Subject"]["CommonName"] =
                            std::move(elem->second);
                    }
                    elem = subjectMap.find("C");
                    if (elem != subjectMap.end())
                    {
                        asyncResp->res.jsonValue["Subject"]["Country"] =
                            std::move(elem->second);
                    }
                    elem = subjectMap.find("O");
                    if (elem != subjectMap.end())
                    {
                        asyncResp->res.jsonValue["Subject"]["Organization"] =
                            std::move(elem->second);
                    }
                    elem = subjectMap.find("OU");
                    if (elem != subjectMap.end())
                    {
                        asyncResp->res
                            .jsonValue["Subject"]["OrganizationalUnit"] =
                            std::move(elem->second);
                    }
                    elem = subjectMap.find("ST");
                    if (elem != subjectMap.end())
                    {
                        asyncResp->res.jsonValue["Subject"]["State"] =
                            std::move(elem->second);
                    }
                }
            }
            else if (property.first == "ValidNotAfter")
            {
                auto value = std::get_if<uint64_t>(&property.second);
                if (value)
                {
                    asyncResp->res.jsonValue["ValidNotAfter"] =
                        std::move(getDateTime(*value));
                }
            }
            else if (property.first == "ValidNotBefore")
            {
                auto value = std::get_if<uint64_t>(&property.second);
                if (value)
                {
                    asyncResp->res.jsonValue["ValidNotBefore"] =
                        std::move(getDateTime(*value));
                }
            }
        }
    };
    crow::connections::systemBus->async_method_call(std::move(getAllProperties),
                                                    service, path, dbusPropIntf,
                                                    "GetAll", certPropIntf);
}

/**
 * Certificate resource describes a certificate used to prove the identity
 * of a component, account or service.
 */
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
        BMCWEB_LOG_DEBUG << "HTTPSCertificate::doGet ID=" << certId;
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
        auto asyncResp = std::make_shared<AsyncResp>(res);
        std::string path =
            std::string(httpsObjectPath) + "/" + std::to_string(certId);
        getCertificateProperties(asyncResp, httpsServiceName, path);
    }
}; // HTTPSCertificate

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

        getCertificateLocations(asyncResp,
                                "/redfish/v1/AccountService/LDAP/Certificates/",
                                ldapServiceName, ldapObjectPath);
    }
}; // CertificateLocations

/**
 * Collection of LDAP certificates
 */
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
        BMCWEB_LOG_DEBUG << "LDAPCertificateCollection::doGet";
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

/**
 * Certificate resource describes a certificate used to prove the identity
 * of a component, account or service.
 */
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
        BMCWEB_LOG_DEBUG << "LDAPCertificate::doGet ID=" << certId;
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
