#pragma once

#include "logging.hpp"

inline int mtlsMetaParseSslUser(std::string& sslUser, std::string& sslUserOut)
{
    // Parses a Meta internal TLS client certificate user name in
    // '<type>:<username or service name>[/<hostname>]' format and writes the
    // resulting POSIX-compatible user name in sslUserOut. Returns 0 on success,
    // -1 otherwise.
    //
    // Examples:
    // "user:a_username/hostname" -> "user_a_username"
    // "svc:an_internal_service_name" -> "svc_an_internal_service_name"
    // "host:/hostname.facebook.com" -> "host_hostname" (note the stripped
    // hostname suffix)

    std::string userType;
    std::string userName;
    std::string userHost;

    std::stringstream ss(sslUser);

    // Parse userType
    if (!getline(ss, userType, ':'))
    {
        BMCWEB_LOG_WARNING("Invalid Meta TLS user name: '{}'", sslUser);
        return -1;
    }
    if (userType != "user" && userType != "svc" && userType != "host")
    {
        BMCWEB_LOG_WARNING("Invalid userType='{}' in Meta TLS user name: '{}'",
                           userType, sslUser);
        return -1;
    }

    // Parse userName
    if (!getline(ss, userName, '/'))
    {
        BMCWEB_LOG_WARNING("Invalid Meta TLS user name: '{}'", sslUser);
        return -1;
    }
    if (userName.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_-.") !=
        std::string::npos)
    {
        BMCWEB_LOG_WARNING("Invalid userName='{}' in Meta TLS user name: '{}'",
                           userName, sslUser);
        return -1;
    }

    // Parse userHost
    ss >> userHost;
    if (!userHost.empty())
    {
        // Remove host suffix (they're not used to uniquely identify hosts and
        // we avoid problems with overly long usernames)
        if (userHost.ends_with(".facebook.com"))
        {
            userHost.resize(userHost.size() - 13);
        }
        else if (userHost.ends_with(".tfbnw.net"))
        {
            userHost.resize(userHost.size() - 10);
        }
        else if (userHost.ends_with(".thefacebook.com"))
        {
            userHost.resize(userHost.size() - 16);
        }
        else
        {
            BMCWEB_LOG_WARNING(
                "Invalid hostname suffix in userHost='{}'. Meta TLS user name: '{}'",
                userHost, sslUser);
            return -1;
        }

        if (userHost.find_first_not_of(
                "abcdefghijklmnopqrstuvwxyz0123456789_-.") != std::string::npos)
        {
            BMCWEB_LOG_WARNING(
                "Invalid userHost='{}' in Meta TLS user name: '{}'", userHost,
                sslUser);
            return -1;
        }
    }

    if (!ss.eof())
    {
        BMCWEB_LOG_WARNING("Invalid Meta TLS user name: '{}'", sslUser);
        return -1;
    }

    // Use the hostname as user name if userType == "host"
    // e.g. "host:/hostname.facebook.com" -> "host_hostname"
    if (userType == "host")
    {
        userName = userHost;
    }

    if (userType.empty() || userName.empty())
    {
        BMCWEB_LOG_DEBUG("Invalid Meta TLS user name: '{}'", sslUser);
        return -1;
    }

    sslUserOut = userType + "_" + userName;

    return 0;
}
