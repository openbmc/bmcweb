#pragma once

#include <crow/app.h>
#include <crow/common.h>
#include <crow/http_request.h>
#include <crow/http_response.h>

#include <boost/container/flat_set.hpp>
#include <pam_authenticate.hpp>
#include <persistent_data_middleware.hpp>
#include <random>
#include <webassets.hpp>

namespace crow
{

namespace token_authorization
{

class Middleware
{
  public:
    struct Context
    {
        std::shared_ptr<crow::persistent_data::UserSession> session;
    };

    void beforeHandle(crow::Request& req, Response& res, Context& ctx)
    {
        if (isOnWhitelist(req))
        {
            return;
        }

        ctx.session = performXtokenAuth(req);
        if (ctx.session == nullptr)
        {
            ctx.session = performCookieAuth(req);
        }
        if (ctx.session == nullptr)
        {
            boost::string_view authHeader = req.getHeaderValue("Authorization");
            if (!authHeader.empty())
            {
                // Reject any kind of auth other than basic or token
                if (boost::starts_with(authHeader, "Token "))
                {
                    ctx.session = performTokenAuth(authHeader);
                }
                else if (boost::starts_with(authHeader, "Basic "))
                {
                    ctx.session = performBasicAuth(authHeader);
                }
            }
        }

        if (ctx.session == nullptr)
        {
            BMCWEB_LOG_WARNING << "[AuthMiddleware] authorization failed";

            // If it's a browser connecting, don't send the HTTP authenticate
            // header, to avoid possible CSRF attacks with basic auth
            if (http_helpers::requestPrefersHtml(req))
            {
                res.result(boost::beast::http::status::temporary_redirect);
                res.addHeader("Location", "/#/login?next=" +
                                              http_helpers::urlEncode(req.url));
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

    template <typename AllContext>
    void afterHandle(Request& req, Response& res, Context& ctx,
                     AllContext& allctx)
    {
        // TODO(ed) THis should really be handled by the persistent data
        // middleware, but because it is upstream, it doesn't have access to the
        // session information.  Should the data middleware persist the current
        // user session?
        if (ctx.session != nullptr &&
            ctx.session->persistence ==
                crow::persistent_data::PersistenceType::SINGLE_REQUEST)
        {
            persistent_data::SessionStore::getInstance().removeSession(
                ctx.session);
        }
    }

  private:
    const std::shared_ptr<crow::persistent_data::UserSession>
        performBasicAuth(boost::string_view auth_header) const
    {
        BMCWEB_LOG_DEBUG << "[AuthMiddleware] Basic authentication";

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

        BMCWEB_LOG_DEBUG << "[AuthMiddleware] Authenticating user: " << user;

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

    const std::shared_ptr<crow::persistent_data::UserSession>
        performTokenAuth(boost::string_view auth_header) const
    {
        BMCWEB_LOG_DEBUG << "[AuthMiddleware] Token authentication";

        boost::string_view token = auth_header.substr(strlen("Token "));
        auto session =
            persistent_data::SessionStore::getInstance().loginSessionByToken(
                token);
        return session;
    }

    const std::shared_ptr<crow::persistent_data::UserSession>
        performXtokenAuth(const crow::Request& req) const
    {
        BMCWEB_LOG_DEBUG << "[AuthMiddleware] X-Auth-Token authentication";

        boost::string_view token = req.getHeaderValue("X-Auth-Token");
        if (token.empty())
        {
            return nullptr;
        }
        auto session =
            persistent_data::SessionStore::getInstance().loginSessionByToken(
                token);
        return session;
    }

    const std::shared_ptr<crow::persistent_data::UserSession>
        performCookieAuth(const crow::Request& req) const
    {
        BMCWEB_LOG_DEBUG << "[AuthMiddleware] Cookie authentication";

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
    bool isOnWhitelist(const crow::Request& req) const
    {
        // it's allowed to GET root node without authentica tion
        if ("GET"_method == req.method())
        {
            if (req.url == "/redfish/v1" || req.url == "/redfish/v1/" ||
                req.url == "/redfish" || req.url == "/redfish/" ||
                req.url == "/redfish/v1/odata" ||
                req.url == "/redfish/v1/odata/")
            {
                return true;
            }
            else if (crow::webassets::routes.find(std::string(req.url)) !=
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
};

// TODO(ed) see if there is a better way to allow middlewares to request
// routes.
// Possibly an init function on first construction?
template <typename... Middlewares> void requestRoutes(Crow<Middlewares...>& app)
{
    static_assert(
        black_magic::Contains<persistent_data::Middleware,
                              Middlewares...>::value,
        "token_authorization middleware must be enabled in app to use "
        "auth routes");
    BMCWEB_ROUTE(app, "/login")
        .methods(
            "POST"_method)([&](const crow::Request& req, crow::Response& res) {
            boost::string_view contentType = req.getHeaderValue("content-type");
            boost::string_view username;
            boost::string_view password;

            bool looksLikeIbm = false;

            // This object needs to be declared at this scope so the strings
            // within it are not destroyed before we can use them
            nlohmann::json loginCredentials;
            // Check if auth was provided by a payload
            if (contentType == "application/json")
            {
                loginCredentials =
                    nlohmann::json::parse(req.body, nullptr, false);
                if (loginCredentials.is_discarded())
                {
                    res.result(boost::beast::http::status::bad_request);
                    res.end();
                    return;
                }

                // check for username/password in the root object
                // THis method is how intel APIs authenticate
                nlohmann::json::iterator userIt =
                    loginCredentials.find("username");
                nlohmann::json::iterator passIt =
                    loginCredentials.find("password");
                if (userIt != loginCredentials.end() &&
                    passIt != loginCredentials.end())
                {
                    const std::string* userStr =
                        userIt->get_ptr<const std::string*>();
                    const std::string* passStr =
                        passIt->get_ptr<const std::string*>();
                    if (userStr != nullptr && passStr != nullptr)
                    {
                        username = *userStr;
                        password = *passStr;
                    }
                }
                else
                {
                    // Openbmc appears to push a data object that contains the
                    // same keys (username and password), attempt to use that
                    auto dataIt = loginCredentials.find("data");
                    if (dataIt != loginCredentials.end())
                    {
                        // Some apis produce an array of value ["username",
                        // "password"]
                        if (dataIt->is_array())
                        {
                            if (dataIt->size() == 2)
                            {
                                nlohmann::json::iterator userIt2 =
                                    dataIt->begin();
                                nlohmann::json::iterator passIt2 =
                                    dataIt->begin() + 1;
                                looksLikeIbm = true;
                                if (userIt2 != dataIt->end() &&
                                    passIt2 != dataIt->end())
                                {
                                    const std::string* userStr =
                                        userIt2->get_ptr<const std::string*>();
                                    const std::string* passStr =
                                        passIt2->get_ptr<const std::string*>();
                                    if (userStr != nullptr &&
                                        passStr != nullptr)
                                    {
                                        username = *userStr;
                                        password = *passStr;
                                    }
                                }
                            }
                        }
                        else if (dataIt->is_object())
                        {
                            nlohmann::json::iterator userIt2 =
                                dataIt->find("username");
                            nlohmann::json::iterator passIt2 =
                                dataIt->find("password");
                            if (userIt2 != dataIt->end() &&
                                passIt2 != dataIt->end())
                            {
                                const std::string* userStr =
                                    userIt2->get_ptr<const std::string*>();
                                const std::string* passStr =
                                    passIt2->get_ptr<const std::string*>();
                                if (userStr != nullptr && passStr != nullptr)
                                {
                                    username = *userStr;
                                    password = *passStr;
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                // check if auth was provided as a headers
                username = req.getHeaderValue("username");
                password = req.getHeaderValue("password");
            }

            if (!username.empty() && !password.empty())
            {
                if (!pamAuthenticateUser(username, password))
                {
                    res.result(boost::beast::http::status::unauthorized);
                }
                else
                {
                    auto session = persistent_data::SessionStore::getInstance()
                                       .generateUserSession(username);

                    if (looksLikeIbm)
                    {
                        // IBM requires a very specific login structure, and
                        // doesn't actually look at the status code.
                        // TODO(ed).... Fix that upstream
                        res.jsonValue = {
                            {"data",
                             "User '" + std::string(username) + "' logged in"},
                            {"message", "200 OK"},
                            {"status", "ok"}};

                        // Hack alert.  Boost beast by default doesn't let you
                        // declare multiple headers of the same name, and in
                        // most cases this is fine.  Unfortunately here we need
                        // to set the Session cookie, which requires the
                        // httpOnly attribute, as well as the XSRF cookie, which
                        // requires it to not have an httpOnly attribute. To get
                        // the behavior we want, we simply inject the second
                        // "set-cookie" string into the value header, and get
                        // the result we want, even though we are technicaly
                        // declaring two headers here.
                        res.addHeader("Set-Cookie",
                                      "XSRF-TOKEN=" + session->csrfToken +
                                          "; Secure\r\nSet-Cookie: SESSION=" +
                                          session->sessionToken +
                                          "; Secure; HttpOnly");
                    }
                    else
                    {
                        // if content type is json, assume json token
                        res.jsonValue = {{"token", session->sessionToken}};
                    }
                }
            }
            else
            {
                res.result(boost::beast::http::status::bad_request);
            }
            res.end();
        });

    BMCWEB_ROUTE(app, "/logout")
        .methods(
            "POST"_method)([&](const crow::Request& req, crow::Response& res) {
            auto& session =
                app.template getContext<token_authorization::Middleware>(req)
                    .session;
            if (session != nullptr)
            {
                persistent_data::SessionStore::getInstance().removeSession(
                    session);
            }
            res.end();
            return;
        });
}
} // namespace token_authorization
} // namespace crow
