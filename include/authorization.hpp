#pragma once

#include "webroutes.hpp"

#include <app.h>
#include <common.h>
#include <http_request.h>
#include <http_response.h>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/container/flat_set.hpp>
#include <http_utility.hpp>
#include <pam_authenticate.hpp>

#include <random>

namespace crow
{

namespace authorization
{

static void cleanupTempSession(Request& req)
{
    // TODO(ed) THis should really be handled by the persistent data
    // middleware, but because it is upstream, it doesn't have access to the
    // session information.  Should the data middleware persist the current
    // user session?
    if (req.session != nullptr &&
        req.session->persistence ==
            persistent_data::PersistenceType::SINGLE_REQUEST)
    {
        persistent_data::SessionStore::getInstance().removeSession(req.session);
    }
}

static const std::shared_ptr<persistent_data::UserSession>
    performBasicAuth(std::string_view auth_header)
{
    BMCWEB_LOG_DEBUG << "[AuthMiddleware] Basic authentication";

    std::string authData;
    std::string_view param = auth_header.substr(strlen("Basic "));
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

    BMCWEB_LOG_DEBUG << "[AuthMiddleware] Authenticating user: " << user;

    int pamrc = pamAuthenticateUser(user, pass);
    bool isConfigureSelfOnly = pamrc == PAM_NEW_AUTHTOK_REQD;
    if ((pamrc != PAM_SUCCESS) && !isConfigureSelfOnly)
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
        user, persistent_data::PersistenceType::SINGLE_REQUEST,
        isConfigureSelfOnly);
}

static const std::shared_ptr<persistent_data::UserSession>
    performTokenAuth(std::string_view auth_header)
{
    BMCWEB_LOG_DEBUG << "[AuthMiddleware] Token authentication";

    std::string_view token = auth_header.substr(strlen("Token "));
    auto session =
        persistent_data::SessionStore::getInstance().loginSessionByToken(token);
    return session;
}

static const std::shared_ptr<persistent_data::UserSession>
    performXtokenAuth(const crow::Request& req)
{
    BMCWEB_LOG_DEBUG << "[AuthMiddleware] X-Auth-Token authentication";

    std::string_view token = req.getHeaderValue("X-Auth-Token");
    if (token.empty())
    {
        return nullptr;
    }
    auto session =
        persistent_data::SessionStore::getInstance().loginSessionByToken(token);
    return session;
}

static const std::shared_ptr<persistent_data::UserSession>
    performCookieAuth(const crow::Request& req)
{
    BMCWEB_LOG_DEBUG << "[AuthMiddleware] Cookie authentication";

    std::string_view cookieValue = req.getHeaderValue("Cookie");
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
    std::string_view authKey =
        cookieValue.substr(startIndex, endIndex - startIndex);

    const std::shared_ptr<persistent_data::UserSession> session =
        persistent_data::SessionStore::getInstance().loginSessionByToken(
            authKey);
    if (session == nullptr)
    {
        return nullptr;
    }
#ifndef BMCWEB_INSECURE_DISABLE_CSRF_PREVENTION
    // RFC7231 defines methods that need csrf protection
    if (req.method() != boost::beast::http::verb::get)
    {
        std::string_view csrf = req.getHeaderValue("X-XSRF-TOKEN");
        // Make sure both tokens are filled
        if (csrf.empty() || session->csrfToken.empty())
        {
            return nullptr;
        }

        if (csrf.size() != persistent_data::sessionTokenSize)
        {
            return nullptr;
        }
        // Reject if csrf token not available
        if (!crow::utility::constantTimeStringCompare(csrf, session->csrfToken))
        {
            return nullptr;
        }
    }
#endif
    return session;
}

static const std::shared_ptr<persistent_data::UserSession>
    performTLSAuth(const crow::Request& req, Response& res,
                   std::weak_ptr<persistent_data::UserSession> session)
{
#ifdef BMCWEB_ENABLE_MUTUAL_TLS_AUTHENTICATION
    if (auto sp = session.lock())
    {
        // set cookie only if this is req from the browser.
        if (req.getHeaderValue("User-Agent").empty())
        {
            BMCWEB_LOG_DEBUG << " TLS session: " << sp->uniqueId
                             << " will be used for this request.";
            return sp;
        }
        else
        {
            std::string_view cookieValue = req.getHeaderValue("Cookie");
            if (cookieValue.empty() ||
                cookieValue.find("SESSION=") == std::string::npos)
            {
                // TODO: change this to not switch to cookie auth
                res.addHeader(
                    "Set-Cookie",
                    "XSRF-TOKEN=" + sp->csrfToken +
                        "; Secure\r\nSet-Cookie: SESSION=" + sp->sessionToken +
                        "; Secure; HttpOnly\r\nSet-Cookie: "
                        "IsAuthenticated=true; Secure");
                BMCWEB_LOG_DEBUG
                    << " TLS session: " << sp->uniqueId
                    << " with cookie will be used for this request.";
                return sp;
            }
        }
    }
#endif
    return nullptr;
}

// checks if request can be forwarded without authentication
static bool isOnWhitelist(const crow::Request& req)
{
    // it's allowed to GET root node without authentication
    if (boost::beast::http::verb::get == req.method())
    {
        if (req.url == "/redfish/v1" || req.url == "/redfish/v1/" ||
            req.url == "/redfish" || req.url == "/redfish/" ||
            req.url == "/redfish/v1/odata" || req.url == "/redfish/v1/odata/")
        {
            return true;
        }
        else if (crow::webroutes::routes.find(std::string(req.url)) !=
                 crow::webroutes::routes.end())
        {
            return true;
        }
    }

    // it's allowed to POST on session collection & login without
    // authentication
    if (boost::beast::http::verb::post == req.method())
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

static void
    authenticate(crow::Request& req, Response& res,
                 std::weak_ptr<persistent_data::UserSession> session)
{
    if (isOnWhitelist(req))
    {
        return;
    }

    const persistent_data::AuthConfigMethods& authMethodsConfig =
        persistent_data::SessionStore::getInstance().getAuthMethodsConfig();

    if (req.session == nullptr && authMethodsConfig.tls)
    {
        req.session = performTLSAuth(req, res, session);
    }
    if (req.session == nullptr && authMethodsConfig.xtoken)
    {
        req.session = performXtokenAuth(req);
    }
    if (req.session == nullptr && authMethodsConfig.cookie)
    {
        req.session = performCookieAuth(req);
    }
    if (req.session == nullptr)
    {
        std::string_view authHeader = req.getHeaderValue("Authorization");
        if (!authHeader.empty())
        {
            // Reject any kind of auth other than basic or token
            if (boost::starts_with(authHeader, "Token ") &&
                authMethodsConfig.sessionToken)
            {
                req.session = performTokenAuth(authHeader);
            }
            else if (boost::starts_with(authHeader, "Basic ") &&
                     authMethodsConfig.basic)
            {
                req.session = performBasicAuth(authHeader);
            }
        }
    }

    if (req.session == nullptr)
    {
        BMCWEB_LOG_WARNING << "[AuthMiddleware] authorization failed";

        // If it's a browser connecting, don't send the HTTP authenticate
        // header, to avoid possible CSRF attacks with basic auth
        if (http_helpers::requestPrefersHtml(req))
        {
            res.result(boost::beast::http::status::temporary_redirect);
            res.addHeader("Location",
                          "/#/login?next=" + http_helpers::urlEncode(req.url));
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
}

} // namespace authorization
} // namespace crow
