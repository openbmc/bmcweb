#pragma once

#include "logging.hpp"

#include <format>
#include <optional>
#include <string>
#include <string_view>

inline std::optional<std::string> mtlsMetaParseSslUser(std::string_view sslUser)
{
    // Parses a Meta internal TLS client certificate Subject CN in
    // '<entityType>:<entity>[/<hostname>]' format and returns the
    // resulting POSIX-compatible local user name on success, null otherwise.
    //
    // Example client subject CN -> local user name:
    // "user:a_username/hostname" -> "user_a_username"
    // "svc:an_internal_service_name" -> "svc_an_internal_service_name"
    // "host:/hostname.facebook.com" -> "host_hostname" (note the stripped
    // hostname suffix)

    // Parse entityType
    size_t colonIndex = sslUser.find(':');
    if (colonIndex == std::string_view::npos)
    {
        BMCWEB_LOG_WARNING("Invalid Meta TLS client cert Subject CN: '{}'",
                           sslUser);
        return std::nullopt;
    }

    std::string_view entityType = sslUser.substr(0, colonIndex);
    sslUser.remove_prefix(colonIndex + 1);
    if (entityType != "user" && entityType != "svc" && entityType != "host")
    {
        BMCWEB_LOG_WARNING(
            "Invalid entityType='{}' in Meta TLS client cert Subject CN: '{}'",
            entityType, sslUser);
        return std::nullopt;
    }

    // Parse entity and hostname
    size_t slashIndex = sslUser.find('/');
    std::string_view hostname;
    std::string_view entity;
    if (slashIndex == std::string_view::npos)
    {
        // No '/' character, Subject CN is just '<entityType>:<entity>'
        entity = sslUser;
    }
    else
    {
        // Subject CN ends with /<hostname>
        entity = sslUser.substr(0, slashIndex);
        sslUser.remove_prefix(slashIndex + 1);

        if (entity.find_first_not_of(
                "abcdefghijklmnopqrstuvwxyz0123456789_-.") != std::string::npos)
        {
            BMCWEB_LOG_WARNING(
                "Invalid entity='{}' in Meta TLS client cert Subject CN: '{}'",
                entity, sslUser);
            return std::nullopt;
        }

        // Parse hostname
        hostname = sslUser;
        bool foundHost = false;
        // Remove host suffix (they're not used to uniquely identify hosts
        // and we avoid problems with overly long entitys)
        for (std::string_view suffix :
             {".facebook.com", ".tfbnw.net", ".thefacebook.com"})
        {
            if (hostname.ends_with(suffix))
            {
                hostname.remove_suffix(suffix.size());
                foundHost = true;
                break;
            }
        }
        if (!foundHost)
        {
            BMCWEB_LOG_WARNING(
                "Invalid hostname suffix in hostname='{}'. Meta TLS client cert Subject CN: '{}'",
                hostname, sslUser);
            return std::nullopt;
        }

        if (hostname.find_first_not_of(
                "abcdefghijklmnopqrstuvwxyz0123456789_-.") != std::string::npos)
        {
            BMCWEB_LOG_WARNING(
                "Invalid hostname='{}' in Meta TLS client cert Subject CN: '{}'",
                hostname, sslUser);
            return std::nullopt;
        }
    }

    // Use the hostname as entity if entityType == "host"
    // e.g. "host:/hostname.facebook.com" -> "host_hostname"
    if (entityType == "host")
    {
        entity = hostname;
    }

    if (entityType.empty() || entity.empty())
    {
        BMCWEB_LOG_DEBUG("Invalid Meta TLS client cert Subject CN: '{}'",
                         sslUser);
        return std::nullopt;
    }

    return std::format("{}_{}", entityType, entity);
}
