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

inline void installCertificate(const std::filesystem::path& certPath)
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
            "xyz.openbmc_project.Certs.Replace", "Replace", certPath.string());
    }
    else
    {
        BMCWEB_LOG_ERROR << "installCertificate Fail..file not exist";
    }
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

    X509* cert = ensuressl::loadCert(certFile);
    if (cert != nullptr)
    {
        const int maxKeySize = 256;
        std::array<char, maxKeySize + 1> cnBuffer{};

        X509_NAME_get_text_by_NID(X509_get_subject_name(cert), NID_commonName,
                                  cnBuffer.data(), maxKeySize);
        std::string cnValue(std::begin(cnBuffer), std::end(cnBuffer));

        unsigned long subjectNameHash = X509_subject_name_hash(cert);
        unsigned long issuerNameHash = X509_issuer_name_hash(cert);

        ASN1_IA5STRING* asn1 = static_cast<ASN1_IA5STRING*>(
            X509_get_ext_d2i(cert, NID_netscape_comment, nullptr, nullptr));
        if (asn1)
        {
            std::array<char, maxKeySize + 1> commentBuffer{};
            size_t asn1Size = static_cast<unsigned int>(asn1->length);
            if (asn1->length > maxKeySize)
            {
                asn1Size = maxKeySize;
            }

            memcpy(commentBuffer.data(), asn1->data, asn1Size);
            ASN1_STRING_free(asn1);
            BMCWEB_LOG_DEBUG << "x509Comment: " << commentBuffer.data();

            if ((!memcmp(commentBuffer.data(), ensuressl::x509Comment,
                         strlen(ensuressl::x509Comment))) &&
                subjectNameHash == issuerNameHash && cnValue != *hostname)
            {
                isNeedNewCert = true;
            }
        }
        X509_free(cert);

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
