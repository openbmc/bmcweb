// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "app.hpp"
#include "async_resp.hpp"
#include "cookies.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "logging.hpp"
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
    // This object needs to be declared at this scope so the strings
    // within it are not destroyed before we can use them
    nlohmann::json loginCredentials;
    if (parseRequestAsJson(req, loginCredentials) != JsonParseResult::Success)
    {
        asyncResp->res.result(boost::beast::http::status::bad_request);
        return;
    }

    const nlohmann::json::object_t* obj =
        loginCredentials.get_ptr<const nlohmann::json::object_t*>();
    if (obj == nullptr)
    {
        BMCWEB_LOG_DEBUG("Recieved json was not an object");
        asyncResp->res.result(boost::beast::http::status::bad_request);
        return;
    }

    // check for username/password in the root object
    auto userIt = obj->find("username");
    if (userIt == obj->end())
    {
        BMCWEB_LOG_DEBUG("Couldn't interpret username");
        asyncResp->res.result(boost::beast::http::status::bad_request);
        return;
    }
    const std::string* userStr = userIt->second.get_ptr<const std::string*>();
    if (userStr == nullptr)
    {
        BMCWEB_LOG_DEBUG("Couldn't interpret username");
        asyncResp->res.result(boost::beast::http::status::bad_request);
        return;
    }
    auto passIt = obj->find("password");
    if (passIt == obj->end())
    {
        BMCWEB_LOG_DEBUG("Couldn't interpret password");
        asyncResp->res.result(boost::beast::http::status::bad_request);
        return;
    }
    const std::string* passStr = passIt->second.get_ptr<const std::string*>();
    if (passStr == nullptr)
    {
        BMCWEB_LOG_DEBUG("Couldn't interpret password");
        asyncResp->res.result(boost::beast::http::status::bad_request);
        return;
    }

    int pamrc = pamAuthenticateUser(*userStr, *passStr, std::nullopt);
    bool isConfigureSelfOnly = pamrc == PAM_NEW_AUTHTOK_REQD;
    if ((pamrc != PAM_SUCCESS) && !isConfigureSelfOnly)
    {
        asyncResp->res.result(boost::beast::http::status::unauthorized);
        return;
    }
    auto session =
        persistent_data::SessionStore::getInstance().generateUserSession(
            *userStr, req.ipAddress, std::nullopt,
            persistent_data::SessionType::Session, isConfigureSelfOnly);

    bmcweb::setSessionCookies(asyncResp->res, *session);

    // if content type is json, assume json token
    asyncResp->res.jsonValue["token"] = session->sessionToken;
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
