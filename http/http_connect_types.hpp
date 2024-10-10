#pragma once

namespace bmcweb
{
enum class HttpType
{
    HTTPS, // Socket supports HTTPS only
    HTTP,  // Socket supports HTTP only
    BOTH   // Socket supports both HTTPS and HTTP, with HTTP Redirect
};
}

namespace crow = bmcweb;
