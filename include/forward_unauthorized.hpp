#pragma once
#include "http_request.hpp"
#include "http_response.hpp"
#include "http_utility.hpp"

namespace forward_unauthorized
{

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static bool hasWebuiRoute = false;

inline void sendUnauthorized(std::string_view url,
                             std::string_view xRequestedWith,
                             std::string_view accept,
                             std::string_view userAgent, crow::Response& res)
{
    // If it's a browser connecting, don't send the HTTP authenticate
    // header, to avoid possible CSRF attacks with basic auth
    if (http_helpers::isContentTypeAllowed(
            accept, http_helpers::ContentType::HTML, false /*allowWildcard*/))
    {
        // If we have a webui installed, redirect to that login page
        if (hasWebuiRoute)
        {
            res.result(boost::beast::http::status::temporary_redirect);
            res.addHeader(boost::beast::http::field::location,
                          "/#/login?next=" + http_helpers::urlEncode(url));
            return;
        }
        // If we don't have a webui installed, just return an unauthorized
        // body
        res.result(boost::beast::http::status::unauthorized);
        res.body() = "Unauthorized";
        return;
    }

    // If unauthorized JSON request sent from WebUI, return 401 status
    if (http_helpers::isContentTypeAllowed(
            accept, http_helpers::ContentType::JSON, false) &&
        http_helpers::checkUserAgent(userAgent, true))
    {
        res.result(boost::beast::http::status::unauthorized);
        res.body() = "Unauthorized";
        return;
    }

    res.result(boost::beast::http::status::unauthorized);

    // XHR requests from a browser will set the X-Requested-With header when
    // doing their requests, even though they might not be requesting html.
    if (!xRequestedWith.empty())
    {
        return;
    }
    // if basic auth is disabled, don't propose it.
    if (!persistent_data::SessionStore::getInstance()
             .getAuthMethodsConfig()
             .basic)
    {
        return;
    }
    res.addHeader(boost::beast::http::field::www_authenticate, "Basic");
}
} // namespace forward_unauthorized
