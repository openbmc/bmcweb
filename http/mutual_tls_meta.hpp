// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "logging.hpp"

#include <format>
#include <optional>
#include <string>
#include <string_view>

inline std::optional<std::string_view>
    mtlsMetaParseSslUser(std::string_view sslUser)
{
    // Parses a Meta internal TLS client certificate Subject CN in
    // '<entityType>:<entity>[/<hostname>]' format and returns the resulting
    // POSIX-compatible local user name on success, null otherwise.
    //
    // Only entityType = "user" is supported for now.
    //
    // Example client subject CN -> local user name:
    // "user:a_username/hostname" -> "a_username"

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
    if (entityType != "user")
    {
        BMCWEB_LOG_WARNING(
            "Invalid/unsupported entityType='{}' in Meta TLS client cert Subject CN: '{}'",
            entityType, sslUser);
        return std::nullopt;
    }

    // Parse entity
    size_t slashIndex = sslUser.find('/');
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
    }

    if (entity.empty())
    {
        BMCWEB_LOG_DEBUG("Invalid Meta TLS client cert Subject CN: '{}'",
                         sslUser);
        return std::nullopt;
    }

    return entity;
}
