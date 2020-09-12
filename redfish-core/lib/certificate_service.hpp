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

#include <boost/convert.hpp>
#include <boost/convert/strtol.hpp>

#include <variant>
namespace redfish
{
namespace certs
{
constexpr char const* httpsObjectPath =
    "/xyz/openbmc_project/certs/server/https";
constexpr char const* certInstallIntf = "xyz.openbmc_project.Certs.Install";
constexpr char const* certReplaceIntf = "xyz.openbmc_project.Certs.Replace";
constexpr char const* objDeleteIntf = "xyz.openbmc_project.Object.Delete";
constexpr char const* certPropIntf = "xyz.openbmc_project.Certs.Certificate";
constexpr char const* dbusPropIntf = "org.freedesktop.DBus.Properties";
constexpr char const* dbusObjManagerIntf = "org.freedesktop.DBus.ObjectManager";
constexpr char const* ldapObjectPath = "/xyz/openbmc_project/certs/client/ldap";
constexpr char const* httpsServiceName =
    "xyz.openbmc_project.Certs.Manager.Server.Https";
constexpr char const* ldapServiceName =
    "xyz.openbmc_project.Certs.Manager.Client.Ldap";
constexpr char const* authorityServiceName =
    "xyz.openbmc_project.Certs.Manager.Authority.Ldap";
constexpr char const* authorityObjectPath =
    "/xyz/openbmc_project/certs/authority/ldap";
} // namespace certs

/**
 * The Certificate schema defines a Certificate Service which represents the
 * actions available to manage certificates and links to where certificates
 * are installed.
 */
class CertificateService : public Node
{
  public:
    CertificateService(App& app) : Node(app, "/redfish/v1/CertificateService/")
    {
        // TODO: Issue#61 No entries are available for Certificate
        // service at https://www.dmtf.org/standards/redfish
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
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>&) override
    {
        res.jsonValue = {
            {"@odata.type", "#CertificateService.v1_0_0.CertificateService"},
            {"@odata.id", "/redfish/v1/CertificateService"},
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
inline long getIDFromURL(const std::string_view url)
{
    std::size_t found = url.rfind('/');
    if (found == std::string::npos)
    {
        return -1;
    }

    if ((found + 1) < url.length())
    {
        std::string_view str = url.substr(found + 1);

        return boost::convert<long>(str, boost::cnv::strtol()).value_or(-1);
    }

    return -1;
}

inline std::string
    getCertificateFromReqBody(const std::shared_ptr<AsyncResp>& asyncResp,
                              const crow::Request& req)
{
    nlohmann::json reqJson = nlohmann::json::parse(req.body, nullptr, false);

    if (reqJson.is_discarded())
    {
        // We did not receive JSON request, proceed as it is RAW data
        return req.body;
    }

    std::string certificate;
    std::optional<std::string> certificateType = "PEM";

    if (!json_util::readJson(reqJson, asyncResp->res, "CertificateString",
                             certificate, "CertificateType", certificateType))
    {
        BMCWEB_LOG_ERROR << "Required parameters are missing";
        messages::internalError(asyncResp->res);
        return std::string();
    }

    if (*certificateType != "PEM")
    {
        messages::propertyValueNotInList(asyncResp->res, *certificateType,
                                         "CertificateType");
        return std::string();
    }

    return certificate;
}

/**
 * Class to create a temporary certificate file for uploading to system
 */
class CertificateFile
{
  public:
    CertificateFile() = delete;
    CertificateFile(const CertificateFile&) = delete;
    CertificateFile& operator=(const CertificateFile&) = delete;
    CertificateFile(CertificateFile&&) = delete;
    CertificateFile& operator=(CertificateFile&&) = delete;
    CertificateFile(const std::string& certString)
    {
        std::array<char, 18> dirTemplate = {'/', 't', 'm', 'p', '/', 'C',
                                            'e', 'r', 't', 's', '.', 'X',
                                            'X', 'X', 'X', 'X', 'X', '\0'};
        char* tempDirectory = mkdtemp(dirTemplate.data());
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
            std::error_code ec;
            std::filesystem::remove_all(certDirectory, ec);
            if (ec)
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
static void getCSR(const std::shared_ptr<AsyncResp>& asyncResp,
                   const std::string& certURI, const std::string& service,
                   const std::string& certObjPath,
                   const std::string& csrObjPath)
{
    BMCWEB_LOG_DEBUG << "getCSR CertObjectPath" << certObjPath
                     << " CSRObjectPath=" << csrObjPath
                     << " service=" << service;
    crow::connections::systemBus->async_method_call(
        [asyncResp, certURI](const boost::system::error_code ec,
                             const std::string& csr) {
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
    CertificateActionGenerateCSR(App& app) :
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
    void doPost(crow::Response& res, const crow::Request& req,
                const std::vector<std::string>&) override
    {
        static const int rsaKeyBitLength = 2048;
        auto asyncResp = std::make_shared<AsyncResp>(res);
        // Required parameters
        std::string city;
        std::string commonName;
        std::string country;
        std::string organization;
        std::string organizationalUnit;
        std::string state;
        nlohmann::json certificateCollection;

        // Optional parameters
        std::optional<std::vector<std::string>> optAlternativeNames =
            std::vector<std::string>();
        std::optional<std::string> optContactPerson = "";
        std::optional<std::string> optChallengePassword = "";
        std::optional<std::string> optEmail = "";
        std::optional<std::string> optGivenName = "";
        std::optional<std::string> optInitials = "";
        std::optional<int64_t> optKeyBitLength = rsaKeyBitLength;
        std::optional<std::string> optKeyCurveId = "secp384r1";
        std::optional<std::string> optKeyPairAlgorithm = "EC";
        std::optional<std::vector<std::string>> optKeyUsage =
            std::vector<std::string>();
        std::optional<std::string> optSurname = "";
        std::optional<std::string> optUnstructuredName = "";
        if (!json_util::readJson(
                req, asyncResp->res, "City", city, "CommonName", commonName,
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
            return;
        }

        // bmcweb has no way to store or decode a private key challenge
        // password, which will likely cause bmcweb to crash on startup if this
        // is not set on a post so not allowing the user to set value
        if (*optChallengePassword != "")
        {
            messages::actionParameterNotSupported(asyncResp->res, "GenerateCSR",
                                                  "ChallengePassword");
            return;
        }

        std::string certURI;
        if (!redfish::json_util::readJson(certificateCollection, asyncResp->res,
                                          "@odata.id", certURI))
        {
            return;
        }

        std::string objectPath;
        std::string service;
        if (boost::starts_with(
                certURI,
                "/redfish/v1/Managers/bmc/NetworkProtocol/HTTPS/Certificates"))
        {
            objectPath = certs::httpsObjectPath;
            service = certs::httpsServiceName;
        }
        else if (boost::starts_with(
                     certURI, "/redfish/v1/AccountService/LDAP/Certificates"))
        {
            objectPath = certs::ldapObjectPath;
            service = certs::ldapServiceName;
        }
        else
        {
            messages::actionParameterNotSupported(
                asyncResp->res, "CertificateCollection", "GenerateCSR");
            return;
        }

        // supporting only EC and RSA algorithm
        if (*optKeyPairAlgorithm != "EC" && *optKeyPairAlgorithm != "RSA")
        {
            messages::actionParameterNotSupported(
                asyncResp->res, "KeyPairAlgorithm", "GenerateCSR");
            return;
        }

        // supporting only 2048 key bit length for RSA algorithm due to time
        // consumed in generating private key
        if (*optKeyPairAlgorithm == "RSA" &&
            *optKeyBitLength != rsaKeyBitLength)
        {
            messages::propertyValueNotInList(asyncResp->res,
                                             std::to_string(*optKeyBitLength),
                                             "KeyBitLength");
            return;
        }

        // validate KeyUsage supporting only 1 type based on URL
        if (boost::starts_with(
                certURI,
                "/redfish/v1/Managers/bmc/NetworkProtocol/HTTPS/Certificates"))
        {
            if (optKeyUsage->size() == 0)
            {
                optKeyUsage->push_back("ServerAuthentication");
            }
            else if (optKeyUsage->size() == 1)
            {
                if ((*optKeyUsage)[0] != "ServerAuthentication")
                {
                    messages::propertyValueNotInList(
                        asyncResp->res, (*optKeyUsage)[0], "KeyUsage");
                    return;
                }
            }
            else
            {
                messages::actionParameterNotSupported(
                    asyncResp->res, "KeyUsage", "GenerateCSR");
                return;
            }
        }
        else if (boost::starts_with(
                     certURI, "/redfish/v1/AccountService/LDAP/Certificates"))
        {
            if (optKeyUsage->size() == 0)
            {
                optKeyUsage->push_back("ClientAuthentication");
            }
            else if (optKeyUsage->size() == 1)
            {
                if ((*optKeyUsage)[0] != "ClientAuthentication")
                {
                    messages::propertyValueNotInList(
                        asyncResp->res, (*optKeyUsage)[0], "KeyUsage");
                    return;
                }
            }
            else
            {
                messages::actionParameterNotSupported(
                    asyncResp->res, "KeyUsage", "GenerateCSR");
                return;
            }
        }

        // Only allow one CSR matcher at a time so setting retry time-out and
        // timer expiry to 10 seconds for now.
        static const int timeOut = 10;
        if (csrMatcher)
        {
            messages::serviceTemporarilyUnavailable(asyncResp->res,
                                                    std::to_string(timeOut));
            return;
        }

        // Make this static so it survives outside this method
        static boost::asio::steady_timer timeout(*req.ioService);
        timeout.expires_after(std::chrono::seconds(timeOut));
        timeout.async_wait([asyncResp](const boost::system::error_code& ec) {
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
             certURI](sdbusplus::message::message& m) {
                timeout.cancel();
                if (m.is_method_error())
                {
                    BMCWEB_LOG_ERROR << "Dbus method error!!!";
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
                for (auto& interface : interfacesProperties)
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
            [asyncResp](const boost::system::error_code& ec,
                        const std::string&) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "DBUS response error: " << ec.message();
                    messages::internalError(asyncResp->res);
                    return;
                }
            },
            service, objectPath, "xyz.openbmc_project.Certs.CSR.Create",
            "GenerateCSR", *optAlternativeNames, *optChallengePassword, city,
            commonName, *optContactPerson, country, *optEmail, *optGivenName,
            *optInitials, *optKeyBitLength, *optKeyCurveId,
            *optKeyPairAlgorithm, *optKeyUsage, organization,
            organizationalUnit, state, *optSurname, *optUnstructuredName);
    }
}; // CertificateActionGenerateCSR

/**
 * @brief Parse and update Certificate Issue/Subject property
 *
 * @param[in] asyncResp Shared pointer to the response message
 * @param[in] str  Issuer/Subject value in key=value pairs
 * @param[in] type Issuer/Subject
 * @return None
 */
static void updateCertIssuerOrSubject(nlohmann::json& out,
                                      const std::string_view value)
{
    // example: O=openbmc-project.xyz,CN=localhost
    std::string_view::iterator i = value.begin();
    while (i != value.end())
    {
        std::string_view::iterator tokenBegin = i;
        while (i != value.end() && *i != '=')
        {
            ++i;
        }
        if (i == value.end())
        {
            break;
        }
        const std::string_view key(tokenBegin,
                                   static_cast<size_t>(i - tokenBegin));
        ++i;
        tokenBegin = i;
        while (i != value.end() && *i != ',')
        {
            ++i;
        }
        const std::string_view val(tokenBegin,
                                   static_cast<size_t>(i - tokenBegin));
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
            ++i;
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
    const std::shared_ptr<AsyncResp>& asyncResp, const std::string& objectPath,
    const std::string& service, long certId, const std::string& certURL,
    const std::string& name)
{
    using PropertyType =
        std::variant<std::string, uint64_t, std::vector<std::string>>;
    using PropertiesMap = boost::container::flat_map<std::string, PropertyType>;
    BMCWEB_LOG_DEBUG << "getCertificateProperties Path=" << objectPath
                     << " certId=" << certId << " certURl=" << certURL;
    crow::connections::systemBus->async_method_call(
        [asyncResp, certURL, certId, name](const boost::system::error_code ec,
                                           const PropertiesMap& properties) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "DBUS response error: " << ec;
                messages::resourceNotFound(asyncResp->res, name,
                                           std::to_string(certId));
                return;
            }
            asyncResp->res.jsonValue = {
                {"@odata.id", certURL},
                {"@odata.type", "#Certificate.v1_0_0.Certificate"},
                {"Id", std::to_string(certId)},
                {"Name", name},
                {"Description", name}};
            for (const auto& property : properties)
            {
                if (property.first == "CertificateString")
                {
                    asyncResp->res.jsonValue["CertificateString"] = "";
                    const std::string* value =
                        std::get_if<std::string>(&property.second);
                    if (value)
                    {
                        asyncResp->res.jsonValue["CertificateString"] = *value;
                    }
                }
                else if (property.first == "KeyUsage")
                {
                    nlohmann::json& keyUsage =
                        asyncResp->res.jsonValue["KeyUsage"];
                    keyUsage = nlohmann::json::array();
                    const std::vector<std::string>* value =
                        std::get_if<std::vector<std::string>>(&property.second);
                    if (value)
                    {
                        for (const std::string& usage : *value)
                        {
                            keyUsage.push_back(usage);
                        }
                    }
                }
                else if (property.first == "Issuer")
                {
                    const std::string* value =
                        std::get_if<std::string>(&property.second);
                    if (value)
                    {
                        updateCertIssuerOrSubject(
                            asyncResp->res.jsonValue["Issuer"], *value);
                    }
                }
                else if (property.first == "Subject")
                {
                    const std::string* value =
                        std::get_if<std::string>(&property.second);
                    if (value)
                    {
                        updateCertIssuerOrSubject(
                            asyncResp->res.jsonValue["Subject"], *value);
                    }
                }
                else if (property.first == "ValidNotAfter")
                {
                    const uint64_t* value =
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
                    const uint64_t* value =
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
    CertificateActionsReplaceCertificate(App& app) :
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
    void doPost(crow::Response& res, const crow::Request& req,
                const std::vector<std::string>&) override
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
        else if (boost::starts_with(
                     certURI,
                     "/redfish/v1/Managers/bmc/Truststore/Certificates/"))
        {
            objectPath = std::string(certs::authorityObjectPath) + "/" +
                         std::to_string(id);
            name = "TrustStore certificate";
            service = certs::authorityServiceName;
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
                    messages::resourceNotFound(asyncResp->res, name,
                                               std::to_string(id));
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
    HTTPSCertificate(App& app) :
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

    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
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
    HTTPSCertificateCollection(App& app) :
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
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>&) override
    {
        res.jsonValue = {
            {"@odata.id",
             "/redfish/v1/Managers/bmc/NetworkProtocol/HTTPS/Certificates"},
            {"@odata.type", "#CertificateCollection.CertificateCollection"},
            {"Name", "HTTPS Certificates Collection"},
            {"Description", "A Collection of HTTPS certificate instances"}};
        auto asyncResp = std::make_shared<AsyncResp>(res);
        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec,
                        const ManagedObjectType& certs) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "DBUS response error: " << ec;
                    messages::internalError(asyncResp->res);
                    return;
                }
                nlohmann::json& members = asyncResp->res.jsonValue["Members"];
                members = nlohmann::json::array();
                for (const auto& cert : certs)
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

    void doPost(crow::Response& res, const crow::Request& req,
                const std::vector<std::string>&) override
    {
        BMCWEB_LOG_DEBUG << "HTTPSCertificateCollection::doPost";
        auto asyncResp = std::make_shared<AsyncResp>(res);
        asyncResp->res.jsonValue = {{"Name", "HTTPS Certificate"},
                                    {"Description", "HTTPS Certificate"}};

        std::string certFileBody = getCertificateFromReqBody(asyncResp, req);

        if (certFileBody.empty())
        {
            BMCWEB_LOG_ERROR << "Cannot get certificate from request body.";
            messages::unrecognizedRequestBody(asyncResp->res);
            return;
        }

        std::shared_ptr<CertificateFile> certFile =
            std::make_shared<CertificateFile>(certFileBody);

        crow::connections::systemBus->async_method_call(
            [asyncResp, certFile](const boost::system::error_code ec,
                                  const std::string& objectPath) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "DBUS response error: " << ec;
                    messages::internalError(asyncResp->res);
                    return;
                }
                long certId = getIDFromURL(objectPath);
                if (certId < 0)
                {
                    BMCWEB_LOG_ERROR << "Invalid objectPath value"
                                     << objectPath;
                    messages::internalError(asyncResp->res);
                    return;
                }
                std::string certURL =
                    "/redfish/v1/Managers/bmc/NetworkProtocol/HTTPS/"
                    "Certificates/" +
                    std::to_string(certId);
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
    CertificateLocations(App& app) :
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
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>&) override
    {
        res.jsonValue = {
            {"@odata.id",
             "/redfish/v1/CertificateService/CertificateLocations"},
            {"@odata.type",
             "#CertificateLocations.v1_0_0.CertificateLocations"},
            {"Name", "Certificate Locations"},
            {"Id", "CertificateLocations"},
            {"Description",
             "Defines a resource that an administrator can use in order to "
             "locate all certificates installed on a given service"}};
        auto asyncResp = std::make_shared<AsyncResp>(res);
        nlohmann::json& links =
            asyncResp->res.jsonValue["Links"]["Certificates"];
        links = nlohmann::json::array();
        getCertificateLocations(
            asyncResp,
            "/redfish/v1/Managers/bmc/NetworkProtocol/HTTPS/Certificates/",
            certs::httpsObjectPath, certs::httpsServiceName);
        getCertificateLocations(asyncResp,
                                "/redfish/v1/AccountService/LDAP/Certificates/",
                                certs::ldapObjectPath, certs::ldapServiceName);
        getCertificateLocations(
            asyncResp, "/redfish/v1/Managers/bmc/Truststore/Certificates/",
            certs::authorityObjectPath, certs::authorityServiceName);
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
    void getCertificateLocations(std::shared_ptr<AsyncResp>& asyncResp,
                                 const std::string& certURL,
                                 const std::string& path,
                                 const std::string& service)
    {
        BMCWEB_LOG_DEBUG << "getCertificateLocations URI=" << certURL
                         << " Path=" << path << " service= " << service;
        crow::connections::systemBus->async_method_call(
            [asyncResp, certURL](const boost::system::error_code ec,
                                 const ManagedObjectType& certs) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "DBUS response error: " << ec;
                    messages::internalError(asyncResp->res);
                    return;
                }
                nlohmann::json& links =
                    asyncResp->res.jsonValue["Links"]["Certificates"];
                for (auto& cert : certs)
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
    LDAPCertificateCollection(App& app) :
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
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>&) override
    {
        res.jsonValue = {
            {"@odata.id", "/redfish/v1/AccountService/LDAP/Certificates"},
            {"@odata.type", "#CertificateCollection.CertificateCollection"},
            {"Name", "LDAP Certificates Collection"},
            {"Description", "A Collection of LDAP certificate instances"}};
        auto asyncResp = std::make_shared<AsyncResp>(res);
        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec,
                        const ManagedObjectType& certs) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "DBUS response error: " << ec;
                    messages::internalError(asyncResp->res);
                    return;
                }
                nlohmann::json& members = asyncResp->res.jsonValue["Members"];
                members = nlohmann::json::array();
                for (const auto& cert : certs)
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

    void doPost(crow::Response& res, const crow::Request& req,
                const std::vector<std::string>&) override
    {
        auto asyncResp = std::make_shared<AsyncResp>(res);
        std::string certFileBody = getCertificateFromReqBody(asyncResp, req);

        if (certFileBody.empty())
        {
            BMCWEB_LOG_ERROR << "Cannot get certificate from request body.";
            messages::unrecognizedRequestBody(asyncResp->res);
            return;
        }

        std::shared_ptr<CertificateFile> certFile =
            std::make_shared<CertificateFile>(certFileBody);

        crow::connections::systemBus->async_method_call(
            [asyncResp, certFile](const boost::system::error_code ec,
                                  const std::string& objectPath) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "DBUS response error: " << ec;
                    messages::internalError(asyncResp->res);
                    return;
                }
                long certId = getIDFromURL(objectPath);
                if (certId < 0)
                {
                    BMCWEB_LOG_ERROR << "Invalid objectPath value"
                                     << objectPath;
                    messages::internalError(asyncResp->res);
                    return;
                }
                std::string certURL =
                    "/redfish/v1/AccountService/LDAP/Certificates/" +
                    std::to_string(certId);
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
    LDAPCertificate(App& app) :
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

    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>&) override
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
/**
 * Collection of TrustStoreCertificate certificates
 */
class TrustStoreCertificateCollection : public Node
{
  public:
    TrustStoreCertificateCollection(App& app) :
        Node(app, "/redfish/v1/Managers/bmc/Truststore/Certificates/")
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::put, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::post, {{"ConfigureComponents"}}}};
    }
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>&) override
    {
        res.jsonValue = {
            {"@odata.id", "/redfish/v1/Managers/bmc/Truststore/Certificates/"},
            {"@odata.type", "#CertificateCollection.CertificateCollection"},
            {"Name", "TrustStore Certificates Collection"},
            {"Description",
             "A Collection of TrustStore certificate instances"}};
        auto asyncResp = std::make_shared<AsyncResp>(res);
        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec,
                        const ManagedObjectType& certs) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "DBUS response error: " << ec;
                    messages::internalError(asyncResp->res);
                    return;
                }
                nlohmann::json& members = asyncResp->res.jsonValue["Members"];
                members = nlohmann::json::array();
                for (const auto& cert : certs)
                {
                    long id = getIDFromURL(cert.first.str);
                    if (id >= 0)
                    {
                        members.push_back(
                            {{"@odata.id", "/redfish/v1/Managers/bmc/"
                                           "Truststore/Certificates/" +
                                               std::to_string(id)}});
                    }
                }
                asyncResp->res.jsonValue["Members@odata.count"] =
                    members.size();
            },
            certs::authorityServiceName, certs::authorityObjectPath,
            certs::dbusObjManagerIntf, "GetManagedObjects");
    }

    void doPost(crow::Response& res, const crow::Request& req,
                const std::vector<std::string>&) override
    {
        auto asyncResp = std::make_shared<AsyncResp>(res);
        std::string certFileBody = getCertificateFromReqBody(asyncResp, req);

        if (certFileBody.empty())
        {
            BMCWEB_LOG_ERROR << "Cannot get certificate from request body.";
            messages::unrecognizedRequestBody(asyncResp->res);
            return;
        }

        std::shared_ptr<CertificateFile> certFile =
            std::make_shared<CertificateFile>(certFileBody);
        crow::connections::systemBus->async_method_call(
            [asyncResp, certFile](const boost::system::error_code ec,
                                  const std::string& objectPath) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "DBUS response error: " << ec;
                    messages::internalError(asyncResp->res);
                    return;
                }
                long certId = getIDFromURL(objectPath);
                if (certId < 0)
                {
                    BMCWEB_LOG_ERROR << "Invalid objectPath value"
                                     << objectPath;
                    messages::internalError(asyncResp->res);
                    return;
                }
                std::string certURL = "/redfish/v1/Managers/bmc/"
                                      "Truststore/Certificates/" +
                                      std::to_string(certId);

                getCertificateProperties(asyncResp, objectPath,
                                         certs::authorityServiceName, certId,
                                         certURL, "TrustStore Certificate");
                BMCWEB_LOG_DEBUG << "TrustStore certificate install file="
                                 << certFile->getCertFilePath();
            },
            certs::authorityServiceName, certs::authorityObjectPath,
            certs::certInstallIntf, "Install", certFile->getCertFilePath());
    }
}; // TrustStoreCertificateCollection

/**
 * Certificate resource describes a certificate used to prove the identity
 * of a component, account or service.
 */
class TrustStoreCertificate : public Node
{
  public:
    TrustStoreCertificate(App& app) :
        Node(app, "/redfish/v1/Managers/bmc/Truststore/Certificates/<str>/",
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

    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>&) override
    {
        auto asyncResp = std::make_shared<AsyncResp>(res);
        long id = getIDFromURL(req.url);
        if (id < 0)
        {
            BMCWEB_LOG_ERROR << "Invalid url value" << req.url;
            messages::internalError(asyncResp->res);
            return;
        }
        BMCWEB_LOG_DEBUG << "TrustStoreCertificate::doGet ID="
                         << std::to_string(id);
        std::string certURL =
            "/redfish/v1/Managers/bmc/Truststore/Certificates/" +
            std::to_string(id);
        std::string objectPath = certs::authorityObjectPath;
        objectPath += "/";
        objectPath += std::to_string(id);
        getCertificateProperties(asyncResp, objectPath,
                                 certs::authorityServiceName, id, certURL,
                                 "TrustStore Certificate");
    }

    void doDelete(crow::Response& res, const crow::Request& req,
                  const std::vector<std::string>& params) override
    {
        auto asyncResp = std::make_shared<AsyncResp>(res);

        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        long id = getIDFromURL(req.url);
        if (id < 0)
        {
            BMCWEB_LOG_ERROR << "Invalid url value: " << req.url;
            messages::resourceNotFound(asyncResp->res, "TrustStore Certificate",
                                       std::string(req.url));
            return;
        }
        BMCWEB_LOG_DEBUG << "TrustStoreCertificate::doDelete ID="
                         << std::to_string(id);
        std::string certPath = certs::authorityObjectPath;
        certPath += "/";
        certPath += std::to_string(id);

        crow::connections::systemBus->async_method_call(
            [asyncResp, id](const boost::system::error_code ec) {
                if (ec)
                {
                    messages::resourceNotFound(asyncResp->res,
                                               "TrustStore Certificate",
                                               std::to_string(id));
                    return;
                }
                BMCWEB_LOG_INFO << "Certificate deleted";
                asyncResp->res.result(boost::beast::http::status::no_content);
            },
            certs::authorityServiceName, certPath, certs::objDeleteIntf,
            "Delete");
    }
}; // TrustStoreCertificate
} // namespace redfish
