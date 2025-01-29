#pragma once

#include "cookies.hpp"
#include "forward_unauthorized.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "http_utility.hpp"
#include "pam_authenticate.hpp"
#include "webroutes.hpp"

#include <boost/container/flat_set.hpp>

#include <random>
#include <utility>

namespace crow
{

namespace authentication
{

inline std::shared_ptr<persistent_data::UserSession> performBasicAuth(
    const boost::asio::ip::address& clientIp, std::string_view authHeader)
{
    BMCWEB_LOG_DEBUG("[AuthMiddleware] Basic authentication");

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

    BMCWEB_LOG_DEBUG("[AuthMiddleware] Authenticating user: {}", user);
    BMCWEB_LOG_DEBUG("[AuthMiddleware] User IPAddress: {}",
                     clientIp.to_string());

    int pamrc = pamAuthenticateUser(user, pass, std::nullopt);
    bool isConfigureSelfOnly = pamrc == PAM_NEW_AUTHTOK_REQD;
    if ((pamrc != PAM_SUCCESS) && !isConfigureSelfOnly)
    {
        return nullptr;
    }

    // Attempt to locate an existing Basic Auth session from the same ip address
    // and user
    for (auto& session :
         persistent_data::SessionStore::getInstance().getSessions())
    {
        if (session->sessionType != persistent_data::SessionType::Basic)
        {
            continue;
        }
        if (session->clientIp != redfish::ip_util::toString(clientIp))
        {
            continue;
        }
        if (session->username != user)
        {
            continue;
        }
        return session;
    }

    return persistent_data::SessionStore::getInstance().generateUserSession(
        user, clientIp, std::nullopt, persistent_data::SessionType::Basic,
        isConfigureSelfOnly);
}

inline std::shared_ptr<persistent_data::UserSession>
    performTokenAuth(std::string_view authHeader)
{
    BMCWEB_LOG_DEBUG("[AuthMiddleware] Token authentication");
    if (!authHeader.starts_with("Token "))
    {
        return nullptr;
    }
    std::string_view token = authHeader.substr(strlen("Token "));
    auto sessionOut =
        persistent_data::SessionStore::getInstance().loginSessionByToken(token);
    return sessionOut;
}

inline std::shared_ptr<persistent_data::UserSession>
    performXtokenAuth(const boost::beast::http::header<true>& reqHeader)
{
    BMCWEB_LOG_DEBUG("[AuthMiddleware] X-Auth-Token authentication");

    std::string_view token = reqHeader["X-Auth-Token"];
    if (token.empty())
    {
        return nullptr;
    }
    auto sessionOut =
        persistent_data::SessionStore::getInstance().loginSessionByToken(token);
    return sessionOut;
}

inline std::shared_ptr<persistent_data::UserSession>
    performCookieAuth(boost::beast::http::verb method [[maybe_unused]],
                      const boost::beast::http::header<true>& reqHeader)
{
    using headers = boost::beast::http::header<true>;
    std::pair<headers::const_iterator, headers::const_iterator> cookies =
        reqHeader.equal_range(boost::beast::http::field::cookie);

    for (auto it = cookies.first; it != cookies.second; it++)
    {
        std::string_view cookieValue = it->value();
        BMCWEB_LOG_DEBUG("Checking cookie {}", cookieValue);
        auto startIndex = cookieValue.find("BMCWEB-SESSION=");
        if (startIndex == std::string::npos)
        {
            BMCWEB_LOG_DEBUG(
                "Cookie was present, but didn't look like a session {}",
                cookieValue);
            continue;
        }
        startIndex += sizeof("BMCWEB-SESSION=") - 1;
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
        sessionOut->cookieAuth = true;

        if constexpr (!BMCWEB_INSECURE_DISABLE_CSRF)
        {
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
                if (!crow::utility::constantTimeStringCompare(
                        csrf, sessionOut->csrfToken))
                {
                    return nullptr;
                }
            }
        }
        return sessionOut;
    }
    return nullptr;
}

inline std::shared_ptr<persistent_data::UserSession> performTLSAuth(
    Response& res, const boost::beast::http::header<true>& reqHeader,
    const std::weak_ptr<persistent_data::UserSession>& session)
{
    if (auto sp = session.lock())
    {
        // set cookie only if this is req from the browser.
        if (reqHeader["User-Agent"].empty())
        {
            BMCWEB_LOG_DEBUG(" TLS session: {} will be used for this request.",
                             sp->uniqueId);
            return sp;
        }
        // TODO: change this to not switch to cookie auth
        bmcweb::setSessionCookies(res, *sp);
        res.addHeader(boost::beast::http::field::set_cookie,
                      "IsAuthenticated=true; Secure");
        BMCWEB_LOG_DEBUG(
            " TLS session: {} with cookie will be used for this request.",
            sp->uniqueId);
        return sp;
    }
    return nullptr;
}

// checks if request can be forwarded without authentication
inline bool isOnAllowlist(std::string_view url, boost::beast::http::verb method)
{
    // Handle the case where the router registers routes as both ending with /
    // and not.
    if (url.ends_with('/') && url != "/")
    {
        url.remove_suffix(1);
    }
    if (boost::beast::http::verb::get == method)
    {
        if ((url == "/redfish") || //
            (url == "/redfish/v1") || //
            (url == "/redfish/v1/odata") || //
            (url == "/redfish/v1/$metadata"))
        {
            return true;
        }
        if (crow::webroutes::routes.find(std::string(url)) !=
            crow::webroutes::routes.end())
        {
            return true;
        }
    }

    // This patch is allowed for service user, without authorization to upload
    // unauthenticated ACF.
    if (boost::beast::http::verb::patch == method)
    {
        if (url == "/redfish/v1/AccountService/Accounts/service")
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

inline std::shared_ptr<persistent_data::UserSession> authenticate(
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
    if constexpr (BMCWEB_MUTUAL_TLS_AUTH)
    {
        if (authMethodsConfig.tls)
        {
            sessionOut = performTLSAuth(res, reqHeader, session);
        }
    }
    if constexpr (BMCWEB_XTOKEN_AUTH)
    {
        if (sessionOut == nullptr && authMethodsConfig.xtoken)
        {
            sessionOut = performXtokenAuth(reqHeader);
        }
    }
    if constexpr (BMCWEB_COOKIE_AUTH)
    {
        if (sessionOut == nullptr && authMethodsConfig.cookie)
        {
            sessionOut = performCookieAuth(method, reqHeader);
        }
    }
    std::string_view authHeader = reqHeader["Authorization"];
    BMCWEB_LOG_DEBUG("authHeader={}", authHeader);
    if constexpr (BMCWEB_SESSION_AUTH)
    {
        if (sessionOut == nullptr && authMethodsConfig.sessionToken)
        {
            sessionOut = performTokenAuth(authHeader);
        }
    }
    if constexpr (BMCWEB_BASIC_AUTH)
    {
        if (sessionOut == nullptr && authMethodsConfig.basic)
        {
            sessionOut = performBasicAuth(ipAddress, authHeader);
        }
    }
    if (sessionOut != nullptr)
    {
        return sessionOut;
    }

    return nullptr;
}

} // namespace authentication
} // namespace crow
