#include <crow/http_response.h>

#include <boost/beast/http/field.hpp>

namespace bmcweb
{
inline void addSecurityHeaders(crow::Response& res)
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
    res.addHeader("X-Content-Security-Policy", "script-src 'self'; "
                                             "object-src 'self'");
    res.addHeader("X-XSS-Protection", "1; "
                                      "mode=block");
    res.addHeader("X-Content-Type-Options", "nosniff");
    res.addHeader("X-UA-Compatible", "IE=11");

#ifdef BMCWEB_INSECURE_DISABLE_XSS_PREVENTION

    res.addHeader(bf::access_control_allow_origin, "http://localhost:8080");
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
} // namespace bmcweb
