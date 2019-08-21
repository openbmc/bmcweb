#pragma once

#include <crow/http_request.h>
#include <crow/http_response.h>

namespace crow
{
struct SecurityHeadersMiddleware
{
    struct Context
    {
    };

    void beforeHandle(crow::Request& req, Response& res, Context& ctx)
    {
#ifdef BMCWEB_INSECURE_DISABLE_XSS_PREVENTION
        if ("OPTIONS"_method == req.method())
        {
            res.end();
        }
#endif
    }

    void afterHandle(Request& req, Response& res, Context& ctx)
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

        std::string csp = "default-src 'none'; "
                          "img-src 'self' data:; "
                          "font-src 'self'; "
                          "style-src 'self'; "
                          "script-src 'self'; ";

        if (req.getHeaderValue("User-Agent").find("Edge/") !=
            std::string_view::npos)
        {
            // Edge doesn't handle 'self' as allowing websockets to the same
            // connection.  This is really unfortunate, as it prevents
            // websockets from working entirely.  Unfortunately, we don't
            // actually know the hostname that the user is connecting with.  It
            // could be an ip address or a hostname, or another name, so the
            // best we can do here is to send back what the browser thinks it's
            // connected to, which should give the same behavior as 'self' on
            // other browsers.
            csp += "connect-src wss://";
            csp += req.getHeaderValue("Host");
        }
        else
        {
            csp += "connect-src 'self'";
        }

        // The KVM currently needs to load images from base64 encoded strings.
        // img-src 'self' data: is used to allow that.
        // https://stackoverflow.com/questions/18447970/content-security-policy-data-not-working-for-base64-images-in-chrome-28
        res.addHeader("Content-Security-Policy", csp);

        res.addHeader("X-XSS-Protection", "1; "
                                          "mode=block");
        res.addHeader("X-Content-Type-Options", "nosniff");

#ifdef BMCWEB_INSECURE_DISABLE_XSS_PREVENTION

        res.addHeader(bf::access_control_allow_origin,
                      "http://" + req.getHeaderValue("Host"));
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

#endif
    }
};
} // namespace crow
