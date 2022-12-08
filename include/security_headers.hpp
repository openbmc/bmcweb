#pragma once

#include <bmcweb_config.h>

#include <http_response.hpp>

inline void addSecurityHeaders(const crow::Request& req [[maybe_unused]],
                               crow::Response& res)
{
    /*
     TODO(ed) these should really check content types.  for example,
     X-UA-Compatible header doesn't make sense when retrieving a JSON or
     javascript file.  It doesn't hurt anything, it's just ugly.
     */
    using bf = boost::beast::http::field;
    res.addHeader(bf::strict_transport_security, "max-age=31536000; "
                                                 "includeSubdomains; "
                                                 "preload");
    res.addHeader(bf::x_frame_options, "DENY");

    res.addHeader(bf::pragma, "no-cache");
    res.addHeader(bf::cache_control, "no-Store,no-Cache");

    res.addHeader("X-XSS-Protection", "1; "
                                      "mode=block");
    res.addHeader("X-Content-Type-Options", "nosniff");

    if (bmcwebInsecureDisableXssPrevention == 0)
    {
        res.addHeader("Content-Security-Policy", "default-src 'none'; "
                                                 "img-src 'self' data:; "
                                                 "font-src 'self'; "
                                                 "style-src 'self'; "
                                                 "script-src 'self'; "
                                                 "connect-src 'self' wss:; "
                                                 "form-action 'none'; "
                                                 "frame-ancestors 'none'; "
                                                 "object-src 'none'; "
                                                 "base-uri 'none' ");
        // The KVM currently needs to load images from base64 encoded
        // strings. img-src 'self' data: is used to allow that.
        // https://stackoverflow.com/questions/18447970/content-security-polic
        // y-data-not-working-for-base64-images-in-chrome-28
    }
    else
    {
        // If XSS is disabled, we need to allow loading from addresses other
        // than self, as the BMC will be hosted elsewhere.
        res.addHeader("Content-Security-Policy", "default-src 'none'; "
                                                 "img-src *; "
                                                 "font-src *; "
                                                 "style-src *; "
                                                 "script-src *; "
                                                 "connect-src *; "
                                                 "form-action *; "
                                                 "frame-ancestors *; "
                                                 "object-src *; "
                                                 "base-uri *");

        std::string_view origin = req.getHeaderValue("Origin");
        res.addHeader(bf::access_control_allow_origin, origin);
        res.addHeader(bf::access_control_allow_methods, "GET, "
                                                        "POST, "
                                                        "PUT, "
                                                        "PATCH, "
                                                        "DELETE");
        res.addHeader(bf::access_control_allow_credentials, "true");
        res.addHeader(bf::access_control_allow_headers, "Origin, "
                                                        "Content-Type, "
                                                        "Accept, "
                                                        "Cookie, "
                                                        "X-XSRF-TOKEN");
    }
}
