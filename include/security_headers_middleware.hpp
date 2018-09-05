#pragma once

#include <crow/http_request.h>
#include <crow/http_response.h>

namespace crow
{
static const char* strictTransportSecurityKey = "Strict-Transport-Security";
static const char* strictTransportSecurityValue =
    "max-age=31536000; includeSubdomains; preload";

static const char* uaCompatabilityKey = "X-UA-Compatible";
static const char* uaCompatabilityValue = "IE=11";

static const char* xframeKey = "X-Frame-Options";
static const char* xframeValue = "DENY";

static const char* xssKey = "X-XSS-Protection";
static const char* xssValue = "1; mode=block";

static const char* contentSecurityKey = "X-Content-Security-Policy";
static const char* contentSecurityValue = "default-src 'self'";

static const char* pragmaKey = "Pragma";
static const char* pragmaValue = "no-cache";

static const char* cacheControlKey = "Cache-Control";
static const char* cacheControlValue = "no-Store,no-Cache";

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
        res.addHeader(strictTransportSecurityKey, strictTransportSecurityValue);
        res.addHeader(uaCompatabilityKey, uaCompatabilityValue);
        res.addHeader(xframeKey, xframeValue);
        res.addHeader(xssKey, xssValue);
        res.addHeader(contentSecurityKey, contentSecurityValue);
        res.addHeader(pragmaKey, pragmaValue);
        res.addHeader(cacheControlKey, cacheControlValue);

#ifdef BMCWEB_INSECURE_DISABLE_XSS_PREVENTION

        res.addHeader("Access-Control-Allow-Origin", "http://localhost:8080");
        res.addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, PATCH");
        res.addHeader("Access-Control-Allow-Credentials", "true");
        res.addHeader("Access-Control-Allow-Headers",
                      "Origin, Content-Type, Accept, Cookie, X-XSRF-TOKEN");

#endif
    }
};
} // namespace crow
