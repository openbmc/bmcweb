#pragma once
#include <http_request.hpp>
#include <http_response.hpp>
#include <http_utility.hpp>

namespace forward_unauthorized
{

static bool hasWebuiRoute = false;

inline void sendUnauthorized(std::string_view url, std::string_view userAgent,
                             std::string_view accept, crow::Response& res)
{
    // If it's a browser connecting, don't send the HTTP authenticate
    // header, to avoid possible CSRF attacks with basic auth
    if (http_helpers::requestPrefersHtml(accept))
    {
        // If we have a webui installed, redirect to that login page
        if (hasWebuiRoute)
        {
            res.result(boost::beast::http::status::temporary_redirect);
            res.addHeader("Location",
                          "/#/login?next=" + http_helpers::urlEncode(url));
        }
        else
        {
            // If we don't have a webui installed, just return a lame
            // unauthorized body
            res.result(boost::beast::http::status::unauthorized);
            res.body() = "Unauthorized";
        }
    }
    else
    {
        res.result(boost::beast::http::status::unauthorized);
        // only send the WWW-authenticate header if this isn't a xhr
        // from the browser.  Most scripts, tend to not set a user-agent header.
        // So key off that to know whether or not we need to suggest basic auth
        if (userAgent.empty())
        {
            res.addHeader("WWW-Authenticate", "Basic");
        }
    }
}
} // namespace forward_unauthorized
