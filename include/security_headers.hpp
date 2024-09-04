#pragma once

#include "bmcweb_config.h"

#include "http_request.hpp"
#include "http_response.hpp"

inline void addSecurityHeaders(crow::Response& res)
{
    using bf = boost::beast::http::field;

    // Recommendations from https://owasp.org/www-project-secure-headers/
    // https://owasp.org/www-project-secure-headers/ci/headers_add.json
    res.addHeader(bf::strict_transport_security, "max-age=31536000; "
                                                 "includeSubdomains");

    res.addHeader(bf::pragma, "no-cache");

    if (res.getHeaderValue(bf::cache_control).empty())
    {
        res.addHeader(bf::cache_control, "no-store, max-age=0");
    }
    res.addHeader("X-Content-Type-Options", "nosniff");

    std::string_view contentType = res.getHeaderValue("Content-Type");
    if (contentType.starts_with("text/html"))
    {
        res.addHeader(bf::x_frame_options, "DENY");
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
}
