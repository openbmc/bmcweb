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
namespace certs
{
constexpr char const *httpsObjectPath =
    "/xyz/openbmc_project/certs/server/https";
constexpr char const *certInstallIntf = "xyz.openbmc_project.Certs.Install";
constexpr char const *certReplaceIntf = "xyz.openbmc_project.Certs.Replace";
constexpr char const *certPropIntf = "xyz.openbmc_project.Certs.Certificate";
constexpr char const *dbusPropIntf = "org.freedesktop.DBus.Properties";
constexpr char const *dbusObjManagerIntf = "org.freedesktop.DBus.ObjectManager";
constexpr char const *mapperBusName = "xyz.openbmc_project.ObjectMapper";
constexpr char const *mapperObjectPath = "/xyz/openbmc_project/object_mapper";
constexpr char const *mapperIntf = "xyz.openbmc_project.ObjectMapper";
constexpr char const *ldapObjectPath = "/xyz/openbmc_project/certs/client/ldap";
constexpr char const *httpsServiceName =
    "xyz.openbmc_project.Certs.Manager.Server.Https";
constexpr char const *ldapServiceName =
    "xyz.openbmc_project.Certs.Manager.Client.Ldap";
} // namespace certs

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
        res.jsonValue["Actions"]["#CertificateService.ReplaceCertificate"] = {
            {"target", "/redfish/v1/CertificateService/Actions/"
                       "CertificateService.ReplaceCertificate"},
            {"CertificateType@Redfish.AllowableValues", {"PEM"}}};
        res.end();
    }
}; // CertificateService

/**
 * @brief Find the ID specified in the URL
 * Finds the numbers specified after the last "/" in the URL and returns.
 * @param[in] path URL
 * @return -1 on failure and number on success
 */
long getIDFromURL(const std::string_view url)
{
    std::size_t found = url.rfind("/");
    if (found == std::string::npos)
    {
        return -1;
    }
    if ((found + 1) < url.length())
    {
        char *endPtr;
        std::string_view str = url.substr(found + 1);
        long value = std::strtol(str.data(), &endPtr, 10);
        if (endPtr != str.end())
        {
            return -1;
        }
        return value;
    }
    return -1;
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
        char *tempDirectory = mkdtemp(dirTemplate);
        if (tempDirectory)
        {
            certDirectory = tempDirectory;
            certificateFile = certDirectory / "cert.pem";
            std::ofstream out(certificateFile, std::ofstream::out |
                                                   std::ofstream::binary |
                                                   std::ofstream::trunc);
            out << certString;
            out.close();
            BMCWEB_LOG_DEBUG << "Creating certificate file" << certificateFile;
        }
    }
    ~CertificateFile()
    {
        if (std::filesystem::exists(certDirectory))
        {
            BMCWEB_LOG_DEBUG << "Removing certificate file" << certificateFile;
            try
            {
                std::filesystem::remove_all(certDirectory);
            }
            catch (const std::filesystem::filesystem_error &e)
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
    std::filesystem::path certificateFile;
    std::filesystem::path certDirectory;
};

/**
 * @brief Parse and update Certficate Issue/Subject property
 *
 * @param[in] asyncResp Shared pointer to the response message
 * @param[in] str  Issuer/Subject value in key=value pairs
 * @param[in] type Issuer/Subject
 * @return None
 */
static void updateCertIssuerOrSubject(nlohmann::json &out,
                                      const std::string_view value)
{
    // example: O=openbmc-project.xyz,CN=localhost
    std::string_view::iterator i = value.begin();
    while (i != value.end())
    {
        std::string_view::iterator tokenBegin = i;
        while (i != value.end() && *i != '=')
        {
            i++;
        }
        if (i == value.end())
        {
            break;
        }
        const std::string_view key(tokenBegin, i - tokenBegin);
        i++;
        tokenBegin = i;
        while (i != value.end() && *i != ',')
        {
            i++;
        }
        const std::string_view val(tokenBegin, i - tokenBegin);
        if (key == "L")
        {
            out["City"] = val;
        }
        else if (key == "CN")
        {
            out["CommonName"] = val;
        }
        else if (key == "C")
        {
            out["Country"] = val;
        }
        else if (key == "O")
        {
            out["Organization"] = val;
        }
        else if (key == "OU")
        {
            out["OrganizationalUnit"] = val;
        }
        else if (key == "ST")
        {
            out["State"] = val;
        }
        // skip comma character
        if (i != value.end())
        {
            i++;
        }
    }
}

/**
 * @brief Retrieve the certificates properties and append to the response
 * message
 *
 * @param[in] asyncResp Shared pointer to the response message
 * @param[in] objectPath  Path of the D-Bus service object
 * @param[in] certId  Id of the certificate
 * @param[in] certURL  URL of the certificate object
 * @param[in] name  name of the certificate
 * @return None
 */
static void getCertificateProperties(
    const std::shared_ptr<AsyncResp> &asyncResp, const std::string &objectPath,
    const std::string &service, long certId, const std::string &certURL,
    const std::string &name)
{
    using PropertyType =
        std::variant<std::string, uint64_t, std::vector<std::string>>;
    using PropertiesMap = boost::container::flat_map<std::string, PropertyType>;
    BMCWEB_LOG_DEBUG << "getCertificateProperties Path=" << objectPath
                     << " certId=" << certId << " certURl=" << certURL;
    crow::connections::systemBus->async_method_call(
        [asyncResp, certURL, certId, name](const boost::system::error_code ec,
                                           const PropertiesMap &properties) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "DBUS response error: " << ec;
                messages::internalError(asyncResp->res);
                return;
            }
            asyncResp->res.jsonValue = {
                {"@odata.id", certURL},
                {"@odata.type", "#Certificate.v1_0_0.Certificate"},
                {"@odata.context",
                 "/redfish/v1/$metadata#Certificate.Certificate"},
                {"Id", std::to_string(certId)},
                {"Name", name},
                {"Description", name}};
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
                    nlohmann::json &keyUsage =
                        asyncResp->res.jsonValue["KeyUsage"];
                    keyUsage = nlohmann::json::array();
                    const std::vector<std::string> *value =
                        std::get_if<std::vector<std::string>>(&property.second);
                    if (value)
                    {
                        for (const std::string &usage : *value)
                        {
                            keyUsage.push_back(usage);
                        }
                    }
                }
                else if (property.first == "Issuer")
                {
                    const std::string *value =
                        std::get_if<std::string>(&property.second);
                    if (value)
                    {
                        updateCertIssuerOrSubject(
                            asyncResp->res.jsonValue["Issuer"], *value);
                    }
                }
                else if (property.first == "Subject")
                {
                    const std::string *value =
                        std::get_if<std::string>(&property.second);
                    if (value)
                    {
                        updateCertIssuerOrSubject(
                            asyncResp->res.jsonValue["Subject"], *value);
                    }
                }
                else if (property.first == "ValidNotAfter")
                {
                    const uint64_t *value =
                        std::get_if<uint64_t>(&property.second);
                    if (value)
                    {
                        std::time_t time = static_cast<std::time_t>(*value);
                        asyncResp->res.jsonValue["ValidNotAfter"] =
                            crow::utility::getDateTime(time);
                    }
                }
                else if (property.first == "ValidNotBefore")
                {
                    const uint64_t *value =
                        std::get_if<uint64_t>(&property.second);
                    if (value)
                    {
                        std::time_t time = static_cast<std::time_t>(*value);
                        asyncResp->res.jsonValue["ValidNotBefore"] =
                            crow::utility::getDateTime(time);
                    }
                }
            }
            asyncResp->res.addHeader("Location", certURL);
        },
        service, objectPath, certs::dbusPropIntf, "GetAll",
        certs::certPropIntf);
}

using GetObjectType =
    std::vector<std::pair<std::string, std::vector<std::string>>>;

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
            {boost::beast::http::verb::patch, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::put, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::post, {{"ConfigureComponents"}}}};
    }

  private:
    void doPost(crow::Response &res, const crow::Request &req,
                const std::vector<std::string> &params) override
    {
        std::string certificate;
        nlohmann::json certificateUri;
        std::optional<std::string> certificateType = "PEM";
        auto asyncResp = std::make_shared<AsyncResp>(res);
        if (!json_util::readJson(req, asyncResp->res, "CertificateString",
                                 certificate, "CertificateUri", certificateUri,
                                 "CertificateType", certificateType))
        {
            BMCWEB_LOG_ERROR << "Required parameters are missing";
            messages::internalError(asyncResp->res);
            return;
        }

        if (!certificateType)
        {
            // should never happen, but it never hurts to be paranoid.
            return;
        }
        if (certificateType != "PEM")
        {
            messages::actionParameterNotSupported(
                asyncResp->res, "CertificateType", "ReplaceCertificate");
            return;
        }

        std::string certURI;
        if (!redfish::json_util::readJson(certificateUri, asyncResp->res,
                                          "@odata.id", certURI))
        {
            messages::actionParameterMissing(
                asyncResp->res, "ReplaceCertificate", "CertificateUri");
            return;
        }

        BMCWEB_LOG_INFO << "Certificate URI to replace" << certURI;
        long id = getIDFromURL(certURI);
        if (id < 0)
        {
            messages::actionParameterValueFormatError(asyncResp->res, certURI,
                                                      "CertificateUri",
                                                      "ReplaceCertificate");
            return;
        }
        std::string objectPath;
        std::string name;
        std::string service;
        if (boost::starts_with(
                certURI,
                "/redfish/v1/Managers/bmc/NetworkProtocol/HTTPS/Certificates/"))
        {
            objectPath =
                std::string(certs::httpsObjectPath) + "/" + std::to_string(id);
            name = "HTTPS certificate";
            service = certs::httpsServiceName;
        }
        else if (boost::starts_with(
                     certURI, "/redfish/v1/AccountService/LDAP/Certificates/"))
        {
            objectPath =
                std::string(certs::ldapObjectPath) + "/" + std::to_string(id);
            name = "LDAP certificate";
            service = certs::ldapServiceName;
        }
        else
        {
            messages::actionParameterNotSupported(
                asyncResp->res, "CertificateUri", "ReplaceCertificate");
            return;
        }

        std::shared_ptr<CertificateFile> certFile =
            std::make_shared<CertificateFile>(certificate);
        crow::connections::systemBus->async_method_call(
            [asyncResp, certFile, objectPath, service, certURI, id,
             name](const boost::system::error_code ec) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "DBUS response error: " << ec;
                    messages::internalError(asyncResp->res);
                    return;
                }
                getCertificateProperties(asyncResp, objectPath, service, id,
                                         certURI, name);
                BMCWEB_LOG_DEBUG << "HTTPS certificate install file="
                                 << certFile->getCertFilePath();
            },
            service, objectPath, certs::certReplaceIntf, "Replace",
            certFile->getCertFilePath());
    }
}; // CertificateActionsReplaceCertificate

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
        auto asyncResp = std::make_shared<AsyncResp>(res);
        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);
            return;
        }
        long id = getIDFromURL(req.url);

        BMCWEB_LOG_DEBUG << "HTTPSCertificate::doGet ID=" << std::to_string(id);
        std::string certURL =
            "/redfish/v1/Managers/bmc/NetworkProtocol/HTTPS/Certificates/" +
            std::to_string(id);
        std::string objectPath = certs::httpsObjectPath;
        objectPath += "/";
        objectPath += std::to_string(id);
        getCertificateProperties(asyncResp, objectPath, certs::httpsServiceName,
                                 id, certURL, "HTTPS Certificate");
    }

}; // namespace redfish

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
        crow::connections::systemBus->async_method_call(
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
                    long id = getIDFromURL(cert.first.str);
                    if (id >= 0)
                    {
                        members.push_back(
                            {{"@odata.id",
                              "/redfish/v1/Managers/bmc/"
                              "NetworkProtocol/HTTPS/Certificates/" +
                                  std::to_string(id)}});
                    }
                }
                asyncResp->res.jsonValue["Members@odata.count"] =
                    members.size();
            },
            certs::httpsServiceName, certs::httpsObjectPath,
            certs::dbusObjManagerIntf, "GetManagedObjects");
    }

    void doPost(crow::Response &res, const crow::Request &req,
                const std::vector<std::string> &params) override
    {
        BMCWEB_LOG_DEBUG << "HTTPSCertificateCollection::doPost";
        auto asyncResp = std::make_shared<AsyncResp>(res);
        asyncResp->res.jsonValue = {{"Name", "HTTPS Certificate"},
                                    {"Description", "HTTPS Certificate"}};

        std::shared_ptr<CertificateFile> certFile =
            std::make_shared<CertificateFile>(req.body);

        crow::connections::systemBus->async_method_call(
            [asyncResp, certFile](const boost::system::error_code ec) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "DBUS response error: " << ec;
                    messages::internalError(asyncResp->res);
                    return;
                }
                // TODO: Issue#84 supporting only 1 certificate
                long certId = 1;
                std::string certURL =
                    "/redfish/v1/Managers/bmc/NetworkProtocol/HTTPS/"
                    "Certificates/" +
                    std::to_string(certId);
                std::string objectPath = std::string(certs::httpsObjectPath) +
                                         "/" + std::to_string(certId);
                getCertificateProperties(asyncResp, objectPath,
                                         certs::httpsServiceName, certId,
                                         certURL, "HTTPS Certificate");
                BMCWEB_LOG_DEBUG << "HTTPS certificate install file="
                                 << certFile->getCertFilePath();
            },
            certs::httpsServiceName, certs::httpsObjectPath,
            certs::certInstallIntf, "Install", certFile->getCertFilePath());
    }
}; // HTTPSCertificateCollection

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
             "Defines a resource that an administrator can use in order to "
             "locate all certificates installed on a given service"}};
        auto asyncResp = std::make_shared<AsyncResp>(res);
        nlohmann::json &links =
            asyncResp->res.jsonValue["Links"]["Certificates"];
        links = nlohmann::json::array();
        getCertificateLocations(
            asyncResp,
            "/redfish/v1/Managers/bmc/NetworkProtocol/HTTPS/Certificates/",
            certs::httpsObjectPath, certs::httpsServiceName);
        getCertificateLocations(asyncResp,
                                "/redfish/v1/AccountService/LDAP/Certificates/",
                                certs::ldapObjectPath, certs::ldapServiceName);
    }
    /**
     * @brief Retrieve the certificates installed list and append to the
     * response
     *
     * @param[in] asyncResp Shared pointer to the response message
     * @param[in] certURL  Path of the certificate object
     * @param[in] path  Path of the D-Bus service object
     * @return None
     */
    void getCertificateLocations(std::shared_ptr<AsyncResp> &asyncResp,
                                 const std::string &certURL,
                                 const std::string &path,
                                 const std::string &service)
    {
        BMCWEB_LOG_DEBUG << "getCertificateLocations URI=" << certURL
                         << " Path=" << path << " service= " << service;
        crow::connections::systemBus->async_method_call(
            [asyncResp, certURL](const boost::system::error_code ec,
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
                    long id = getIDFromURL(cert.first.str);
                    if (id >= 0)
                    {
                        links.push_back(
                            {{"@odata.id", certURL + std::to_string(id)}});
                    }
                }
                asyncResp->res.jsonValue["Links"]["Certificates@odata.count"] =
                    links.size();
            },
            service, path, certs::dbusObjManagerIntf, "GetManagedObjects");
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
        crow::connections::systemBus->async_method_call(
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
                    long id = getIDFromURL(cert.first.str);
                    if (id >= 0)
                    {
                        members.push_back(
                            {{"@odata.id", "/redfish/v1/AccountService/"
                                           "LDAP/Certificates/" +
                                               std::to_string(id)}});
                    }
                }
                asyncResp->res.jsonValue["Members@odata.count"] =
                    members.size();
            },
            certs::ldapServiceName, certs::ldapObjectPath,
            certs::dbusObjManagerIntf, "GetManagedObjects");
    }

    void doPost(crow::Response &res, const crow::Request &req,
                const std::vector<std::string> &params) override
    {
        std::shared_ptr<CertificateFile> certFile =
            std::make_shared<CertificateFile>(req.body);
        auto asyncResp = std::make_shared<AsyncResp>(res);
        crow::connections::systemBus->async_method_call(
            [asyncResp, certFile](const boost::system::error_code ec) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "DBUS response error: " << ec;
                    messages::internalError(asyncResp->res);
                    return;
                }
                //// TODO: Issue#84 supporting only 1 certificate
                long certId = 1;
                std::string certURL =
                    "/redfish/v1/AccountService/LDAP/Certificates/" +
                    std::to_string(certId);
                std::string objectPath = std::string(certs::ldapObjectPath) +
                                         "/" + std::to_string(certId);
                getCertificateProperties(asyncResp, objectPath,
                                         certs::ldapServiceName, certId,
                                         certURL, "LDAP Certificate");
                BMCWEB_LOG_DEBUG << "LDAP certificate install file="
                                 << certFile->getCertFilePath();
            },
            certs::ldapServiceName, certs::ldapObjectPath,
            certs::certInstallIntf, "Install", certFile->getCertFilePath());
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
        auto asyncResp = std::make_shared<AsyncResp>(res);
        long id = getIDFromURL(req.url);
        if (id < 0)
        {
            BMCWEB_LOG_ERROR << "Invalid url value" << req.url;
            messages::internalError(asyncResp->res);
            return;
        }
        BMCWEB_LOG_DEBUG << "LDAP Certificate ID=" << std::to_string(id);
        std::string certURL = "/redfish/v1/AccountService/LDAP/Certificates/" +
                              std::to_string(id);
        std::string objectPath = certs::ldapObjectPath;
        objectPath += "/";
        objectPath += std::to_string(id);
        getCertificateProperties(asyncResp, objectPath, certs::ldapServiceName,
                                 id, certURL, "LDAP Certificate");
    }
}; // LDAPCertificate
} // namespace redfish
