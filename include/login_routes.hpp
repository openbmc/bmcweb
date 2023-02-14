#pragma once

#include "app.hpp"
#include "common.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "multipart_parser.hpp"
#include "pam_authenticate.hpp"
#include "webassets.hpp"

#include <boost/container/flat_set.hpp>

#include <random>

namespace crow
{

namespace login_routes
{

inline void requestRoutes(App& app)
{
    BMCWEB_ROUTE(app, "/login")
        .methods(boost::beast::http::verb::post)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
        MultipartParser parser;
        std::string_view contentType = req.getHeaderValue("content-type");
        std::string_view username;
        std::string_view password;

        bool looksLikePhosphorRest = false;

        // This object needs to be declared at this scope so the strings
        // within it are not destroyed before we can use them
        nlohmann::json loginCredentials;
        // Check if auth was provided by a payload
        if (contentType.starts_with("application/json"))
        {
            loginCredentials =
                nlohmann::json::parse(req.body(), nullptr, false);
            if (loginCredentials.is_discarded())
            {
                BMCWEB_LOG_DEBUG << "Bad json in request";
                asyncResp->res.result(boost::beast::http::status::bad_request);
                return;
            }

            // check for username/password in the root object
            // THis method is how intel APIs authenticate
            nlohmann::json::iterator userIt = loginCredentials.find("username");
            nlohmann::json::iterator passIt = loginCredentials.find("password");
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
                            nlohmann::json::iterator userIt2 = dataIt->begin();
                            nlohmann::json::iterator passIt2 =
                                dataIt->begin() + 1;
                            looksLikePhosphorRest = true;
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
        else if (contentType.starts_with("multipart/form-data"))
        {
            looksLikePhosphorRest = true;
            ParserError ec = parser.parse(req);
            if (ec != ParserError::PARSER_SUCCESS)
            {
                // handle error
                BMCWEB_LOG_ERROR << "MIME parse failed, ec : "
                                 << static_cast<int>(ec);
                asyncResp->res.result(boost::beast::http::status::bad_request);
                return;
            }

            for (const FormPart& formpart : parser.mime_fields)
            {
                boost::beast::http::fields::const_iterator it =
                    formpart.fields.find("Content-Disposition");
                if (it == formpart.fields.end())
                {
                    BMCWEB_LOG_ERROR << "Couldn't find Content-Disposition";
                    asyncResp->res.result(
                        boost::beast::http::status::bad_request);
                    continue;
                }

                BMCWEB_LOG_INFO << "Parsing value " << it->value();

                if (it->value() == "form-data; name=\"username\"")
                {
                    username = formpart.content;
                }
                else if (it->value() == "form-data; name=\"password\"")
                {
                    password = formpart.content;
                }
                else
                {
                    BMCWEB_LOG_INFO << "Extra format, ignore it."
                                    << it->value();
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
            int pamrc = pamAuthenticateUser(username, password);
            bool isConfigureSelfOnly = pamrc == PAM_NEW_AUTHTOK_REQD;
            if ((pamrc != PAM_SUCCESS) && !isConfigureSelfOnly)
            {
                asyncResp->res.result(boost::beast::http::status::unauthorized);
            }
            else
            {
                auto session =
                    persistent_data::SessionStore::getInstance()
                        .generateUserSession(
                            username, req.ipAddress, std::nullopt,
                            persistent_data::PersistenceType::TIMEOUT,
                            isConfigureSelfOnly);

                if (looksLikePhosphorRest)
                {
                    // Phosphor-Rest requires a very specific login
                    // structure, and doesn't actually look at the status
                    // code.
                    // TODO(ed).... Fix that upstream

                    asyncResp->res.jsonValue["data"] =
                        "User '" + std::string(username) + "' logged in";
                    asyncResp->res.jsonValue["message"] = "200 OK";
                    asyncResp->res.jsonValue["status"] = "ok";

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
                    asyncResp->res.addHeader(
                        "Set-Cookie",
                        "XSRF-TOKEN=" + session->csrfToken +
                            "; SameSite=Strict; Secure\r\nSet-Cookie: "
                            "SESSION=" +
                            session->sessionToken +
                            "; SameSite=Strict; Secure; HttpOnly");
                }
                else
                {
                    // if content type is json, assume json token
                    asyncResp->res.jsonValue["token"] = session->sessionToken;
                }
            }
        }
        else
        {
            BMCWEB_LOG_DEBUG << "Couldn't interpret password";
            asyncResp->res.result(boost::beast::http::status::bad_request);
        }
        });

    BMCWEB_ROUTE(app, "/logout")
        .methods(boost::beast::http::verb::post)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
        const auto& session = req.session;
        if (session != nullptr)
        {
            asyncResp->res.jsonValue["data"] =
                "User '" + session->username + "' logged out";
            asyncResp->res.jsonValue["message"] = "200 OK";
            asyncResp->res.jsonValue["status"] = "ok";

            asyncResp->res.addHeader("Set-Cookie",
                                     "SESSION="
                                     "; SameSite=Strict; Secure; HttpOnly; "
                                     "expires=Thu, 01 Jan 1970 00:00:00 GMT");

            persistent_data::SessionStore::getInstance().removeSession(session);
        }
        });
}
} // namespace login_routes
} // namespace crow
