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

inline void install_certificate(const std::string& certPath)
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
        BMCWEB_LOG_ERROR << "install_certificate Fail..file not exist";
    }
}

inline std::string parseSubject(const std::string& subject)
{
    std::size_t cnPos = subject.find("CN=");
    if (cnPos != std::string::npos)
    {
        std::string cn = subject.substr(cnPos);
        cnPos = cn.find(",");
        if (cnPos != std::string::npos)
        {
            cn = cn.substr(0, cnPos);
        }
        cnPos = cn.find("=");
        std::string cnValue = cn.substr(cnPos + 1);
        return cnValue;
    }
    BMCWEB_LOG_ERROR << "Can't find CN in subject";
    return "";
}

inline void onPropertyUpdate(sdbusplus::message::message& message)
{
    try
    {
        std::string iface;
        boost::container::flat_map<std::string, std::variant<std::string>>
            changed_properties;
        std::string hostname;
        message.read(iface, changed_properties);
        auto it = changed_properties.find("HostName");
        if (it != changed_properties.end())
        {
            hostname = std::get<std::string>(it->second);
            BMCWEB_LOG_DEBUG << "Read hostname from signal: " << hostname;
            crow::connections::systemBus->async_method_call(
                [hostname](
                    boost::system::error_code ec,
                    const std::variant<std::string>& currentCertSubject) {
                    if (ec)
                    {
                        return;
                    }
                    const std::string* subject =
                        std::get_if<std::string>(&currentCertSubject);

                    if (subject == nullptr)
                    {
                        BMCWEB_LOG_ERROR << "Unable to read subject";
                        return;
                    }
                    std::string cnValue = parseSubject(*subject);
                    if (!cnValue.empty())
                    {
                        BMCWEB_LOG_DEBUG
                            << "Current HTTPs Certificate Subject: " << cnValue
                            << ", New HostName: " << hostname;

                        if (cnValue != hostname)
                        {
                            BMCWEB_LOG_INFO
                                << "Ready to generate new HTTPs "
                                << "certificate with subject cn: " << hostname;

                            ensuressl::generateSslCertificate(tmpCertPath,
                                                              hostname);
                            install_certificate(tmpCertPath);
                        }
                    }
                },
                "xyz.openbmc_project.Certs.Manager.Server.Https",
                "/xyz/openbmc_project/certs/server/https/1",
                "org.freedesktop.DBus.Properties", "Get",
                "xyz.openbmc_project.Certs.Certificate", "Subject");
        }
    }
    catch (std::exception& e)
    {
        BMCWEB_LOG_WARNING << "Unable to read hostname";
        return;
    }
}

inline void register_hostname_signal()
{
    BMCWEB_LOG_INFO << "Register HostName PropertiesChanged Signal";
    std::string propertiesMatchString;

    propertiesMatchString =
        ("type='signal',"
         "interface='org.freedesktop.DBus.Properties',"
         "path='/xyz/openbmc_project/network/config',"
         "arg0='xyz.openbmc_project.Network.SystemConfiguration',"
         "member='PropertiesChanged'");

    hostnameSignalMonitor = std::make_unique<sdbusplus::bus::match::match>(
        *crow::connections::systemBus, propertiesMatchString, onPropertyUpdate);
}
} // namespace hostname_monitor
} // namespace crow
#endif
