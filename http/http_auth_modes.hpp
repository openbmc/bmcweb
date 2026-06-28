#pragma once

#include "logging.hpp"

#include <string_view>

namespace crow
{
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
} // namespace crow
