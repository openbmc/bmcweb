#pragma once

#include <crow/http_request.h>
#include <crow/http_response.h>

#include <boost/container/flat_set.hpp>
#include <pam_authenticate.hpp>
#include <random>
#include <webassets_routes.hpp>

namespace crow
{

namespace token_authorization
{

inline const std::shared_ptr<crow::persistent_data::UserSession>
    performBasicAuth(boost::string_view auth_header)
{
    BMCWEB_LOG_DEBUG << "[Auth] Basic authentication";

    std::string authData;
    boost::string_view param = auth_header.substr(strlen("Basic "));
    if (!crow::utility::base64Decode(param, authData))
    {
        return nullptr;
    }
    std::size_t separator = authData.find(':');
    if (separator == std::string::npos)
    {
        return nullptr;
    }

    std::string user = authData.substr(0, separator);
    separator += 1;
    if (separator > authData.size())
    {
        return nullptr;
    }
    std::string pass = authData.substr(separator);

    BMCWEB_LOG_DEBUG << "[Auth] Authenticating user: " << user;

    if (!pamAuthenticateUser(user, pass))
    {
        return nullptr;
    }

    // TODO(ed) generateUserSession is a little expensive for basic
    // auth, as it generates some random identifiers that will never be
    // used.  This should have a "fast" path for when user tokens aren't
    // needed.
    // This whole flow needs to be revisited anyway, as we can't be
    // calling directly into pam for every request
    return persistent_data::SessionStore::getInstance().generateUserSession(
        user, crow::persistent_data::PersistenceType::SINGLE_REQUEST);
}

inline const std::shared_ptr<crow::persistent_data::UserSession>
    performTokenAuth(boost::string_view auth_header)
{
    BMCWEB_LOG_DEBUG << "[Auth] Token authentication";

    boost::string_view token = auth_header.substr(strlen("Token "));
    auto session =
        persistent_data::SessionStore::getInstance().loginSessionByToken(token);
    return session;
}

inline const std::shared_ptr<crow::persistent_data::UserSession>
    performXtokenAuth(const crow::Request& req)
{
    BMCWEB_LOG_DEBUG << "[Auth] X-Auth-Token authentication";

    boost::string_view token = req.getHeaderValue("X-Auth-Token");
    if (token.empty())
    {
        return nullptr;
    }
    auto session =
        persistent_data::SessionStore::getInstance().loginSessionByToken(token);
    return session;
}

inline const std::shared_ptr<crow::persistent_data::UserSession>
    performCookieAuth(const crow::Request& req)
{
    BMCWEB_LOG_DEBUG << "[Auth] Cookie authentication";

    boost::string_view cookieValue = req.getHeaderValue("Cookie");
    if (cookieValue.empty())
    {
        return nullptr;
    }

    auto startIndex = cookieValue.find("SESSION=");
    if (startIndex == std::string::npos)
    {
        return nullptr;
    }
    startIndex += sizeof("SESSION=") - 1;
    auto endIndex = cookieValue.find(";", startIndex);
    if (endIndex == std::string::npos)
    {
        endIndex = cookieValue.size();
    }
    boost::string_view authKey =
        cookieValue.substr(startIndex, endIndex - startIndex);

    const std::shared_ptr<crow::persistent_data::UserSession> session =
        persistent_data::SessionStore::getInstance().loginSessionByToken(
            authKey);
    if (session == nullptr)
    {
        return nullptr;
    }
#ifndef BMCWEB_INSECURE_DISABLE_CSRF_PREVENTION
    // RFC7231 defines methods that need csrf protection
    if (req.method() != "GET"_method)
    {
        boost::string_view csrf = req.getHeaderValue("X-XSRF-TOKEN");
        // Make sure both tokens are filled
        if (csrf.empty() || session->csrfToken.empty())
        {
            return nullptr;
        }
        // Reject if csrf token not available
        if (csrf != session->csrfToken)
        {
            return nullptr;
        }
    }
#endif
    return session;
}

// checks if request can be forwarded without authentication
inline bool isOnWhitelist(const crow::Request& req)
{
    // it's allowed to GET root node without authentica tion
    if ("GET"_method == req.method())
    {
        if (req.url == "/redfish/v1" || req.url == "/redfish/v1/" ||
            req.url == "/redfish" || req.url == "/redfish/" ||
            req.url == "/redfish/v1/odata" || req.url == "/redfish/v1/odata/")
        {
            return true;
        }

        if (crow::webassets::routes.find(std::string(req.url)) !=
                 crow::webassets::routes.end())
        {
            return true;
        }
    }

    // it's allowed to POST on session collection & login without
    // authentication
    if ("POST"_method == req.method())
    {
        if ((req.url == "/redfish/v1/SessionService/Sessions") ||
            (req.url == "/redfish/v1/SessionService/Sessions/") ||
            (req.url == "/login"))
        {
            return true;
        }
    }

    return false;
}

inline void authorizeRequest(crow::Request& req, Response& res)
{
    if (isOnWhitelist(req))
    {
        return;
    }

    req.session = performXtokenAuth(req);
    if (req.session == nullptr)
    {
        req.session = performCookieAuth(req);
    }
    if (req.session == nullptr)
    {
        boost::string_view authHeader = req.getHeaderValue("Authorization");
        if (!authHeader.empty())
        {
            // Reject any kind of auth other than basic or token
            if (boost::starts_with(authHeader, "Token "))
            {
                req.session = performTokenAuth(authHeader);
            }
            else if (boost::starts_with(authHeader, "Basic "))
            {
                req.session = performBasicAuth(authHeader);
            }
        }
    }

    if (req.session == nullptr)
    {
        BMCWEB_LOG_WARNING << "[Auth] authorization failed";

        // If it's a browser connecting, don't send the HTTP authenticate
        // header, to avoid possible CSRF attacks with basic auth
        if (http_helpers::requestPrefersHtml(req))
        {
            res.result(boost::beast::http::status::temporary_redirect);
            res.addHeader("Location", "/#/login");
        }
        else
        {
            res.result(boost::beast::http::status::unauthorized);
            // only send the WWW-authenticate header if this isn't a xhr
            // from the browser.  most scripts,
            if (req.getHeaderValue("User-Agent").empty())
            {
                res.addHeader("WWW-Authenticate", "Basic");
            }
        }

        res.end();
        return;
    }

    // TODO get user privileges here and propagate it via MW Context
    // else let the request continue unharmed
}

inline void cleanupTemporarySession(Request& req)
{
    // TODO(ed) THis should really be handled by the persistent data
    // middleware, but because it is upstream, it doesn't have access to the
    // session information.  Should the data middleware persist the current
    // user session?
    if (req.session != nullptr &&
        req.session->persistence ==
            crow::persistent_data::PersistenceType::SINGLE_REQUEST)
    {
        persistent_data::SessionStore::getInstance().removeSession(req.session);
    }
}

} // namespace token_authorization
} // namespace crow
