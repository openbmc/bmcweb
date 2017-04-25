#include <security_headers_middleware.hpp>

namespace crow {

static const std::string strict_transport_security_key =
    "Strict-Transport-Security";
static const std::string strict_transport_security_value =
    "max-age=31536000; includeSubdomains; preload";

static const std::string ua_compatability_key = "X-UA-Compatible";
static const std::string ua_compatability_value = "IE=11";

static const std::string xframe_key = "X-Frame-Options";
static const std::string xframe_value = "DENY";

static const std::string xss_key = "X-XSS-Protection";
static const std::string xss_value = "1; mode=block";

static const std::string content_security_key = "X-Content-Security-Policy";
static const std::string content_security_value = "default-src 'self'";

void SecurityHeadersMiddleware::before_handle(crow::request& req, response& res,
                                              context& ctx) {}

void SecurityHeadersMiddleware::after_handle(request& /*req*/, response& res,
                                             context& ctx) {
  /*
   TODO(ed) these should really check content types.  for example,
   X-UA-Compatible header doesn't make sense when retrieving a JSON or
   javascript file.  It doesn't hurt anything, it's just ugly.
   */
  res.add_header(strict_transport_security_key,
                 strict_transport_security_value);
  res.add_header(ua_compatability_key, ua_compatability_value);
  res.add_header(xframe_key, xframe_value);
  res.add_header(xss_key, xss_value);
  res.add_header(content_security_key, content_security_value);
}
}
