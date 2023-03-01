#pragma once

#include "common.hpp"
#include "forward_unauthorized.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "http_utility.hpp"
#include "pam_authenticate.hpp"
#include "webroutes.hpp"

#include <random>
#include <utility>

namespace crow
{

namespace authentication
{

inline void cleanupTempSession(const Request& req)
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

    if (!authHeader.starts_with("Basic "))
    {
        return nullptr;
    }

    std::string_view param = authHeader.substr(strlen("Basic "));
    std::string authData;

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
    return persistent_data::SessionStore::getInstance().generateUserSession(
        user, clientIp, std::nullopt,
        persistent_data::PersistenceType::SINGLE_REQUEST, isConfigureSelfOnly);
}
#endif

#ifdef BMCWEB_ENABLE_SESSION_AUTHENTICATION
static std::shared_ptr<persistent_data::UserSession>
    performTokenAuth(std::string_view authHeader)
{
    BMCWEB_LOG_DEBUG << "[AuthMiddleware] Token authentication";
    if (!authHeader.starts_with("Token "))
    {
        return nullptr;
    }
    std::string_view token = authHeader.substr(strlen("Token "));
    auto sessionOut =
        persistent_data::SessionStore::getInstance().loginSessionByToken(token);
    return sessionOut;
}
#endif

#ifdef BMCWEB_ENABLE_XTOKEN_AUTHENTICATION
static std::shared_ptr<persistent_data::UserSession>
    performXtokenAuth(const boost::beast::http::header<true>& reqHeader)
{
    BMCWEB_LOG_DEBUG << "[AuthMiddleware] X-Auth-Token authentication";

    std::string_view token = reqHeader["X-Auth-Token"];
    if (token.empty())
    {
        return nullptr;
    }
    auto sessionOut =
        persistent_data::SessionStore::getInstance().loginSessionByToken(token);
    return sessionOut;
}
#endif

#ifdef BMCWEB_ENABLE_COOKIE_AUTHENTICATION
static std::shared_ptr<persistent_data::UserSession>
    performCookieAuth(boost::beast::http::verb method,
                      const boost::beast::http::header<true>& reqHeader)
{
    BMCWEB_LOG_DEBUG << "[AuthMiddleware] Cookie authentication";

    std::string_view cookieValue = reqHeader["Cookie"];
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
    auto endIndex = cookieValue.find(';', startIndex);
    if (endIndex == std::string::npos)
    {
        endIndex = cookieValue.size();
    }
    std::string_view authKey =
        cookieValue.substr(startIndex, endIndex - startIndex);

    std::shared_ptr<persistent_data::UserSession> sessionOut =
        persistent_data::SessionStore::getInstance().loginSessionByToken(
            authKey);
    if (sessionOut == nullptr)
    {
        return nullptr;
    }
#ifndef BMCWEB_INSECURE_DISABLE_CSRF_PREVENTION
    // RFC7231 defines methods that need csrf protection
    if (method != boost::beast::http::verb::get)
    {
        std::string_view csrf = reqHeader["X-XSRF-TOKEN"];
        // Make sure both tokens are filled
        if (csrf.empty() || sessionOut->csrfToken.empty())
        {
            return nullptr;
        }

        if (csrf.size() != persistent_data::sessionTokenSize)
        {
            return nullptr;
        }
        // Reject if csrf token not available
        if (!crow::utility::constantTimeStringCompare(csrf,
                                                      sessionOut->csrfToken))
        {
            return nullptr;
        }
    }
#endif
    return sessionOut;
}
#endif

#ifdef BMCWEB_ENABLE_MUTUAL_TLS_AUTHENTICATION
static std::shared_ptr<persistent_data::UserSession>
    performTLSAuth(Response& res,
                   const boost::beast::http::header<true>& reqHeader,
                   const std::weak_ptr<persistent_data::UserSession>& session)
{
    if (auto sp = session.lock())
    {
        // set cookie only if this is req from the browser.
        if (reqHeader["User-Agent"].empty())
        {
            BMCWEB_LOG_DEBUG << " TLS session: " << sp->uniqueId
                             << " will be used for this request.";
            return sp;
        }
        // TODO: change this to not switch to cookie auth
        res.addHeader("Set-Cookie",
                      "XSRF-TOKEN=" + sp->csrfToken +
                          "; SameSite=Strict; Secure\r\nSet-Cookie: SESSION=" +
                          sp->sessionToken +
                          "; SameSite=Strict; Secure; HttpOnly\r\nSet-Cookie: "
                          "IsAuthenticated=true; Secure");
        BMCWEB_LOG_DEBUG << " TLS session: " << sp->uniqueId
                         << " with cookie will be used for this request.";
        return sp;
    }
    return nullptr;
}
#endif

// checks if request can be forwarded without authentication
[[maybe_unused]] static bool isOnAllowlist(std::string_view url,
                                           boost::beast::http::verb method)
{
    if (boost::beast::http::verb::get == method)
    {
        if (url == "/redfish/v1" || url == "/redfish/v1/" ||
            url == "/redfish" || url == "/redfish/" ||
            url == "/redfish/v1/odata" || url == "/redfish/v1/odata/")
        {
            return true;
        }
        if (std::find(crow::webroutes::routes.begin(),
                      crow::webroutes::routes.end(),
                      url) != crow::webroutes::routes.end())
        {
            return true;
        }
    }

    // it's allowed to POST on session collection & login without
    // authentication
    if (boost::beast::http::verb::post == method)
    {
        if ((url == "/redfish/v1/SessionService/Sessions") ||
            (url == "/redfish/v1/SessionService/Sessions/") ||
            (url == "/redfish/v1/SessionService/Sessions/Members") ||
            (url == "/redfish/v1/SessionService/Sessions/Members/") ||
            (url == "/login"))
        {
            return true;
        }
    }

    return false;
}

[[maybe_unused]] static std::shared_ptr<persistent_data::UserSession>
    authenticate(
        const boost::asio::ip::address& ipAddress [[maybe_unused]],
        Response& res [[maybe_unused]],
        boost::beast::http::verb method [[maybe_unused]],
        const boost::beast::http::header<true>& reqHeader,
        [[maybe_unused]] const std::shared_ptr<persistent_data::UserSession>&
            session)
{
    const persistent_data::AuthConfigMethods& authMethodsConfig =
        persistent_data::SessionStore::getInstance().getAuthMethodsConfig();

    std::shared_ptr<persistent_data::UserSession> sessionOut = nullptr;
#ifdef BMCWEB_ENABLE_MUTUAL_TLS_AUTHENTICATION
    if (authMethodsConfig.tls)
    {
        sessionOut = performTLSAuth(res, reqHeader, session);
    }
#endif
#ifdef BMCWEB_ENABLE_XTOKEN_AUTHENTICATION
    if (sessionOut == nullptr && authMethodsConfig.xtoken)
    {
        sessionOut = performXtokenAuth(reqHeader);
    }
#endif
#ifdef BMCWEB_ENABLE_COOKIE_AUTHENTICATION
    if (sessionOut == nullptr && authMethodsConfig.cookie)
    {
        sessionOut = performCookieAuth(method, reqHeader);
    }
#endif
    std::string_view authHeader = reqHeader["Authorization"];
    BMCWEB_LOG_DEBUG << "authHeader=" << authHeader;

    if (sessionOut == nullptr && authMethodsConfig.sessionToken)
    {
#ifdef BMCWEB_ENABLE_SESSION_AUTHENTICATION
        sessionOut = performTokenAuth(authHeader);
#endif
    }
    if (sessionOut == nullptr && authMethodsConfig.basic)
    {
#ifdef BMCWEB_ENABLE_BASIC_AUTHENTICATION
        sessionOut = performBasicAuth(ipAddress, authHeader);
#endif
    }
    if (sessionOut != nullptr)
    {
        return sessionOut;
    }

    return nullptr;
}

} // namespace authentication
} // namespace crow
