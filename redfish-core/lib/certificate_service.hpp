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
        res.jsonValue["Actions"]["#CertificateService.GenerateCSR"] = {
            {"target", "/redfish/v1/CertificateService/Actions/"
                       "CertificateService.GenerateCSR"}};
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

static std::unique_ptr<sdbusplus::bus::match::match> csrMatcher;
/**
 * @brief Read data from CSR D-bus object and set to response
 *
 * @param[in] asyncResp Shared pointer to the response message
 * @param[in] certURI Link to certifiate collection URI
 * @param[in] service D-Bus service name
 * @param[in] certObjPath certificate D-Bus object path
 * @param[in] csrObjPath CSR D-Bus object path
 * @return None
 */
static void getCSR(const std::shared_ptr<AsyncResp> &asyncResp,
                   const std::string &certURI, const std::string &service,
                   const std::string &certObjPath,
                   const std::string &csrObjPath)
{
    BMCWEB_LOG_DEBUG << "getCSR CertObjectPath" << certObjPath
                     << " CSRObjectPath=" << csrObjPath
                     << " service=" << service;
    crow::connections::systemBus->async_method_call(
        [asyncResp, certURI](const boost::system::error_code ec,
                             const std::string &csr) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "DBUS response error: " << ec;
                messages::internalError(asyncResp->res);
                return;
            }
            if (csr.empty())
            {
                BMCWEB_LOG_ERROR << "CSR read is empty";
                messages::internalError(asyncResp->res);
                return;
            }
            asyncResp->res.jsonValue["CSRString"] = csr;
            asyncResp->res.jsonValue["CertificateCollection"] = {
                {"@odata.id", certURI}};
        },
        service, csrObjPath, "xyz.openbmc_project.Certs.CSR", "CSR");
}

/**
 * Action to Generate CSR
 */
class CertificateActionGenerateCSR : public Node
{
  public:
    CertificateActionGenerateCSR(CrowApp &app) :
        Node(app, "/redfish/v1/CertificateService/Actions/"
                  "CertificateService.GenerateCSR/")
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
        auto asyncResp = std::make_shared<AsyncResp>(res);
        // Required parameters
        std::optional<std::string> city;
        std::optional<std::string> commonName;
        std::optional<std::string> country;
        std::optional<std::string> organization;
        std::optional<std::string> organizationalUnit;
        std::optional<std::string> state;
        std::optional<nlohmann::json> certificateCollection;

        // Optional parameters
        std::optional<std::vector<std::string>> optAlternativeNames =
            std::vector<std::string>();
        std::optional<std::string> optContactPerson = "";
        std::optional<std::string> optChallengePassword = "";
        std::optional<std::string> optEmail = "";
        std::optional<std::string> optGivenName = "";
        std::optional<std::string> optInitials = "";
        std::optional<int64_t> optKeyBitLength = 2048;
        std::optional<std::string> optKeyCurveId = "";
        std::optional<std::string> optKeyPairAlgorithm = "RSA";
        std::optional<std::vector<std::string>> optKeyUsage =
            std::vector<std::string>();
        std::optional<std::string> optSurname = "";
        std::optional<std::string> optUnstructuredName = "";
        if (!json_util::readJson(
                req, res, "City", city, "CommonName", commonName,
                "ContactPerson", optContactPerson, "Country", country,
                "Organization", organization, "OrganizationalUnit",
                organizationalUnit, "State", state, "CertificateCollection",
                certificateCollection, "AlternativeNames", optAlternativeNames,
                "ChallengePassword", optChallengePassword, "Email", optEmail,
                "GivenName", optGivenName, "Initials", optInitials,
                "KeyBitLength", optKeyBitLength, "KeyCurveId", optKeyCurveId,
                "KeyPairAlgorithm", optKeyPairAlgorithm, "KeyUsage",
                optKeyUsage, "Surname", optSurname, "UnstructuredName",
                optUnstructuredName))
        {
            BMCWEB_LOG_ERROR << "Failure to read required parameters";
            messages::internalError(asyncResp->res);
            return;
        }
        if (!city)
        {
            messages::actionParameterMissing(asyncResp->res, "GenerateCSR",
                                             "City");
            return;
        }
        if (!commonName)
        {
            messages::actionParameterMissing(asyncResp->res, "GenerateCSR",
                                             "CommonName");
            return;
        }
        if (!country)
        {
            messages::actionParameterMissing(asyncResp->res, "GenerateCSR",
                                             "Country");
            return;
        }
        if (!organization)
        {
            messages::actionParameterMissing(asyncResp->res, "GenerateCSR",
                                             "Organization");
            return;
        }
        if (!organizationalUnit)
        {
            messages::actionParameterMissing(asyncResp->res, "GenerateCSR",
                                             "OrganizationalUnit");
            return;
        }
        if (!state)
        {
            messages::actionParameterMissing(asyncResp->res, "GenerateCSR",
                                             "State");
            return;
        }
        if (!certificateCollection)
        {
            messages::actionParameterMissing(asyncResp->res, "GenerateCSR",
                                             "CertificateCollection");
            return;
        }
        std::string certURI;
        if (!redfish::json_util::readJson(*certificateCollection, res,
                                          "@odata.id", certURI))
        {
            messages::actionParameterMissing(asyncResp->res, "GenerateCSR",
                                             "CertificateCollection");
            return;
        }
        BMCWEB_LOG_DEBUG << "Certificate URI value received is" << certURI;
        std::string objectPath;
        std::string service;
        if (certURI ==
            "/redfish/v1/Managers/bmc/NetworkProtocol/HTTPS/Certificates/")
        {
            objectPath = certs::httpsObjectPath;
            service = certs::httpsServiceName;
        }
        else
        {
            messages::actionParameterValueFormatError(asyncResp->res, certURI,
                                                      "CertificateCollection",
                                                      "GenerateCSR");
            return;
        }
        // if not default value validate the input value
        if (*optKeyPairAlgorithm != "")
        {
            if (*optKeyPairAlgorithm != "RSA" && *optKeyPairAlgorithm != "EC")
            {
                messages::actionParameterNotSupported(
                    asyncResp->res, "KeyPairAlgorithm", "GenerateCSR");
                return;
            }
        }

        // validate KeyUsage
        for (const std::string &usage : *optKeyUsage)
        {
            if (!isKeyUsageFound(usage))
            {
                messages::actionParameterValueFormatError(
                    asyncResp->res, usage, "KeyUsage", "GenerateCSR");
                return;
            }
        }

        // Only allow one CSR matcher at a time so setting retry time-out and
        // timer expiry to 60 seconds for now. D-Bus time-out is 60 seconds.
        if (csrMatcher)
        {
            res.addHeader("Retry-After", "60");
            messages::serviceTemporarilyUnavailable(asyncResp->res, "60");
            return;
        }

        // Make this static so it survives outside this method
        static boost::asio::steady_timer timeout(*req.ioService);
        timeout.expires_after(std::chrono::seconds(60));
        timeout.async_wait([asyncResp](const boost::system::error_code &ec) {
            csrMatcher = nullptr;
            if (ec)
            {
                // operation_aborted is expected if timer is canceled before
                // completion.
                if (ec != boost::asio::error::operation_aborted)
                {
                    BMCWEB_LOG_ERROR << "Async_wait failed " << ec;
                }
                return;
            }
            BMCWEB_LOG_ERROR << "Timed out waiting for Generating CSR";
            messages::internalError(asyncResp->res);
        });

        // create a matcher to wait on CSR object
        BMCWEB_LOG_DEBUG << "create matcher with path " << objectPath;
        std::string match("type='signal',"
                          "interface='org.freedesktop.DBus.ObjectManager',"
                          "path='" +
                          objectPath +
                          "',"
                          "member='InterfacesAdded'");
        csrMatcher = std::make_unique<sdbusplus::bus::match::match>(
            *crow::connections::systemBus, match,
            [asyncResp, service, objectPath,
             certURI](sdbusplus::message::message &m) {
                boost::system::error_code ec;
                timeout.cancel(ec);
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "error canceling timer " << ec;
                }
                if (m.is_method_error())
                {
                    BMCWEB_LOG_DEBUG << "Dbus method error!!!";
                    messages::internalError(asyncResp->res);
                    return;
                }
                std::vector<std::pair<
                    std::string, std::vector<std::pair<
                                     std::string, std::variant<std::string>>>>>
                    interfacesProperties;
                sdbusplus::message::object_path csrObjectPath;
                m.read(csrObjectPath, interfacesProperties);
                BMCWEB_LOG_DEBUG << "CSR object added" << csrObjectPath.str;
                for (auto &interface : interfacesProperties)
                {
                    if (interface.first == "xyz.openbmc_project.Certs.CSR")
                    {
                        getCSR(asyncResp, certURI, service, objectPath,
                               csrObjectPath.str);
                        break;
                    }
                }
            });
        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec,
                        const std::string &path) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "DBUS response error: " << ec.message();
                    messages::internalError(asyncResp->res);
                    return;
                }
            },
            service, objectPath, "xyz.openbmc_project.Certs.CSR.Create",
            "GenerateCSR", *optAlternativeNames, *optChallengePassword, *city,
            *commonName, *optContactPerson, *country, *optEmail, *optGivenName,
            *optInitials, *optKeyBitLength, *optKeyCurveId,
            *optKeyPairAlgorithm, *optKeyUsage, *organization,
            *organizationalUnit, *state, *optSurname, *optUnstructuredName);
    }

    /**
     * @brief Check if keyusage retrieved from Certificate is of redfish
     * supported type
     *
     * @param[in] str keyusage value retrieved from certificate
     * @return true if it is of redfish type else false
     */
    bool isKeyUsageFound(const std::string &str)
    {
        std::array<const char *, 15> usageList = {
            "DigitalSignature",     "NonRepudiation",       "KeyEncipherment",
            "DataEncipherment",     "KeyAgreement",         "KeyCertSign",
            "CRLSigning",           "EncipherOnly",         "DecipherOnly",
            "ServerAuthentication", "ClientAuthentication", "CodeSigning",
            "EmailProtection",      "Timestamping",         "OCSPSigning"};
        auto it = std::find_if(usageList.begin(), usageList.end(),
                               [&str](const char *s) {
                                   if (strcmp(s, str.c_str()) == 0)
                                   {
                                       return true;
                                   }
                                   return false;
                               });
        if (it != usageList.end())
        {
            return true;
        }
        return false;
    }
}; // CertificateActionGenerateCSR

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
