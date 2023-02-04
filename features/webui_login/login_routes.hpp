// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "app.hpp"
#include "async_resp.hpp"
#include "cookies.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "logging.hpp"
#include "multipart_parser.hpp"
#include "pam_authenticate.hpp"
#include "sessions.hpp"

#include <security/_pam_types.h>

#include <boost/beast/http/fields.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/verb.hpp>
#include <nlohmann/json.hpp>

#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace crow
{

namespace login_routes
{

inline void handleLogin(const crow::Request& req,
                        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    MultipartParser parser;
    std::string_view contentType = req.getHeaderValue("content-type");
    std::string_view username;
    std::string_view password;

    // This object needs to be declared at this scope so the strings
    // within it are not destroyed before we can use them
    nlohmann::json loginCredentials;
    // Check if auth was provided by a payload
    if (contentType.starts_with("application/json"))
    {
        loginCredentials = nlohmann::json::parse(req.body(), nullptr, false);
        if (loginCredentials.is_discarded())
        {
            BMCWEB_LOG_DEBUG("Bad json in request");
            asyncResp->res.result(boost::beast::http::status::bad_request);
            return;
        }
        const nlohmann::json::object_t* obj =
            loginCredentials.get_ptr<const nlohmann::json::object_t*>();
        if (obj != nullptr)
        {
            // check for username/password in the root object
            // This method is how intel APIs authenticate
            auto userIt = obj->find("username");
            auto passIt = obj->find("password");
            if (userIt != obj->end() && passIt != obj->end())
            {
                const std::string* userStr =
                    userIt->second.get_ptr<const std::string*>();
                const std::string* passStr =
                    passIt->second.get_ptr<const std::string*>();
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
                auto dataIt = obj->find("data");
                if (dataIt != obj->end())
                {
                    // Some apis produce an array of value ["username",
                    // "password"]
                    const nlohmann::json::array_t* arr =
                        dataIt->second
                            .get_ptr<const nlohmann::json::array_t*>();
                    if (arr != nullptr)
                    {
                        if (arr->size() == 2)
                        {
                            nlohmann::json::array_t::const_iterator userIt2 =
                                arr->begin();
                            nlohmann::json::array_t::const_iterator passIt2 =
                                arr->begin() + 1;
                            if (userIt2 != arr->end() && passIt2 != arr->end())
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
                    const nlohmann::json::object_t* obj2 =
                        dataIt->second
                            .get_ptr<const nlohmann::json::object_t*>();

                    if (obj2 != nullptr)
                    {
                        nlohmann::json::object_t::const_iterator userIt2 =
                            obj2->find("username");
                        nlohmann::json::object_t::const_iterator passIt2 =
                            obj2->find("password");
                        if (userIt2 != obj2->end() && passIt2 != obj2->end())
                        {
                            const std::string* userStr =
                                userIt2->second.get_ptr<const std::string*>();
                            const std::string* passStr =
                                passIt2->second.get_ptr<const std::string*>();
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
    }
    else if (contentType.starts_with("multipart/form-data"))
    {
        ParserError ec = parser.parse(req);
        if (ec != ParserError::PARSER_SUCCESS)
        {
            // handle error
            BMCWEB_LOG_ERROR("MIME parse failed, ec : {}",
                             static_cast<int>(ec));
            asyncResp->res.result(boost::beast::http::status::bad_request);
            return;
        }

        for (const FormPart& formpart : parser.mime_fields)
        {
            boost::beast::http::fields::const_iterator it =
                formpart.fields.find("Content-Disposition");
            if (it == formpart.fields.end())
            {
                BMCWEB_LOG_ERROR("Couldn't find Content-Disposition");
                asyncResp->res.result(boost::beast::http::status::bad_request);
                continue;
            }

            BMCWEB_LOG_INFO("Parsing value {}", it->value());

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
                BMCWEB_LOG_INFO("Extra format, ignore it.{}", it->value());
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
        int pamrc = pamAuthenticateUser(username, password, std::nullopt);
        bool isConfigureSelfOnly = pamrc == PAM_NEW_AUTHTOK_REQD;
        if ((pamrc != PAM_SUCCESS) && !isConfigureSelfOnly)
        {
            asyncResp->res.result(boost::beast::http::status::unauthorized);
        }
        else
        {
            auto session =
                persistent_data::SessionStore::getInstance()
                    .generateUserSession(username, req.ipAddress, std::nullopt,
                                         persistent_data::SessionType::Session,
                                         isConfigureSelfOnly);

            bmcweb::setSessionCookies(asyncResp->res, *session);

            // if content type is json, assume json token
            asyncResp->res.jsonValue["token"] = session->sessionToken;
        }
    }
    else
    {
        BMCWEB_LOG_DEBUG("Couldn't interpret password");
        asyncResp->res.result(boost::beast::http::status::bad_request);
    }
}

inline void handleLogout(const crow::Request& req,
                         const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    const auto& session = req.session;
    if (session != nullptr)
    {
        asyncResp->res.jsonValue["data"] =
            "User '" + session->username + "' logged out";
        asyncResp->res.jsonValue["message"] = "200 OK";
        asyncResp->res.jsonValue["status"] = "ok";

        bmcweb::clearSessionCookies(asyncResp->res);
        persistent_data::SessionStore::getInstance().removeSession(session);
    }
}

inline void requestRoutes(App& app)
{
    BMCWEB_ROUTE(app, "/login")
        .methods(boost::beast::http::verb::post)(handleLogin);

    BMCWEB_ROUTE(app, "/logout")
        .methods(boost::beast::http::verb::post)(handleLogout);
}
} // namespace login_routes
} // namespace crow
