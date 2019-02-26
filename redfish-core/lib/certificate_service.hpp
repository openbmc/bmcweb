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
constexpr char const *certPropIntf = "xyz.openbmc_project.Certs.Certificate";
constexpr char const *dbusPropIntf = "org.freedesktop.DBus.Properties";
constexpr char const *dbusObjManagerIntf = "org.freedesktop.DBus.ObjectManager";
constexpr char const *mapperBusName = "xyz.openbmc_project.ObjectMapper";
constexpr char const *mapperObjectPath = "/xyz/openbmc_project/object_mapper";
constexpr char const *mapperIntf = "xyz.openbmc_project.ObjectMapper";
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
        if (fs::exists(certDirectory))
        {
            BMCWEB_LOG_DEBUG << "Removing certificate file" << certificateFile;
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
 * @brief Parse and update Certficate Issue/Subject property
 *
 * @param[in] asyncResp Shared pointer to the response message
 * @param[in] str  Issuer/Subject value in key=value pairs
 * @param[in] type Issuer/Subject
 * @return None
 */
static void
    updateCertIssuerOrSubject(const std::shared_ptr<AsyncResp> &asyncResp,
                              const std::string &value, const std::string &type)
{

    using Tokenizer = boost::tokenizer<boost::char_separator<char>>;
    boost::char_separator<char> commaSep{","};
    Tokenizer tokenComma{value, commaSep};
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
            if (first == "L")
            {
                asyncResp->res.jsonValue[type]["City"] = std::move(second);
            }
            else if (first == "CN")
            {
                asyncResp->res.jsonValue[type]["CommonName"] =
                    std::move(second);
            }
            else if (first == "C")
            {
                asyncResp->res.jsonValue[type]["Country"] = std::move(second);
            }
            else if (first == "O")
            {
                asyncResp->res.jsonValue[type]["Organization"] =
                    std::move(second);
            }
            else if (first == "OU")
            {
                asyncResp->res.jsonValue[type]["OrganizationalUnit"] =
                    std::move(second);
            }
            else if (first == "ST")
            {
                asyncResp->res.jsonValue[type]["State"] = std::move(second);
            }
        }
    } // end for
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
    const std::string &certId, const std::string &certURL,
    const std::string &name)
{
    using PropertyType =
        std::variant<std::string, uint64_t, std::vector<std::string>>;
    using PropertiesMap = boost::container::flat_map<std::string, PropertyType>;
    BMCWEB_LOG_DEBUG << "getCertificateProperties Path=" << objectPath
                     << " certId=" << certId << " certURl=" << certURL;
    crow::connections::systemBus->async_method_call(
        [asyncResp, objectPath, certURL, certId,
         name](const boost::system::error_code ec, const GetObjectType &resp) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "DBUS response error: " << ec;
                messages::internalError(asyncResp->res);
                return;
            }
            if (resp.size() > 1 || resp.empty())
            {
                BMCWEB_LOG_ERROR << "Invalid number of objects found "
                                 << resp.size();
                messages::internalError(asyncResp->res);
                return;
            }
            const std::string &service = resp.begin()->first;
            crow::connections::systemBus->async_method_call(
                [asyncResp, certURL, certId,
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
                        {"@odata.context",
                         "/redfish/v1/$metadata#Certificate.Certificate"},
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
                                asyncResp->res.jsonValue["CertificateString"] =
                                    *value;
                            }
                        }
                        else if (property.first == "KeyUsage")
                        {
                            nlohmann::json &keyUsage =
                                asyncResp->res.jsonValue["KeyUsage"];
                            keyUsage = nlohmann::json::array();
                            const std::vector<std::string> *value =
                                std::get_if<std::vector<std::string>>(
                                    &property.second);
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
                                updateCertIssuerOrSubject(asyncResp, *value,
                                                          "Issuer");
                            }
                        }
                        else if (property.first == "Subject")
                        {
                            const std::string *value =
                                std::get_if<std::string>(&property.second);
                            if (value)
                            {
                                updateCertIssuerOrSubject(asyncResp, *value,
                                                          "Subject");
                            }
                        }
                        else if (property.first == "ValidNotAfter")
                        {
                            const uint64_t *value =
                                std::get_if<uint64_t>(&property.second);
                            if (value)
                            {
                                std::time_t time =
                                    static_cast<std::time_t>(*value);
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
                                std::time_t time =
                                    static_cast<std::time_t>(*value);
                                asyncResp->res.jsonValue["ValidNotBefore"] =
                                    crow::utility::getDateTime(time);
                            }
                        }
                    }
                    asyncResp->res.addHeader("Location", certURL);
                },
                service, objectPath, certs::dbusPropIntf, "GetAll",
                certs::certPropIntf);
        },
        certs::mapperBusName, certs::mapperObjectPath, certs::mapperIntf,
        "GetObject", objectPath,
        std::array<const char *, 1>{certs::certPropIntf});
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
        auto asyncResp = std::make_shared<AsyncResp>(res);
        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);
            return;
        }
        std::string certId = params[0];
        dbus::utility::escapePathForDbus(certId);
        BMCWEB_LOG_DEBUG << "HTTPSCertificate::doGet ID=" << certId;
        std::string certURL =
            "/redfish/v1/Managers/bmc/NetworkProtocol/HTTPS/Certificates/" +
            certId;
        std::string objectPath = certs::httpsObjectPath;
        objectPath += "/";
        objectPath += certId;
        getCertificateProperties(asyncResp, objectPath, certId, certURL,
                                 "HTTPS Certificate");
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
                        const GetObjectType &resp) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "DBUS response error: " << ec;
                    messages::internalError(asyncResp->res);
                    return;
                }
                if (resp.size() > 1 || resp.empty())
                {
                    BMCWEB_LOG_ERROR << "Invalid number of objects found "
                                     << resp.size();
                    messages::internalError(asyncResp->res);
                    return;
                }
                const std::string &service = resp.begin()->first;
                crow::connections::systemBus->async_method_call(
                    [asyncResp](const boost::system::error_code ec,
                                const ManagedObjectType &certs) {
                        if (ec)
                        {
                            BMCWEB_LOG_ERROR << "DBUS response error: " << ec;
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        nlohmann::json &members =
                            asyncResp->res.jsonValue["Members"];
                        members = nlohmann::json::array();
                        for (const auto &cert : certs)
                        {
                            int id = getIDFromURL(cert.first.str);
                            if (id)
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
                    service, certs::httpsObjectPath, certs::dbusObjManagerIntf,
                    "GetManagedObjects");
            },
            certs::mapperBusName, certs::mapperObjectPath, certs::mapperIntf,
            "GetObject", certs::httpsObjectPath,
            std::array<const char *, 1>{certs::certInstallIntf});
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
            [asyncResp, certFile](const boost::system::error_code ec,
                                  const GetObjectType &resp) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "DBUS response error: " << ec;
                    messages::internalError(asyncResp->res);
                    return;
                }
                if (resp.size() > 1 || resp.empty())
                {
                    BMCWEB_LOG_ERROR << "Invalid number of objects found "
                                     << resp.size();
                    messages::internalError(asyncResp->res);
                    return;
                }
                const std::string &service = resp.begin()->first;
                crow::connections::systemBus->async_method_call(
                    [asyncResp, certFile](const boost::system::error_code ec) {
                        if (ec)
                        {
                            BMCWEB_LOG_ERROR << "DBUS response error: " << ec;
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        // TODO: Issue#84 supporting only 1 certificate
                        std::string certId = "1";
                        std::string certURL =
                            "/redfish/v1/Managers/bmc/NetworkProtocol/HTTPS/"
                            "Certificates/";
                        certURL += certId;
                        std::string objectPath =
                            std::string(certs::httpsObjectPath) + "/" + certId;
                        getCertificateProperties(asyncResp, objectPath, certId,
                                                 certURL, "HTTPS Certificate");
                        BMCWEB_LOG_DEBUG << "HTTPS certificate install file="
                                         << certFile->getCertFilePath();
                    },
                    service, certs::httpsObjectPath, certs::certInstallIntf,
                    "Install", certFile->getCertFilePath());
            },
            certs::mapperBusName, certs::mapperObjectPath, certs::mapperIntf,
            "GetObject", certs::httpsObjectPath,
            std::array<const char *, 1>{certs::certInstallIntf});
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
    crow::connections::systemBus->async_method_call(
        [asyncResp, path, certURL](const boost::system::error_code ec,
                                   const GetObjectType &resp) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "DBUS response error: " << ec;
                messages::internalError(asyncResp->res);
                return;
            }
            if (resp.size() > 1 || resp.empty())
            {
                BMCWEB_LOG_ERROR << "Invalid number of objects found "
                                 << resp.size();
                messages::internalError(asyncResp->res);
                return;
            }
            const std::string &service = resp.begin()->first;
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
                        int id = getIDFromURL(cert.first.str);
                        if (id)
                        {
                            links.push_back(
                                {{"@odata.id", certURL + std::to_string(id)}});
                        }
                    }
                    asyncResp->res
                        .jsonValue["Links"]["Certificates@odata.count"] =
                        links.size();
                },
                service, path, certs::dbusObjManagerIntf, "GetManagedObjects");
        },
        certs::mapperBusName, certs::mapperObjectPath, certs::mapperIntf,
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
            certs::httpsObjectPath);
    }
}; // CertificateLocations
} // namespace redfish
