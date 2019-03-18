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
static const char *certReplaceIntf = "xyz.openbmc_project.Certs.Replace";
static const char *certPropIntf = "xyz.openbmc_project.Certs.Certificate";
static const char *dbusPropIntf = "org.freedesktop.DBus.Properties";
static const char *dbusObjManagerIntf = "org.freedesktop.DBus.ObjectManager";
static const char *mapperBusName = "xyz.openbmc_project.ObjectMapper";
static const char *mapperObjectPath = "/xyz/openbmc_project/object_mapper";
static const char *mapperIntf = "xyz.openbmc_project.ObjectMapper";
static const char *ldapObjectPath = "/xyz/openbmc_project/certs/client/ldap";

static void getCertificateProperties(
    const std::shared_ptr<AsyncResp> &asyncResp, const std::string &objectPath,
    const std::string& certId, const std::string &certURL,
    const std::string &name);

static bool isKeyUsageFound(const std::string &str);
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
        auto &replaceCert =
            res.jsonValue["Actions"]["#CertificateService.ReplaceCertificate"];
        replaceCert["target"] = "/redfish/v1/CertificateService/Actions/"
                                "CertificateService.ReplaceCertificate";
        auto &generateCSR =
            res.jsonValue["Actions"]["#CertificateService.GenerateCSR"];
        generateCSR["target"] = "/redfish/v1/CertificateService/Actions/"
                                "CertificateService.GenerateCSR";
        res.end();
    }
}; // CertificateService
/**
 * @brief Find the ID specified in the URL
 * Finds the numbers specified after the last "/" in the URL and returns.
 * @param[in] path URL
 * @return 0 on failure and number on success
 */
int getIDFromURL(const std::string &url)
{
    std::size_t found = url.rfind("/");
    if (found == std::string::npos)
    {
        return 0;
    }
    if ((found + 1) < url.length())
    {
        char *endPtr;
        long value = std::strtol(url.substr(found + 1).c_str(), &endPtr, 10);
        if (*endPtr != '\0')
        {
            return 0;
        }
        return value;
    }
    return 0;
}

namespace fs = std::filesystem;
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
        std::optional<std::string> certificate;
        std::optional<std::string> certificateUri;
        if (!json_util::readJson(req, res, "CertificateString", certificate,
                                 "CertificateUri", certificateUri))
        {
            BMCWEB_LOG_ERROR << "Required parameters are missing";
            messages::internalError(res);
            return;
        }
        if (!certificate)
        {
            messages::actionParameterMissing(res, "ReplaceCertificate",
                                             "CertificateString");
            return;
        }
        if (!certificateUri)
        {
            messages::actionParameterMissing(res, "ReplaceCertificate",
                                             "CertificateUri");
            return;
        }

        auto id = getIDFromURL(*certificateUri);
        if (!id)
        {
            messages::actionParameterValueFormatError(
                res, *certificateUri, "CertificateUri", "ReplaceCertificate");
            return;
        }
        std::string certId = std::to_string(id);
        std::string objectPath;
        std::string name;
        if (boost::starts_with(
                *certificateUri,
                "/redfish/v1/Managers/bmc/NetworkProtocol/HTTPS/Certificates/"))
        {
            objectPath = std::string(httpsObjectPath) + "/" + certId;
            name = "HTTPS certificate";
        }
        else if (boost::starts_with(
                     *certificateUri,
                     "/redfish/v1/AccountService/LDAP/Certificates/"))
        {
            objectPath = std::string(ldapObjectPath) + "/" + certId;
            name = "LDAP certificate";
        }
        else
        {
            BMCWEB_LOG_ERROR << "Unsupported certificate URI"
                             << *certificateUri;
            return;
        }

        std::shared_ptr<CertificateFile> certFile =
            std::make_shared<CertificateFile>(*certificate);
        auto asyncResp = std::make_shared<AsyncResp>(res);
        auto replaceCertificate = [asyncResp, certFile, objectPath,
                                   certificateUri, certId,
                                   name](const boost::system::error_code ec) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "DBUS response error: " << ec;
                messages::internalError(asyncResp->res);
                return;
            }
            getCertificateProperties(asyncResp, objectPath, certId,
                                     *certificateUri, name);
            asyncResp->res.addHeader("Location", *certificateUri);
            asyncResp->res.result(boost::beast::http::status::accepted);
            BMCWEB_LOG_DEBUG << "HTTPS certificate install file="
                             << certFile->getCertFilePath();
        };
        auto getServiceName =
            [asyncResp, replaceCertificate(std::move(replaceCertificate)),
             objectPath, certFile](const boost::system::error_code ec,
                                   const GetObjectType &resp) {
                if (ec || resp.empty())
                {
                    BMCWEB_LOG_ERROR << "DBUS response error: " << ec;
                    messages::internalError(asyncResp->res);
                    return;
                }
                std::string service = resp.begin()->first;
                crow::connections::systemBus->async_method_call(
                    std::move(replaceCertificate), service, objectPath,
                    certReplaceIntf, "Replace", certFile->getCertFilePath());
            };
        crow::connections::systemBus->async_method_call(
            std::move(getServiceName), mapperBusName, mapperObjectPath,
            mapperIntf, "GetObject", objectPath, std::array<std::string, 0>());
    }
}; // CertificateActionsReplaceCertificate

static void getCSR(const std::shared_ptr<AsyncResp> &asyncResp,
                   const std::string &objectPath)
{
    BMCWEB_LOG_DEBUG << "getCSR object Path=" << objectPath;
    auto getCsr = [asyncResp](const boost::system::error_code ec,
                              const std::string &csr) {
        if (ec || csr.empty())
        {
            BMCWEB_LOG_ERROR << "DBUS response error: " << ec;
            messages::internalError(asyncResp->res);
            return;
        }
        BMCWEB_LOG_DEBUG << "getCSR csr=" << csr;
        asyncResp->res.write(csr);
    };
    auto getServiceName = [asyncResp, getCsr(std::move(getCsr)),
                           objectPath](const boost::system::error_code ec,
                                       const GetObjectType &resp) {
        if (ec || resp.empty())
        {
            BMCWEB_LOG_ERROR << "DBUS response error: " << ec;
            messages::internalError(asyncResp->res);
            return;
        }
        BMCWEB_LOG_DEBUG << "getCSR getServiceName objectPath=" << objectPath;
        std::string service = resp.begin()->first;
        crow::connections::systemBus->async_method_call(
            std::move(getCsr), service, objectPath,
            "xyz.openbmc_project.Certs.CSR.View", "CSR");
    };
    crow::connections::systemBus->async_method_call(
        std::move(getServiceName), mapperBusName, mapperObjectPath, mapperIntf,
        "GetObject", objectPath, std::array<std::string, 0>());
}

static std::unique_ptr<sdbusplus::bus::match::match> csrMatcher;
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
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    void doPost(crow::Response &res, const crow::Request &req,
                const std::vector<std::string> &params) override
    {
        // Required parameters
        std::optional<std::string> city;
        std::optional<std::string> commonName;
        std::optional<std::string> contactPerson;
        std::optional<std::string> country;
        std::optional<std::string> organization;
        std::optional<std::string> organizationUnit;
        std::optional<std::string> state;
        std::optional<std::string> certificateCollection;

        // Optional parameters
        std::vector<std::string> alternativeNames;
        std::string challengePassword;
        std::string email;
        std::string givenName;
        std::string initials;

        // Default value if user do not provide is 1024
        uint64_t keyBitLength = 1024;
        std::string keyCurveId;

        // Default is RSA and can take values of RSA/DSA
        std::string keyPairAlgorithm = "RSA";
        std::vector<std::string> keyUsage;
        std::string surName;
        std::string unstructuredName;
        if (!json_util::readJson(
                req, res, "City", city, "CommonName", commonName,
                "ContactPerson", contactPerson, "Country", country,
                "Organization", organization, "OrganizationUnit",
                organizationUnit, "State", state, "CertificateCollection",
                certificateCollection, "AlternativeNames", alternativeNames,
                "ChallengePassword", challengePassword, "Email", email,
                "GivenName", givenName, "Initials", initials, "KeyBitLength",
                keyBitLength, "KeyCurveId", keyCurveId, "KeyPairAlgorithm",
                keyPairAlgorithm, "KeyUsage", keyUsage, "SurName", surName,
                "UnstructuredName", unstructuredName))
        {
            BMCWEB_LOG_ERROR << "Failure to read required parameters";
            messages::internalError(res);
            res.end();
            return;
        }
        if (!city || (*city).empty())
        {
            messages::actionParameterMissing(res, "GenerateCSR", "City");
            res.end();
            return;
        }
        if (!commonName || (*commonName).empty())
        {
            messages::actionParameterMissing(res, "GenerateCSR", "CommonName");
            res.end();
            return;
        }
        if (!contactPerson || (*contactPerson).empty())
        {
            messages::actionParameterMissing(res, "GenerateCSR",
                                             "ContactPerson");
            res.end();
            return;
        }
        if (!country || (*country).empty())
        {
            messages::actionParameterMissing(res, "GenerateCSR", "Country");
            res.end();
            return;
        }
        if (!organization || (*organization).empty())
        {
            messages::actionParameterMissing(res, "GenerateCSR",
                                             "Organization");
            res.end();
            return;
        }

        if (!organizationUnit || (*organizationUnit).empty())
        {
            messages::actionParameterMissing(res, "GenerateCSR",
                                             "OrganizationUnit");
            res.end();
            return;
        }

        if (!state || (*state).empty())
        {
            messages::actionParameterMissing(res, "GenerateCSR", "State");
            res.end();
            return;
        }

        if (!certificateCollection || (*certificateCollection).empty())
        {
            messages::actionParameterMissing(res, "GenerateCSR",
                                             "CertificateCollection");
            res.end();
            return;
        }

        std::string objectPath;
        if (*certificateCollection ==
            "/redfish/v1/Managers/bmc/NetworkProtocol/HTTPS/Certificates/")
        {
            objectPath = httpsObjectPath;
        }
        else if (*certificateCollection ==
                 "/redfish/v1/AccountService/LDAP/Certificates/")
        {
            objectPath = ldapObjectPath;
        }
        else
        {
            messages::actionParameterValueFormatError(
                res, *certificateCollection, "CertificateCollection",
                "GenerateCSR");
            res.end();
            return;
        }

        // validate keyusage if user has specfied a value as this is optional
        if (!keyUsage.empty())
        {
            for (const auto &usage : keyUsage)
            {
                if (!isKeyUsageFound(usage))
                {
                    messages::actionParameterValueFormatError(
                        res, usage, "KeyUsage", "GenerateCSR");
                    res.end();
                    return;
                }
            }
        }

        // validate email if user has specified a value as this is optional
        if (!email.empty())
        {
            const std::regex pattern("(\\w+)(\\.|_)?(\\w*)@(\\w+)(\\.(\\w+))+");
            // try to match the string with the regular expression
            if (!std::regex_match(email, pattern))
            {
                messages::actionParameterValueFormatError(res, email, "Email",
                                                          "GenerateCSR");
                res.end();
                return;
            }
        }

        // validate KeyPairAlgorithm if user has specified a value else use
        // default value
        if (!keyPairAlgorithm.empty())
        {
            if (keyPairAlgorithm != "RSA" && keyPairAlgorithm != "DSA")
            {
                messages::actionParameterValueFormatError(
                    res, keyPairAlgorithm, "keyPairAlgorithm", "GenerateCSR");
                res.end();
                return;
            }
        }
        // validate keyBitLength if user has specified a value, else use default
        // value
        if (keyBitLength != 0)
        {
            if (keyBitLength != 1024 && keyBitLength && 2048 &&
                keyBitLength != 4096)
            {
                messages::actionParameterValueFormatError(
                    res, keyPairAlgorithm, "keyPairAlgorithm", "GenerateCSR");
                res.end();
                return;
            }
        }
        // Only allow one CSR matcher at a time
        if (csrMatcher)
        {
            res.addHeader("Retry-After", "30");
            messages::serviceTemporarilyUnavailable(res, "3");
            res.end();
            return;
        }
        auto asyncResp = std::make_shared<AsyncResp>(res);

        // Make this static so it survives outside this method
        static boost::asio::deadline_timer timeout(*req.ioService);
        timeout.expires_from_now(boost::posix_time::seconds(30));
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
            BMCWEB_LOG_ERROR << "Timed out waiting for immediate log";
            messages::internalError(asyncResp->res);
        });

        auto csrCallback = [asyncResp,
                            objectPath](sdbusplus::message::message &m) {
            BMCWEB_LOG_DEBUG << "CSR object creation match found";
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
                std::string,
                std::vector<std::pair<std::string, std::variant<std::string>>>>>
                interfacesProperties;
            sdbusplus::message::object_path csrObjectPath;
            m.read(csrObjectPath, interfacesProperties);
            BMCWEB_LOG_DEBUG << "obj path = " << csrObjectPath.str;
            for (auto &interface : interfacesProperties)
            {
                BMCWEB_LOG_DEBUG << "interface = " << interface.first;
                if (interface.first == "xyz.openbmc_project.Certs.CSR.View/")
                {
                    getCSR(asyncResp, objectPath);
                    break;
                }
            }
        };

        auto generateCSR = [asyncResp, objectPath,
                            csrCallback(std::move(csrCallback))](
                               const boost::system::error_code ec,
                               const std::string &path) {
            BMCWEB_LOG_DEBUG << "generateCSR handler  path " << path;
            if (ec)
            {
                BMCWEB_LOG_ERROR << "DBUS response error: " << ec.message();
                messages::internalError(asyncResp->res);
                return;
            }
            csrMatcher = std::make_unique<sdbusplus::bus::match::match>(
                *crow::connections::systemBus,
                "interface='org.freedesktop.DBus.ObjectManager', type='signal',"
                "member='InterfacesAdded', path='/xyz/openbmc_project/certs'",
                csrCallback);
        };
        auto getServiceName =
            [asyncResp, objectPath, generateCSR(std::move(generateCSR)),
             alternativeNames(std::move(alternativeNames)),
             challengePassword(std::move(challengePassword)),
             city(std::move(city)), commonName(std::move(commonName)),
             contactPerson(std::move(contactPerson)),
             country(std::move(country)), email(std::move(email)),
             givenName(std::move(givenName)), initials(std::move(initials)),
             keyBitLength(std::move(keyBitLength)),
             keyCurveId(std::move(keyCurveId)),
             keyPairAlgorithm(std::move(keyPairAlgorithm)),
             keyUsage(std::move(keyUsage)),
             organization(std::move(organization)),
             organizationUnit(std::move(organizationUnit)),
             state(std::move(state)), surName(std::move(surName)),
             unstructuredName(std::move(unstructuredName))](
                const boost::system::error_code ec, const GetObjectType &resp) {
                if (ec || resp.empty())
                {
                    BMCWEB_LOG_ERROR << "DBUS response error: " << ec;
                    messages::internalError(asyncResp->res);
                    return;
                }
                std::string service = resp.begin()->first;
                crow::connections::systemBus->async_method_call(
                    std::move(generateCSR), service, objectPath,
                    "xyz.openbmc_project.Certs.CSR.Create", "GenerateCSR",
                    alternativeNames, challengePassword, *city, *commonName,
                    *contactPerson, *country, email, givenName, initials,
                    keyBitLength, keyCurveId, keyPairAlgorithm, keyUsage,
                    *organization, *organizationUnit, *state, surName,
                    unstructuredName);
            };
        crow::connections::systemBus->async_method_call(
            std::move(getServiceName), mapperBusName, mapperObjectPath,
            mapperIntf, "GetObject", objectPath, std::array<std::string, 0>());
    }
}; // CertificateActionGenerateCSR

/**
 * @brief Converts EPOCH time to redfish time format
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
    using Tokenizer = boost::tokenizer<boost::char_separator<char>>;
    boost::char_separator<char> commaSep{","};
    Tokenizer tokenComma{str, commaSep};
    for (auto &tok : tokenComma)
    {
        std::string data = tok;
        boost::algorithm::trim(data);
        boost::char_separator<char> equalSep{"="};
        Tokenizer tokEqual{data, equalSep};
        auto dist = std::distance(tokEqual.begin(), tokEqual.end());
        if (dist == 2)
        {
            auto it = tokEqual.begin();
            std::string first = std::move(*it);
            ++it;
            std::string second = std::move(*it);
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
        asyncResp->res.jsonValue[type]["organizationUnit"] =
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
 * @param[in] objectPath  Path of the D-Bus service object
 * @param[in] certId  Id of the certificate
 * @param[in] certURL  URL of the certificate object
 * @param[in] name  certificate property
 * @return None
 */
static void getCertificateProperties(
    const std::shared_ptr<AsyncResp> &asyncResp, const std::string &objectPath,
    const std::string &certId, const std::string &certURL,
    const std::string &name)
{
    using PropertyType =
        std::variant<std::string, uint64_t, std::vector<std::string>>;
    using PropertiesMap = boost::container::flat_map<std::string, PropertyType>;
    BMCWEB_LOG_DEBUG << "getCertificateProperties Path=" << objectPath
                     << " certId=" << certId << " certURl=" << certURL
                     << "name =" << name;
    auto getAllProperties = [asyncResp, certURL, certId,
                             name](const boost::system::error_code ec,
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
            {"@odata.context", "/redfish/v1/$metadata#Certificate.Certificate"},
            {"Id", certId},
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
        asyncResp->res.addHeader("Location", certURL);
        asyncResp->res.result(boost::beast::http::status::ok);
    };
    auto getServiceName =
        [asyncResp, getAllProperties(std::move(getAllProperties)), objectPath](
            const boost::system::error_code ec, const GetObjectType &resp) {
            if (ec || resp.empty())
            {
                BMCWEB_LOG_ERROR << "DBUS response error: " << ec;
                messages::internalError(asyncResp->res);
                return;
            }
            std::string service = resp.begin()->first;
            crow::connections::systemBus->async_method_call(
                std::move(getAllProperties), service, objectPath, dbusPropIntf,
                "GetAll", certPropIntf);
        };
    crow::connections::systemBus->async_method_call(
        std::move(getServiceName), mapperBusName, mapperObjectPath, mapperIntf,
        "GetObject", objectPath, std::array<std::string, 0>());
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
        std::string certURL =
            "/redfish/v1/Managers/bmc/NetworkProtocol/HTTPS/Certificates/" +
            certId;
        auto asyncResp = std::make_shared<AsyncResp>(res);
        std::string objectPath = httpsObjectPath;
        objectPath += "/";
        objectPath += certId;
        std::string name = "HTTPS Certificate";
        getCertificateProperties(asyncResp, objectPath, certId, certURL, name);
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
        BMCWEB_LOG_DEBUG << "HTTPSCertificateCollection::doPost";

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
            //// TODO: Issue#3 supproting only 1 certificate
            std::string certId = "1";
            std::string certURL =
                "/redfish/v1/Managers/bmc/NetworkProtocol/HTTPS/Certificates/";
            certURL += certId;
            std::string name = "HTTPS Certificate";
            std::string objectPath =
                std::string(httpsObjectPath) + "/" + certId;
            getCertificateProperties(asyncResp, objectPath, certId, certURL,
                                     name);
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
        auto getCertificateList = [asyncResp](
                                      const boost::system::error_code ec,
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
            asyncResp->res.jsonValue["Members@odata.count"] = members.size();
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
        std::string filePath("/tmp/" + boost::uuids::to_string(
                                           boost::uuids::random_generator()()));
        std::shared_ptr<CertificateFile> certFile =
            std::make_shared<CertificateFile>(req.body);
        auto asyncResp = std::make_shared<AsyncResp>(res);
        auto installCertificate = [asyncResp, certFile](
                                      const boost::system::error_code ec) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "DBUS response error: " << ec;
                messages::internalError(asyncResp->res);
                return;
            }
            //// TODO: Issue#3 supproting only 1 certificate
            std::string certId = "1";
            std::string certURL =
                "/redfish/v1/AccountService/LDAP/Certificates/";
            certURL += certId;
            std::string name = "LDAP Certificate";
            std::string objectPath = std::string(ldapObjectPath) + "/" + certId;
            getCertificateProperties(asyncResp, objectPath, certId, certURL,
                                     name);
            BMCWEB_LOG_DEBUG << "LDAP certificate install file="
                             << certFile->getCertFilePath();
        };
        auto getServiceName =
            [asyncResp, installCertificate(std::move(installCertificate)),
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
                    std::move(installCertificate), service, ldapObjectPath,
                    certInstallIntf, "Install", certFile->getCertFilePath());
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
        std::string certURL = "/redfish/v1/AccountService/LDAP/Certificates/";
        certURL += certId;
        auto asyncResp = std::make_shared<AsyncResp>(res);
        std::string objectPath = ldapObjectPath;
        objectPath += "/";
        objectPath += certId;
        std::string name = "LDAP Certificate";
        getCertificateProperties(asyncResp, objectPath, certId, certURL, name);
    }
}; // LDAPCertificate

} // namespace redfish
