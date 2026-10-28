#pragma once

namespace crow
{
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
inline int connectionCount = 0;

enum class HttpType
{
    HTTPS, // Socket supports HTTPS only
    HTTP,  // Socket supports HTTP only
    BOTH   // Socket supports both HTTPS and HTTP, with HTTP Redirect
};
} // namespace crow
