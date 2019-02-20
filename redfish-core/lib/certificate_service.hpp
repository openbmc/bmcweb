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
static const char *httpsObjectPath = "/xyz/openbmc_project/certs/server/https";
static const char *certInstallIntf = "xyz.openbmc_project.Certs.Install";
static const char *certPropIntf = "xyz.openbmc_project.Certs.Certificate";
static const char *dbusPropIntf = "org.freedesktop.DBus.Properties";
static const char *dbusObjManagerIntf = "org.freedesktop.DBus.ObjectManager";
static const char *mapperBusName = "xyz.openbmc_project.ObjectMapper";
static const char *mapperObjectPath = "/xyz/openbmc_project/object_mapper";
static const char *mapperIntf = "xyz.openbmc_project.ObjectMapper";
static const char *ldapObjectPath = "/xyz/openbmc_project/certs/client/ldap";

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
        // TODO (devenrao) No entries are available for Certificate
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
/**
 * @brief Find the ID specified in the URL
 * Finds the numbers specified after the last "/" in the URL and returns.
 * @param[in] path URL
 * @return 0 on failure and number on success
 */
int getIDFromURL(const std::string &path)
{
    std::size_t found = path.rfind("/");
    if ((found != std::string::npos) && (found++ <= path.length()))
    {
        return atoi(path.substr(found).c_str());
    }
    return 0;
}
using GetObjectType =
    std::vector<std::pair<std::string, std::vector<std::string>>>;

/**
 * @brief Converts EPOC time to redfish time format
 *
 * @param[in] epochTime Time value in EPOCH format
 * @return Time value in redfish format
 */
static std::string epochToString(uint64_t epochTime)
{
    std::array<char, 128> dateTime;
    std::string redfishDateTime("0000-00-00T00:00:00");
    std::time_t time = static_cast<std::time_t>(epochTime);
    if (std::strftime(dateTime.begin(), dateTime.size(), "%FT%T%z",
                      std::localtime(&time)))
    {
        // insert the colon required by the ISO 8601 standard
        redfishDateTime = std::string(dateTime.data());
        redfishDateTime.insert(redfishDateTime.end() - 2, ':');
    }
    return redfishDateTime;
}

using CertPropMap = std::map<std::string, std::string>;
/**
 * @brief Parse the comma sperated key value pairs and return as a map
 *
 * @param[in] str  comma seperated key value pairs
 * @return Map map of key value pairs
 */
static CertPropMap parseCertProperty(const std::string &str)
{
    CertPropMap propMap;
    typedef boost::tokenizer<boost::char_separator<char>> tokenizer;
    boost::char_separator<char> commaSep{","};
    tokenizer tokenComma{str, commaSep};
    for (auto &tok : tokenComma)
    {
        std::string data = tok;
        boost::algorithm::trim(data);
        boost::char_separator<char> equalsep{"="};
        tokenizer tokequal{data, equalsep};
        auto dist = std::distance(tokequal.begin(), tokequal.end());
        if (dist == 2)
        {
            tokenizer::iterator it = tokequal.begin();
            std::string first = *it;
            ++it;
            std::string second = *it;
            propMap.emplace(std::pair<std::string, std::string>(
                std::move(first), std::move(second)));
        }
    }
    return propMap;
}

/**
 * @brief Parse and update Certficate Issue/Subject property
 *
 * @param[in] asyncResp Shared pointer to the response message
 * @param[in] str  Issuer/Subject value in key=value pairs
 * @param[in] type Issuer/Subject
 * @return None
 */
static void updateCertIssuerOrSubject(std::shared_ptr<AsyncResp> asyncResp,
                                      const std::string *value,
                                      const std::string &type)
{
    CertPropMap propMap = parseCertProperty(*value);
    auto elem = propMap.find("L");
    if (elem != propMap.end())
    {
        asyncResp->res.jsonValue[type]["City"] = std::move(elem->second);
    }
    elem = propMap.find("CN");
    if (elem != propMap.end())
    {
        asyncResp->res.jsonValue[type]["CommonName"] = std::move(elem->second);
    }
    elem = propMap.find("C");
    if (elem != propMap.end())
    {
        asyncResp->res.jsonValue[type]["Country"] = std::move(elem->second);
    }
    elem = propMap.find("O");
    if (elem != propMap.end())
    {
        asyncResp->res.jsonValue[type]["Organization"] =
            std::move(elem->second);
    }
    elem = propMap.find("OU");
    if (elem != propMap.end())
    {
        asyncResp->res.jsonValue[type]["OrganizationalUnit"] =
            std::move(elem->second);
    }
    elem = propMap.find("ST");
    if (elem != propMap.end())
    {
        asyncResp->res.jsonValue[type]["State"] = std::move(elem->second);
    }
}

/**
 * @brief Check if keyusage retrieved from Certificate is of redfish supported
 * type
 *
 * @param[in] str keyusage value retrieved from certificate
 * @return true if it is of redfish type else false
 */
static bool isKeyUsageFound(const std::string &str)
{
    boost::container::flat_set<std::string> usageList = {
        "DigitalSignature",     "NonRepudiation",       "KeyEncipherment",
        "DataEncipherment",     "KeyAgreement",         "KeyCertSign",
        "CRLSigning",           "EncipherOnly",         "DecipherOnly",
        "ServerAuthentication", "ClientAuthentication", "CodeSigning",
        "EmailProtection",      "Timestamping",         "OCSPSigning"};
    auto it = usageList.find(str);
    if (it != usageList.end())
    {
        return true;
    }
    return false;
}

/**
 * @brief Retrieve the certificates properties and append to the response
 * message
 *
 * @param[in] asyncResp Shared pointer to the response message
 * @param[in] path  Path of the D-Bus service object
 * @return None
 */
static void getCertificateProperties(std::shared_ptr<AsyncResp> &asyncResp,
                                     const std::string &path)
{
    using PropertyType =
        std::variant<std::string, uint64_t, std::vector<std::string>>;
    using PropertiesMap = boost::container::flat_map<std::string, PropertyType>;

    BMCWEB_LOG_DEBUG << "Update certificate properties Path=" << path;
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
                const std::string *value =
                    std::get_if<std::string>(&property.second);
                if (value)
                {
                    asyncResp->res.jsonValue["CertificateString"] = *value;
                }
            }
            else if (property.first == "KeyUsage")
            {
                nlohmann::json &keyUsage = asyncResp->res.jsonValue["KeyUsage"];
                keyUsage = nlohmann::json::array();
                const std::vector<std::string> *value =
                    std::get_if<std::vector<std::string>>(&property.second);
                if (value)
                {
                    for (const std::string &usage : *value)
                    {
                        if (isKeyUsageFound(usage))
                        {
                            keyUsage.push_back(std::move(usage));
                        }
                    }
                }
            }
            else if (property.first == "Issuer")
            {
                const std::string *value =
                    std::get_if<std::string>(&property.second);
                if (value)
                {
                    updateCertIssuerOrSubject(asyncResp, value, "Issuer");
                }
            }
            else if (property.first == "Subject")
            {
                const std::string *value =
                    std::get_if<std::string>(&property.second);
                if (value)
                {
                    updateCertIssuerOrSubject(asyncResp, value, "Subject");
                }
            }
            else if (property.first == "ValidNotAfter")
            {
                const uint64_t *value = std::get_if<uint64_t>(&property.second);
                if (value)
                {
                    asyncResp->res.jsonValue["ValidNotAfter"] =
                        epochToString(*value);
                }
            }
            else if (property.first == "ValidNotBefore")
            {
                const uint64_t *value = std::get_if<uint64_t>(&property.second);
                if (value)
                {
                    asyncResp->res.jsonValue["ValidNotBefore"] =
                        epochToString(*value);
                }
            }
        }
    };
    auto getServiceName =
        [asyncResp, getAllProperties(std::move(getAllProperties)),
         path](const boost::system::error_code ec, const GetObjectType &resp) {
            if (ec || resp.empty())
            {
                BMCWEB_LOG_ERROR << "DBUS response error: " << ec;
                messages::internalError(asyncResp->res);
                return;
            }
            std::string service = resp.begin()->first;
            BMCWEB_LOG_DEBUG << "getServiceName service: " << service;
            crow::connections::systemBus->async_method_call(
                std::move(getAllProperties), service, path, dbusPropIntf,
                "GetAll", certPropIntf);
        };
    crow::connections::systemBus->async_method_call(
        std::move(getServiceName), mapperBusName, mapperObjectPath, mapperIntf,
        "GetObject", path, std::array<std::string, 0>());
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
        const std::string &certId = params[0];
        BMCWEB_LOG_DEBUG << "HTTPSCertificate::doGet ID=" << certId;
        res.jsonValue = {
            {"@odata.id",
             "/redfish/v1/Managers/bmc/NetworkProtocol/HTTPS/Certificates/" +
                 certId},
            {"@odata.type", "#Certificate.v1_0_0.Certificate"},
            {"@odata.context", "/redfish/v1/$metadata#Certificate.Certificate"},
            {"Id", certId},
            {"Name", "HTTPS Certificate"},
            {"Description", "HTTPS Certificate"},
            {"CertificateType", "PEM"}};
        auto asyncResp = std::make_shared<AsyncResp>(res);
        std::string path = std::string(httpsObjectPath) + "/" + certId;
        getCertificateProperties(asyncResp, path);
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
            asyncResp->res.jsonValue["Members@odata.count"] = certs.size();
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
            BMCWEB_LOG_DEBUG << "getServiceName service: " << service;
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
        BMCWEB_LOG_DEBUG << "HTTPSCertificateCollection::doPost";

        std::string filepath("/tmp/" + boost::uuids::to_string(
                                           boost::uuids::random_generator()()));
        std::ofstream out(filepath, std::ofstream::out | std::ofstream::binary |
                                        std::ofstream::trunc);
        out << req.body;
        out.close();

        auto asyncResp = std::make_shared<AsyncResp>(res);
        auto installCert = [asyncResp,
                            filepath](const boost::system::error_code ec) {
            std::remove(filepath.c_str());
            if (ec)
            {
                BMCWEB_LOG_ERROR << "DBUS response error: " << ec;
                messages::internalError(asyncResp->res);
                return;
            }
            messages::success(asyncResp->res);
        };

        auto getServiceName = [asyncResp, installCert(std::move(installCert)),
                               filepath](const boost::system::error_code ec,
                                         const GetObjectType &resp) {
            if (ec || resp.empty())
            {
                BMCWEB_LOG_ERROR << "DBUS response error: " << ec;
                messages::internalError(asyncResp->res);
                return;
            }
            std::string service = resp.begin()->first;
            BMCWEB_LOG_DEBUG << "getServiceName service: " << service;
            crow::connections::systemBus->async_method_call(
                std::move(installCert), service, httpsObjectPath,
                certInstallIntf, "Install", filepath);
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
void getCertificateLocations(std::shared_ptr<AsyncResp> &asyncResp,
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
            BMCWEB_LOG_DEBUG << "getServiceName service: " << service;
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
        getCertificateLocations(asyncResp,
                                "/redfish/v1/AccountService/LDAP/Certificates/",
                                ldapObjectPath);
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
        res.jsonValue = {
            {"@odata.id", "/redfish/v1/AccountService/LDAP/Certificates"},
            {"@odata.type", "#CertificateCollection.CertificateCollection"},
            {"@odata.context",
             "/redfish/v1/"
             "$metadata#CertificateCollection.CertificateCollection"},
            {"Name", "LDAP Certificates Collection"},
            {"Description", "A Collection of LDAP certificate instances"}};
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
                nlohmann::json &members = asyncResp->res.jsonValue["Members"];
                members = nlohmann::json::array();
                for (const auto &cert : certs)
                {
                    auto id = getIDFromURL(cert.first.str);
                    if (id)
                    {
                        members.push_back(
                            {{"@odata.id",
                              "/redfish/v1/AccountService/LDAP/Certificates/" +
                                  std::to_string(id)}});
                    }
                }
                asyncResp->res.jsonValue["Members@odata.count"] = certs.size();
            };
        auto getServiceName =
            [asyncResp, getCertificateList(std::move(getCertificateList))](
                const boost::system::error_code ec, const GetObjectType &resp) {
                if (ec || resp.empty())
                {
                    BMCWEB_LOG_ERROR << "DBUS response error: " << ec;
                    messages::internalError(asyncResp->res);
                    return;
                }
                std::string service = resp.begin()->first;
                BMCWEB_LOG_DEBUG << "getServiceName service: " << service;
                crow::connections::systemBus->async_method_call(
                    std::move(getCertificateList), service, ldapObjectPath,
                    dbusObjManagerIntf, "GetManagedObjects");
            };
        crow::connections::systemBus->async_method_call(
            std::move(getServiceName), mapperBusName, mapperObjectPath,
            mapperIntf, "GetObject", ldapObjectPath,
            std::array<std::string, 0>());
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
                std::remove(filepath.c_str());
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "DBUS response error: " << ec;
                    messages::internalError(asyncResp->res);
                    return;
                }
                messages::success(asyncResp->res);
            };

        auto getServiceName = [asyncResp, installCertificate,
                               filepath](const boost::system::error_code ec,
                                         const GetObjectType &resp) {
            if (ec || resp.empty())
            {
                BMCWEB_LOG_ERROR << "DBUS response error: " << ec;
                messages::internalError(asyncResp->res);
                return;
            }
            std::string service = resp.begin()->first;
            BMCWEB_LOG_DEBUG << "getServiceName service: " << service;
            crow::connections::systemBus->async_method_call(
                std::move(installCertificate), service, ldapObjectPath,
                certInstallIntf, "Install", filepath);
        };
        crow::connections::systemBus->async_method_call(
            std::move(getServiceName), mapperBusName, mapperObjectPath,
            mapperIntf, "GetObject", ldapObjectPath,
            std::array<std::string, 0>());
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
        const std::string &certId = params[0];
        BMCWEB_LOG_DEBUG << "LDAP Certificate ID=" << certId;
        res.jsonValue = {
            {"@odata.id",
             "/redfish/v1/AccountService/LDAP/Certificates/" + certId},
            {"@odata.type", "#Certificate.v1_0_0.Certificate"},
            {"@odata.context", "/redfish/v1/$metadata#Certificate.Certificate"},
            {"Id", certId},
            {"Name", "LDAP Certificate"},
            {"Description", "LDAP Certificate"},
            {"CertificateType", "PEM"}};
        auto asyncResp = std::make_shared<AsyncResp>(res);
        std::string path = std::string(ldapObjectPath) + "/" + certId;
        getCertificateProperties(asyncResp, path);
    }
}; // LDAPCertificate

} // namespace redfish
