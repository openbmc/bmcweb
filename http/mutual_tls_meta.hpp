#pragma once

#include "logging.hpp"

#include <format>
#include <optional>
#include <string>
#include <string_view>

inline std::optional<std::string> mtlsMetaParseSslUser(std::string_view sslUser)
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

    size_t colonIndex = sslUser.find(':');
    if (colonIndex == std::string_view::npos)
    {
        BMCWEB_LOG_WARNING("Invalid Meta TLS user name: '{}'", sslUser);
        return std::nullopt;
    }

    std::string_view userType = sslUser.substr(0, colonIndex);
    sslUser.remove_prefix(colonIndex + 1);
    if (userType != "user" && userType != "svc" && userType != "host")
    {
        BMCWEB_LOG_WARNING("Invalid userType='{}' in Meta TLS user name: '{}'",
                           userType, sslUser);
        return std::nullopt;
    }

    size_t slashIndex = sslUser.find('/');
    std::string_view userHost;
    std::string_view userName;
    if (slashIndex == std::string_view::npos)
    {
        userName = sslUser;
    }
    else
    {
        userName = sslUser.substr(0, slashIndex);
        sslUser.remove_prefix(slashIndex + 1);

        if (userName.find_first_not_of(
                "abcdefghijklmnopqrstuvwxyz0123456789_-.") != std::string::npos)
        {
            BMCWEB_LOG_WARNING(
                "Invalid userName='{}' in Meta TLS user name: '{}'", userName,
                sslUser);
            return std::nullopt;
        }

        // Parse userHost
        userHost = sslUser;
        if (!userHost.empty())
        {
            bool foundHost = false;
            // Remove host suffix (they're not used to uniquely identify hosts
            // and we avoid problems with overly long usernames)
            for (std::string_view suffix :
                 {".facebook.com", ".tfbnw.net", ".thefacebook.com"})
            {
                if (userHost.ends_with(suffix))
                {
                    userHost.remove_suffix(suffix.size());
                    foundHost = true;
                }
            }
            if (!foundHost)
            {
                // BMCWEB_LOG_WARNING(
                //     "Invalid hostname suffix in userHost='{}'. Meta TLS user
                //     name: '{}'", userHost, sslUser);
                // return std::nullopt;
            }

            if (userHost.find_first_not_of(
                    "abcdefghijklmnopqrstuvwxyz0123456789_-.") !=
                std::string::npos)
            {
                BMCWEB_LOG_WARNING(
                    "Invalid userHost='{}' in Meta TLS user name: '{}'",
                    userHost, sslUser);
                return std::nullopt;
            }
        }
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
        return std::nullopt;
    }

    return std::format("{}_{}", userType, userName);
}
