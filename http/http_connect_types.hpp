// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "bmcweb_config.h"

#include "logging.hpp"

#include <string_view>

namespace crow
{
enum class HttpType
{
    HTTPS, // Socket supports HTTPS only
    HTTP,  // Socket supports HTTP only
    BOTH   // Socket supports both HTTPS and HTTP, with HTTP Redirect
};

enum class AuthMode
{
    NOAUTH, // Socket disables authentication and authorization
    AUTH,   // Socket enable authentication and authorization
};

inline AuthMode getAuthMode(std::string_view authModeString)
{
    if (authModeString == "noauth")
    {
        BMCWEB_LOG_DEBUG("Got http no authen config");
        return AuthMode::NOAUTH;
    }
    if (authModeString == "auth")
    {
        BMCWEB_LOG_DEBUG("Got http authen config");
        return AuthMode::AUTH;
    }
    BMCWEB_LOG_ERROR("Unknown http auth mode={} assuming auth mode",
                     authModeString);
    return AuthMode::AUTH;
}

inline HttpType getHttpType(std::string_view httpTypeString)
{
    if (httpTypeString == "http")
    {
        BMCWEB_LOG_DEBUG("Got http socket");
        return HttpType::HTTP;
    }
    if (httpTypeString == "https")
    {
        BMCWEB_LOG_DEBUG("Got https socket");
        return HttpType::HTTPS;
    }
    if (httpTypeString == "both")
    {
        BMCWEB_LOG_DEBUG("Got hybrid socket");
        return HttpType::BOTH;
    }
    if (BMCWEB_INSECURE_DISABLE_SSL)
    {
        BMCWEB_LOG_ERROR(
            "Unknown http type={} and TLS is disabled, assuming HTTP",
            httpTypeString);
        return HttpType::HTTP;
    }
    BMCWEB_LOG_ERROR("Unknown http type={} assuming HTTPS only",
                     httpTypeString);
    return HttpType::HTTPS;
}
} // namespace crow
