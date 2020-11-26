#pragma once
#ifdef BMCWEB_ENABLE_SSL
#include <boost/container/flat_map.hpp>
#include <dbus_singleton.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/message/types.hpp>
#include <ssl_key_handler.hpp>

namespace crow
{
namespace hostname_monitor
{
static constexpr const char* tmpCertPath = "/tmp/hostname_cert.tmp";
static std::unique_ptr<sdbusplus::bus::match::match> hostnameSignalMonitor;

inline void installCertificate(const std::string& certPath)
{
    if (access(certPath.c_str(), F_OK) == 0)
    {
        crow::connections::systemBus->async_method_call(
            [certPath](boost::system::error_code ec) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "Replace Certificate Fail..";
                    return;
                }

                BMCWEB_LOG_INFO << "Replace HTTPs Certificate Success, "
                                   "remove temporary certificate file..";
                remove(certPath.c_str());
            },
            "xyz.openbmc_project.Certs.Manager.Server.Https",
            "/xyz/openbmc_project/certs/server/https/1",
            "xyz.openbmc_project.Certs.Replace", "Replace", certPath);
    }
    else
    {
        BMCWEB_LOG_ERROR << "installCertificate Fail..file not exist";
    }
}

inline std::string parseSubject(const std::string& subject)
{
    std::size_t cnPos = subject.find("CN=");
    if (cnPos != std::string::npos)
    {
        std::string cn = subject.substr(cnPos);
        cnPos = cn.find(',');
        if (cnPos != std::string::npos)
        {
            cn = cn.substr(0, cnPos);
        }
        cnPos = cn.find('=');
        std::string cnValue = cn.substr(cnPos + 1);
        return cnValue;
    }
    BMCWEB_LOG_ERROR << "Can't find CN in subject";
    return "";
}

inline int onPropertyUpdate(sd_bus_message* m,
                            void* userdata __attribute__((unused)),
                            sd_bus_error* ret_error)
{
    if (ret_error == nullptr || sd_bus_error_is_set(ret_error))
    {
        BMCWEB_LOG_ERROR << "Got sdbus error on match";
        return 0;
    }

    sdbusplus::message::message message(m);
    std::string iface;
    boost::container::flat_map<std::string, std::variant<std::string>>
        changedProperties;

    message.read(iface, changedProperties);
    auto it = changedProperties.find("HostName");
    if (it == changedProperties.end())
    {
        return 0;
    }

    std::string* hostname = std::get_if<std::string>(&it->second);
    if (hostname == nullptr)
    {
        BMCWEB_LOG_ERROR << "Unable to read hostname";
        return 0;
    }

    BMCWEB_LOG_DEBUG << "Read hostname from signal: " << *hostname;
    bool isNeedNewCert = false;
    const std::string certFile = "/etc/ssl/certs/https/server.pem";
    ensuressl::X509_Ptr cert = ensuressl::loadCert(certFile);
    if (cert != nullptr)
    {
        const int maxKeySize = 256;
        char subBuffer[maxKeySize + 1] = {0};
        ensuressl::BIO_MEM_Ptr subBio(BIO_new(BIO_s_mem()), BIO_free);
        // This pointer cannot be freed independantly.
        X509_NAME* sub = X509_get_subject_name(cert.get());
        X509_NAME_print_ex(subBio.get(), sub, 0, XN_FLAG_SEP_COMMA_PLUS);
        BIO_read(subBio.get(), subBuffer, maxKeySize);
        std::string cnValue = parseSubject(subBuffer);

        unsigned long subjectNameHash = X509_subject_name_hash(cert.get());
        unsigned long issuerNameHash = X509_issuer_name_hash(cert.get());

        ASN1_IA5STRING* asn1;
        if ((asn1 = static_cast<ASN1_IA5STRING*>(X509_get_ext_d2i(
                 cert.get(), NID_netscape_comment, NULL, NULL))))
        {
            char commentBuffer[maxKeySize + 1] = {0};
            snprintf(commentBuffer, maxKeySize, "%s", asn1->data);
            ASN1_STRING_free(asn1);
            BMCWEB_LOG_DEBUG << "x509Comment: " << commentBuffer;

            if (strcmp(ensuressl::x509Comment, commentBuffer) == 0 &&
                subjectNameHash == issuerNameHash && cnValue != *hostname)
            {
                isNeedNewCert = true;
            }
        }

        BMCWEB_LOG_DEBUG << "Current HTTPs Certificate Subject CN: " << cnValue
                         << ", New HostName: " << *hostname;
    }

    if (isNeedNewCert)
    {
        BMCWEB_LOG_INFO << "Ready to generate new HTTPs "
                        << "certificate with subject cn: " << *hostname;
        ensuressl::generateSslCertificate(tmpCertPath, *hostname);
        installCertificate(tmpCertPath);
    }
    return 0;
}

inline void registerHostnameSignal()
{
    BMCWEB_LOG_INFO << "Register HostName PropertiesChanged Signal";
    std::string propertiesMatchString =
        ("type='signal',"
         "interface='org.freedesktop.DBus.Properties',"
         "path='/xyz/openbmc_project/network/config',"
         "arg0='xyz.openbmc_project.Network.SystemConfiguration',"
         "member='PropertiesChanged'");

    hostnameSignalMonitor = std::make_unique<sdbusplus::bus::match::match>(
        *crow::connections::systemBus, propertiesMatchString, onPropertyUpdate,
        nullptr);
}
} // namespace hostname_monitor
} // namespace crow
#endif
