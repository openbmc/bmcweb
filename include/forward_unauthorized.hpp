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

        bool isBrowser = true;

        // Try to "rule out" a browser by using some common scripting libraries;
        // In practice, this is unfortunate that we have to do this, but we
        // don't want a www-authenticate login prompt to show up on a browser in
        // the event of a bad request.
        for (const std::string_view agent :
             std::to_array({"curl/", "python-requests/", "wget/"}))
        {
            if (boost::istarts_with(userAgent, agent))
            {
                isBrowser = false;
                break;
            }
        }

        // All browsers will set a user-agent, so it's empty, it must be a
        // script/application
        if (userAgent.empty())
        {
            isBrowser = false;
        }

        // If there's any chance this is a browser, we don't want to propose
        // basic auth
        if (!isBrowser)
        {
            // If basic auth is disabled, we shouldn't propose it as an auth
            // option.
            if (persistent_data::SessionStore::getInstance()
                    .getAuthMethodsConfig()
                    .basic)
            {
                res.addHeader("WWW-Authenticate", "Basic");
            }
        }
    }
}
} // namespace forward_unauthorized
