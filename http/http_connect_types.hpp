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
