#pragma once

#include "webroutes.hpp"

#include <app.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/container/flat_set.hpp>
#include <common.hpp>
#include <forward_unauthorized.hpp>
#include <http_request.hpp>
#include <http_response.hpp>
#include <http_utility.hpp>
#include <pam_authenticate.hpp>

#include <random>
#include <utility>

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

#ifdef BMCWEB_ENABLE_BASIC_AUTHENTICATION
static std::shared_ptr<persistent_data::UserSession>
    performBasicAuth(const boost::asio::ip::address& clientIp,
                     std::string_view authHeader)
{
    BMCWEB_LOG_DEBUG << "[AuthMiddleware] Basic authentication";

    std::string authData;
    std::string_view param = authHeader.substr(strlen("Basic "));
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
    BMCWEB_LOG_DEBUG << "[AuthMiddleware] User IPAddress: "
                     << clientIp.to_string();

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
    std::string unsupportedClientId = "";
    return persistent_data::SessionStore::getInstance().generateUserSession(
        user, clientIp.to_string(), unsupportedClientId,
        persistent_data::PersistenceType::SINGLE_REQUEST, isConfigureSelfOnly);
}
#endif

#ifdef BMCWEB_ENABLE_SESSION_AUTHENTICATION
static std::shared_ptr<persistent_data::UserSession>
    performTokenAuth(std::string_view authHeader)
{
    BMCWEB_LOG_DEBUG << "[AuthMiddleware] Token authentication";

    std::string_view token = authHeader.substr(strlen("Token "));
    auto session =
        persistent_data::SessionStore::getInstance().loginSessionByToken(token);
    return session;
}
#endif

#ifdef BMCWEB_ENABLE_XTOKEN_AUTHENTICATION
static std::shared_ptr<persistent_data::UserSession>
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
#endif

#ifdef BMCWEB_ENABLE_COOKIE_AUTHENTICATION
static std::shared_ptr<persistent_data::UserSession>
    performCookieAuth(const crow::Request& req,
    boost::beast::http::verb method)
{
    BMCWEB_LOG_DEBUG << "[AuthMiddleware] Cookie authentication";

    std::string_view cookieValue = req.getHeaderValue("Cookie");
    if (cookieValue.empty())
    {
        BMCWEB_LOG_DEBUG << "JEBR cookie value mt";
        return nullptr;
    }

    auto startIndex = cookieValue.find("SESSION=");
    if (startIndex == std::string::npos)
    {
        BMCWEB_LOG_DEBUG << "JEB SESSION found found in cookie";
        return nullptr;
    }
    startIndex += sizeof("SESSION=") - 1;
    auto endIndex = cookieValue.find(';', startIndex);
    if (endIndex == std::string::npos)
    {
        endIndex = cookieValue.size();
    }
    std::string_view authKey =
        cookieValue.substr(startIndex, endIndex - startIndex);

    std::shared_ptr<persistent_data::UserSession> session =
        persistent_data::SessionStore::getInstance().loginSessionByToken(
            authKey);
    if (session == nullptr)
    {
        BMCWEB_LOG_DEBUG << "JEBR cookie key not found";
        return nullptr;
    }
#ifndef BMCWEB_INSECURE_DISABLE_CSRF_PREVENTION
    // RFC7231 defines methods that need csrf protection
    if (method != boost::beast::http::verb::get)
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
#endif

#ifdef BMCWEB_ENABLE_MUTUAL_TLS_AUTHENTICATION
static std::shared_ptr<persistent_data::UserSession>
    performTLSAuth(const crow::Request& req, Response& res,
                   const std::shared_ptr<persistent_data::UserSession>& session)
{
    {
        // set cookie only if this is req from the browser.
        if (req.getHeaderValue("User-Agent").empty())
        {
            BMCWEB_LOG_DEBUG << " TLS session: " << session->uniqueId
                             << " will be used for this request.";
            return session;
        }
        std::string_view cookieValue = req.getHeaderValue("Cookie");
        if (cookieValue.empty() ||
            cookieValue.find("SESSION=") == std::string::npos)
        {
            // TODO: change this to not switch to cookie auth
            res.addHeader(
                "Set-Cookie",
                "XSRF-TOKEN=" + session->csrfToken +
                    "; SameSite=Strict; Secure\r\nSet-Cookie: SESSION=" +
                    session->sessionToken +
                    "; SameSite=Strict; Secure; HttpOnly\r\nSet-Cookie: "
                    "IsAuthenticated=true; Secure");
            BMCWEB_LOG_DEBUG << " TLS session: " << session->uniqueId
                             << " with cookie will be used for this request.";
            return session;
        }
    }
    return nullptr;
}
#endif

// checks if request can be forwarded without authentication
static bool isOnWhitelist(const crow::Request& req,
    boost::beast::http::verb method)
{
    BMCWEB_LOG_DEBUG << "isOnWhitelist";
    if (boost::beast::http::verb::get == method)
    {
        BMCWEB_LOG_DEBUG << "about to redref req:";
        BMCWEB_LOG_DEBUG << "req.url=" << req.url;
        if (req.url == "/redfish/v1" || req.url == "/redfish/v1/" ||
            req.url == "/redfish" || req.url == "/redfish/" ||
            req.url == "/redfish/v1/odata" || req.url == "/redfish/v1/odata/")
        {
            BMCWEB_LOG_DEBUG << "isOnWhitelist, leaving";
            return true;
        }
        if (crow::webroutes::routes.find(std::string(req.url)) !=
            crow::webroutes::routes.end())
        {
            return true;
        }
    }

    BMCWEB_LOG_DEBUG << "white list middle?";
    // it's allowed to POST on session collection & login without
    // authentication
    if (boost::beast::http::verb::post == method)
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

static std::shared_ptr<persistent_data::UserSession> authenticate(
    crow::Request& req, Response& res,
    boost::beast::http::verb method,
    // const boost::asio::ip::address& clientIp,
    [[maybe_unused]] const std::shared_ptr<persistent_data::UserSession>& session)
{
    BMCWEB_LOG_DEBUG << "JEBR starting auth";
    if (isOnWhitelist(req, method))
    {
        BMCWEB_LOG_DEBUG << "JEBR in on white on list";
        return nullptr;
    }
    BMCWEB_LOG_DEBUG << "not on white list";
    const persistent_data::AuthConfigMethods& authMethodsConfig =
        persistent_data::SessionStore::getInstance().getAuthMethodsConfig();

#ifdef BMCWEB_ENABLE_MUTUAL_TLS_AUTHENTICATION
    if (authMethodsConfig.tls)
    {
        std::shared_ptr<persistent_data::UserSession> session
            = performTLSAuth(req, res, session);
        if (session) return session;
    }
#endif
#ifdef BMCWEB_ENABLE_XTOKEN_AUTHENTICATION
    if (authMethodsConfig.xtoken)
    {
        std::shared_ptr<persistent_data::UserSession> session
         = performXtokenAuth(req);
        if (session) return session;
    }
#endif
#ifdef BMCWEB_ENABLE_COOKIE_AUTHENTICATION
    if (authMethodsConfig.cookie)
    {
        std::shared_ptr<persistent_data::UserSession> session
          = performCookieAuth(req, method);
        if (session) return session;
    }
#endif
    BMCWEB_LOG_WARNING << "JEBR AUTH: BEFORE getHeaderValue";
    std::string_view authHeader = req.getHeaderValue("Authorization");
    BMCWEB_LOG_WARNING << "JEBR AUTH: after get headerValue"; 

    if (!authHeader.empty())
    {
        std::shared_ptr<persistent_data::UserSession> session
         = performXtokenAuth(req);
        // Reject any kind of auth other than basic or token
        if (boost::starts_with(authHeader, "Token ") &&
            authMethodsConfig.sessionToken)
        {
#ifdef BMCWEB_ENABLE_SESSION_AUTHENTICATION
            session = performTokenAuth(authHeader);
#endif
        }
        else if (boost::starts_with(authHeader, "Basic ") &&
                 authMethodsConfig.basic)
        {
#ifdef BMCWEB_ENABLE_BASIC_AUTHENTICATION
            session = performBasicAuth(req.ipAddress, authHeader);
#endif
        }
        if (session) return session;
     }
     BMCWEB_LOG_WARNING << "[AuthMiddleware] authorization failed";
     forward_unauthorized::sendUnauthorized(req, res);
     res.end();
     return nullptr;
}

} // namespace authorization
} // namespace crow
