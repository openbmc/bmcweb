#pragma once

#include "bmcweb_config.h"

#include "http_request.hpp"
#include "http_response.hpp"

inline void addSecurityHeaders(const crow::Request& req [[maybe_unused]],
                               crow::Response& res)
{
    /*
     TODO(ed) these should really check content types.  for example,
     X-Content-Type-Options header doesn't make sense when retrieving a JSON or
     javascript file.  It doesn't hurt anything, it's just ugly.
     */
    using bf = boost::beast::http::field;

    // Recommendations from https://owasp.org/www-project-secure-headers/
    // https://owasp.org/www-project-secure-headers/ci/headers_add.json
    res.addHeader(bf::strict_transport_security, "max-age=31536000; "
                                                 "includeSubdomains");
    res.addHeader(bf::x_frame_options, "DENY");

    res.addHeader(bf::pragma, "no-cache");
    res.addHeader(bf::cache_control, "no-store, max-age=0");

    res.addHeader("X-Content-Type-Options", "nosniff");

    res.addHeader("Referrer-Policy", "no-referrer");
    res.addHeader("Permissions-Policy", "accelerometer=(),"
                                        "ambient-light-sensor=(),"
                                        "autoplay=(),"
                                        "battery=(),"
                                        "camera=(),"
                                        "display-capture=(),"
                                        "document-domain=(),"
                                        "encrypted-media=(),"
                                        "fullscreen=(),"
                                        "gamepad=(),"
                                        "geolocation=(),"
                                        "gyroscope=(),"
                                        "layout-animations=(self),"
                                        "legacy-image-formats=(self),"
                                        "magnetometer=(),"
                                        "microphone=(),"
                                        "midi=(),"
                                        "oversized-images=(self),"
                                        "payment=(),"
                                        "picture-in-picture=(),"
                                        "publickey-credentials-get=(),"
                                        "speaker-selection=(),"
                                        "sync-xhr=(self),"
                                        "unoptimized-images=(self),"
                                        "unsized-media=(self),"
                                        "usb=(),"
                                        "screen-wak-lock=(),"
                                        "web-share=(),"
                                        "xr-spatial-tracking=()");
    res.addHeader("X-Permitted-Cross-Domain-Policies", "none");
    res.addHeader("Cross-Origin-Embedder-Policy", "require-corp");
    res.addHeader("Cross-Origin-Opener-Policy", "same-origin");
    res.addHeader("Cross-Origin-Resource-Policy", "same-origin");
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
