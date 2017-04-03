#include <security_headers_middleware.hpp>

namespace crow {

void SecurityHeadersMiddleware::before_handle(crow::request& req, response& res,
                                              context& ctx) {}

void SecurityHeadersMiddleware::after_handle(request& /*req*/, response& res,
                                             context& ctx) {
  // TODO(ed) these should really check content types.  for example, X-UA-Compatible
  // header doesn't make sense when retrieving a JSON or javascript file.  It doesn't
  // hurt anything, it's just ugly.
  res.set_header("Strict-Transport-Security",
                 "max-age=31536000; includeSubdomains; preload");
  res.set_header("X-UA-Compatible", "IE=11");
  res.set_header("X-Frame-Options", "DENY");
  res.set_header("X-XSS-Protection", "1; mode=block");
  res.set_header("X-Content-Security-Policy", "default-src 'self'");
}
}
