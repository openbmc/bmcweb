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
static std::unique_ptr<sdbusplus::bus::match::match> hostnameSignalMonitor;

inline void installCertificate(const std::filesystem::path& certPath)
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

inline int onPropertyUpdate(sd_bus_message* m, void* /* userdata */,
                            sd_bus_error* retError)
{
    if (retError == nullptr || sd_bus_error_is_set(retError))
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
    const std::string certFile = "/etc/ssl/certs/https/server.pem";

    X509* cert = ensuressl::loadCert(certFile);
    if (cert == nullptr)
    {
        BMCWEB_LOG_ERROR << "Failed to read cert";
        return 0;
    }

    const int maxKeySize = 256;
    std::array<char, maxKeySize> cnBuffer{};

    int cnLength =
        X509_NAME_get_text_by_NID(X509_get_subject_name(cert), NID_commonName,
                                  cnBuffer.data(), cnBuffer.size());
    if (cnLength == -1)
    {
        BMCWEB_LOG_ERROR << "Failed to read NID_commonName";
        X509_free(cert);
        return 0;
    }
    std::string_view cnValue(std::begin(cnBuffer),
                             static_cast<size_t>(cnLength));

    EVP_PKEY* pPubKey = X509_get_pubkey(cert);
    if (pPubKey == nullptr)
    {
        BMCWEB_LOG_ERROR << "Failed to get public key";
        X509_free(cert);
        return 0;
    }
    int isSelfSigned = X509_verify(cert, pPubKey);
    EVP_PKEY_free(pPubKey);

    BMCWEB_LOG_DEBUG << "Current HTTPs Certificate Subject CN: " << cnValue
                     << ", New HostName: " << *hostname
                     << ", isSelfSigned: " << isSelfSigned;

    ASN1_IA5STRING* asn1 = static_cast<ASN1_IA5STRING*>(
        X509_get_ext_d2i(cert, NID_netscape_comment, nullptr, nullptr));
    if (asn1)
    {
        std::string_view comment(reinterpret_cast<const char*>(asn1->data),
                                 static_cast<size_t>(asn1->length));
        BMCWEB_LOG_DEBUG << "x509Comment: " << comment;

        if (ensuressl::x509Comment == comment && isSelfSigned == 1 &&
            cnValue != *hostname)
        {
            BMCWEB_LOG_INFO << "Ready to generate new HTTPs "
                            << "certificate with subject cn: " << *hostname;

            ensuressl::generateSslCertificate("/tmp/hostname_cert.tmp",
                                              *hostname);
            installCertificate("/tmp/hostname_cert.tmp");
        }
        ASN1_STRING_free(asn1);
    }
    X509_free(cert);
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
